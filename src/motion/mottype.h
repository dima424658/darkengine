/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/mottype.h,v 1.8 2000/01/31 09:51:05 adurant Exp $
#pragma once

#ifndef __MOTTYPE_H
#define __MOTTYPE_H

#include <motion.h>

// motion types
#define MDT_NONE                          -1

#define MDT_TEST                          0
#define MDT_GUARD_CALIB                   1

#define MDT_STAND_GUARD_CASUAL            2
#define MDT_WALK_START_GUARD_CASUAL       3
#define MDT_WALK_END_GUARD_CASUAL         4
#define MDT_WALK_LEFT_GUARD_CASUAL        5
#define MDT_WALK_RIGHT_GUARD_CASUAL       6
#define MDT_CLIMB_LEFT_GUARD_CASUAL       7
#define MDT_CLIMB_RIGHT_GUARD_CASUAL      8
#define MDT_DESCEND_LEFT_GUARD_CASUAL     9
#define MDT_DESCEND_RIGHT_GUARD_CASUAL    10

#define MDT_STAND_GUARD_READY            22
#define MDT_WALK_START_GUARD_READY       23
#define MDT_WALK_END_GUARD_READY         24
#define MDT_WALK_LEFT_GUARD_READY        25
#define MDT_WALK_RIGHT_GUARD_READY       26
#define MDT_CLIMB_LEFT_GUARD_READY       27
#define MDT_CLIMB_RIGHT_GUARD_READY      28
#define MDT_DESCEND_LEFT_GUARD_READY     29
#define MDT_DESCEND_RIGHT_GUARD_READY    30
#define MDT_WALK_SEARCH_START_GUARD_READY  31
#define MDT_WALK_SEARCH_CYCLE_GUARD_READY 32

#define MDT_GESTURE_GUARD_CASUAL         40
#define MDT_ALERT1_GUARD                 41 
#define MDT_ALERT2_GUARD                 42
#define MDT_ALERT3_GUARD                 43 
#define MDT_CALLHELP_GUARD               44 
#define MDT_SWORD_ATTACK_GUARD           45 
#define MDT_SWORD_PARRY_GUARD            46
#define MDT_SWORD_DODGE_GUARD            47 
#define MDT_RETREAT_GUARD                48
#define MDT_ADVANCE_GUARD                49

#define MDT_DIE_GUARD                    51
#define MDT_HURT_GUARD                   52
#define MDT_SIDESTEP_LEFT_GUARD          53
#define MDT_SIDESTEP_RIGHT_GUARD         54

#define MDT_AWAKEN_SERVANT               55 
#define MDT_RUN_LEFT_SERVANT             56
#define MDT_RUN_RIGHT_SERVANT            57
#define MDT_RUN_TURN_180                 58

#define MDT_SWORD_ATTACK_SHORT           59
#define MDT_SWORD_ATTACK_MEDIUM          60 
#define MDT_SWORD_ATTACK_LONG            61 

#define MDT_HURT_BADLY                   62
#define MDT_HURT_LIGHTLY                 63

#define MDT_COMBAT_IDLE                  64
#define MDT_STUN_GUARD                   65

#define MDT_SURPRISE_LEFT                66  
#define MDT_SURPRISE_RIGHT               67
#define MDT_SURPRISE_AHEAD               68
#define MDT_SURPRISE_BEHIND              69 


#define NUM_MOTDESC_TYPES 70

#endif
