/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/meshtex.cpp,v 1.1 2000/01/14 10:40:19 MAT Exp $

/* --<<= -+-/-/-/=/=/\/- <<< ((( ((( / \ ))) ))) >>> -\/\=\=\-\-\-+- =>>-- *\
   meshtex.cpp

\* --<<= -+-\-\-\=\=\/\- <<< ((( ((( \ / ))) ))) >>> -/\/=/=/-/-/-+- =>>-- */


#include <2d.h>
#include <lgd3d.h>
#include <tmgr.h>
#include <mms.h>
#include <mm.h>
#include <resapi.h>
#include <storeapi.h>
#include <imgrstyp.h>
#include <palrstyp.h>
#include <dynarray.h>

#include <prophash.h>
#include <dataops.h>
#include <obprpimp.h>
#include <sdesbase.h>
#include <sdesc.h>
#include <objsys.h>

#include <command.h>
#include <palmgr.h>
#include <objmodel.h>
#include <crjoint.h>
#include <face.h>

#include <meshtex.h>
#include <dbmem.h>
#include <initguid.h>
#include <meshtexg.h>


// @HACK: We want to load textures using an internal objmodel path so
// the custom mesh textures can be in the same directories as the
// regular ones.
EXTERN ISearchPath *MeshModelTexturesPath;


/* --<<= -+-/-/-/=/=/\/- <<< ((( ((( / \ ))) ))) >>> -\/\=\=\-\-\-+- =>>-- *\

   predeclarations

\* --<<= -+-\-\-\=\=\/\- <<< ((( ((( \ / ))) ))) >>> -/\/=/=/-/-/-+- =>>-- */

struct sMeshTexRemap;


/* --<<= -+-/-/-/=/=/\/- <<< ((( ((( / \ ))) ))) >>> -\/\=\=\-\-\-+- =>>-- *\

   constants

\* --<<= -+-\-\-\=\=\/\- <<< ((( ((( \ / ))) ))) >>> -/\/=/=/-/-/-+- =>>-- */

// number of things in property
static const int kNumTextures = 12;

// max materials we can handle
static const int kMaxTextureSubst = 128;


/* --<<= -+-/-/-/=/=/\/- <<< ((( ((( / \ ))) ))) >>> -\/\=\=\-\-\-+- =>>-- *\

   globals

\* --<<= -+-\-\-\=\=\/\- <<< ((( ((( \ / ))) ))) >>> -/\/=/=/-/-/-+- =>>-- */

// info on the object we're rendering
static mms_model *g_pCurrentModel;
static ObjID g_CurrentObj;
static sMeshTexRemap *g_pCurrentRemap;

struct sResTexture
{
   char m_aszName[16];
   IRes *m_pRes;
   grs_bitmap *m_pBitmap;
};
static cDynArray<sResTexture> g_Textures; 

// to stash off the parts of the real model we'll be overwriting
static ulong g_aszDefaultMaterial[kMaxTextureSubst];

// other
static IResMan *g_pResMan;


/* --<<= -+-/-/-/=/=/\/- <<< ((( ((( / \ ))) ))) >>> -\/\=\=\-\-\-+- =>>-- *\

   our property

   We have a horrible little struct which holds the parts of the mesh
   model which we swap in on-the-fly.

\* --<<= -+-\-\-\=\=\/\- <<< ((( ((( \ / ))) ))) >>> -/\/=/=/-/-/-+- =>>-- */

struct sTexture
{
   char m_aszDefault[16];
   char m_aszCustom[16];
};

struct sTextureSubst
{
   int m_iMaterialIndex;        // within mms_model, that is
   grs_bitmap *m_pBitmap;
};


///////////////////////////////////////////////
// the structure used for the property

struct sMeshTexRemap
{
   // We do this so the cDynArrays won't try to get copied on load,
   // with their pointers left over from when they were saved off.
   // The data we're losing is derived from the data we're keeping.
   sMeshTexRemap &operator=(const sMeshTexRemap &from)
   {
      memcpy(&m_Texture, &from.m_Texture, sizeof(m_Texture));
      memset(&m_TextureSubst, 0, sizeof(m_TextureSubst));
      m_bSet = FALSE;

      return *this;
   }


   // data

   ///////////////////////////////////////////////
   // Here's the part which the designer sets.
   sTexture m_Texture[kNumTextures];

