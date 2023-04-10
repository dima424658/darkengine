/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkarmpr.cpp,v 1.4 2000/02/19 12:36:35 toml Exp $

#include <shkarmpr.h>

#include <propert_.h>
#include <dataops_.h>
#include <prophash.h>

#include <sdesc.h>
#include <sdesbase.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

/////////////////////////////////////////////////////////////
// Armor values
/////////////////////////////////////////////////////////////

// the all new improved property system...
// data ops
class cArmorDataOps: public cClassDataOps<sArmor>
{
};

// storage class
class cArmorStore: public cHashPropertyStore<cArmorDataOps>
{
};

// property implementation class
class cArmorProp: public cSpecificProperty<IArmorProperty, &IID_IArmorProperty, sArmor*, cArmorStore>
{
   typedef cSpecificProperty<IArmorProperty, &IID_IArmorProperty, sArmor*, cArmorStore> cParent; 

public:
   cArmorProp(const sPropertyDesc* desc)
      : cParent(desc)
   {
   }

   STANDARD_DESCRIBE_TYPE(sArmor); 

};

IArmorProperty *g_ArmorProperty;

static sFieldDesc ArmorFields[] = 
{
   {"Combat", kFieldTypeFloat, FieldLocation(sArmor, m_combat),},
   {"Toxic", kFieldTypeFloat, FieldLocation(sArmor, m_toxic),},
   {"Radiation", kFieldTypeFloat, FieldLocation(sArmor, m_radiation),},
};

static sStructDesc ArmorStructDesc = 
   StructDescBuild(sArmor, kStructFlagNone, ArmorFields);

IArmorProperty *CreateArmorProperty(sPropertyDesc *desc, ePropertyImpl impl)
{
   StructDescRegister(&ArmorStructDesc);
   return new cArmorProp(desc);
}

static sPropertyDesc ArmorDesc = 
{
   PROP_Armor, 0,
   NULL, 0, 0,
   {"GameSys", "Armor"},
};

void ArmorPropertyInit()
{
   g_ArmorProperty = CreateArmorProperty(&ArmorDesc, kPropertyImplDense);
}

void ArmorPropertyTerm()
{
   SafeRelease(g_ArmorProperty);
}

BOOL ArmorGet(ObjID objID, sArmor **ppArmor)
{
   return g_ArmorProperty->Get(objID, ppArmor);
}


