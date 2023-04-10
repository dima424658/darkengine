/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/rendprop.cpp,v 1.48 2000/02/18 14:17:51 MAT Exp $

#include <lg.h>
#include <mprintf.h>

#include <wrtype.h>
#include <wrobj.h>
#include <refsys.h>

#include <rendprop.h>
#include <property.h>
#include <propert_.h>
#include <propface.h>
#include <propfac_.h>
#include <dataops_.h>
#include <propsprs.h>
#include <propbase.h>
#include <prophash.h>
#include <propbase.h>
#include <objquery.h>

#include <linkbase.h>
#include <lnkquery.h>
#include <relation.h>

#include <traitman.h>
#include <trait.h>
#include <traitbas.h>
#include <objsys.h>
#include <iobjsys.h>
#include <objnotif.h>
#include <objedit.h>
#include <editobj.h>
#include <iobjnet.h>

#include <parttype.h>
#include <pgrpprop.h>
#include <objpos.h>
#include <fasprop.h>
#include <fastype.h>
#include <rndflash.h>
#include <viewmgr.h>

#include <sdesbase.h>
#include <sdestool.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
// HAS REFS PROPERTIES
//

static sPropertyDesc refPropDesc =
{
   PROP_REF_NAME,
   kPropertyInstantiate|kPropertyNoInherit|kPropertyNoClone, // flags
   NULL, // constraints
   0, 0, // Version
   { "Renderer", "Has Refs" }, // ui strings
};

typedef cGenericBoolProperty cRefPropBase;

class cRefProp : public cRefPropBase
{
public:
   cRefProp (const sPropertyDesc* desc)
      : cRefPropBase(desc,kPropertyImplSparseHash)
   {
   }

   void OnListenMsg(ePropertyListenMsg msg, ObjID obj, uPropListenerValue val)
   {
      ObjPosTouch(obj);
      cRefPropBase::OnListenMsg(msg,obj,val);
   }
};


IBoolProperty* RefProp = NULL;

BOOL ObjHasRefs(ObjID obj)
{
   Assert_(RefProp);
   BOOL result = TRUE;
   RefProp->Get(obj,&result);
   return result;
}

void ObjSetHasRefs(ObjID obj,BOOL hasrefs)
{
   Assert_(RefProp);
   RefProp->Set(obj,hasrefs);
}


////////////////////////////////////////////////////////////////////
// This property is used in some lighting situations (light gem) to
// cast against those objects objects that have this property
//

#define OBJSHAD_IMP  kPropertyImplVerySparse

sPropertyDesc objshad_render_desc =
{
   PROP_OBJSHAD_NAME,
   0,      // flags
   NULL,   // constraints
   1,0,    // version
   {
      "Renderer",
      "Runtime Object Shadow"
   }
};

IBoolProperty *g_pObjShadowProp = NULL;


static void ObjShadPropInit(void)
{
   g_pObjShadowProp = CreateBoolProperty(&objshad_render_desc, OBJSHAD_IMP);
}



////////////////////////////////////////////////////////////
// RENDER ALPHA PROPERTY
//

#define ALPHA_IMP  kPropertyImplVerySparse

sPropertyDesc alpha_render_desc =
{
   PROP_ALPHA_NAME,
   0,      // flags
   NULL,   // constraints
   1,0,    // version
   {
      "Renderer",
      "Transparency (alpha)"
   }
};

IFloatProperty *gAlphaRenderProp;
static void AlphaRenderPropInit(void)
{
   gAlphaRenderProp = CreateFloatProperty(&alpha_render_desc, ALPHA_IMP);
}

////////////////////////////////////////////////////////////
// SELF-ILLUMINATION PROPERTY
//

#define SELF_ILLUM_IMP  kPropertyImplVerySparse

sPropertyDesc self_illum_render_desc =
{
   PROP_SELF_ILLUM_NAME,
   0,      // flags
   NULL,   // constraints
   1,0,    // version
   {
      "Renderer",
      "Self Illumination"
   },
};

IFloatProperty *g_pSelfIlluminationProp;
static void SelfIllumPropInit(void)
{
   g_pSelfIlluminationProp = CreateFloatProperty(&self_illum_render_desc, SELF_ILLUM_IMP);
}

////////////////////////////////////////////////////////////
// RENDER TYPE PROPERTY
//

