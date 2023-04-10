/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/netmsg.h,v 1.21 2000/01/29 13:21:26 adurant Exp $
//
// Simple network messaging
//
// The cNetMsg class provides a relatively straightforward way for normal
// network messages to be created and sent. It can't handle every case,
// but it *can* deal with most messages needed by the Dark engine; it's
// quite flexible. It particularly deals with a lot of the random overhead
// that most callers don't care about, like marshalling and unmarshalling
// parameters, checking the networking is running, validating objects, and
// so on.
//
// It is specifically back-compatible with older network code; systems that
// use netman directly can ignore cNetMsg without harm. The intent is that
// most messages will eventually migrate to this mechanism, because it is
// generally a bit clearer than the old one.
//
// Note that we assume a single-threaded system; Send() is *not* thread-safe.
//
#pragma once

#ifndef __NETMSG_H
#define __NETMSG_H

#include <stdarg.h>
#include <comtools.h>
#include <objtype.h>
#include <nettypes.h>

F_DECLARE_INTERFACE(INetManager);
F_DECLARE_INTERFACE(IObjectNetworking);
F_DECLARE_INTERFACE(IObjectSystem);

//////////
//
// Message descriptor information
//
// You initialize a message by constructing a cNetMsg, initializing it with
// a message descriptor. This descriptor gives all kinds of information
// about how the message works.
//

// The types of parameters that are currently understood. All get passed
// into Send() by value, except as noted:
typedef enum eNetMsgParamType {
   // Integer types:
   kNMPT_Int,
   kNMPT_UInt,
   kNMPT_Short,
   kNMPT_UShort,
   kNMPT_Byte,
   kNMPT_UByte,

   // Normal boolean; param is standard int-width, but transmitted as
   // a single byte (or bit):
   kNMPT_BOOL,

   // Kinds of ObjIDs:
   // An object owned by the sender:
   kNMPT_SenderObjID,
   // An object owned by the receiver:
   kNMPT_ReceiverObjID,
   // An abstract object (generally an archetype):
   kNMPT_AbstractObjID,
   // An object whose ownership isn't dealt with (only for special
   // circumstances):
   kNMPT_ObjID,
   // An object whose ownership is unknown (might be on any machine):
   kNMPT_GlobalObjID,

   // A float:
   kNMPT_Float,

   // A vector; pass in as (mxs_vector *):
   kNMPT_Vector,

   // Blocks of opaque data. Maximum length of such a block is currently
   // 65535.
   // A block whose size is specified at compile-time, in the size field;
   // pass in a pointer to the block:
   kNMPT_Block,
   // A block whose size will be specified at runtime; pass in the size
   // of the block as an integer, *then* the pointer to the block:
   kNMPT_DynBlock,

   // A string; pass in a const char *:
   kNMPT_String,

   // A MultiParm, from the script system. This can resolve to an
   // integer, an object, a float, a string, or a vector. Note that you must
   // *EXPLICITLY* pass a pointer to the cMultiParm in Send(), rather than
   // assuming the compiler will deal with it as usual. This is due to
   // the limitations of variable-length argument lists.
   kNMPT_MultiParm,
   // A MultiParm, which should be resolved as an object if it's an integer.
   // @OBSOLETE: this was formerly used to indicate that the cMultiParm
   // should be treated as an object. Now, however, we detect which one
   // the cMultiParm thinks it is...
   kNMPT_GlobalObjIDMultiParm,

   // The null parameter, which should be used to indicate the end
   // of the descriptor:
   kNMPT_End,
} eNetMsgParamType;

// Flags controlling the behaviour of how we marshal and unmarshal params:
typedef enum eNetMsgParamFlags {
   // Flags for kNMPT_SenderObjID params
   kNMPF_None = 0,
   // Test that the object is owned here, and don't send if not:
   kNMPF_IfHostedHere = 1 << 0,
   // Don't assert that the SenderObjID object is owned here:
   kNMPF_NoAssertHostedHere = 1 << 1,
   // Permit OBJ_NULL to be sent through this parameter:
   kNMPF_AllowObjNull = 1 << 2,

   // Flags for kNMPT_ReceiverObjID params
   // Messages should be sent to the owner of this param's object:
   kNMPF_SendToOwner = 1 << 3,

   // Flags for all params
   // Suppress this parameter in spews
   kNMPF_SuppressSpew = 1 << 4,
   // Don't reject this message if this ObjID doesn't exist. Normally,
   // if a Sender, Receiver, or GlobalObjID in the message doesn't exist,
   // the message is rejected. If this is set, it is the handler's
   // responsibility to check for object existence.
   kNMPF_AllowNonExistentObj = 1 << 5,
} eNetMsgParamFlags;

