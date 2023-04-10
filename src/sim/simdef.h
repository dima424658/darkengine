/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/simdef.h,v 1.1 1998/04/20 13:41:31 mahk Exp $
#pragma once  
#ifndef __SIMDEF_H
#define __SIMDEF_H

#include <types.h>
#include <comtools.h>

////////////////////////////////////////////////////////////
// TYPE DEFINITIONS FOR SIM MANAGER 
//

//
// Message types
//

enum eSimMsg_ 
{
   kSimInit    = 1 << 0, // sim starting (first time ever for this database)
   kSimTerm    = 1 << 1, // sim finishing
   kSimSuspend = 1 << 2, // suspending (prep for save, etc) 
   kSimResume  = 1 << 3, // resuming, after suspend.
}; 



#endif // __SIMDEF_H






