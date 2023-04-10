/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/physics/collide.cpp,v 1.43 2000/02/19 12:32:13 toml Exp $

// basic collision stuff

#include <collide.h>

#include <collprop.h>
#include <partprop.h>
#include <physapi.h>
#include <sndgen.h>

#include <dmgmodel.h>
#include <iobjsys.h>
#include <dmgbase.h>
#include <appagg.h>
#include <globalid.h>

#include <phclsn.H>
#include <phcollev.h>

#include <textarch.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

static BOOL is_terrain(ObjID obj)
{
   return GetObjTextureIdx(obj) >= 0; 
}

IDamageModel* damage_model()
{
   static IDamageModel* dam = NULL;
   if (dam == NULL)
   {
      dam = AppGetObj(IDamageModel);
   }
   return dam;
}

IObjectSystem* object_system()
{
   static IObjectSystem* objsys = NULL;
   if (objsys == NULL)
      objsys = AppGetObj(IObjectSystem);
   return objsys;
}

#define SOUND_BITS (COLLISION_NO_SOUND_US|COLLISION_NO_SOUND_THEM)


// Apply a collision result to an object 
static int apply_result(ObjID us, ObjID them, int bits, sChainedEvent* ev)
{
   int rv = 0; 
   if (bits & COLLISION_SLAY)
   {
      eDamageResult result = damage_model()->SlayObject(us,them,ev);
      if (result >= kDamageSlay)
         rv |= COLLISION_SLAY;
   }

   if (bits & COLLISION_KILL)
   {
      object_system()->Destroy(us); 
      rv |= COLLISION_KILL;
   }

   if (!PhysObjHasPhysics(us))
      rv |= COLLISION_NON_PHYS;

   return rv; 

}

// returns flags of what it killed
int CollideEventRaw(ObjID us, ObjID them, int bits, float mag, cPhysClsn* clsn)
{
   sImpact impact = { mag}; 
   int rv=0;

   int type1 = 0, type2 = 0;

   if (!ObjGetCollisionType(us, &type1))
      type1 = COLLISION_NONE;
   if (!ObjGetCollisionType(them, &type2))
      type2 = COLLISION_NONE;

   if (type1 & COLLISION_NO_SOUND)
      rv |= COLLISION_NO_SOUND_US;
   if (type2 & COLLISION_NO_SOUND)
      rv |= COLLISION_NO_SOUND_THEM;

   sPhysClsnEvent event(clsn); 

   eDamageResult result;
   result = damage_model()->HandleImpact(us,them,&impact,&event);
   if (result > kDamageSlay)
      rv |= COLLISION_SLAY_US;

   // By convention, the terrain is always second 
   if (!is_terrain(them))
   {
      result = damage_model()->HandleImpact(them,us,&impact,&event);
      if (result >= kDamageSlay)
         rv |= COLLISION_SLAY_THEM;
   }

   rv |= apply_result(us,them,bits >> CF_US_SHF,&event)     << CF_US_SHF; 
   rv |= apply_result(them,us,bits >> CF_THEM_SHF,&event)   << CF_THEM_SHF; 

   // terrain has no physics model
   if (is_terrain(them) || (them == OBJ_NULL))
      rv &= ~COLLISION_NON_PHYS_THEM;

   return rv;
}


// returns flags of what it killed
int CollideEvent(ObjID obj1, ObjID obj2, int bits, float mag, cPhysClsn* coll)
{
#ifdef NETWORK_ENABLED
   if ((bits & COLLISION_LOCAL_ONLY) == 0)
      SendCollidePacket(obj1, obj2, bits, data);
#endif

   if (bits != COLLISION_NONE)
   {
      if ((bits & SOUND_BITS) != SOUND_BITS)
         CollisionSoundObjects(obj1, obj2, bits, mag);
   }

   int rv = CollideEventRaw(obj1, obj2, bits, mag, coll);


   return rv;
}


int CollideTest(ObjID obj1, ObjID obj2)
{
   int type1, type2; 
   if (!ObjGetCollisionType(obj1, &type1))
      type1 = COLLISION_NONE;
   if (!ObjGetCollisionType(obj2, &type2))
      type2 = COLLISION_NONE;

   return (type1 << CF_US_SHF) | (type2 << CF_THEM_SHF);
}