   ///////////////////////////////////////////////
   // This part is set up whenever the property is modified.  It's the
   // actual stuff we swap in and out.
   cDynArray<sTextureSubst> m_TextureSubst;
   BOOL m_bSet;         // TRUE if this instance has been set up since load
};


#undef  INTERFACE
#define INTERFACE IMeshTexProperty

DECLARE_PROPERTY_INTERFACE(IMeshTexProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sMeshTexRemap *);
};


static sPropertyDesc g_MeshTexDesc
= {
   "MeshTex",
   0,           // flags
   NULL,        // constraints
   1, 0,        // version
   { "Renderer", "Mesh Textures", "Mesh Texture Substitution" },
   kPropertyChangeLocally,
};

class cMeshTexOps : public cClassDataOps<sMeshTexRemap>
{
};

class cMeshTexStore : public cHashPropertyStore<cMeshTexOps>
{
};

typedef cSpecificProperty<IMeshTexProperty,
   &IID_IMeshTexProperty, sMeshTexRemap *,
   cMeshTexStore> cBaseMeshTexProperty;

class cMeshTexProperty : public cBaseMeshTexProperty
{
public:
   cMeshTexProperty(const sPropertyDesc *desc)
      : cBaseMeshTexProperty(desc)
   {
   }

   void OnListenMsg(ePropertyListenMsg msg, ObjID obj, uPropListenerValue val)
   {
      sMeshTexRemap *pRemap = (sMeshTexRemap *) val.ptrval;

      // Zero out the dynarrays on load/modify
      if (msg & (kListenPropLoad | kListenPropModify)) {
         if (msg & kListenPropLoad) {
            memset(&pRemap->m_TextureSubst, 0, sizeof(pRemap->m_TextureSubst));
         } else {
            pRemap->m_TextureSubst.SetSize(0);
         }
         pRemap->m_bSet = FALSE;
      }
   }

   STANDARD_DESCRIBE_TYPE(cMeshTexProperty);

protected:
   void CreateEditor();
};


static sFieldDesc g_aMeshTexFieldDesc[]
= {
   { "texture in model", kFieldTypeString,
     FieldLocation(sMeshTexRemap, m_Texture[0].m_aszDefault) },
   { "replace with", kFieldTypeString,
     FieldLocation(sMeshTexRemap, m_Texture[0].m_aszCustom) },

   { "texture in model", kFieldTypeString,
     FieldLocation(sMeshTexRemap, m_Texture[1].m_aszDefault) },
   { "replace with", kFieldTypeString,
     FieldLocation(sMeshTexRemap, m_Texture[1].m_aszCustom) },

   { "texture in model", kFieldTypeString,
     FieldLocation(sMeshTexRemap, m_Texture[2].m_aszDefault) },
   { "replace with", kFieldTypeString,
     FieldLocation(sMeshTexRemap, m_Texture[2].m_aszCustom) },

   { "texture in model", kFieldTypeString,
     FieldLocation(sMeshTexRemap, m_Texture[3].m_aszDefault) },
   { "replace with", kFieldTypeString,
     FieldLocation(sMeshTexRemap, m_Texture[3].m_aszCustom) },

   { "texture in model", kFieldTypeString,
     FieldLocation(sMeshTexRemap, m_Texture[4].m_aszDefault) },
   { "replace with", kFieldTypeString,
     FieldLocation(sMeshTexRemap, m_Texture[4].m_aszCustom) },

   { "texture in model", kFieldTypeString,
     FieldLocation(sMeshTexRemap, m_Texture[5].m_aszDefault) },
   { "replace with", kFieldTypeString,
     FieldLocation(sMeshTexRemap, m_Texture[5].m_aszCustom) },

   { "texture in model", kFieldTypeString,
     FieldLocation(sMeshTexRemap, m_Texture[6].m_aszDefault) },
   { "replace with", kFieldTypeString,
     FieldLocation(sMeshTexRemap, m_Texture[6].m_aszCustom) },

   { "texture in model", kFieldTypeString,
     FieldLocation(sMeshTexRemap, m_Texture[7].m_aszDefault) },
   { "replace with", kFieldTypeString,
     FieldLocation(sMeshTexRemap, m_Texture[7].m_aszCustom) },

   { "texture in model", kFieldTypeString,
     FieldLocation(sMeshTexRemap, m_Texture[8].m_aszDefault) },
   { "replace with", kFieldTypeString,
     FieldLocation(sMeshTexRemap, m_Texture[8].m_aszCustom) },

   { "texture in model", kFieldTypeString,
     FieldLocation(sMeshTexRemap, m_Texture[9].m_aszDefault) },
   { "replace with", kFieldTypeString,
     FieldLocation(sMeshTexRemap, m_Texture[9].m_aszCustom) },

   { "texture in model", kFieldTypeString,
     FieldLocation(sMeshTexRemap, m_Texture[10].m_aszDefault) },
   { "replace with", kFieldTypeString,
     FieldLocation(sMeshTexRemap, m_Texture[10].m_aszCustom) },

   { "texture in model", kFieldTypeString,
     FieldLocation(sMeshTexRemap, m_Texture[11].m_aszDefault) },
   { "replace with", kFieldTypeString,
     FieldLocation(sMeshTexRemap, m_Texture[11].m_aszCustom) },
};

