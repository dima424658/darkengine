/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/pickscrs.cpp,v 1.2 2000/02/19 13:09:03 toml Exp $
//

#include <lg.h>

// script headers
#include <scrptapi.h>
#include <scrptbas.h>
#include <scrptsrv.h>
#include <objscrpt.h>

// pick systems
#include <pickbase.h>
#include <picklock.h>
#include <pickscrs.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

DECLARE_SCRIPT_SERVICE_IMPL(cPickLockSrv, PickLock)
{
public:
   // ready the picks
   STDMETHOD_(BOOL,Ready)(object picker, object pick_obj)
   {
      return PickLockReady(ScriptObjID(picker),ScriptObjID(pick_obj));
   }

   // unready the picks
   STDMETHOD_(BOOL,UnReady)(object picker, object pick_obj)
   {
      return PickLockUnReady(ScriptObjID(picker),ScriptObjID(pick_obj));
   }
   
   // Start Picking a Lock
   STDMETHOD_(BOOL,StartPicking)(object picker, object pick_obj, object locked_obj)
   {
      return PickLockStartPicking(ScriptObjID(picker),ScriptObjID(pick_obj),ScriptObjID(locked_obj));
   }

   // Stop Picking a Lock
   STDMETHOD_(BOOL,FinishPicking)(object pick_obj)
   {
      ObjID pick=ScriptObjID(pick_obj);
      return PickLockFinishPicking(pick);
   }

   // check if the pick_obj opens stage "stage" of the locked_obj
   STDMETHOD_(BOOL,CheckPick)(object pick_obj, object locked_obj, int stage)
   {
      return PickLockMatchLevel(ScriptObjID(pick_obj), ScriptObjID(locked_obj), stage);
   }

   STDMETHOD_(BOOL,DirectMotion)(BOOL start)
   {
      return PickLockMotionOnly(start);
   }
};

IMPLEMENT_SCRIPT_SERVICE_IMPL(cPickLockSrv, PickLock);
