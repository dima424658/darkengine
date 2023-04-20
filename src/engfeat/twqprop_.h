/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/twqprop_.h,v 1.3 2000/01/31 09:45:48 adurant Exp $
// internal header for tweq script/property stuff
#pragma once

#ifndef __TWQPROP__H
#define __TWQPROP__H

#include <reaction.h>

//////
// in twqscrpt.cpp

// actually create the thing
EXTERN ReactionID create_tweq_control_reaction(void);

// send a tweq message to the script
EXTERN void SendTweqMessage(ObjID to, enum eTweqType type, enum eTweqOperation op, enum eTweqDirection dir);

//////
// in tweqprop.cpp

EXTERN const char *tweq_type_names[];

EXTERN IProperty *tweq_prop_states[];
EXTERN IProperty *tweq_prop_config[];

#endif  // __TWQPROP__H
