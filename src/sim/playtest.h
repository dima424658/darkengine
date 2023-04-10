/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/playtest.h,v 1.5 2000/02/19 12:37:01 toml Exp $
#pragma once
#ifndef __PLAYTEST_H
#define __PLAYTEST_H

////////////////////////////////////////////////////////////
// PLAYTEST FEATURES
//

#ifdef PLAYTEST

EXTERN void PlaytestInitReal();
EXTERN void PlaytestTermReal();
EXTERN void PlaytestFrameReal();

#define PlaytestInit()  PlaytestInitReal()
#define PlaytestTerm()  PlaytestTermReal()
#define PlaytestFrame() PlaytestFrameReal()

EXTERN void hello_debugger(void);

EXTERN BOOL cheats_active;
#define CheatsActive() (cheats_active)

EXTERN void CheatCheckToggleBool(bool *var, char *cmd, char *name);
#define CheatCheckToggle(var,cmd,name) CheatCheckToggleBool((bool *)(var),cmd,name)

// Accessor to allocator tools for .c files

EXTERN void _MallocPushCredit(const char * pszFile, int line);
EXTERN void _MallocPopCredit();

#define MallocPushCredit()          _MallocPushCredit(__FILE__, __LINE__)
#define MallocPushCreditTagged(tag) _MallocPushCredit(__FILE__ tag, __LINE__)
#define MallocPopCredit()           _MallocPopCredit()

#else // !PLAYTEST

#define PlaytestInit()
#define PlaytestTerm()
#define PlaytestFrame()

// really, you should ifdef PLAYTEST the code on that side too....
#define CheatsActive() (FALSE)

#define hello_debugger()

#define MallocPushCredit()
#define MallocPushCreditTagged(tag)
#define MallocPopCredit()

#endif  // !PLAYTEST

#endif // __PLAYTEST_H
