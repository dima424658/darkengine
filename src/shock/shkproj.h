/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkproj.h,v 1.17 2000/01/31 09:58:41 adurant Exp $
#pragma once

This file has been moved to gunproj.h  AMSD


#ifndef __SHKPROJ_H
#define __SHOPROJ_H

#include <dmgmodel.h>
#include <objtype.h>
#include <matrixs.h>
#include <propface.h>
#include <wrtype.h>

eDamageResult ShockProjTerrImpactHandler(ObjID projID);

/////////////////////////////////////////
//
// Projectile launcher
//

enum eLaunchParamFlags
{
   kLaunchCheck = 0x0001,           // do launch physics check - fill in valid loc field in params or will use owner's loc
   kLaunchCheckPlayerHead = 0x0002, // launch check - will use player head as valid loc
   kLaunchRelativeVelocity = 0x0004,// add in owner's velocity to launch
   kLaunchTellAI = 0x0008,          // inform AI about launch
   kLaunchLocOverride = 0x0010,     // override launch location
   kLaunchPitchOverride = 0x0020,   // override launch pitch       
   kLaunchHeadingOverride = 0x0040, // override launch heading       
   kLaunchBankOverride = 0x0080,    // override launch bank
};

struct sLaunchParams
{
   int flags;     
   float speedMult;     // speed multiplier
   float intensityMult; // intensity multiplier
   mxs_vector loc;      // launch location or location offset (defaults to offset)
   mxs_ang pitch;       // pitch or pitch offset (defaults to offset)
   mxs_ang heading;     // heading or heading offset (defaults to offset)
   mxs_ang bank;        // bank or bank offset (defaults to offset)
   mxs_ang error;       // random angle for all launched 
   Location* pValidLoc; // known location in world to use for launch check
};

// Use these params as a template if you want - these are the defaults
// No flags are set by default, everything else zero or one as appropriate
extern sLaunchParams g_defaultLaunchParams;

// Launch 'em
// Returns object ID of last projectile launched (if there is a spray of more than 1)
// If you want all the objIDs, then get them out of the launch params
EXTERN ObjID ShockLaunchProjectile(ObjID owner, ObjID archetype, sLaunchParams *pParams = NULL);

// Startup and shutdown
EXTERN void ShockProjectileInit();
EXTERN void ShockProjectileTerm();

#endif
