/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/particle.h,v 1.10 2000/01/31 09:53:02 adurant Exp $
#pragma once

// fake particle system for neat effects

#include <objtype.h>
#include <matrixs.h>

#define MAX_PARTICLES 128
#define PARTICLE_DURATION 5000      // in milliseconds
#define FIRE_DURATION 500

typedef struct
{
   bool active;         // active or not
   ObjID objid;         // ID in the object system
   ushort type;         // maybe we'll have different kinds of particles?
   ulong time;          // time since particle creation
   ulong duration;      // lifetime of particle
} ParticleStruct;

EXTERN void InitParticleArchetype(void);
EXTERN void ParticlesInit(void);
EXTERN bool ParticlesAdd(ObjID obj, mxs_vector pos);
EXTERN void ParticlesUpdate(ulong dt);
EXTERN void ParticlesDeleteOldest(void);
EXTERN void ParticlesDeleteAll(void);
EXTERN void ParticlesDelete(int index);
EXTERN void ParticlesDeleteFromObjID(ObjID objID);
EXTERN void ParticlesInformOfDeletion(ObjID objID);
EXTERN ushort ParticlesGetType(ObjID objID);
EXTERN ulong ParticlesGetTime(ObjID objID);

