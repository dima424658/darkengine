/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <dpcganpr.h>

#include <propert_.h>
#include <dataops_.h>
#include <prophash.h>

#include <sdesc.h>
#include <sdesbase.h>

// Must be last header
#include <dbmem.h>

// Base gun description
// the all new improved property system...
// data ops
class cGunAnimDataOps: public cClassDataOps<sGunAnim>
{
};

// storage class
class cGunAnimStore: public cHashPropertyStore<cGunAnimDataOps>
{
};

// property implementation class
class cGunAnimProperty: public cSpecificProperty<IGunAnimProperty, &IID_IGunAnimProperty, sGunAnim*, cGunAnimStore>
{
   typedef cSpecificProperty<IGunAnimProperty, &IID_IGunAnimProperty, sGunAnim*, cGunAnimStore> cParent; 

public:
   cGunAnimProperty(const sPropertyDesc* desc)
      : cParent(desc)
   {
   }

   STANDARD_DESCRIBE_TYPE(sGunAnim); 

};

static char *flagBits[] = 
{
   "Target 2",
};

static sFieldDesc GunAnimFields[] = 
{
   {"Flags",     kFieldTypeBits,    FieldLocation(sGunAnim, m_jointAnim.m_params.m_flags), kFieldFlagNone, 0, 1, 1, flagBits,},
   {"Joint Num", kFieldTypeInt,     FieldLocation(sGunAnim, m_jointAnim.m_num),},
   {"Rate 1",    kFieldTypeFloat,   FieldLocation(sGunAnim, m_jointAnim.m_params.m_rate1),},
   {"Rate 2",    kFieldTypeFloat,   FieldLocation(sGunAnim, m_jointAnim.m_params.m_rate2),},
   {"Target 1",  kFieldTypeFloat,   FieldLocation(sGunAnim, m_jointAnim.m_params.m_target1),},
   {"Target 2",  kFieldTypeFloat,   FieldLocation(sGunAnim, m_jointAnim.m_params.m_target2),},
};

static sStructDesc GunAnimStructDesc = 
   StructDescBuild(sGunAnim, kStructFlagNone, GunAnimFields);

IGunAnimProperty *CreateGunAnimProperty(sPropertyDesc *desc, ePropertyImpl impl)
{
   StructDescRegister(&GunAnimStructDesc);
   return new cGunAnimProperty(desc);
}

static sPropertyDesc gunAnimPostDesc = 
{
   PROP_GUN_ANIM_POST_NAME, 
   0,
   NULL, 
   1,    // version
   0,
   {"Gun", "Gun Anim Post"},
};

static sPropertyDesc gunAnimPreDesc = 
{
   PROP_GUN_ANIM_PRE_NAME, 
   0,
   NULL, 
   1,    // version
   0,
   {"Gun", "Gun Anim Pre"},
};

IGunAnimProperty *g_gunAnimPreProperty;
IGunAnimProperty *g_gunAnimPostProperty;

void GunAnimPropertyInit()
{
   g_gunAnimPreProperty = CreateGunAnimProperty(&gunAnimPreDesc, kPropertyImplSparse);
   g_gunAnimPostProperty = CreateGunAnimProperty(&gunAnimPostDesc, kPropertyImplSparse);
}

void GunAnimPropertyTerm(void)
{
   SafeRelease(g_gunAnimPreProperty);
   SafeRelease(g_gunAnimPostProperty);
}

