/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/picklock.h,v 1.6 2000/01/29 20:22:08 adurant Exp $
// lockpick service, properties, the whole thing...

#pragma once
#ifndef __PICKLOCK_H
#define __PICKLOCK_H

#include <objtype.h>
#include <property.h>
#include <pickbase.h>

#undef INTERFACE
#define INTERFACE IPickCfgProperty
DECLARE_PROPERTY_INTERFACE(IPickCfgProperty)
{
   DECLARE_UNKNOWN_PURE(); 
   DECLARE_PROPERTY_PURE(); 
   DECLARE_PROPERTY_ACCESSORS(sPickCfg*); 
}; 

#undef INTERFACE

#define INTERFACE IAdvPickTransCfgProperty
DECLARE_PROPERTY_INTERFACE(IAdvPickTransCfgProperty)
{
  DECLARE_UNKNOWN_PURE();
  DECLARE_PROPERTY_PURE();
  DECLARE_PROPERTY_ACCESSORS(sAdvPickTransCfg*);
};
#undef INTERFACE

#define INTERFACE IAdvPickStateCfgProperty
DECLARE_PROPERTY_INTERFACE(IAdvPickStateCfgProperty)
{
  DECLARE_UNKNOWN_PURE();
  DECLARE_PROPERTY_PURE();
  DECLARE_PROPERTY_ACCESSORS(sAdvPickStateCfg*);
};
#undef INTERFACE

#define INTERFACE IAdvPickSoundCfgProperty
DECLARE_PROPERTY_INTERFACE(IAdvPickSoundCfgProperty)
{
  DECLARE_UNKNOWN_PURE();
  DECLARE_PROPERTY_PURE();
  DECLARE_PROPERTY_ACCESSORS(sAdvPickSoundCfg*);
};
#undef INTERFACE

#define INTERFACE IPickStateProperty
DECLARE_PROPERTY_INTERFACE(IPickStateProperty)
{
   DECLARE_UNKNOWN_PURE(); 
   DECLARE_PROPERTY_PURE(); 
   DECLARE_PROPERTY_ACCESSORS(sPickState*); 
}; 

#define PROP_PICKSRC_NAME   "PickSrc"
#define PROP_PICKCFG_NAME   "PickCfg"
#define PROP_PICKSTATE_NAME "PickState"
#define PROP_ADVPICKTRANSCFG_NAME "AdvPickTransCfg"
#define PROP_ADVPICKSTATECFG_NAME "AdvPickStateCfg"
#define PROP_ADVPICKSOUNDCFG_NAME "AdvPickSoundCfg"

// do player hand motion setup/other master control for pick readiness
EXTERN BOOL PickLockReady(ObjID frobber, ObjID pick_obj);
EXTERN BOOL PickLockUnReady(ObjID frobber, ObjID pick_obj);

// fancy, doug-style, hold down, all inclusive actually do some picking, eh?
EXTERN BOOL PickLockStartPicking(ObjID frobber, ObjID pick_obj, ObjID locked_obj);
EXTERN BOOL PickLockFinishPicking(ObjID pick_obj);

// for stats, really
EXTERN int PickLockCountPickableLockedDoors(void);

// for doing a rollyourown incrementally sort of picking
// does this pick open stage stage of the lock?
EXTERN ePickResult PickLockMatchLevel(ObjID pick, ObjID lock, int stage);
// just start or stop the motion, if you are rolling your own
EXTERN BOOL PickLockMotionOnly(BOOL start);

// initialize/close down the system
EXTERN BOOL PickLockInit(void);
EXTERN BOOL PickLockTerm(void);

// since we dont use player arm anymore
EXTERN void PickLockPerFrameCallback(ulong dt);

void ResetPickState(ObjID lock);

#endif  // __PICKLOCK_H