static sStructDesc g_MeshTexStructDesc
= StructDescBuild(cMeshTexProperty, kStructFlagNone,
                  g_aMeshTexFieldDesc);

void cMeshTexProperty::CreateEditor()
{
   AutoAppIPtr(StructDescTools);
   pStructDescTools->Register(&g_MeshTexStructDesc);
   cPropertyBase::CreateEditor(this);
}

IMeshTexProperty *g_pMeshTexProperty;


/* --<<= -+-/-/-/=/=/\/- <<< ((( ((( / \ ))) ))) >>> -\/\=\=\-\-\-+- =>>-- *\

   helpers

\* --<<= -+-\-\-\=\=\/\- <<< ((( ((( \ / ))) ))) >>> -/\/=/=/-/-/-+- =>>-- */

static void SetUpRemap(ObjID Obj, mms_model *pModel, sMeshTexRemap *pRemap)
{
   mms_smatr *pMaterial = mm_smatr_list(pModel);
   mms_segment *pSegment = mm_segment_list(pModel);

   // set up remapping of textures
   for (int iTexSub = 0; iTexSub < kNumTextures; ++iTexSub) {
      if (!pRemap->m_Texture[iTexSub].m_aszDefault[0])
         continue;

      int iMat;

      // find material which matches our string
      for (iMat = 0; iMat < pModel->smatrs; ++iMat)
         if (!stricmp(pMaterial[iMat].name,
                      pRemap->m_Texture[iTexSub].m_aszDefault))
            break;

#ifndef SHIP
      if (iMat == pModel->smatrs) {
         Warning(("meshtex: Can't find texture %s for object %s.\n",
                  pRemap->m_Texture[iTexSub].m_aszDefault,
                  AppGetObj(IObjectSystem)->GetName(Obj)));
         break;
      }
#endif // ~SHIP

      sTextureSubst Subst;
      Subst.m_iMaterialIndex = iMat;
      Subst.m_pBitmap=MeshTexGetBitmap(pRemap->m_Texture[iTexSub].m_aszCustom);

#ifndef SHIP
      if (Subst.m_pBitmap == 0) {
         Warning(("meshtex: Can't find texture %s for object %s.\n",
                  pRemap->m_Texture[iTexSub].m_aszCustom,
                  AppGetObj(IObjectSystem)->GetName(Obj)));
         break;
      }
#endif // ~SHIP

      pRemap->m_TextureSubst.Append(Subst);
   }

   pRemap->m_bSet = TRUE;
}


/* --<<= -+-/-/-/=/=/\/- <<< ((( ((( / \ ))) ))) >>> -\/\=\=\-\-\-+- =>>-- *\

   commands

\* --<<= -+-\-\-\=\=\/\- <<< ((( ((( \ / ))) ))) >>> -/\/=/=/-/-/-+- =>>-- */

#ifdef EDITOR

static void DoNothing()
{
}


static Command g_aCommands[]
= {
   { "mesh_nothing", FUNC_VOID, &DoNothing, "Maybe later.", HK_ALL},
   { "face_process", FUNC_VOID, &BuildFacePosFile,
     "find textures to match speech samples.", HK_ALL},
};
#endif // EDITOR


/* --<<= -+-/-/-/=/=/\/- <<< ((( ((( / \ ))) ))) >>> -\/\=\=\-\-\-+- =>>-- *\

   exposed functions

\* --<<= -+-\-\-\=\=\/\- <<< ((( ((( \ / ))) ))) >>> -/\/=/=/-/-/-+- =>>-- */