//
// A single parameter in the network message.
//
// You must specify the type of each parameter. Flags are optional, and
// can be left as 0 in all cases to get default behaviour. Size is only
// relevant in a few types. Name is the name of the parameter, for printing
// in spews, and is always optional.
//
struct sNetMsgParam {
   eNetMsgParamType type;
   int flags;  // should be from eNetMsgParamFlags
   const char *name;
   int size;
};

// Flags controlling message behaviour in general
typedef enum eNetMsgFlags {
   kNMF_None = 0,
   // This message should be sent non-guaranteed
   kNMF_Nonguaranteed = 1 << 0,

   // Send flags. If none are given, then the message is sent
   // to the player specified in the Send() method:
   // This message should be sent even during metagame, before synch
   kNMF_MetagameBroadcast = 1 << 1,
   // This message gets broadcast to all the other players
   kNMF_Broadcast = 1 << 2,
   // This message is sent to the owner of the receive objIDs
   kNMF_SendToObjOwner = 1 << 3,
   // This message is sent to the default host
   kNMF_SendToHost = 1 << 4,

   // Append the sending player's ObjID onto the receiver's params. This
   // ObjID comes *before* the pClientData, but *after* all other params.
   kNMF_AppendSenderID = 1 << 5,
   // Do not forward this message, if the target object has been rehosted.
   // This should generally be used if SendToObjOwner is set, which allows
   // forwarding, and AppendSenderID is set, since the SenderID will be
   // incorrect. If NoForwarding is set, the handler must be prepared to
   // cope if the message is received and we don't own the object any
   // longer.
   kNMF_NoForwarding = 1 << 6,
   // *Always* send this message, even if we're in a nominally unnetworked
   // level. Must be used in conjunction with MetagameBroadcast, and should
   // be used with extreme care.
   kNMF_AlwaysBroadcast = 1 << 7,
} eNetMsgFlags;

//
// Message handler function type
//
// This should return void, and have a signature that matches the
// parameters specified. (Optionally ending with the pClientData passed
// in at startup.)
//
typedef void *tMsgHandlerFunc;

//
// The full descriptor of what this message looks like
//
struct sNetMsgDesc {
   // The general behaviour of this message (mask from eNetMsgFlags)
   ulong behaviour;
   // The short name of this message (optional)
   const char *shortName;
   // The user-printable name of this message (optional)
   const char *msgName;
   // The name of the spew flag for this message (optional)
   // If omitted, defaults to "net_spew"
   const char *spewName;
   // A routine to dispatch to when we get the message. This is optional;
   // if present, its signature should match the parameters given here.
   // If pClientData was passed in when the message was created, then that
   // will be sent as the final parameter.
   tMsgHandlerFunc msgHandler;
   // An array of parameters, ending with kNMPT_End
   sNetMsgParam params[];
};

//////////
//
// The cNetMsg class itself
//
// Instantiate one of these for each kind of message. Pass in an
// sNetMsgDesc describing the message.
//
class cNetMsg {
public:
   // The constructor. Desc describes most of the abilities of this
   // message. pClientData is optional, to use as needed. (Mostly, it
   // gets filled with pointers to a c++ object.)
   cNetMsg(sNetMsgDesc *pDesc, void *pClientData = NULL);
   virtual ~cNetMsg();

   // Send a message out. Player is ignored if the target of the message
   // is calculated from the params, or if it is a broadcast message.
   // The parameters should match the declared params for this message;
   // however, don't worry about casting between simple numeric types.
   //
   // This is *not* thread-safe, because we're using a single static
   // buffer for all messages. If we find that we need to send messages
   // from multiple threads, then we will either have to put a lock on
   // Send, or we'll have to go back to having separate (possibly dynamic)
   // message buffers.
   virtual void Send(ObjID player, ...);

protected:

   //////////
   //
   // Internal Methods
   //

   //
   // Interpret an incoming message. This is the one that instances will
   // most often want to override. It is called after parameters have
   // been unmarshalled; you should generally call GetParam to get
   // the desired parameters and do things with them.
   //
   // By default, this does nothing.
   //
   virtual void InterpretMsg(ObjID fromPlayer) {}

   //
   // Deal with a raw message from the network. This method is responsible
   // for unmarshalling; you shouldn't override it unless you need to do
   // custom unmarshalling for this message type.
   //
   virtual void HandleMsg(const sNetMsg_Generic *pGenericMsg, 
                          ulong msgSize, 
                          ObjID fromPlayer);

