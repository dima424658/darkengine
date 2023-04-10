/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkobjsc.h,v 1.2 2000/01/31 09:58:12 adurant Exp $
//
// Object script stuff
//
#pragma once

#ifndef __SHKOBJSC_H
#define __SHKOBJSC_H

#include <scrptmsg.h>

#include <engscrpt.h>

///////////////////////////////////////////////////////////////////////////////
//
// SERVICES
//

///////////////////////////////////////
//
// Game system and interface support functions
//

DECLARE_SCRIPT_SERVICE(ShockObj, 0x1d9)
{
   STDMETHOD_(ObjID, FindScriptDonor)(ObjID objID, string name) PURE;
};

#endif /* !__SHKOBJSC_H */
