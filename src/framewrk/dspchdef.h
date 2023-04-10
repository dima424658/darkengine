/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/dspchdef.h,v 1.1 1998/04/20 13:40:56 mahk Exp $
#pragma once  
#ifndef __DSPCHDEF_H
#define __DSPCHDEF_H

typedef ulong eDispatchMsg; 
typedef ulong eDispatchMsgSet; // bitmask 


//
// Message type 
//

struct sDispatchMsg
{
   ulong kind;   // bitmask specifying message kind 
   void* data;   // other message data
};

//
// Message callback 
// 

typedef void (*tDispatchListenFunc)(const struct sDispatchMsg* msg, const struct sDispatchListenerDesc* desc); 

//
// Listener desc
//

struct sDispatchListenerDesc 
{
   const GUID* pID;        // My unique id 
   eDispatchMsgSet interests;   // which messages I want 
   tDispatchListenFunc func;    // my callback 
   void* data;             // data used by my callback 
}; 


#endif // __DSPCHDEF_H

