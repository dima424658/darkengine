/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/tweqexp.h,v 1.6 2000/01/31 09:45:45 adurant Exp $
// expose the tweq properties
#pragma once

#ifndef __TWEQEXP_H
#define __TWEQEXP_H

#include <tweqprop.h>

#define NUM_EXTRA_EMITTERS  4

// if you really want the properties in person
EXTERN ITweqVectorProperty       *tweq_scale_prop;
EXTERN ITweqVectorProperty       *tweq_rotate_prop;
EXTERN ITweqJointsProperty       *tweq_joints_prop;
EXTERN ITweqModelsProperty       *tweq_models_prop;
EXTERN ITweqSimpleProperty       *tweq_delete_prop;
EXTERN ITweqSimpleProperty       *tweq_flicker_prop;
EXTERN ITweqEmitterProperty      *tweq_emitter_prop;
EXTERN ITweqEmitterProperty      *tweq_extra_emitter_prop[NUM_EXTRA_EMITTERS];
EXTERN ITweqLockProperty         *tweq_lock_prop;

EXTERN ITweqVectorStateProperty  *tweq_scalestate_prop;
EXTERN ITweqVectorStateProperty  *tweq_rotatestate_prop;
EXTERN ITweqJointsStateProperty  *tweq_jointsstate_prop;
EXTERN ITweqSimpleStateProperty  *tweq_modelsstate_prop;
EXTERN ITweqSimpleStateProperty  *tweq_deletestate_prop;
EXTERN ITweqSimpleStateProperty  *tweq_flickerstate_prop;
EXTERN ITweqSimpleStateProperty  *tweq_emitterstate_prop;
EXTERN ITweqSimpleStateProperty  *tweq_extra_emitterstate_prop[NUM_EXTRA_EMITTERS];
EXTERN ITweqLockStateProperty    *tweq_lockstate_prop;

#endif  // __TWEQEXP_H
