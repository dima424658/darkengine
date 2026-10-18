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

// NewDark's LM_PARAM 0.1 block.  These are deliberately fixed-width disk
// fields: the same block is the handoff contract for the KCTools sidecar.
struct sMissionLightmapParams
{
   int data_size;
   float attenuation;
   float saturation;
   uint shadow_type;
   uint shadow_softness;
   float center_weight;
   uint shadow_depth;
   BOOL lightmapped_water;
   int lightmap_scale;
   uint anim_light_cutoff;
};

typedef struct sMissionLightmapParams sMissionLightmapParams;

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
EXTERN const sMissionLightmapParams* GetMissionLightmapParams(void);
EXTERN void SetMissionLightmapParams(const sMissionLightmapParams* params);


//
// Set the mission pal as the 2d palette. 
//

EXTERN void MissionPalSet(); 



#endif // __MISSREND_H
