/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/lockscpt.h,v 1.2 1998/01/19 22:21:06 dc Exp $
#pragma once  
#ifndef __LOCKSCPT_H
#define __LOCKSCPT_H

#include <objscrpt.h>

///////////////////////////////////////
//
// Property system service 
//

DECLARE_SCRIPT_SERVICE(Locked, 0xfb)
{
   STDMETHOD_(BOOL,IsLocked)(const object &obj) PURE;
};

#define SCR_LOCK_MSG   "NowLocked"
#define SCR_UNLOCK_MSG "NowUnlocked"

#ifdef SCRIPT
#define OnNowLocked()           SCRIPT_MESSAGE_HANDLER_SIGNATURE(NowLocked)
#define DefaultOnNowLocked()    SCRIPT_CALL_BASE(NowLocked)
#define OnNowUnlocked()         SCRIPT_MESSAGE_HANDLER_SIGNATURE(NowUnlocked)
#define DefaultOnNowUnlocked()  SCRIPT_CALL_BASE(NowUnlocked)
#endif

#endif  // __LOCKSCPT_H
