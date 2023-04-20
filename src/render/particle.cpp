/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/particle.cpp,v 1.42 1998/10/05 17:26:55 mahk Exp $

// fake particle system for neat effects
// these are all local effects - no network packets here!

#include <stdlib.h>
#include <particle.h>
#include <string.h>
#include <physapi.h>
#include <objsys.h>
#include <iobjsys.h>
#include <osysbase.h>
#include <objpos.h>
#include <globalid.h>
#include <collprop.h>

#include <partprop.h>
#include <objmodel.h>

#include <comtools.h>
#include <traitman.h>

#include <rnd.h>
#include <timer.h>
#include <rand.h>
#include <mprintf.h>
#include <propface.h>
#include <phprop.h>


// Must be last header 
#include <dbmem.h>


ParticleStruct particles[MAX_PARTICLES];

int next_free_particle = 0;

ObjID partArch = ROOT_ARCHETYPE;

//
// set up the particle archetype
//

#define PARTICLE_COLLISION  (COLLISION_NONE)

static const char name[] = "Particle" ;

void InitParticleArchetype(void)
{
   ObjID arch = OBJ_NULL;
   AutoAppIPtr_(TraitManager,TraitMan);
   HRESULT result = TraitMan->CreateBaseArchetype(name,&arch);
   
   if (SUCCEEDED(result) && result != S_FALSE)
   {
      // set the collision property
      ObjSetCollisionType(arch, PARTICLE_COLLISION);
      // set its particle property
      ObjSetParticle(arch);
   }
   partArch = arch;

}

//
// initialize the particle system
//

static RNDSTREAM_STD(particleStream);

void ParticlesInit()
{
   int i;

   next_free_particle = 0;

   for (i=0; i<MAX_PARTICLES; i++)
      memset(particles, 0, sizeof(ParticleStruct));

   RndSeed(&particleStream, tm_get_millisec());
}

static int ParticleRand()
{
   return Rnd(&particleStream);
}

#ifdef OLD_PHYSICS
// grav gets or'd in based on gravity param
//#define PARTICLE_CREATE_FLAGS (gocrPhysModel|gocrLocalOnly)
#define PARTICLE_PHYS_FLAGS (PHYS_LOCAL_ONLY|PHYS_GRAVITY)
#endif

//
// add a single particle at pos with an initial random velocity and
// angular velocity
//
bool ParticlesAdd(ObjID arch, mxs_vector pos)
{
   ushort type = 0;
   int    last;
   ObjID  newid;
   mxs_vector vel;
   mxs_vector angvel;
   mxs_angvec ang;
   bool retval = FALSE;
   int  flags  = kPMCF_Default;

   if (arch == OBJ_NULL)
      arch = partArch;

   // Make sure it's a valid position
   Location test_loc;
   MakeLocationFromVector(&test_loc, &pos);
   if (CellFromLoc(&test_loc) == CELL_INVALID)
      return retval;

   // angle between 0x0000 and 0xffff
   ang.tx = (fixang) (Rand() << 1);
   ang.ty = (fixang) (Rand() << 1);
   ang.tz = (fixang) (Rand() << 1);

   // velocity between -4.0 and 4.0
   vel.x = ((Rand() % 80) - 40) / 10.0;
   vel.y = ((Rand() % 80) - 40) / 10.0;
   vel.z = ((Rand() % 80) - 40) / 10.0;

   // angular velocity between -pi/2 and pi/2 (-90 and 90 degrees)
   angvel.x = ((Rand() % 314) - 157) / 100.0;
   angvel.y = ((Rand() % 314) - 157) / 100.0;
   angvel.z = ((Rand() % 314) - 157) / 100.0;

   AutoAppIPtr_(ObjectSystem,pObjSys); 

   newid = pObjSys->BeginCreate(arch, kObjectConcrete);

   if (newid != OBJ_NULL)
   {
      int model = -1;

      // set all of our fields that we care about
      ObjPosUpdate(newid,&pos,&ang);

      cPhysTypeProp typeProp;
      typeProp.type = kSphereProp;
      g_pPhysTypeProp->Set(newid, &typeProp);

      pObjSys->SetObjTransience(newid,FALSE);
      pObjSys->EndCreate(newid);

      PhysSetModLocation(newid, &pos);
      PhysSetModRotation(newid, &ang);
      PhysSetSubModRadius(newid, 0, 0);

      particles[next_free_particle].active = TRUE;
      particles[next_free_particle].objid = newid;

      particles[next_free_particle].type = type;
      particles[next_free_particle].time = 0;
      particles[next_free_particle].duration = PARTICLE_DURATION;

      // set the initial velocity and angular velocity
      PhysSetVelocity(newid, &vel);
      PhysSetRotationalVelocity(newid, &angvel);

      // advance the index to the next available particle slot
      last = next_free_particle - 1;
      if (last < 0)
         last = MAX_PARTICLES - 1;

      while (next_free_particle != last)
      {
         if (!particles[next_free_particle].active)
            break;
         else
            if (++next_free_particle == MAX_PARTICLES)
               next_free_particle = 0;
      }

      // if the particle list is full, delete the oldest one
      if (next_free_particle == last)
         ParticlesDeleteOldest();

      retval = TRUE;
   }
   else
      Warning(("ParticlesAdd - obj Create says no free objects!\n"));
   
   return retval;
}

