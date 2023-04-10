/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/propreac.h,v 1.4 2000/01/29 13:19:57 adurant Exp $
#pragma once

#ifndef __PROPREAC_H
#define __PROPREAC_H

////////////////////////////////////////////////////////////
// INITIALIZE THE PROPERTY SYSTEM'S ACT/REACT REACTIONS
//

EXTERN void InitPropReactions(void);

//
// Reaction Names
//

#define REACTION_PROP_ADD        "add_prop" 
#define REACTION_PROP_REMOVE     "rem_prop"
#define REACTION_METAPROP_ADD    "add_metaprop"
#define REACTION_METAPROP_REMOVE "rem_metaprop"
#define REACTION_OBJ_CREATE      "create_obj"
#define REACTION_OBJ_DESTROY     "destroy_obj"
#define REACTION_PROP_CLONE      "clone_props"
#define REACTION_OBJ_MOVE        "move_obj"

#endif // __PROPREAC_H
