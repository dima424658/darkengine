/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/init.h,v 1.3 2000/01/29 13:21:03 adurant Exp $
#pragma once

#ifndef __INIT_H
#define __INIT_H

#include <lg.h>
#include <comtools.h>
#include <appagg.h>
#include <looptype.h>

////////////////////////////////////////////////////////////
// Global CMDLINE args
//

EXTERN const char** g_argv;
EXTERN int g_argc; 

////////////////////////////////////////////////////////////
// Initial loopmode.  
//
EXTERN sLoopInstantiator* gPrimordialMode;


////////////////////////////////////////////////////////////
// Core Engine 
// 

EXTERN tResult LGAPI CoreEngineCreateObjects(int argc, const char* argv[]);
EXTERN tResult LGAPI CoreEngineAppExit(void);
EXTERN tResult LGAPI CoreEngineAppInit(void);

#endif // __INIT_H
