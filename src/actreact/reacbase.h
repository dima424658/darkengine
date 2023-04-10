/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/reacbase.h,v 1.6 2000/01/29 12:44:34 adurant Exp $
#pragma once

#ifndef __REACBASE_H
#define __REACBASE_H

#include <stimtype.h>
#include <senstype.h>

////////////////////////////////////////////////////////////
// ACT/REACT REACTION BASE TYPES
//

//
// Reaction Event
//

struct sReactionEvent
{
   sStimEvent* stim;
   sStimTrigger* trigger;
   ObjID sensor_obj;
};


//
// Reaction Descriptor
//

struct sReactionTypeDesc 
{
   char name[32]; 
};

#define REACTION_PARAM_TYPE(x) { #x }
#define NO_REACTION_PARAM { "None" }

struct sReactionDesc 
{
   char name[32]; // reaction name
   char friendly_name[64];  // human-readable name
   struct sReactionTypeDesc param_type; // type of parameter data
   ulong flags;
};

//
// Flags for Reaction Descriptor
//
// @NOTE: only use kReactionWorkOnProxy if you know what you're doing. This
// means that the effect will be propagated separately on both the host and
// proxies for this machine. It's your responsibility to make sure that
// this doesn't cause the effect to happen doubly (or worse)...
//

enum eReactionFlags
{
   kReactionNoFlags        = 0, 
   kReactionEdgeTrigger    = 1 << 0,  // trigger to start or stop (NYI)
   kReactionHasDirectObj      = 1 << 1,    // param uses direct object
   kReactionHasIndirectObj    = 1 << 2,    // param uses indirect object
   kReactionWorkOnProxy    = 1 << 3,  // propagate even to proxy objects
}; 

//
// Reaction results
//

enum eReactionResult_
{
   kReactionNormal,  // everything is fine
   kReactionMutate,  // I mutated the event.  Carry on with the changed version
   kReactionAbort,   // Don't fire more effects 
};

//
// Reaction parameter
//

//
// Pointy-headed weenie that I am, I agonized over whether the
// parameter should be fixed size or not.   How could I possibly
// decide on a size that was adequate for all reactions?  In the end I
// decided that the ease of memory management on the receptron side
// made it worth doing.   (MAHK 9/3)
//

enum eReactionParamObjects
{
   kReactDirectObj,     // direct object
   kReactIndirectObj,   // indirect object
   kReactNumObjs,       // num of object params
};

struct sReactParamData 
{
   char bits[32];           // We can increase this size as needed, 
};                         // remembering to version the receptron 
                           // loader.   

struct sReactionParam 
{
   ObjID obj[kReactNumObjs];
   sReactParamData data;
};


#endif // __REACBASE_H




