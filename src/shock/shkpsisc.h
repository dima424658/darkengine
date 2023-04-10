/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkpsisc.h,v 1.4 2000/01/31 09:58:53 adurant Exp $
//
// psi script stuff
//
#pragma once

#ifndef __SHKPSSCR_H
#define __SHKPSSCR_H

#include <scrptmsg.h>
#include <simtime.h>
#include <engscrpt.h>

// argh
enum ePsiPowers;

///////////////////////////////////////////////////////////////////////////////
//
// SERVICES
//

///////////////////////////////////////
//
// Game system and interface support functions
//

DECLARE_SCRIPT_SERVICE(ShockPsi, 0x1d7)
{
   STDMETHOD(OnDeactivate)(ePsiPowers power) PURE;
   STDMETHOD_(tSimTime, GetActiveTime)(ePsiPowers power) PURE;
   STDMETHOD_(BOOL, IsOverloaded)(ePsiPowers power) PURE;
};

#endif /* !__SHKPSSCR_H */
