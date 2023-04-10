/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkinvsc.h,v 1.2 1998/09/29 15:06:23 CCAROLLO Exp $
// lockpick service, properties, the whole thing...

#pragma once
#ifndef __DRKINVSC_H
#define __DRKINVSC_H

#include <drkienum.h>

DECLARE_SCRIPT_SERVICE(DrkInv,0x150)
{
   // change the current inventory capabilities...   
   STDMETHOD_(void,CapabilityControl)(eDrkInvCap cap_change, eDrkInvControl control) PURE;
   
   // set the speed factor for the player (based on what they are holding)
   STDMETHOD_(void,AddSpeedControl)(const char *name, float speed_fac, float rot_fac) PURE;
   STDMETHOD_(void,RemoveSpeedControl)(const char *name) PURE;
};

#endif  // __DRKINVSC_H
