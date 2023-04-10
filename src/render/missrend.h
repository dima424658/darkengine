/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/missrend.h,v 1.4 2000/01/14 10:42:04 MAT Exp $
#pragma once  
#ifndef __MISSREND_H
#define __MISSREND_H

#include <matrixs.h>

////////////////////////////////////////////////////////////
// Mission Rendering Params
//

#define PAL_RES_SUBDIR "pal"

struct sMissionRenderParams
{
   char pal_res[16];    // palette resource
   mxs_vector ambient_light;// ambient light for level
   BOOL use_sun;
   BOOL sunlight_quad;
   mxs_vector sunlight_vector;
   float sun_h;
   float sun_s;
   float sun_b;
   mxs_vector sun_scaled_rgb;
   mxs_vector sun_rgb;  // updated when filevar changes
};

typedef struct sMissionRenderParams sMissionRenderParams; 

//
// Init/Term
//
EXTERN void MissionRenderInit(); 
EXTERN void MissionRenderTerm(); 

//
// Get/Set
//
EXTERN const sMissionRenderParams* GetMissionRenderParams(void); 
EXTERN void SetMissionRenderParams(const sMissionRenderParams* params); 


//
// Set the mission pal as the 2d palette. 
//

EXTERN void MissionPalSet(); 



#endif // __MISSREND_H
