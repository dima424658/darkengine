/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkinvpr.cpp,v 1.6 2000/02/19 12:27:18 toml Exp $
#include <drkinvpr.h>
#include <sdestool.h>
#include <sdesbase.h>
#include <propfac_.h>
#include <propbase.h>
#include <gamestr.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

//
// Inventory type sdesc 
//

static const char* inv_type_strings[] =
{ 
   "Junk", 
   "Item",
   "Weapon", 
};

#define NUM_STRINGS (sizeof(inv_type_strings)/sizeof(inv_type_strings[0]))

static sFieldDesc invtype_fields[] = 
{
   { "", kFieldTypeEnum, sizeof(int), 0, kFieldFlagUnsigned, 0, NUM_STRINGS, NUM_STRINGS, inv_type_strings },

}; 

static sStructDesc invtype_sdesc = StructDescBuild(eInventoryType,kStructFlagNone,invtype_fields); 

//////////////////////////////////////////////////////////////
// INVENTORY TYPE PROPERTY IMPLEMENTATION
//

class cInvTypeProp : public cGenericIntProperty
{
public:
   cInvTypeProp(const sPropertyDesc* desc, IPropertyStore* store)
      : cGenericIntProperty(desc,store)
   {
   }

   STANDARD_DESCRIBE_TYPE(eInventoryType); 
};


//
// Property Desc
//

static sPropertyDesc invtype_propdesc = 
{
   PROP_INVENTORY_TYPE_NAME,
   0, // flags
   NULL, // constraints
   1, 0,  // Version
   { "Inventory", "Type" }, 
};

////////////////////////////////////////

IIntProperty* CreateInvTypeProperty()
{
   AutoAppIPtr_(StructDescTools,pTools);
   pTools->Register(&invtype_sdesc); 

   cAutoIPtr<IPropertyStore> store (CreateGenericPropertyStore(kPropertyImplHash));
   return new cInvTypeProp(&invtype_propdesc, store); 
}

////////////////////////////////////////////////////////////
// GAME NAME PROPERTY 
// 

static sPropertyDesc gamename_pdesc = 
{
   PROP_GAME_NAME_NAME,
   0, // flags
   NULL, // constraints
   1, 0,  // Version
   { "Inventory", "Object Name" }, 
};

IStringProperty* CreateGameNameProperty()
{
   IStringProperty* prop = CreateStringProperty(&gamename_pdesc,kPropertyImplSparseHash); 
   AutoAppIPtr(GameStrings); 
   pGameStrings->RegisterProp(GAME_NAME_TABLE,prop); 
   return prop; 
}

////////////////////////////////////////////////////////////
// DESCRIPTION PROPERTY 
// 

static sPropertyDesc description_pdesc = 
{
   PROP_DESCRIPTION_NAME,
   0, // flags
   NULL, // constraints
   1, 0,  // Version
   { "Inventory", "Long Description" }, 
};

IStringProperty* CreateDescriptionProperty()
{
   IStringProperty* prop = CreateStringProperty(&description_pdesc,kPropertyImplSparseHash); 
   AutoAppIPtr(GameStrings); 
   pGameStrings->RegisterProp(DESCRIPTION_TABLE,prop); 
   return prop; 
}


////////////////////////////////////////////////////////////
// STORE PROPERTY 
//

static sPropertyDesc store_pdesc = 
{
   PROP_STORE_NAME,
   kPropertyInstantiate|kPropertyNoInherit, // flags
   NULL, // constraints
   1, 0,  // Version
   { "Inventory", "Store" }, 
};

IBoolProperty* CreateStoreProperty()
{
   return CreateBoolProperty(&store_pdesc,kPropertyImplLlist); 
}


////////////////////////////////////////////////////////////
// PRICE PROPERTY 
//

static sPropertyDesc price_pdesc = 
{
   PROP_PRICE_NAME,
   0, // flags
   NULL, // constraints
   1, 0,  // Version
   { "Inventory", "Purchase Price" }, 
};

IIntProperty* CreatePriceProperty()
{
   return CreateIntProperty(&price_pdesc,kPropertyImplSparseHash); 
}

////////////////////////////////////////////////////////////
// CYCLE ORDER PROPERTY 
//

static sPropertyDesc cycle_order_pdesc = 
{
   PROP_CYCLE_ORDER_NAME,
   0, // flags
   NULL, // constraints
   1, 0,  // Version
   { "Inventory", "Cycle Order" }, 
};

IStringProperty* CreateCycleOrderProperty()
{
   return CreateStringProperty(&cycle_order_pdesc,kPropertyImplSparseHash); 
}

////////////////////////////////////////////////////////////
// BEING TAKEN PROPERTY 
//

static sPropertyDesc being_taken_pdesc = 
{
   PROP_BEING_TAKEN_NAME,
   kPropertyNoEdit, // flags
   NULL, // constraints
   1, 0,  // Version
   { "Inventory", "Being Taken" }, 
};

IBoolProperty* CreateBeingTakenProperty()
{
   return CreateBoolProperty(&being_taken_pdesc,kPropertyImplSparseHash); 
}
