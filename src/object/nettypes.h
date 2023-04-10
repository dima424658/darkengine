/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/nettypes.h,v 1.8 2000/01/29 13:23:52 adurant Exp $
#pragma once

#ifndef __NETTYPES_H
#define __NETTYPES_H

#include <objtype.h>

// Initial bytes of a message contain this ID of the message handler.
typedef ubyte tNetMsgHandlerID; 

// all networking messages will start with one byte that specifies the handler, the rest
// of the message will be determined by the handler that does the message parsing.
typedef struct sNetMsg_Generic 
{
   tNetMsgHandlerID handlerID;
} sNetMsg_Generic;

// Representation of a player in a way that can be passed as part of a network message.
typedef ulong tNetPlayerID;
EXTERN const tNetPlayerID NULL_NET_ID;

// A compact object ID for sending over the network; should be the
// shortest type actually capable of holding all objects
typedef short NetObjID;

// A compact ID for player number. Since player numbers are simply ordinals,
// and since we don't support a *lot* of players, this can be very short:
typedef uchar tNetPlayerNum;

// The globally unique ID for this object. This identifies a player whose
// "namespace" this object should be dereferenced from.
typedef struct sGlobalObjID {
   tNetPlayerNum host;
   NetObjID obj;
} sGlobalObjID;

// Network listener messages. These are actually defined in netnotif.h:
typedef ulong eNetListenMsgs;

#endif //__NETTYPES_H
