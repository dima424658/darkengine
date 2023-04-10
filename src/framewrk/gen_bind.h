/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/gen_bind.h,v 1.3 1999/01/15 15:09:09 ccarollo Exp $

#ifndef __GEN_BIND_H
#define __GEN_BIND_H
#pragma once
//game specific input binding variable/commands

#include <inpbnd_i.h>

EXTERN IInputBinder *g_pInputBinder;

EXTERN void InitIBVars ();
EXTERN void InstallIBHandler (ulong context, ulong events, BOOL poll); 
EXTERN void RemoveIBHandler ();

EXTERN void SetGameIBVarsFunc (void (*)());

EXTERN BOOL g_joystickActive;

#endif
