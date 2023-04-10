/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/physics/phmedtr.h,v 1.2 1998/08/27 18:21:52 CCAROLLO Exp $
#pragma once  
#ifndef __PHMEDTR_H
#define __PHMEDTR_H

#include <objtype.h>
#include <objmedia.h>

////////////////////////////////////////////////////////////
// PHYSICS MEDIA TRANSITION SUPPORT 
//
// Super-lightweight api for getting callbacks whenever an object changes media
// 

EXTERN void PhysMediaTransitionsInit(void); 
EXTERN void PhysMediaTransitionsTerm(void); 

//
// Media events 
//

struct sPhysMediaEvent 
{
   ObjID obj;     // transiting object
   int cell;      // our new cell
   eMediaState old_med;   // our old medium
   eMediaState new_med;   // our new medium
}; 

typedef struct sPhysMediaEvent sPhysMediaEvent; 
 
//
// Listening to events
//

typedef void (LGAPI * tPhysMediaHandler)(const struct sPhysMediaEvent* event, void* data); 

EXTERN void PhysListenToMediaEvents(tPhysMediaHandler func, void* data); 

//
// Generating events (used internally by physics) 
//
EXTERN void PhysSendMediaEvent(const sPhysMediaEvent* event); 

#endif // __PHMEDTR_H

