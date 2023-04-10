/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <mprintf.h>

#include <appagg.h>
#include <lgd3d.h>

#include <port.h>
#include <rendobj.h>
#include <render.h>

#include <dpccobpr.h>
#include <rendprop.h>
#include <prcniter.h>

#include <dpcpgapi.h>

#include <plycbllm.h>

#include <propbase.h> // for property iter struct

// look at all these wonderful things that I have to extern!

EXTERN void render_overlays(void);

EXTERN BOOL g_lgd3d;

// argh, why isn't this in a header somewhere?
enum bounds
{
   BOUNDING_RECTANGLE,
   BOUNDING_OCTAGON
};

EXTERN void GetObjScreenBounds(ObjID obj, fix *mn, fix *mx, enum bounds bound);

#define max(x, y) (((x)>(y))?(x):(y))

///////////////////////////////////////
// render a single overlay object
//
void DPC_render_overlay_obj(ObjID objID)
{
   if (g_lgd3d)
   {
      fix min[2], max[2];
      
      GetObjScreenBounds(objID, min, max, BOUNDING_RECTANGLE);
      if (((max[0]-min[0])>0) && ((max[1]-min[1])>0))
      {
         // clear z buffer
         // @HACK: awful hack to extend the y axis by -10 since our melee weapons don't return correct bounds data
         //lgd3d_clear_z_rect(fix_int(min[0]), max(fix_int(min[1])-10, 0), fix_int(fix_ceil(max[0])), fix_int(fix_ceil(max[1])));
         lgd3d_clear_z_rect(fix_int(min[0]), 0, fix_int(fix_ceil(max[0])), fix_int(fix_ceil(max[1])));
         // render the gun
         rendobj_render_object(objID, NULL, OBJ_NO_SPLIT);
      }
   }
   else
      // software - this won't sort right
      rendobj_render_object(objID, NULL, OBJ_NO_SPLIT);
}

//////////////////////////////////////
// render all overlay objects
// note: currently, these objects are not going to be z-sorted with each other
// not sure what the right behavior here is, but i'm trying to avoid z-clearing the whole screen
// because we usually only have one overlay object up (the gun)
//
EXTERN void DPC_render_overlays(void)
{
   ObjID objID;
   
   double   dCurZNear;
   double   dCurZFar;

   if (g_lgd3d) // set z params
   {
      lgd3d_get_znearfar( &dCurZNear, &dCurZFar );//remember Citadel
      SetZNearFar(0.1, 20);
   }

   // draw objects attached to camera
   cConcreteIter iter(g_CameraObjProperty);

   iter.Start();   
   while (iter.Next(&objID))
   {
      eRenderType_ renderType;
      cCameraObj *pCamObj;

      g_CameraObjProperty->Get(objID, &pCamObj);
      if (pCamObj->m_draw)
      {
         // @HACK: temp set render type to normal so that we can make it not rendered in non-overlay render
         renderType = eRenderType_(ObjRenderType(objID));
         ObjSetRenderType(objID, kRenderNormally);
         DPC_render_overlay_obj(objID);
         ObjSetRenderType(objID, renderType);
      }
   }
   iter.Stop();

   // draw the player gun, if any
   AutoAppIPtr(PlayerGun);
   if ((objID = pPlayerGun->GetHand()) == OBJ_NULL)
      objID = PlayerArm();
   if (objID != OBJ_NULL)
      DPC_render_overlay_obj(objID);

   if (g_lgd3d)
      // reset z params
      SetZNearFar( dCurZNear, dCurZFar );

   // call the base engine callback
   render_overlays();
}
