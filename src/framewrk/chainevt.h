/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/chainevt.h,v 1.1 1998/07/16 13:22:05 mahk Exp $

#ifndef __CHAINEVT_H
#define __CHAINEVT_H
#pragma once

////////////////////////////////////////////////////////////
// LIGHTWEIGHT "CHAINED EVENT" TYPE
//
// Numerous systems provide their clients with APIs for notification on 
// events.  Sometimes, one such system can be a client of another system; 
// the client system receives an event, executes some logic, and notifies its 
// own clients, effectively passing a transformed version of the event on. 
// When this chain of event-passing gets long, the initial event information 
// mutates (and often degrades) as it passes along the pipe, leaving late 
// clients unable to make use of information earlier in the pipe. 
//
// Thus, this extremely simple system, in which every event structure points 
// at the event that caused it.  Event structures are tagged by the
// system that signals them, so late clients can grovel through the
// event history looking for a previous system's info. 
// 

typedef ulong eChainedEventKind; 

struct sChainedEvent 
{
   // wrap the event fields to avoid field name conflicts
   struct sChainedEvPrefix
   {
      eChainedEventKind kind;  // what kind of event am I 
      ulong size;              // how big am I? 
      sChainedEvent* prev;     // next in chain 
   } ev; 

   sChainedEvent(eChainedEventKind k, ulong s, sChainedEvent* p = NULL)
   {
      ev.kind = k;
      ev.size = s; 
      ev.prev = p; 
   }; 


   //
   // Grovel through the the chain for an event of a particular type.
   // return the first one.  
   //
   const sChainedEvent* Find(eChainedEventKind kind) const; 

   //
   // new() a deep-copy of the event chain 
   // 
   sChainedEvent* Copy() const; 

};


#endif // __CHAINEVT_H

