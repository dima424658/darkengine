/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/textarch.cpp,v 1.12 1998/11/10 23:06:31 mahk Exp $
#include <textarch.h>

#include <string.h>

#include <lg.h>
#include <mprintf.h>

#include <resapi.h>
#include <imgrstyp.h>
#include <texmem.h>
#include <resdata.h>
#include <traitman.h>
#include <traitbas.h>
#include <objquery.h>
#include <iobjsys.h>
#include <appagg.h>
#include <dynarray.h>

#include <rendprop.h>
#include <editobj.h>

#include <propface.h>
#include <propfac_.h>
#include <propbase.h>
#include <str.h>

// Must be last header 
#include <dbmem.h>


#define TEXROOT "Texture" 


ObjID gTexRoot = OBJ_NULL; 
static IObjectSystem* gpObjSys = NULL; 
static ITraitManager* gpTraitMan = NULL;


//
// The mapping table
// 


static ObjID gArchMap[TEXMEM_MAX]; 

////////////////////////////////////////

static IIntProperty* gTextIDProp = NULL; 

class cTextIDProp: public cGenericIntProperty
{
public:
   cTextIDProp(const sPropertyDesc* desc)
      :cGenericIntProperty(desc,kPropertyImplHash)
   {
   }

}; 

static sPropertyDesc id_desc = 
{
   "TextureID",
   kPropertyTransient, // flags
   NULL, // constraints,
   0, 0, // version
   {
      "Texture",
      "Index",
   }
};

static void create_textid_prop(void)
{
   gTextIDProp = new cTextIDProp(&id_desc);  
}



////////////////////////////////////////

void ClearTextureArchetypeMapping(void)
{
   memset(gArchMap,0,sizeof(gArchMap)); 
}

////////////////////////////////////////

#define TEX_NAME_PREFIX "t_"

static cStr get_tex_name(IRes* pRes)
{
   char buf[256]; 
   char *pCanonPathName;

   strcpy(buf,TEX_NAME_PREFIX); 
   pRes->GetCanonPathName(&pCanonPathName);
   strcat(buf, pCanonPathName);
   Free(pCanonPathName);

         // The resource system returns backslashes, but all our old
         // data uses forward slashes. So switch it around:
   char *p;
   for (p = buf;
        *p != '\0';
        p++)
   {
      if (*p == '\\') {
         *p = '/';
      }
   }
   return buf; 
}

////////////////////////////////////////

ObjID BuildOneTextureArchetype(int i)
{
   IRes *pRes = texmemGetHandle(i);
   if (pRes)
   {
      cStr name = get_tex_name(pRes); 

      ObjID arch = gpObjSys->GetObjectNamed(name); 
      if (arch == OBJ_NULL)
         arch = gpTraitMan->CreateArchetype(name,gTexRoot); 
               
      // add arch to archetype map 
      gArchMap[i] = arch; 
      return arch; 
   }
   return OBJ_NULL;
}

ObjID ForciblyRebuildOneTextureArchetype(int i)
{
   if (i >= BACKHACK_IDX) 
      return OBJ_NULL; 

   if (OBJ_IS_CONCRETE(gArchMap[i]))
   {
      gpObjSys->Destroy(gArchMap[i]);
      gArchMap[i] = OBJ_NULL; 
   }

   return BuildOneTextureArchetype(i); 
}


////////////////////////////////////////

#define SKY_TEX_MIN (BACKHACK_IDX + 1)
#define SKY_TEX_LIM 256

void BuildTextureArchetypes(void)
{
   // refresh texroot, just in case 
   gTexRoot = gpObjSys->GetObjectNamed(TEXROOT); 

   for (int i = 0; i < texmemGetMax(); i++)
      BuildOneTextureArchetype(i); 
   BuildOneTextureArchetype(WATERIN_IDX); 
   BuildOneTextureArchetype(WATEROUT_IDX); 
}

static void rebuild_one_passively(int i)
{
   if (gArchMap[i] == OBJ_NULL)
      BuildOneTextureArchetype(i); 

}

void RebuildTextureArchetypes(void)
{
   int n = texmemGetMax(); 
   if (n <= 0 || !gpObjSys) 
      return; 
   // refresh texroot, just in case 
   gTexRoot = gpObjSys->GetObjectNamed(TEXROOT); 

   for (int i = 0; i < n; i++)
      rebuild_one_passively(i); 
   rebuild_one_passively(WATERIN_IDX);
   rebuild_one_passively(WATEROUT_IDX);
}



////////////////////////////////////////

