/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/dispatch.h,v 1.7 2000/01/29 13:20:39 adurant Exp $
#pragma once
#ifndef __DISPATCH_H
#define __DISPATCH_H

////////////////////////////////////////////////////////////
// DISPATCH LAYER
//
// This is an app-side layer on the loop manager that supports a number
// of app-specific dispatch messages.
//

typedef struct DispatchData DispatchData;
typedef ulong DispatchKind;


//------------------------------------------------------------
// Dispatch a message.
//

EXTERN void DispatchMsg(DispatchKind kind, DispatchData* data);

//------------------------------------------------------------
// Determine whether a message is currently being sent.  (i.e. whether we're
// beneath DispatchMsg in the call stack.)  Returns true if there is a current 
// message and it is one of the messages specified by the mask.
// 

EXTERN BOOL DispatchInMsg(DispatchKind mask); 

//
// Iff there is a message outstanding, put its kind and data in the arguments
// and return TRUE.
//

EXTERN BOOL DispatchCurrentMsg(DispatchKind* kind, DispatchData** data);

//------------------------------------------------------------
// Send a message to ALL clients, not just those in the current mode
// Really only used for init/term messages
// Flags are from the loopmode dispatch api
//
EXTERN void DispatchMsgAllClients(DispatchKind kind, DispatchData* data, int flags);

//------------------------------------------------------------
// Init/Shutdown
//

EXTERN void DispatchInit(void);
EXTERN void DispatchShutdown(void);

#endif // __DISPATCH_H




