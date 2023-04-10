/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/vismeter.cpp,v 1.11 2000/02/19 13:09:05 toml Exp $

#include <vismeter.h>
#include <inv_rend.h>
#include <invrndpr.h>

#include <2d.h>

#include <aiprcore.h>
#include <aivision.h>
#include <playrobj.h>
#include <objmodel.h>

#include <config.h>
#include <cfgdbg.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

////////////////////////////////////////////////////////////
//
// VISIBILITY METER IMPLEMENTATION
//

struct sVisMeterState
{
   invRendState* rend; 
   int last_val; 
   int last_time; 
};

static sVisMeterState gState = { NULL, 0, 0, }; 

#ifndef DEEPC
#define VISMETER_MODELNAME "uicry%02d"
#else
#define VISMETER_MODELNAME "watch%02d"
#endif // DEEPC

#define NUM_MODELS 16

static int  vismeter_md_idxs[NUM_MODELS];
static uint vismeter_cutoffs[NUM_MODELS];
static int  vismeter_refresh = 50; 
static int  vismeter_frame_skip = 2; 

void VisMeterEnterMode()
{
   gState.last_time = 0; 
   gState.last_val = -1; 
   Rect r;
   int i;

   r.ul.x = 0;
   r.ul.y = 0;
   r.lr.x = grd_canvas->bm.w;
   r.lr.y = grd_canvas->bm.h;
#ifndef DEEPC
   sInvRenderType type = { kInvRenderModel, "uicry01" }; 
#else
   sInvRenderType type = { kInvRenderModel, "watch01" }; 
#endif // DEEPC

   gState.rend = invRendBuildStateFromType(INVREND_HARDWARE_QUEUE,&type,&r,NULL); 

   sInvRendView view;
   invRendGetView(gState.rend,&view); 

   int zoom = 100; 
   config_get_int("vismeter_zoom",&zoom);
   view.cam_dist *= 1.0*zoom/100; 
   
   int ang[3] = { 0, 0, 0};
   int angcnt = 3;
   config_get_value("vismeter_ang",CONFIG_INT_TYPE,ang,&angcnt); 
   for (i = 0; i < 3 ; i++)
      view.off_ang.el[i] += ang[i]*FIXANG_PI/180;

   config_get_int("vismeter_refresh",&vismeter_refresh); 
   config_get_int("vismeter_frame_skip",&vismeter_frame_skip); 

   invRendSetView(gState.rend,&view); 

   for (i=0; i<NUM_MODELS; i++)
   {
      char buf[64];
      sprintf(buf,VISMETER_MODELNAME,i+1);
      vismeter_md_idxs[i]=objmodelLoadModel(buf);
      if (vismeter_md_idxs[i]!=MIDX_INVALID)
         objmodelIncRef(vismeter_md_idxs[i]);
   }
}

void VisMeterExitMode()
{
   if (gState.rend)
   {
      invRendFreeState(gState.rend);
      gState.rend = NULL; 

      for (int i=0; i<NUM_MODELS; i++)
         if (vismeter_md_idxs[i]!=MIDX_INVALID)
            objmodelDecRef(vismeter_md_idxs[i]);
   }
}

void VisMeterSetCutoffs(const int* cutoffs, int n)
{
   Assert_(n >= 0 && n < NUM_MODELS);
   memcpy(vismeter_cutoffs,cutoffs,sizeof(*cutoffs)*n); 
}

static void recompute()
{
   // Compute cutoffs

   int level = 100; 
   sAIVisibilityControl * pVisCtrl = AIGetVisCtrl(PlayerObject());
   
   int low = pVisCtrl->lowVisibility; 
   int med = pVisCtrl->midVisibility; 
   int hi  = pVisCtrl->highVisibility; 

   int i; 
   int n = 2*NUM_MODELS/3; 
   for (i = 0; i < n; i++)
      vismeter_cutoffs[i] = (hi*i + low*(n-i))/n;

   for (i = 0 ; i < NUM_MODELS - n - 1; i++)
      vismeter_cutoffs[i + n] = (100*i + hi*(NUM_MODELS - n - i - 1))/(NUM_MODELS - n - 1 );

   vismeter_cutoffs[NUM_MODELS-1] = 0x7FFFFFFF;

   //
   // Compute model
   //
   
   sAIVisibility* vis = AIGetVisibility(PlayerObject()); 
   if (vis)
      level = vis->level; 

   for (i = 0; i < NUM_MODELS; i++)
   {
      ConfigSpew("vis_meter_spew",("%d ",vismeter_cutoffs[i])); 
      if (level < vismeter_cutoffs[i])
         break; 
   }
   ConfigSpew("vis_meter_spew",("\n")); 

   Assert_(i < NUM_MODELS); 

   // creep last_val towards i
   if (gState.last_val == -1)
      gState.last_val = i; 
   else
   {
      // figure out magnitude and direction of change
      int absdiff = i - gState.last_val; 
      int sign = 1;
      if (absdiff < 0) 
         sign = -1, absdiff = -absdiff; 

      // clamp absdiff
      if (absdiff >= vismeter_frame_skip)
         absdiff = vismeter_frame_skip; 
      
      gState.last_val += absdiff*sign; 
   }

   ConfigSpew("vis_meter_spew",("Player vis level at %d, using model %d on the way to %d\n",level,gState.last_val,i)); 

   sInvRenderType type = { kInvRenderModel }; 
   sprintf(type.resource.text,VISMETER_MODELNAME,NUM_MODELS-gState.last_val); 
   invRendSetType(gState.rend,&type); 
}

void VisMeterUpdate(ulong time, const Rect* r)
{
   if (!PlayerObjectExists())
      return;

   if (time - gState.last_time > vismeter_refresh  || gState.last_val == -1)
   {
      recompute(); 
      gState.last_time = time; 
   }

   invRendSetRect(gState.rend, r); 

   invRendDrawUpdate(gState.rend); 
}




