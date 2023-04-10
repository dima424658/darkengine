/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/scrnloop.h,v 1.7 2000/01/31 09:53:17 adurant Exp $
#pragma once

#ifndef __SCRNLOOP_H
#define __SCRNLOOP_H

DEFINE_LG_GUID(LOOPID_ScrnMan, 0x1e);
EXTERN struct sLoopClientDesc ScrnManClientDesc;

//
// Context for loopmode go 
//

typedef struct _ScrnManContext
{
   struct sModeParams
   {
      struct sScrnMode *preferred; // preferred mode parameters
      struct sScrnMode *min_mode;  // minimum mode parameters
      struct sScrnMode *max_mode;  // maximum mode parameters 
   } mode_params; 
   const char* pal_res;  // palette resource name 
} ScrnManContext; 


// we set this during mode change if you failed to set the screenmode 
EXTERN BOOL gScrnLoopSetModeFailed; 


#endif // __SCRNLOOP_H



