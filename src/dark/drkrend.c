/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkrend.c,v 1.30 2000/01/06 18:49:19 BFarquha Exp $
// air specific rendering code

#include <lgd3d.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <mprintf.h>

#include <portal.h>
#include <md.h>
#include <rendobj.h>
#include <camera.h>

#include <config.h>
#include <player.h>
#include <playrobj.h>
#include <particle.h>
#include <objpos.h>

#include <mnumprop.h>
#include <partprop.h>
#include <prjctile.h>

#include <simtime.h>
#include <rendprop.h>

// for pick hack for now
//#include <g2.h>

#include <plycbllm.h>
#include <weapbow.h> // for arrow

#include <osystype.h>
#include <frobprop.h>
#include <pick.h>
#include <drkinv.h>
#include <plycbllm.h>  // PlayerArm()
#include <weapbow.h>   // PlayerArrow()
#include <drkrutil.h>  // dark rendering utilities

#include <dbmem.h>     // must be last header

// hold old portal render call, if we need it
static void (*next_render_call)(ObjID o, uchar *clut, ulong fragment)=NULL;
static BOOL (*next_visible_call)(ObjID o);
static BOOL (*next_block_call)(ObjID o1, ObjID o2);
static void (*next_overlay_call)(void);

// this is at end of file
void dark_render_object(ObjID o, uchar *clut, ulong fragment);
BOOL dark_object_blocks(ObjID blocker, ObjID blockee);
void dark_render_overlays(void);

// set up our function pointers for the portal render object stuff
void dark_init_object_rend(void)
{
   extern void (*portal_render_overlays_cback)(void);

   next_render_call = portal_render_object;
   portal_render_object = dark_render_object;

   next_block_call = portal_object_blocks;
   portal_object_blocks = dark_object_blocks;

   next_overlay_call = portal_render_overlays_cback;
   portal_render_overlays_cback = dark_render_overlays;

   _darkru_attached_obj_init();
}

void dark_term_object_rend(void)
{
   _darkru_attached_obj_term();
}

BOOL dark_object_blocks(ObjID o1, ObjID o2)
{
   Camera* cam = PlayerCamera();  // @TODO: pass this in?
   ObjID o = CameraGetObjID(cam);

   // camera blocks everything
   if (o==o1)
      return TRUE;
   // nothing blocks camera
   if (o==o2)
      return FALSE;

   return next_block_call(o1,o2);
}

// THIS IS HORRIFYING!!

static const float fInvTime = 1.000/1000.000;

// the actual portal object callback for Dark
static void dark_render_object(ObjID o, uchar *clut, ulong fragment)
{
   Camera* cam = PlayerCamera();  // @TODO: pass this in?
   mxs_vector *loc = ObjPosGetLocVector(o);
   ObjID attached_obj=OBJ_NULL;

   // WHAT IS THIS!!!!!
   if ((CameraGetMode(cam) == FIRST_PERSON) && IsPlayerObj(o))
      return;           // if not a player, no effects, so head home

   if (IsPlayerObj(o))
   {
      int nTime;
      if (PROPERTY_GET(g_pIsInvisibleProperty, o, &nTime))
      {
         tSimTime nFrameDelta = GetSimFrameTime(); // tSimTime is a ulong
         if (nTime >= 0) // We are becomming invisible
         {
            float fAlpha;
            nTime += nFrameDelta;
            PROPERTY_SET(g_pIsInvisibleProperty, o, nTime);
            fAlpha = nTime*fInvTime;
            if (fAlpha > 1)
               fAlpha = 1;
            else if (fAlpha < 0)
               fAlpha = 0;

            PROPERTY_SET(gAlphaRenderProp, o, fAlpha);
         }
         else // we are becomming visible
         {
            float fAlpha;
            nTime -= nFrameDelta;
            PROPERTY_SET(g_pIsInvisibleProperty, o, nTime);
            fAlpha = 1+nTime*fInvTime;
            if (fAlpha > 1)
               fAlpha = 1;
            else if (fAlpha < 0)
               fAlpha = 0;

            PROPERTY_SET(gAlphaRenderProp, o, fAlpha);
         }
      }

      goto skip_pick;
   }

   // we suck!!!
#ifdef SHIP
   if (loc!=NULL)  // if we have a loc
      if (((*(int *)(&loc->x))|(*(int *)(&loc->y))|(*(int *)(&loc->z)))==0)
         return;   // with bitwise or of Zero, then skip it, ack
#endif

#ifndef OLD_PICK_METHOD
   if (fragment == OBJ_SPLIT_OTHER)
      goto skip_pick;

   if (!head_focus_nonfrob)
   {
      uchar frob_res = ObjFrobResultForLoc(kFrobLocWorld, o);
      if ((!frob_res) || ((frob_res & kFrobIgnore) == kFrobIgnore))
         goto skip_pick;
   }
   rendobj_seen = PickWeighObject(o);
#endif

skip_pick:
   // go check for attached stuff
   attached_obj=_darkru_setup_attached_objs(o);

   // Render the 3D model
   next_render_call(o, clut, fragment);

   // detach it here
   if (attached_obj)
      _darkru_remove_attached_objs(o);
}

static void dark_render_overlays(void)
{
   EXTERN BOOL g_lgd3d;
   EXTERN BOOL g_zbuffer;

   if ((g_lgd3d)&&(g_zbuffer))
      lgd3d_set_zcompare(FALSE);

   if (PlayerArm() != OBJ_NULL)
   {
      int nTime;
      if (PROPERTY_GET(g_pIsInvisibleProperty, PlayerObject(), &nTime))
      {
         tSimTime nFrameDelta = GetSimFrameTime(); // tSimTime is a ulong
         if (nTime >= 0) // We are becomming invisible
         {
            float fAlpha;
            nTime += nFrameDelta;
            PROPERTY_SET(g_pIsInvisibleProperty, PlayerObject(), nTime);
            fAlpha = nTime*fInvTime;
            if (fAlpha > 1)
               fAlpha = 1;
            else if (fAlpha < 0)
               fAlpha = 0;

            PROPERTY_SET(gAlphaRenderProp, PlayerArm(), fAlpha);
         }
         else // we are becomming visible
         {
            float fAlpha;
            nTime -= nFrameDelta;
            PROPERTY_SET(g_pIsInvisibleProperty, PlayerObject(), nTime);
            fAlpha = 1+nTime*fInvTime;
            if (fAlpha > 1)
               fAlpha = 1;
            else if (fAlpha < 0)
               fAlpha = 0;

            PROPERTY_SET(gAlphaRenderProp, PlayerArm(), fAlpha);
         }
      }

      rendobj_render_object(PlayerArm(),NULL,0);
   }

   if (PlayerArrow() != OBJ_NULL)
      rendobj_render_object(PlayerArrow(),NULL,0);

   if ((g_lgd3d)&&(g_zbuffer))
      lgd3d_set_zcompare(TRUE);

   if (next_overlay_call)
      next_overlay_call();
}
