/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/ghostlst.h,v 1.5 2000/01/29 13:41:09 adurant Exp $
// internal data structures/lists for ghost control
#pragma once

#ifndef __GHOSTLST_H
#define __GHOSTLST_H

// note the bulk of the API is in ghostapi, as it is public
#include <ghostint.h>
#include <ghostcfg.h>

// for now, the hashes are here for use in main ghost module
EXTERN cGhostLocalHash  gGhostLocals;
EXTERN cGhostRemoteHash gGhostRemotes;

// internal API for ghost lookups within the system
EXTERN sGhostRemote *GhostGetRemote(ObjID obj);
EXTERN sGhostLocal  *GhostGetLocal(ObjID obj);

// internal API for list maintenance
EXTERN void GhostListInit(void);
EXTERN void GhostListTerm(void);
EXTERN void GhostListReset(void);

// interal setup, in case you want to build a Ghost struct in some other file (ghostlup, say)
EXTERN void GhostInitLocalStruct(sGhostLocal *pGL);
EXTERN void GhostInitRemoteStruct(sGhostRemote *pGR);

// debugging control - note flag is thrown in ghostcfg based on playtest
#ifdef GHOST_DEBUGGING
// call with any ONE of the three, if you have the pG, use that, save time
// sets up the debugging globals for ghost spew stuff
EXTERN void _GhostDebugSetup(ObjID obj, sGhostLocal *pGL, sGhostRemote *pGR);
EXTERN void _GhostPrintGhostPos(ObjID obj, sGhostHeartbeat *pGH, int flags, char *prefix, int seq_id);
#else
#define _GhostDebugSetup(o,l,r)
#define _GhostPrintGhostPos(o,pGH,f,p,s)
#endif // GHOST_DEBUGGING

#define _GhostDebugSetupLocal(pGL)  _GhostDebugSetup(OBJ_NULL,pGL,NULL)
#define _GhostDebugSetupRemote(pGR) _GhostDebugSetup(OBJ_NULL,NULL,pGR)
#define _GhostDebugSetupObj(obj)    _GhostDebugSetup(obj,NULL,NULL)

#endif // __GHOSTLST_H