// If there's no texture for the name we return 0, but don't warn
// because it's the callers business whether the texture is needed.
grs_bitmap *MeshTexGetBitmap(char *pszName)
{
   strlwr(pszName);

   // Do we already have this one?  We usually will.
   for (int i = 0; i < g_Textures.Size(); ++i)
      if (!strcmp(g_Textures[i].m_aszName, pszName))
         return g_Textures[i].m_pBitmap;

   IRes *pRes = g_pResMan->Bind(pszName, RESTYPE_IMAGE, MeshModelTexturesPath);
   if (!pRes)
      return 0;

   // @HACK: This bit here where we distinguish regular textures
   // from palletized ones is copied directly from objmodel, and
   // duplicates the "txt16" path used there rather than relying
   // on a constant shared by both modules.  Ain't I a bum.
   IStore *pCanonStore = pRes->GetCanonStore();
   if (!stricmp(pCanonStore->GetName(), "txt16\\")) {
      // Okay. Get the palette...
      IRes *pPallRes = g_pResMan->Retype(pRes, RESTYPE_PALETTE, 0);
      if (pPallRes) {
         void *pPall = pPallRes->Lock();
         grs_bitmap *pbm = (grs_bitmap *) pRes->Lock();

         uchar b = pbm->align;
         pbm->align = palmgr_alloc_pal((uchar *) pPall);
         if (b != pbm->align)
            if ((g_tmgr != NULL) && (pbm->flags & BMF_LOADED)) {
               lgd3d_unload_texture(pbm);
               lgd3d_load_texture(pbm);
            }

         pRes->Unlock();
         pPallRes->Unlock();
         // We're done with the palette; the palette manager will now
         // take charge of it.  So we can drop it from memory.
         pPallRes->Drop();
         pPallRes->Release();
      }
   }
   pCanonStore->Release();

   sResTexture NewPair;
   strncpy(NewPair.m_aszName, pszName, 16);
   NewPair.m_pRes = pRes;
   NewPair.m_pBitmap = (grs_bitmap *) pRes->Lock();
   g_Textures.Append(NewPair);

   return NewPair.m_pBitmap;
}


void MeshTexInit()
{
   g_pMeshTexProperty = new cMeshTexProperty(&g_MeshTexDesc);
   g_pResMan = AppGetObj(IResMan);
   FaceInit();

#ifdef EDITOR
   COMMANDS(g_aCommands, HK_ALL);
#endif // EDITOR
}


void MeshTexTerm()
{
   FaceTerm();
}


void MeshTexReset()
{
   for (int i = 0; i < g_Textures.Size(); ++i) {
      g_Textures[i].m_pRes->Unlock();
      g_Textures[i].m_pRes->Release();
   }
   g_Textures.SetSize(0);
   FaceReset();
}


// stamp in our substitute textures, storing off the originals
void MeshTexPrerender(ObjID Obj, mms_model *pModel)
{
   sMeshTexRemap *pRemap;

   // Does this object have our special property?
   if (!g_pMeshTexProperty->Get(Obj, &pRemap)) {
      g_pCurrentModel = 0;
      return;
   }

   g_pCurrentModel = pModel;
   g_CurrentObj = Obj;
   g_pCurrentRemap = pRemap;

   if (!pRemap->m_bSet)
      SetUpRemap(Obj, pModel, pRemap);

   mms_smatr *pMaterial = mm_smatr_list(pModel);
   int iNumTextureSubst = pRemap->m_TextureSubst.Size();
   sTextureSubst *pTextureSubst = pRemap->m_TextureSubst.AsPointer();

   Assert_(iNumTextureSubst <= kMaxTextureSubst);

   for (int i = 0; i < iNumTextureSubst; ++i) {
      g_aszDefaultMaterial[i]
         = pMaterial[pTextureSubst[i].m_iMaterialIndex].handle;
      pMaterial[pTextureSubst[i].m_iMaterialIndex].handle
         = (ulong) pTextureSubst[i].m_pBitmap;
   }

   FacePrerender(Obj, pModel);
}


// restore the regular textures to the model
void MeshTexPostrender()
{
   if (!g_pCurrentModel)
      return;

   mms_smatr *pMaterial = mm_smatr_list(g_pCurrentModel);
   int iNumTextureSubst = g_pCurrentRemap->m_TextureSubst.Size();
   sTextureSubst *pTextureSubst = g_pCurrentRemap->m_TextureSubst.AsPointer();

   for (int i = 0; i < iNumTextureSubst; ++i)
      pMaterial[pTextureSubst[i].m_iMaterialIndex].handle
         = g_aszDefaultMaterial[i];

   FacePostrender();
}