//
// update the particle system for x milliseconds
// actual physics are done in the real physics system
//
void ParticlesUpdate(ulong ms)
{
   int i;

   for (i=0; i<MAX_PARTICLES; i++)
      if (particles[i].active)
      {
         particles[i].time += ms;

         if (particles[i].time > particles[i].duration)
            ParticlesDelete(i);
      }
}

//
// delete the oldest particle in the system
//
void ParticlesDeleteOldest()
{
   int i;
   ulong oldest = 0;
   int oldest_index = -1;

   for (i=0; i<MAX_PARTICLES; i++)
      if (particles[i].active)
         if (particles[i].time > oldest)
         {
            oldest = particles[i].time;
            oldest_index = i;
         }

   ParticlesDelete(oldest_index);
   next_free_particle = oldest_index;
}

//
// delete a single particle
//
void ParticlesDelete(int index)
{
   if ((index >= 0) && (index < MAX_PARTICLES))
   {
      AutoAppIPtr_(ObjectSystem,ObjSys);
      ObjSys->Destroy(particles[index].objid);
      memset(&particles[index], 0, sizeof(particles[index]));
   }
   else
      Warning(("ParticlesDelete - invalid index %d!\n", index));
}

//
// Delete all particles 
//

void ParticlesDeleteAll(void)
{
   int i;
   for (i = 0; i < MAX_PARTICLES; i++)
      if (particles[i].active)
         ParticlesDelete(i);
}

//
// delete a single particle given its object id (used from collisions
// in physics.c)
//
void ParticlesDeleteFromObjID(ObjID objID)
{
   int i, found = -1;

   for (i=0; i<MAX_PARTICLES; i++)
      if (particles[i].active && particles[i].objid == objID)
      {
         found = i;
         break;
      }

   if ((found >= 0) && (found < MAX_PARTICLES))
   {
      ParticlesDelete(found);
   }
   else
      Warning(("ParticlesDeleteFromObjID - objid %d invalid index %d!\n", objID, found));
}

//
// Informs the particle system that an object has been deleted, so
// it no longer should think it owns that objid
//
void ParticlesInformOfDeletion(ObjID objID)
{
   int i, found = -1;

   for (i=0; i<MAX_PARTICLES; i++)
   {
      if (particles[i].active && particles[i].objid == objID)
      {
         found = i;
         break;
      }
   }

   if ((found >= 0) && (found < MAX_PARTICLES))
      memset(&particles[found], 0, sizeof(particles[found]));
}

//
// Get a particle's type
//
ushort ParticlesGetType(ObjID objID)
{
   int i, found = -1;

   for (i=0; i<MAX_PARTICLES; i++)
      if (particles[i].objid == objID)
      {
         found = i;
         break;
      }

   if ((found >= 0) && (found < MAX_PARTICLES))
      return particles[found].type;
   else
      Warning(("ParticlesGetType - objid %d invalid index %d!\n", objID, found));

   return 0;
}

//
// Get a particle's age
//
ulong ParticlesGetTime(ObjID objID)
{
   int i, found = -1;

   for (i=0; i<MAX_PARTICLES; i++)
      if (particles[i].objid == objID)
      {
         found = i;
         break;
      }

   if ((found >= 0) && (found < MAX_PARTICLES))
      return particles[found].time;
   else
      Warning(("ParticlesGetType - objid %d invalid index %d!\n", objID, found));

   return 0;
}
