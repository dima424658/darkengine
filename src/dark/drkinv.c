/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkinv.c,v 1.15 2000/02/19 12:27:17 toml Exp $
// dark specific inventory stuff
// including expose of the ui

#include <config.h>

#include <command.h>
#include <matrix.h>

#include <frobctrl.h>
#include <headmove.h>
#include <playrobj.h>
#include <osystype.h>
#include <rendobj.h>
#include <objlight.h>
#include <simtime.h>
#include <objsys.h>
#include <objpos.h>
#include <physapi.h>
#include <pick.h>
#include <mprintf.h>

#include <drkinv.h>
#include <drkinvui.h>

// for player arm and skill stuff
#include <plycbllm.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

//////////////////////////////
// random global setup

static BOOL drkFrobSearch=FALSE;
static BOOL world_use_in_progress=FALSE;
static BOOL head_focus_active=FALSE;     // need to "actively" focus your head
static BOOL inv_tool_only=TRUE;          // only inv frobs can be tools

static BOOL allow_cycle=TRUE;
static BOOL allow_world_frob=TRUE;
static BOOL allow_inv_frob=TRUE;
static BOOL allow_world_focus=TRUE;

static int  ms_on_obj[2];  // world, inv

////////////////////
// inv actions (pop-up, pop-down, cycle, so on)

// controllers in dark for focus/actions
//
static int   head_focus_ang_tol=0x200;       // was 180
static float head_focus_vel2_tol=(0.6*0.6);  // was 0.4*0.4
static int   head_focus_persist_mul=4;
static float head_focus_speed2_tol=1.0;

static float head_focus_hilight_base=0.21;
static float head_focus_hilight_level=0.47;
static int   head_focus_hilight_ms=129;
static float head_focus_slow_head=0.1;

static int   head_time_off=0;

// These are exported since picking is part of the rendering pipeline
// in drkrend.c.
float head_focus_dist2_tol=64.0;
BOOL head_focus_nonfrob=FALSE;

void drkCheckHeadFocus(void)
{
   ObjID new_highlit=OBJ_NULL;
   int cur_sim_time=GetSimTime();

   if (g_PickCurrentObj != OBJ_NULL 
       && allow_world_focus
       && ValidWorldFocusTarget(g_PickCurrentObj))
   {
      float bonus_mul=1.0;
      BOOL new_val=TRUE;

      if (highlit_obj == g_PickCurrentObj)    // so we are less likely to flicker on/off
         bonus_mul=head_focus_persist_mul;
      if (new_val)
         if (g_PickCurrentObj==PlayerArm())
            new_val=FALSE;     // for now, ultra-special case this
         else if (head_focus_active && (!(world_use_in_progress||drkFrobSearch)))
            new_val=FALSE;     // not a world frob situation
         else
         {     // is the player moving too much
            float ang_rat=headmove_ang_disp/(float)(head_focus_ang_tol*bonus_mul);
            float vel_rat=headmove_vec2_disp/(float)(head_focus_vel2_tol*bonus_mul);
            if (ang_rat>1.0)
               new_val=FALSE;
            else if (ang_rat>head_focus_slow_head)  // turning some, normal vel cap
               new_val=(vel_rat<1.0);
            else if (vel_rat>4.0)   // can go faster if not turning much
               new_val=FALSE;
         }
      if (new_val)
         if (head_focus_speed2_tol>0)
         {
            mxs_vector vel;
            if (PhysObjHasPhysics(g_PickCurrentObj))
            {
               PhysGetVelocity(g_PickCurrentObj,&vel);
               if (mx_mag2_vec(&vel)>head_focus_speed2_tol)
                  new_val=FALSE;
            }
         }
      if (new_val)
         new_highlit=g_PickCurrentObj;
   }
   if (highlit_obj!=new_highlit)
   {
      BOOL do_switch=TRUE;
      if (new_highlit==OBJ_NULL)
      {
         do_switch=FALSE;
         if (head_time_off==0)
            head_time_off=cur_sim_time;
         if (head_time_off+head_focus_hilight_ms<=cur_sim_time)
         {
            do_switch=TRUE;
            head_time_off=0;
         }
      }
      else
         head_time_off=0;
      if (do_switch)
      {
         highlit_obj=new_highlit;
         FrobSetCurrent(kFrobLocWorld,kFrobLevSelect,highlit_obj);
         ms_on_obj[kFrobLocWorld]=cur_sim_time;
      }
   }
   else
      head_time_off=0;
   if (highlit_obj)
   {  // manage hilight fade here
      float use_level=head_focus_hilight_level;
      float use_base=head_focus_hilight_base;
      int ms_on;
      if (head_time_off)
      {
         ms_on=head_time_off+head_focus_hilight_ms-cur_sim_time;
         use_base=0; // want to fade all the way out
      }
      else
         ms_on=cur_sim_time-ms_on_obj[kFrobLocWorld];
      if (ms_on<head_focus_hilight_ms)
         use_level=(((float)ms_on/(float)head_focus_hilight_ms)*
                     (head_focus_hilight_level-use_base))+use_base;
      objlight_highlight_level=use_level;
   }
}


////////////////////
// Script Interactions

// change the current inventory capabilities...
void drkInvCapControl(eDrkInvCap cap, eDrkInvControl control)
{
   BOOL *var;

   switch (cap)
   {
      case kDrkInvCapCycle:      var=&allow_cycle; break;
      case kDrkInvCapWorldFrob:  var=&allow_world_frob; break;
      case kDrkInvCapWorldFocus: var=&allow_world_focus; break;
      case kDrkInvCapInvFrob:    var=&allow_inv_frob; break;
      default: Warning(("Invalid Cap %d to drkInvCapControl\n",cap)); return;
   }
   switch (control)
   {
      case kDrkInvControlOn:     *var=TRUE; break;
      case kDrkInvControlOff:    *var=FALSE; break;
      case kDrkInvControlToggle: *var=!(*var); break;
   }
}

//////////////////////////////
// actual commands, and initialization

void drkInvInit(void)
{  // we count on engfeat to initialize the FrobPropsInit();
#ifndef SHIP
   head_focus_active=config_is_defined("head_focus_active");   // configure head in world code
   head_focus_nonfrob=config_is_defined("head_focus_nonfrob");   // configure head in world code
   inv_tool_only=!config_is_defined("allow_world_tool");
   config_get_int("head_focus_ang_tol",&head_focus_ang_tol);
   config_get_float("head_focus_vel_tol",&head_focus_vel2_tol);
      head_focus_vel2_tol*=head_focus_vel2_tol;
   config_get_int("head_focus_persist_mul",&head_focus_persist_mul);
   config_get_float("head_focus_slow_head",&head_focus_slow_head);
   if (config_get_float("head_focus_dist_tol",&head_focus_dist2_tol))
      head_focus_dist2_tol*=head_focus_dist2_tol;
   if (config_get_float("head_focus_speed_tol",&head_focus_speed2_tol))
      head_focus_speed2_tol*=head_focus_speed2_tol;
   if (config_is_defined("highlight_level"))
   {
      int val;
      config_get_int("highlight_level",&val);
      head_focus_hilight_level=(float)val/100.0;
      head_focus_hilight_base=head_focus_hilight_level/3;
   }
   config_get_float("head_hilight_level",&head_focus_hilight_level);
   config_get_float("head_hilight_base",&head_focus_hilight_base);
   config_get_int("head_hilight_ms",&head_focus_hilight_ms);
#endif   
}
