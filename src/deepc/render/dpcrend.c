/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// Deep Cover specific rendering code

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <r3d.h>
#include <md.h>
#include <mds.h>

#include <2d.h>
#include <rect.h>
#include <mprintf.h>
#include <mouse.h>

#include <lresname.h>
#include <lress.h>
#include <portal.h>
#include <fixtime.h>
#include <editobj.H>
#include <camera.h>
#include <simtime.h>
#include <render.h>

#include <config.h>
#include <player.h>
#include <playrobj.h>
#include <particle.h>
#include <globalid.h>
#include <objscale.h>

#include <mnumprop.h>
#include <partprop.h>
#include <prjctile.h>

#include <objmodel.h>
#include <rendobj.h>
#include <robjbase.h> 
#include <pick.h>
#include <dpcpick.h>

#include <dpcgame.h>
#include <dpchud.h>
#include <dpcprop.h>
#include <dpcrndov.h>

//zb for your hacks, only
#include <lgd3d.h >

// because pick.c doesn't export anything
#define kPickInfinity 1000000000.0

// globals used for distance pick
ObjID g_distPickObj;
static float g_distBestPickWeight;

 // hold old portal render call, if we need it
void (*next_render_call)(ObjID o, uchar *clut, ulong fragment)=NULL;
 // this is at end of file
void DPC_render_object(ObjID o, uchar *clut, ulong fragment);


// set up our function pointers for the portal render object stuff
void DPC_init_object_rend(void)
{
   next_render_call = portal_render_object;
   portal_render_object = DPC_render_object;
}

void DPC_term_object_rend(void)
{
}

/*
void set_basic_lighting(void)
{           
   md_light_set_type(MD_LT_AMB|MD_LT_DIFF);
   mdd_sun_vec.el[0]=0.6;
   mdd_sun_vec.el[1]=0;
   mdd_sun_vec.el[2]=0.8;
   mdd_lt_amb=0.3;
   mdd_lt_diff=0.7;
}
*/

enum bounds
{
   BOUNDING_RECTANGLE,
   BOUNDING_OCTAGON
};
 
EXTERN void GetObjScreenBounds(ObjID obj, fix *mn, fix *mx, enum bounds bound);

void DPC_pick_reset(void)
{
   g_distPickObj = OBJ_NULL;
   g_distBestPickWeight = kPickInfinity;
}

// the actual portal object callback for AIR
void DPC_render_object(ObjID o, uchar *clut, ulong fragment)
{
   Camera* cam = PlayerCamera();  // @TODO: pass this in? 
   fix mn[2],mx[2];
   Rect r;
   int t;
   int nAlphaWasOn; //zb

   // never show the camera object
   if (o==CameraGetObjID(cam))
      return;

   if (fragment != OBJ_SPLIT_OTHER)
   {
      rendobj_seen = PickWeighObject(o);
      // do our distance pick
      DPCPickWeighObject(o, 0, &g_distPickObj, &g_distBestPickWeight);
   }

   // tell the HUD system to draw our rectangle
   if (PROPERTY_GET(gPropHUDTime,o,&t))
   {
      if ((t != 0) && (GetSimTime() > t))
         IProperty_Delete(gPropHUDTime,o);
      else
      {
         GetObjScreenBounds(o, mn, mx, BOUNDING_RECTANGLE);
         r.ul = MakePoint(fix_int(mn[0]),fix_int(mn[1]));
         r.lr = MakePoint(fix_int(mx[0]),fix_int(mx[1]));
         DPCHUDDrawRect(r,o);
      }
   }

   //zb hack it to the limit!
   //@TODO: figure out where this thing should be
   nAlphaWasOn = lgd3d_is_alpha_blending_on();
   if( nAlphaWasOn )
       lgd3d_set_blend( FALSE );

   // Render the 3D model 
   next_render_call(o,clut,fragment);

   //zb on the hack again
   if( nAlphaWasOn )
       lgd3d_set_blend( TRUE );
}

EXTERN void (*portal_render_overlays_cback)(void);

void DPC_init_renderer(void)
{
   // this is a hack to try to get our guns sorting properly
   // we may have to take it out if it causes other sorting problems in the distance
   // hey, look we took it out
   //   SetZNearFar(1, 200);
   portal_render_overlays_cback = DPC_render_overlays;
}

void DPC_term_renderer(void)
{
}
