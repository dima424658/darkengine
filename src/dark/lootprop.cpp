/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/lootprop.cpp,v 1.2 1998/08/25 18:52:57 mahk Exp $
#include <lootprop.h>
#include <propert_.h>
#include <propsto_.h>
#include <propsprs.h>

#include <appagg.h>
#include <sdesbase.h>
#include <sdestool.h>

// include these last
#include <dbmem.h>
#include <initguid.h>
#include <lootiid.h>

//------------------------------------------------------------
// LOOT PROPERTY CLASSES
//

class cLootOps : public cClassDataOps<sLootProp> {}; 

class cLootStore: public cSparseHashPropertyStore<cLootOps> {}; 

typedef cSpecificProperty<ILootProperty,&IID_ILootProperty,sLootProp*,cLootStore> cBaseLootProp; 

class cLootProp: public cBaseLootProp
{
   
public:
   cLootProp(const sPropertyDesc* desc) : cBaseLootProp(desc) {}; 

   STANDARD_DESCRIBE_TYPE(sLootProp); 
};

//------------------------------------------------------------
// PROPERTY SDESC
//

static const char* special_bits[] = 
{
   "Special 1", 
   "Special 2", 
   "Special 3", 
   "Special 4", 
   "Special 5", 
   "Special 6", 
   "Special 7", 
   "Special 8", 
}; 

static sFieldDesc loot_fields[] = 
{
   { "Gold", kFieldTypeInt, FieldLocation(sLootProp,count[0]) },
   { "Gems", kFieldTypeInt, FieldLocation(sLootProp,count[1]) },
   { "Art",  kFieldTypeInt, FieldLocation(sLootProp,count[2]) },
   { "Special",  kFieldTypeBits, FieldLocation(sLootProp,count[3]), FullFieldNames(special_bits) },
   
}; 

static sStructDesc loot_sdesc = StructDescBuild(sLootProp,kStructFlagNone,loot_fields); 

//------------------------------------------------------------
// PROPERTY DESC AND FACTORY 
//

static sPropertyDesc loot_pdesc = 
{
   "Loot",
   0,  // Flags
   NULL, // Constraints
   0, 0, // version
   { "Dark Gamesys", "Loot" }
}; 

ILootProperty* InitLootProp(void)
{
   AutoAppIPtr_(StructDescTools,pTools); 
   pTools->Register(&loot_sdesc); 
   return new cLootProp(&loot_pdesc); 
}
