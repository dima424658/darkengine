/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/render/pgroup.h,v 1.6 2000/01/31 09:53:05 adurant Exp $
//
// Particle group header
//
#pragma once

#ifndef __PGROUP_H
#define __PGROUP_H

////////////////////

EXTERN void ParticleGroupRender(ObjID obj);
EXTERN void ParticleGroupRenderAroundObjectStart(ObjID obj, float z);
EXTERN void ParticleGroupRenderAroundObject(ObjID obj, BOOL in_front);
EXTERN void ParticleGroupRenderAroundObjectEnd(ObjID obj);
EXTERN void ParticleGroupUpdateModeStart(void); 
EXTERN void ParticleGroupUpdateModeFinish(void); 

EXTERN BOOL ParticleGroupTranslucent(ObjID obj);

////////////////////

EXTERN void ParticleGroupCleanup();

EXTERN void particle_group_delete(ObjID obj);
EXTERN void particle_group_update(ObjID obj);
EXTERN BOOL ParticleGroupSim(ObjID obj);
EXTERN void ParticleGroupUpdateMode(ObjID obj);
EXTERN BOOL ParticleGroupShutdown(ObjID obj);

////////////////////

#endif