static sPropertyDesc rTypePropDesc =
{
   PROP_RENDER_TYPE_NAME,
   0,  // flags
   NULL, // constraints
   0, 0, // Version
   { "Renderer", "Render Type" }, // ui strings
};

//
// Render type sdesc
//
static char* rend_type_strings[] =
{
   "Normal",
   "Not Rendered",
   "Unlit",
   "Editor Only",
};

#define NUM_REND_STRINGS (sizeof(rend_type_strings)/sizeof(rend_type_strings[0]))

#define RENDTYPE_TYPENAME "tRendType"

static sFieldDesc rendtype_field[] =
{
   { "", kFieldTypeEnum, sizeof(int), 0, kFieldFlagUnsigned, 0, NUM_REND_STRINGS, NUM_REND_STRINGS, rend_type_strings },
};

static sStructDesc rendtype_sdesc =
{
   RENDTYPE_TYPENAME,
   sizeof(int),
   kStructFlagNone,
   sizeof(rendtype_field)/sizeof(rendtype_field[0]),
   rendtype_field,
};

static sPropertyTypeDesc rendtype_tdesc =
{
   RENDTYPE_TYPENAME,
   sizeof(int),
};

#define RTYPE_IMPL kPropertyImplSparseHash

IIntProperty* RenderTypeProp = NULL;

eRenderType ObjRenderType(ObjID obj)
{
   Assert_(RenderTypeProp);
   eRenderType result = kRenderNormally;
   RenderTypeProp->Get(obj,&result);
   return result;
}

void ObjSetRenderType(ObjID obj,eRenderType type)
{
   Assert_(RenderTypeProp);
   RenderTypeProp->Set(obj,type);
}

static void RendTypePropInit()
{
   AutoAppIPtr_(StructDescTools,pTools);
   pTools->Register(&rendtype_sdesc);
   RenderTypeProp = CreateIntegralProperty(&rTypePropDesc,&rendtype_tdesc,RTYPE_IMPL);
}

////////////////////////////////////////////////////////////
// BITMAP WORLDSPACE PROPERTY
//

#undef  INTERFACE
#define INTERFACE IBitmapWorldspaceProperty

DECLARE_PROPERTY_INTERFACE(IBitmapWorldspaceProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sBitmapWorldspace *);
};

static sPropertyDesc g_BitmapWorldspaceDesc
= {
   "BitmapWorld",
   0,           // flags
   NULL,        // constraints
   1, 0,        // version
   { "Renderer", "Bitmap Worldspace", "bitmap not always facing camera" },
   kPropertyChangeLocally,
};

class cBitmapWorldspaceOps : public cClassDataOps<sBitmapWorldspace>
{
};

class cBitmapWorldspaceStore : public cHashPropertyStore<cBitmapWorldspaceOps>
{
};

typedef cSpecificProperty<IBitmapWorldspaceProperty,
   &IID_IBitmapWorldspaceProperty, sBitmapWorldspace *,
   cBitmapWorldspaceStore> cBaseBitmapWorldspaceProperty;

static void UpdateObjBrush(ObjID obj)
{
   mxs_vector Scale;
   mx_mk_vec(&Scale, 0, 0, 0);
   editobjUpdateBrushFromObjScale(obj, &Scale);
   mx_mk_vec(&Scale, 1, 1, 1);
   editobjUpdateBrushFromObjScale(obj, &Scale);
}

class cBitmapWorldspaceProperty : public cBaseBitmapWorldspaceProperty
{
public:
   cBitmapWorldspaceProperty(const sPropertyDesc *desc)
      : cBaseBitmapWorldspaceProperty(desc)
   {
   }

   STANDARD_DESCRIBE_TYPE(cBitmapWorldspaceProperty);

   // In the editor we want changes to this property to force updates
   // of object brushes, since this property is sort of acting as a
   // scale.
#ifdef EDITOR
   void OnListenMsg(ePropertyListenMsg type, ObjID obj, uPropListenerValue val)
   {
      cBaseBitmapWorldspaceProperty::OnListenMsg(type, obj, val);

      if (type & kListenPropLoad)
         return;

      if (type & (kListenPropModify|kListenPropUnset)) {
         if (OBJ_IS_CONCRETE(obj)) {
            UpdateObjBrush(obj);
         } else {
            IObjectQuery* query = mpDonors->GetAllHeirs(obj, kObjectConcrete);
            for (; !query->Done(); query->Next()) {
               UpdateObjBrush(query->Object());
            }
            SafeRelease(query);
         }
         vm_redraw_from_camera();
      }
   }
#endif // EDITOR

protected:
   void CreateEditor();
};


