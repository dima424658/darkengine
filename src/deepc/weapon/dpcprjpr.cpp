/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <dpcprjpr.h>

#include <propert_.h>
#include <dataops_.h>
#include <prophash.h>

#include <sdesc.h>
#include <sdesbase.h>

// must be last header
#include <dbmem.h>

// data ops
class cProjectileDataOps: public cClassDataOps<sProjectile>
{
};

// storage class
class cProjectileStore: public cHashPropertyStore<cProjectileDataOps>
{
};

// property implementation class
class cProjectileProperty: public cSpecificProperty<IProjectileProperty, &IID_IProjectileProperty, sProjectile*, cProjectileStore>
{
   typedef cSpecificProperty<IProjectileProperty, &IID_IProjectileProperty, sProjectile*, cProjectileStore> cParent; 

public:
   cProjectileProperty(const sPropertyDesc* desc)
      : cParent(desc)
   {
   }

   STANDARD_DESCRIBE_TYPE(sProjectile); 

};

IProjectileProperty *g_pProjectileProperty;

static sFieldDesc ProjectileFields[] = 
{
   {"Spray", kFieldTypeInt, FieldLocation(sProjectile, m_spray),},
   {"Spread", kFieldTypeInt, FieldLocation(sProjectile, m_spread),kFieldFlagHex,},
};

static sStructDesc ProjectileStructDesc = 
   StructDescBuild(sProjectile, kStructFlagNone, ProjectileFields);

IProjectileProperty *CreateProjectileProperty(sPropertyDesc *desc, 
                                                          ePropertyImpl impl)
{
   StructDescRegister(&ProjectileStructDesc);
   return new cProjectileProperty(desc);
}

static sPropertyDesc ProjectileDesc = 
{
   PROP_PROJECTILE_DESC, 
   0,
   NULL, 
   1,    // version
   0,
   {"Gun", "Projectile Description"},
};

void ProjectilePropertyInit()
{
   g_pProjectileProperty = CreateProjectileProperty(&ProjectileDesc, kPropertyImplSparse);
}

BOOL ProjectileGet(ObjID objID, sProjectile **ppProjectile)
{
   return g_pProjectileProperty->Get(objID, ppProjectile);
}
