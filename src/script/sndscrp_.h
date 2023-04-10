/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/script/sndscrp_.h,v 1.1 1998/11/03 20:52:24 dc Exp $
//
// sndscrpt.h

#pragma once

#ifndef __SNDSCRP__H
#define __SNDSCRP__H

#include <tagfile.h>

// for save/load
EXTERN void ScriptSoundSave(ITagFile *file);
EXTERN void ScriptSoundLoad(ITagFile *file);
EXTERN void ScriptSoundPostLoad(void);         // this is dumb
EXTERN void ScriptSoundEnterModeHack(void);    // this is even dumber

// for reset
EXTERN void ScriptSoundReset(void);

// init/term
EXTERN void ScriptSoundInit(void);
EXTERN void ScriptSoundTerm(void);

#endif
