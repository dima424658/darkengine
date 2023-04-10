/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/pickscrs.h,v 1.1 1998/02/17 21:33:32 dc Exp $
// lockpick service, properties, the whole thing...

#pragma once
#ifndef __PICKSCRS_H
#define __PICKSCRS_H

DECLARE_SCRIPT_SERVICE(PickLock,0x111)
{
   // ready/unready the picks
   STDMETHOD_(BOOL,Ready)(object picker, object pick_obj) PURE;
   STDMETHOD_(BOOL,UnReady)(object picker, object pick_obj) PURE;
   
   // Start Picking a Lock
   STDMETHOD_(BOOL,StartPicking)(object picker, object pick_obj, object locked_obj) PURE;

   // Stop Picking a Lock
   STDMETHOD_(BOOL,FinishPicking)(object pick_obj) PURE;

   // check if the pick_obj opens stage "stage" of the locked_obj
   STDMETHOD_(BOOL,CheckPick)(object pick_obj, object locked_obj, int stage) PURE;

   STDMETHOD_(BOOL,DirectMotion)(BOOL start) PURE;
};

#endif  // __PICKSCRS_H
