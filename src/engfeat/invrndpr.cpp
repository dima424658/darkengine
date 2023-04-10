/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/invrndpr.cpp,v 1.1 1998/05/24 19:44:08 mahk Exp $
#include <appagg.h>
#include <invrndpr.h>
#include <propert_.h>
#include <dataops_.h>
#include <propsprs.h>

#include <sdesbase.h>
#include <sdestool.h>

// include these last
#include <dbmem.h>
#include <initguid.h>
#include <invrguid.h>

//------------------------------------------------------------
// Property class
//

class cInvRenderOps : public cClassDataOps<sInvRenderType>
{
}; 

class cInvRenderStore : public cSparseHashPropertyStore<cInvRenderOps>
{
}; 


typedef cSpecificProperty<IInvRenderProperty,&IID_IInvRenderProperty,sInvRenderType*,cInvRenderStore> cBaseInvRendProp; 

class cInvRendProp : public cBaseInvRendProp
{
public:
   cInvRendProp(const sPropertyDesc* desc) : cBaseInvRendProp(desc) {}; 

   STANDARD_DESCRIBE_TYPE(sInvRenderType); 
   
}; 

//------------------------------------------------------------
// Property desc 
//

static sPropertyDesc type_pdesc = 
{
   "InvRendType",
   0, //flags
   NULL, // constraints,
   1, 1, // version
   { "Inventory", "Render Type"  },
};

//------------------------------------------------------------
// Property Sdesc 
//

static const char* type_strings[] = 
{
   "Default",
   "Alternate Model",
   "Alternate Bitmap", 
};

#define NUM_STRINGS (sizeof(type_strings)/sizeof(type_strings[0]))

static sFieldDesc type_fields[] = 
{
   { "Type", kFieldTypeEnum, FieldLocation(sInvRenderType,type), kFieldFlagUnsigned, 0, kInvRenderTypeMax, NUM_STRINGS, type_strings },
   { "Resource", kFieldTypeString, FieldLocation(sInvRenderType,resource) }, 
};


static sStructDesc type_sdesc = StructDescBuild(sInvRenderType,kStructFlagNone,type_fields); 

////////////////////////////////////////////////////////////

IInvRenderProperty* CreateInvRenderProp()
{
   AutoAppIPtr_(StructDescTools,pTools);
   pTools->Register(&type_sdesc); 

   return new cInvRendProp(&type_pdesc); 
}




