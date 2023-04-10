/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phscrt.h,v 1.6 2000/01/31 09:52:08 adurant Exp $
//
#pragma once

#ifndef __PHSCRT_H
#define __PHSCRT_H

enum ePhysScriptMsgType_
{
   kNoMsg          = 0x0000,
   // subscribed messages
   kCollisionMsg   = 0x0001,
   kContactMsg     = 0x0002,
   kEnterExitMsg   = 0x0004,
   kFellAsleepMsg  = 0x0008,
   kWokeUpMsg      = 0x0010,
   // non-subscribed messages
   kMadePhysMsg    = 0x0100,
   kMadeNonPhysMsg = 0x0200,
};

#define kAllMsgs (kCollisionMsg | kContactMsg | kEnterExitMsg)

enum ePhysCollisionType_
{
   kCollNone     = 0x0000,
   kCollTerrain  = 0x0001,
   kCollObject   = 0x0002,
};

enum ePhysContactType_
{
   kContactNone      = 0x0000,
   kContactFace      = 0x0001,
   kContactEdge      = 0x0002,
   kContactVertex    = 0x0004,
   kContactSphere    = 0x0008,
   kContactSphereHat = 0x0010,
   kContactOBB       = 0x0020,
};

#define kContactTerrain (kContactFace | kContactEdge | kContactVertex)
#define kContactObject  (kContactSphere | kContactSphereHat | kContactOBB)

enum ePhysContact_
{
   kContactCreate  = 0x0000,
   kContactDestroy = 0x0001,
};

enum ePhysEnterExit_
{
   kEnter = 0x0000,
   kExit  = 0x0001,
};


enum ePhysMessageResult_
{
   kPM_StatusQuo = 0x0000,
   kPM_Nothing   = 0x0001,
   kPM_Bounce    = 0x0002,
   kPM_Slay      = 0x0003,
   kPM_NonPhys   = 0x0004
};

typedef int ePhysScriptMsgType;
typedef int ePhysCollisionType;
typedef int ePhysContactType;

typedef int ePhysContact;
typedef int ePhysEnterExit;

typedef int ePhysMessageResult;

#endif // __PHSCRT_H