static sFieldDesc g_aBitmapWorldspaceFieldDesc[] =
{
   { "x size (feet)", kFieldTypeFloat,
     FieldLocation(sBitmapWorldspace, m_fXSize) },
   { "y size (feet)", kFieldTypeFloat,
     FieldLocation(sBitmapWorldspace, m_fYSize) },
   { "x feet per tile", kFieldTypeFloat,
     FieldLocation(sBitmapWorldspace, m_fXFeetPerTile) },
   { "y feet per tile", kFieldTypeFloat,
     FieldLocation(sBitmapWorldspace, m_fYFeetPerTile) },
};

static sStructDesc g_BitmapWorldspaceStructDesc
= StructDescBuild(cBitmapWorldspaceProperty, kStructFlagNone,
                  g_aBitmapWorldspaceFieldDesc);

void cBitmapWorldspaceProperty::CreateEditor()
{
   AutoAppIPtr(StructDescTools);
   pStructDescTools->Register(&g_BitmapWorldspaceStructDesc);
   cPropertyBase::CreateEditor(this);
}

IBitmapWorldspaceProperty *g_pBitmapWorldspaceProperty;

static void BitmapWorldspacePropInit()
{
   AutoAppIPtr_(StructDescTools, pTools);
   pTools->Register(&g_BitmapWorldspaceStructDesc);
   g_pBitmapWorldspaceProperty
      = new cBitmapWorldspaceProperty(&g_BitmapWorldspaceDesc);
}

BOOL ObjBitmapWorldspace(ObjID obj, sBitmapWorldspace **ppBWS)
{
   return g_pBitmapWorldspaceProperty->Get(obj, ppBWS);
}


////////////////////////////////////////////////////////////
// BITMAP ANIMATION PROPERTY
//

static sPropertyDesc baPropDesc =
{
   PROP_BITMAP_ANIMATION_NAME,
   kPropertyInstantiate,
   NULL, // constraints
   0, 0, // Version
   { "Renderer", "Bitmap Animation" }, // ui strings
};

static char* bitmap_flags[] =
{
   "Kill on completion",
};

#define BA_TYPENAME "tBitmapAnimationType"

static sFieldDesc ba_field[] =
{
   { "Flags", kFieldTypeBits, sizeof(int), 0, FullFieldNames(bitmap_flags) },
};

static sStructDesc ba_sdesc =
{
   BA_TYPENAME,
   sizeof(int),
   kStructFlagNone,
   sizeof(ba_field)/sizeof(ba_field[0]),
   ba_field,
};

static sPropertyTypeDesc ba_tdesc =
{
   BA_TYPENAME,
   sizeof(int),
};

#define BA_IMPL kPropertyImplHash

static IIntProperty* BitmapFlagsProp = NULL;

BitmapFlags ObjBitmapFlags(ObjID obj)
{
   Assert_(BitmapFlagsProp);
   BitmapFlags result = 0;
   BitmapFlagsProp->Get(obj,&result);
   return result;
}

static void BitmapAnimationPropInit()
{
   AutoAppIPtr_(StructDescTools,pTools);
   pTools->Register(&ba_sdesc);
   BitmapFlagsProp = CreateIntegralProperty(&baPropDesc,&ba_tdesc,BA_IMPL);
}

EXTERN BOOL rendobj_bitmap_retire(ObjID obj);
EXTERN void UpdateBitmapAnimations(int time)
{
   sPropertyObjIter iter;
   ObjID obj;
   BitmapFlags flags;

   BitmapFlagsProp->IterStart(&iter);
   while (BitmapFlagsProp->IterNextValue(&iter, &obj, &flags))
   {
      if (OBJ_IS_CONCRETE(obj) && (flags & kKillOnComplete))
      {
         FrameAnimationConfig *cfg = ObjGetFrameAnimationConfig(obj);
         if (cfg && cfg->clamp && !cfg->bounce) // kill on wrap is set
            if (rendobj_bitmap_retire(obj))  // and we can detect the case
            {
#ifdef NEW_NETWORK_ENABLED
               AutoAppIPtr(ObjectNetworking);
               // In multiplayer, all of the machines will be running this
               // destroy in parallel; only the object's owner should actually
               // destroy it.
               if (!pObjectNetworking->ObjIsProxy(obj))
                  DestroyObject(obj);
#else
               DestroyObject(obj);
#endif
            }
      }
   }
   BitmapFlagsProp->IterStop(&iter);
}

