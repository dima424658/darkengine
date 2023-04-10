/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkaisc.h,v 1.6 2000/01/29 13:40:20 adurant Exp $
//
// AI script stuff
//
#pragma once

#ifndef __SHKAISC_H
#define __SHKAISC_H

#include <scrptmsg.h>
#include <engscrpt.h>

///////////////////////////////////////////////////////////////////////////////
//
// SERVICES
//

DECLARE_SCRIPT_SERVICE(ShockAI, 0x21b)
{
   STDMETHOD_(BOOL, Stun)(object who, string startTags, string loopTags, float sec) PURE;
   STDMETHOD_(BOOL, IsStunned)(object who) PURE;
   STDMETHOD_(BOOL, UnStun)(object who) PURE;

   STDMETHOD_(BOOL, Freeze)(object who, float sec) PURE;
   STDMETHOD_(BOOL, IsFrozen)(object who) PURE;
   STDMETHOD_(BOOL, UnFreeze)(object who) PURE;

   STDMETHOD_(void, NotifyEnterTripwire)(object who, object what) PURE;
   STDMETHOD_(void, NotifyExitTripwire)(object who, object what) PURE;
   STDMETHOD_(BOOL, ObjectLocked)(object obj) PURE;   // doesn't really belong here, but used by AI

   STDMETHOD_(void, ValidateSpawn)(object creature, object spawnMarker) PURE;  // again, not really belonging here, but whatever
};

#endif /* !__SHKAISC_H */