   //
   // Assemble the actual message into m_pMsgBuffer, based on the given
   // argument list. The default method will generally do the right thing,
   // but a custom message could potentially want to do this itself.
   // Returns FALSE iff we shouldn't send this message, for whatever
   // reason.
   //
   virtual BOOL MarshalArguments(va_list pArgs, ObjID *objPlayer, uint *size);

   //
   // Take apart the raw network message, and put the pieces into the
   // params array. If, upon return, pForwardTo is not OBJ_NULL, then this
   // message should be forwarded to that player.
   //
   virtual BOOL UnmarshalParams(uchar *ptr, 
                                ObjID fromPlayer, 
                                ObjID *pForwardTo);
   //
   // Clean up after the marshalling, as needed.
   //
   virtual void ClearParams();

   //
   // Loopback
   // This will get called if we decide that this message is simply going
   // to the local player, and doesn't need to go through networking.
   // It eliminates the marshal/unmarshal procedure.
   //
   // Note that we loopback if the message is going to the default host,
   // and that's us, or if it's SendToObjOwner and that's us *and* the
   // ReceiverObjID is the first param. (To make the check fast.) So if
   // you want to take advantage of loopback, put the ReceiverObjIDs first.
   //
   virtual void DoLoopback(va_list pArgs);

   //
   // Forwarding
   // If a message is SendToObjOwner, but the object's owner changes while
   // the message is in transit, the old owner needs to pass it along.
   // This method will send the already-parsed message on to its new owner.
   //
   virtual void ForwardMsg(ObjID forwardTo);

   //
   // Spew methods
   //
   // By default, we spew the parameters as specified in the sNetMsgDesc.
   // This isn't very aesthetic, though, so you can give a method to
   // spew more carefully if desired.
   //
   // Spew a message at send time, based on the given params:
   virtual void SendSpew(va_list pArgs);
   // Spew a message at receive time, based on the unmarshall params:
   virtual void ReceiveSpew(ObjID fromPlayer);

   // Get a specific parameter. (Zero-based.) The caller is responsible
   // for casting the value appropriately.
   DWORD GetParam(int index);

   // Send the message off to an outside handler
   void SendToHandler();

   // Check a proxy's archetype against one that was sent
   void CheckArchetypes(sGlobalObjID obj, ObjID proxy, ObjID sentArch);

   //////////
   //
   // Member variables
   //

   // The descriptor for the message:
   sNetMsgDesc *m_pDesc;
   // The number of parameters, possibly including the player ID:
   int m_NumDispatchParams;
   // The number of *real* parameters in the message:
   int m_NumParams;

   // The handlerID we got from netman:
   tNetMsgHandlerID m_handlerID;

   // The client-specific data, if any:
   void *m_pClientData;

   // The unmarshalled parameters. You will generally get at these through
   // GetParam.
   DWORD *m_ppParams;

   // TRUE iff we should always spew this particular message:
   BOOL m_bSpewMe;

   // TRUE iff we are in the middle of forwarding a message:
   BOOL m_bForwarding;

private:

   //////////
   //
   // Static stuff
   //

   //
   // Methods
   //
   // Initialization code for the whole net-message system. Will get called
   // automatically when the first message gets created:
   static void InitNetMsgs();
   // Shutdown code. Will get called when the last message dies:
   static void TermNetMsgs();

   // Listener. This will be called by netman whenever something really
   // interesting happens:
   static void NetListen(eNetListenMsgs situation,
                         DWORD data,
                         void *pClientData);

   // Top-level dispatcher. This will send network messages to the right
   // cNetMsg object.
   static void NetDispatcher(const sNetMsg_Generic *pMsg,
                             ulong msgSize,
                             ObjID fromPlayer,
                             void *pClientData);

   //
   // Global Members
   //

   // The message buffer
   // Note that we assume that a single message buffer is okay: we assume
   // that we don't have multiple threads and aren't re-entrant
   static uchar *gm_pMsgBuffer;

   // The Network Manager
   static INetManager *gm_pNetMan;

   // The Object Networking manager
   static IObjectNetworking *gm_pObjNet;

   // The Object System
   static IObjectSystem *gm_pObjSys;

   // The number of messages that have been registered through netmsg
   static int gm_NumMsgs;

   // Are we currently networking?
   static BOOL gm_bNetworking;

   // Should we do archetype checking?
   static BOOL gm_bAssertArchetypes;
};

//////////

inline DWORD cNetMsg::GetParam(int index)
{
   return m_ppParams[index];
}

#endif // !__NETMSG_H
