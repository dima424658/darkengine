/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/physics/phcollev.h,v 1.3 2000/02/22 20:01:08 toml Exp $
#pragma once  
#ifndef __PHCOLLEV_H
#define __PHCOLLEV_H

#include <chainevt.h>
#include <chevkind.h>
#include <phclsn.h>

#include <mprintf.h>

#include <dbmem.h>

//
// A phys collision chained event.  This is in no way a flat
// structure, and thus can't be deep-copied correctly.  So all uses of
// this event have to be on the stack, which is perilous.  
//

struct sPhysClsnEvent : public sChainedEvent
{
   cPhysClsn *collision; 

   sPhysClsnEvent(cPhysClsn* coll, sChainedEvent *ev = NULL)
      : sChainedEvent(kEventKindCollision, sizeof(*this), ev)
   {
      collision = new cPhysClsn(coll);
   } 

   ~sPhysClsnEvent()
   {
      delete collision;
   }
}; 

#include <undbmem.h>

#endif // __PHCOLLEV_H
