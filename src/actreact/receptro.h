/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/receptro.h,v 1.3 2000/01/29 12:44:38 adurant Exp $
#pragma once

#ifndef __RECEPTRO_H
#define __RECEPTRO_H

#include <stimtype.h>
#include <senstype.h>
#include <reactype.h>
#include <sensbase.h>
#include <reacbase.h> // for sReactionParam


////////////////////////////////////////////////////////////
// RECEPTRON BASE TYPES
//

//
// Receptron structure
//

// @TODO: move this to separate effects header?

struct sReceptronReaction
{
   ReactionID kind;     // kind of effect
   sReactionParam param;           // effect params, depends on effect type
};

struct sReceptron
{
   ulong order;               // higher goes later
   sStimTrigger trigger;      // trigger
   sReceptronReaction effect;       // effect when fired
};




#endif // __RECEPTRO_H








