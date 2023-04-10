/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/tweqenum.h,v 1.7 2000/01/31 09:45:44 adurant Exp $
// silly bridge header
#pragma once

#ifndef __TWEQENUM_H
#define __TWEQENUM_H

//////////////
// enums for behaviors
enum eTweqType
{
   kTweqTypeScale,
   kTweqTypeRotate,
   kTweqTypeJoints,
   kTweqTypeModels,
   kTweqTypeDelete,
   kTweqTypeEmitter,
   kTweqTypeFlicker,
   kTweqTypeLock,
   kTweqTypeAll,
   kTweqTypeNull,
   kTweqTypeSpaceWasterJoy=0xffffffff
};

enum eTweqDirection
{
   kTweqDirForward,
   kTweqDirReverse,
   kTweqDirSpaceWasterJoy=0xffffffff
};

enum eTweqOperation
{
   kTweqOpKillAll,
   kTweqOpRemoveTweq,
   kTweqOpHaltTweq,
   kTweqOpStatusQuo,
   kTweqOpSlayAll,
   kTweqOpFrameEvent,
   kTweqOpSpaceWasterJoy=0xffffffff
};

enum eTweqDo
{
   kTweqDoDefault,
   kTweqDoActivate,
   kTweqDoHalt,
   kTweqDoReset,
   kTweqDoContinue,
   kTweqDoForward,
   kTweqDoReverse,
   kTweqDoSpaceWasterJoy=0xffffffff
};

#endif  // __TWEQENUM_H