ObjID GetTextureObj(int idx)
{
   if (idx < 0 || idx >= BACKHACK_IDX)
      return OBJ_NULL;

   if (gArchMap[idx] == OBJ_NULL)
      RebuildTextureArchetypes(); 

   ObjID obj = gArchMap[idx]; 
   
   if (OBJ_IS_ABSTRACT(obj))  // lazy instantiate
   {
      obj = gpObjSys->Create(obj,kObjectConcrete);
      gpObjSys->SetObjTransience(obj,TRUE); 

      char buf[32]; 
      sprintf(buf,"Texture %d",idx); 
      gpObjSys->NameObject(obj,buf); 

      gArchMap[idx] = obj; 
      gTextIDProp->Set(obj,idx); 
   }
   return obj; 
}

////////////////////////////////////////

ObjID GetTextureObjNamed(const char* respath, const char* resname)
{
   // Use resman to canonicalize the name 
   AutoAppIPtr(ResMan); 
   cAutoIPtr<IRes> pRes = pResMan->Lookup(resname,RESTYPE_IMAGE,respath); 
   
   return GetResTextureObj(pRes); 
}

ObjID GetResTextureObj(IRes* pRes)
{
   if (!pRes)  
      return OBJ_NULL; 

   // Check the app data to see if we're a terrain texture, in which case 
   // we have to go through the mapping table
   DWORD adata = pRes->GetAppData(); 
   if (adata != NO_RES_APP_DATA)
   {
      return GetTextureObj(GetResTexIdx(pRes)); 
   }

   // Find the archetype with our name, or create it. 
   cStr name = get_tex_name(pRes); 
   ObjID arch = gpObjSys->GetObjectNamed(name); 
   BOOL create = FALSE; 

   if (arch == OBJ_NULL)
   {
      arch = gpTraitMan->CreateArchetype(name,gTexRoot); 
      create = TRUE; 
   }

   // If we didn't create the archetype, there might be a 
   // concrete that we can just use...

   if (!create)
   {
      cAutoIPtr<IObjectQuery> query ( gpTraitMan->Query(arch,kTraitQueryDescendents)); 
      for (; !query->Done(); query->Next())
      {
         ObjID obj = query->Object(); 
         if (OBJ_IS_CONCRETE(obj))
            return obj; 
      }
   }

   // 
   // No eligible concrete, we have to make it.
   //

   ObjID obj = gpObjSys->BeginCreate(arch,kObjectConcrete);
   gpObjSys->SetObjTransience(obj,TRUE); 
   ObjSetHasRefs(obj,FALSE); 
   gTextIDProp->Set(obj,0); 
   gpObjSys->EndCreate(obj); 

   return obj; 
}; 

////////////////////////////////////////

BOOL IsTextureObj(ObjID obj)
{
   if (gTextIDProp->IsRelevant(obj))
      return TRUE; 
   return gpTraitMan->ObjHasDonor(obj,gTexRoot); 
}

////////////////////////////////////////

const char* GetTextureObjName(ObjID obj)
{
   // we suspect this will only go up one level.
   while (OBJ_IS_CONCRETE(obj))
      obj = gpTraitMan->GetArchetype(obj); 

   const char* name = gpObjSys->GetName(obj); 
   if (!name) return NULL; 
   
   if (strncmp(name,TEX_NAME_PREFIX,strlen(TEX_NAME_PREFIX)) != 0)
      return NULL; 

   name += strlen(TEX_NAME_PREFIX); 
   
   return name; 
}


////////////////////////////////////////

int GetObjTextureIdx(ObjID obj)
{
   int idx = -1; 
   gTextIDProp->Get(obj,&idx); 
   return idx; 
}

////////////////////////////////////////

void TextureArchetypesInit(void)
{
   create_textid_prop(); 
   gpObjSys = AppGetObj(IObjectSystem); 
   gpTraitMan = AppGetObj(ITraitManager); 
}

void TextureArchetypesTerm(void)
{
   SafeRelease(gTextIDProp); 
   SafeRelease(gpObjSys); 
   SafeRelease(gpTraitMan); 
}

void TextureArchetypesPostLoad(void)
{
   AutoAppIPtr_(TraitManager,pTraitMan); 
   pTraitMan->CreateBaseArchetype(TEXROOT,&gTexRoot);
   pTraitMan->SetArchetype(gTexRoot,OBJ_NULL); // detach from tree
   ObjSetHasRefs(gTexRoot,FALSE);
#ifdef EDITOR
   ObjSetCanHaveBrush(gTexRoot,FALSE); 
#endif 

}

