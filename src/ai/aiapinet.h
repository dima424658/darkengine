/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiapinet.h,v 1.5 2000/01/29 12:45:01 adurant Exp $
//
//
//
#pragma once

#ifndef __AIAPINET_H
#define __AIAPINET_H

#include <comtools.h>
#include <matrixs.h>

#include <objtype.h>
#include <nettypes.h>

///////////////////////////////////////////////////////////////////////////////

F_DECLARE_INTERFACE(IAINetManager);
F_DECLARE_INTERFACE(IAINetServices);

///////////////////////////////////////////////////////////////////////////////
//
// STRUCT: sAINetTransfer
//
// The set of data that needs to be transmitted to transfer an AI to another
// machine
//

struct sAINetTransfer
{
   unsigned size;
   void *pData;
};

///////////////////////////////////////////////////////////////////////////////
//
// STRUCT: sAIImpulse
//
// Describes the current impulse desires of an AI
//

enum eAIImpulseFlags
{
   kAIIF_Rotating = 0x01
};

///////////////////

struct sAIImpulse
{
   mxs_vector vec;
   float      facing;
   unsigned   flags;
};

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IAINetManager
//
// Used to control networking aspects of the AI
//

DECLARE_INTERFACE_(IAINetManager, IUnknown)
{
   //
   // Query if an AI is a proxy
   //
   STDMETHOD_(BOOL, IsProxy)(ObjID id) PURE;

   //
   // Get the state information about this AI that will be needed to
   // transfer it from machine to machine. May return NULL.
   //
   STDMETHOD (GetAITransferInfo)(ObjID id, sAINetTransfer **) PURE;

   //
   // Turn an AI into a proxy, passing it off to a new player.
   //
   STDMETHOD (MakeProxyAI)(ObjID id, ObjID targetPlayer) PURE;
   
   //
   // Turn a proxy into a full AI given received data. AI will not
   // attempt to free the data.
   //
   STDMETHOD (MakeFullAI)(ObjID id, const sAINetTransfer *) PURE;

   //
   // Run through the AIs, and make sure they are all properly owned.
   // This should be run when we have reason to believe that some of
   // our AIs might have an incorrect understanding of whether they are
   // Proxies or Full.
   //
   STDMETHOD (RecheckAIOwnership)() PURE;

   ///////////////////////////////////
   //
   // The handler id to use in network messages that should be handled by the AI manager.
   // @TBD (mtr): this should also be made internal.
   // @TBD (justin): this should simply go away, when we get a chance
   // to change everything to use modern-style network msgs.
   //
   STDMETHOD_(tNetMsgHandlerID,  NetMsgHandlerID)() PURE;
};

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IAINetServices
//
// AIs interface to the external networking systems
//

#define kAI_MaxPlayerList 7
typedef ObjID tAIPlayerList[kAI_MaxPlayerList + 1];

///////////////////////////////////////

DECLARE_INTERFACE_(IAINetServices, IUnknown)
{
   //
   // Query if in a network game
   //
   STDMETHOD_(BOOL, Networking)() PURE;
   
   //
   // Get a list of all player objects. List is null terminated; also returns number
   //
   STDMETHOD_(int, GetPlayers)(tAIPlayerList *) PURE;

   //
   // Set the physics for a ballistic AI
   //
   STDMETHOD_(void, SetObjImpulse)(ObjID obj, float x, float y, float z, float facing, BOOL rotating = TRUE) PURE;

   //
   // Initiate a transfer of an AI to another machine
   //
   STDMETHOD_(BOOL, TransferAI)(ObjID objAI, ObjID objDestPlayer) PURE;
   
   //
   // Query what the last movement desire was, in the form of an
   // impulse. Returns TRUE if there actually was one
   //
   STDMETHOD_(BOOL, GetTargetVel)(ObjID ai, sAIImpulse * pResult) PURE;
};

///////////////////////////////////////////////////////////////////////////////

#endif /* !__AIAPINET_H */
