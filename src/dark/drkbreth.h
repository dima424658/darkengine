/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkbreth.h,v 1.1 1998/06/22 23:26:20 mahk Exp $
#pragma once  
#ifndef __DRKBRETH_H
#define __DRKBRETH_H
#include <objtype.h>

////////////////////////////////////////////////////////////
// Dark Breathing/Air Economy System
//

//
// Breath params for an object
//

struct sBreathConfig 
{
   int max_air; // how many ms of air can I have
   int drown_freq; // how often you take drowning damage
   int drown_damage; // how much drowning damage you take 
   float recover_rate; // how fast you recover your air meter
};

typedef struct sBreathConfig sBreathConfig; 

EXTERN const sBreathConfig* GetObjBreathConfig(ObjID obj);
EXTERN void SetObjBreathConfig(ObjID obj, const sBreathConfig* );

#define PROP_BREATH_NAME "BreathConfig"

//
// Air Supply Property
//

EXTERN void SetObjAirSupply(ObjID obj, int air); 
EXTERN int  GetObjAirSupply(ObjID obj); 

#define PROP_AIRSUPPLY_NAME "AirSupply"

//
// Run breath sim for a frame
//

EXTERN void BreathSimUpdateFrame(int ms); 

//
// Init/Term
// 

EXTERN void BreathSimInit(); 
EXTERN void BreathSimTerm(); 

#endif // __DRKBRETH_H






