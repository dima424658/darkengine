/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: x:/prj/tech/libsrc/object/RCS/nettypes.h 1.5 1999/01/27 16:00:42 mahk Exp $

#ifndef __NETTYPES_H
#define __NETTYPES_H

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

// The globally unique ID for this object.  It should only be necessary to use this when
// sending an object in a network message where the object is not necessarily hosted
// by either the sender or the receiver of the message (in those cases, you should just
// send the object's objID on its host).
typedef struct sGlobalObjID {
   tNetPlayerID host;
   NetObjID obj;
} sGlobalObjID;

#endif //__NETTYPES_H