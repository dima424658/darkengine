/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCPROJ_H
#define __DPCPROJ_H

#ifndef __DMGMODEL_H
#include <dmgmodel.h>
#endif // !__DMGMODEL_H

#ifndef _OBJTYPE_H
#include <objtype.h>
#endif // ! _OBJTYPE_H

#ifndef __MATRIXS_H
#include <matrixs.h>
#endif // ! __MATRIXS_H

#ifndef PROPFACE_H
#include <propface.h>
#endif // !PROPFACE_H

#ifndef __WRTYPE_H
#include <wrtype.h>
#endif // !__WRTYPE_H

eDamageResult DPCProjTerrImpactHandler(ObjID projID);

/////////////////////////////////////////
//
// Projectile launcher
//

enum eLaunchParamFlags
{
   kLaunchCheck            = 0x0001, // do launch physics check - fill in valid loc field in params or will use owner's loc
   kLaunchCheckPlayerHead  = 0x0002, // launch check - will use player head as valid loc                                   
   kLaunchRelativeVelocity = 0x0004, // add in owner's velocity to launch                                                  
   kLaunchTellAI           = 0x0008, // inform AI about launch                                                                      
   kLaunchLocOverride      = 0x0010, // override launch location                                                               
   kLaunchPitchOverride    = 0x0020, // override launch pitch                                                                
   kLaunchHeadingOverride  = 0x0040, // override launch heading                                                            
   kLaunchBankOverride     = 0x0080, // override launch bank                                                                  
};

struct sLaunchParams
{
   int        flags;                
   float      speedMult;            // speed multiplier                                         
   float      intensityMult;        // intensity multiplier                                     
   mxs_vector loc;                  // launch location or location offset (defaults to offset)  
   mxs_ang    pitch;                // pitch or pitch offset (defaults to offset)               
   mxs_ang    heading;              // heading or heading offset (defaults to offset)           
   mxs_ang    bank;                 // bank or bank offset (defaults to offset)                 
   mxs_ang    error;                // random angle for all launched                            
   Location*  pValidLoc;            // known location in world to use for launch check          
};

// Use these params as a template if you want - these are the defaults
// No flags are set by default, everything else zero or one as appropriate
extern sLaunchParams g_defaultLaunchParams;

// Launch 'em
// Returns object ID of last projectile launched (if there is a spray of more than 1)
// If you want all the objIDs, then get them out of the launch params
EXTERN ObjID DPCLaunchProjectile(ObjID owner, ObjID archetype, sLaunchParams *pParams = NULL);

// Startup and shutdown
EXTERN void DPCProjectileInit();
EXTERN void DPCProjectileTerm();

#endif  // __DPCPROJ_H