////////////////////////////////////////////////////////////
// JOINT POSITIONS
//

class cJointPosOps : public cClassDataOps<sJointPos>
{
};

class cJointPosStore : public cSparseHashPropertyStore<cJointPosOps>
{

};

class cJointPosProperty : public cSpecificProperty<IJointPosProperty,&IID_IJointPosProperty,sJointPos*,cJointPosStore>
{
   typedef cSpecificProperty<IJointPosProperty,&IID_IJointPosProperty,sJointPos*,cJointPosStore> cParent;

public:
   cJointPosProperty(const sPropertyDesc* desc)
      : cParent(desc)
   {

   }

   STANDARD_DESCRIBE_TYPE(sJointPos);

};

// @TBD (justin 10-14-98): Currently, this property is being set locally
// on each machine. This isn't ideal -- we would like at least major
// changes to be reflected the same on all machines. However, this property
// is currently being changed for every tweq joint, every frame, causing
// huge numbers of network messages. We need to make this subtle, so that
// only real, important state changes get sent.
static sPropertyDesc jointPosPropDesc =
{
   PROP_JOINTPOS_TYPE_NAME,
   kPropertyNoInherit|kPropertyInstantiate,
   NULL, // constraints
   0,0, // version
   { "Shape", "Joint Positions" },
   kPropertyChangeLocally,
};

// structure descriptor fun
static sFieldDesc joint_fields [] =
{
   { "Joint 1", kFieldTypeFloat, FieldLocation(sJointPos,el[0]) },
   { "Joint 2", kFieldTypeFloat, FieldLocation(sJointPos,el[1]) },
   { "Joint 3", kFieldTypeFloat, FieldLocation(sJointPos,el[2]) },
   { "Joint 4", kFieldTypeFloat, FieldLocation(sJointPos,el[3]) },
   { "Joint 5", kFieldTypeFloat, FieldLocation(sJointPos,el[4]) },
   { "Joint 6", kFieldTypeFloat, FieldLocation(sJointPos,el[5]) }
};

static sStructDesc jointpos_struct = StructDescBuild(sJointPos,kStructFlagNone,joint_fields);

IJointPosProperty* JointPosProp = NULL;

BOOL JointPosPropInit(void)
{
   AutoAppIPtr_(StructDescTools,pTools);
   pTools->Register(&jointpos_struct);

   JointPosProp=new cJointPosProperty(&jointPosPropDesc);
   return TRUE;
}

static float null_parms[MAX_REND_JOINTS];

float *ObjJointPos(ObjID obj)
{
   Assert_(JointPosProp);
   float *result = null_parms;
   JointPosProp->Get(obj,(sJointPos **)&result);
   return result;
}

void ObjSetJointPos(ObjID obj, float *parms)
{
   Assert_(JointPosProp);
   JointPosProp->Set(obj,(sJointPos *)parms);
}




IIntProperty *g_pIsInvisibleProperty = 0;
static sPropertyDesc _g_IsInvisibleProperty =
{
  "INVISIBLE",
  0,
  NULL, 0, 0,
  { "Renderer", "Invisible" },
   kPropertyChangeLocally,  // netflags
};


//////////////////
// init

void RenderPropsInit(void)
{
   RefProp = new cRefProp(&refPropDesc);
   RendTypePropInit();
   JointPosPropInit();
   AlphaRenderPropInit();
   SelfIllumPropInit();
   ObjShadPropInit();
   ParticleGroupPropInit();
   FASPropInit();
   RendFlashPropInit();
   BitmapWorldspacePropInit();
   BitmapAnimationPropInit();

   g_pIsInvisibleProperty = CreateIntProperty(&_g_IsInvisibleProperty,kPropertyImplSparse);
}

void RenderPropsShutdown(void)
{
   SafeRelease(g_pIsInvisibleProperty);
   SafeRelease(JointPosProp);
   SafeRelease(RenderTypeProp);
   SafeRelease(gAlphaRenderProp);
   SafeRelease(g_pObjShadowProp);
   SafeRelease(g_pSelfIlluminationProp);
   ParticleGroupPropTerm();
   SafeRelease(BitmapFlagsProp);
   FASPropShutdown();
   RendFlashPropShutdown();
}
