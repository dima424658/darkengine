/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/chainevt.cpp,v 1.2 1998/10/05 17:20:42 mahk Exp $
#include <chainevt.h>
#include <string.h>

// must be last header
#include <memall.h>
#include <dbmem.h>

////////////////////////////////////////////////////////////
// sChainedEvent implementation
//

const sChainedEvent* sChainedEvent::Find(eChainedEventKind kind) const
{
   for (const sChainedEvent* event = this; event != NULL; event = event->ev.prev)
      if (event->ev.kind == kind)
         return event; 
   return NULL; 
}

sChainedEvent* sChainedEvent::Copy() const 
{
   sChainedEvent* out = (sChainedEvent*) new char[ev.size]; 
   *out = *this; 
   if (ev.prev)
      out->ev.prev = ev.prev->Copy();  // recurse
   return out; 
}
