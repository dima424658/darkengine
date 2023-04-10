/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/tanimprp.cpp,v 1.3 1998/10/31 07:17:00 mahk Exp $
#include <tanimprp.h>
#include <propbase.h>
#include <dataops_.h>
#include <proplist.h>
#include <propert_.h>

#include <sdesbase.h>
#include <sdestool.h>

// For rebuild concrete
#include <anim_txt.h>
#include <traitman.h>
#include <resapi.h>
#include <imgrstyp.h>
#include <appagg.h>
#include <textarch.h>
#include <iobjsys.h>
#include <lazyagg.h>

// Last, please
#include <dbmem.h>
#include <initguid.h>
#include <tanimiid.h>

/////////////////////////////////////////////////////////////
// SDESC
//

static const char* animtex_bitnames[] = 
{
   "Wrap",
   "Randing",
   "Reverse",
   "Portal", 
}; 

#define NUM_BITNAMES (sizeof(animtex_bitnames)/sizeof(animtex_bitnames[0]))


static sFieldDesc animtex_fields[] = 
{
   { "Rate", kFieldTypeInt, FieldLocation(sAnimTexProp,rate), kFieldFlagUnsigned }, 
   { "Flags", kFieldTypeBits, FieldLocation(sAnimTexProp,flags), kFieldFlagUnsigned, 0, NUM_BITNAMES, NUM_BITNAMES, animtex_bitnames }, 
};

static sStructDesc animtex_sdesc = StructDescBuild(sAnimTexProp,0,animtex_fields); 

static void setup_sdesc()
{
   AutoAppIPtr_(StructDescTools,pTools);
   pTools->Register(&animtex_sdesc); 
}

////////////////////////////////////////////////////////////
// PROPERTY CLASSES

static LazyAggMember(IObjectSystem) gpObjSys; 

static sAnimTexProp def_values = { 250, kAnimTexWrap }; 

struct sAnimTexOpsProp: public sAnimTexProp 
{
   sAnimTexOpsProp() { *(sAnimTexProp*)this = def_values; }; 
   sAnimTexOpsProp(const sAnimTexOpsProp& dat) { *this = dat; }; 
};

//
// Data ops 
//

class cAnimTexOps: public cClassDataOps<sAnimTexOpsProp>
{
public:
   // don't auto-clear on new 
   cAnimTexOps() : cClassDataOps<sAnimTexOpsProp>(kNoFlags) {}; 
}; 

//
// Store
//

class cAnimTexStore: public cListPropertyStore<cAnimTexOps>
{
}; 

//
// Property 
//

typedef cSpecificProperty<IAnimTexProperty,&IID_IAnimTexProperty,sAnimTexProp*,cAnimTexStore> cAnimTexPropBase; 
 
class cAnimTexProp: public cAnimTexPropBase
{
protected:
   IResMan* mpResMan; 
   
public: 
   cAnimTexProp(const sPropertyDesc* desc) 
      : cAnimTexPropBase(desc),
        mpResMan(AppGetObj(IResMan))
   { 
      SetRebuildConcretes(TRUE); 
   }; 

   ~cAnimTexProp()
   {
      SafeRelease(mpResMan); 
   }

   STANDARD_DESCRIBE_TYPE(sAnimTexProp); 

   void RebuildConcrete(ObjID obj, BOOL fIsRelevant, uPropListenerValue val, ObjID donor)
   {
      sAnimTexProp* prop = (fIsRelevant) ? (sAnimTexProp*)val.ptrval : &def_values;
      if (!fIsRelevant && !gpObjSys->Exists(obj))
         return ;
      const char* name = GetTextureObjName(obj); 
      IRes* res = mpResMan->Lookup(name,RESTYPE_IMAGE,NULL); 
      ectsAnimRefresh(res,prop); 
      SafeRelease(res); 
   }
}; 

////////////////////////////////////////////////////////////
// PROPERTY FACTORY 
//

//
// Descriptor
//

static sPropertyDesc texprop_desc = 
{
   PROP_ANIM_TEX_NAME,
   0, // flags
   NULL, // constraints
   0, 0, // version
   {  "Renderer", "Texture Anim Data" }, 
}; 

IAnimTexProperty* CreateAnimTexProperty(void)
{
   setup_sdesc(); 
   return new cAnimTexProp(&texprop_desc); 
}

