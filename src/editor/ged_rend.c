/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/ged_rend.c,v 1.10 2000/02/19 13:10:46 toml Exp $

#include <g2.h>
#include <r3d.h>
#include <prof.h>

#include <ged_rend.h>
#include <ged_line.h>
#include <viewmgr.h>
#include <editbr.h>    // should change to brrend soon
#include <vbrush.h>
#include <brrend.h>
#include <brlist.h>

//#include <vumanui.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

// am i supposed to be showing the current selection?
BOOL gedrend_show_current=TRUE;


// silly callback function of correct type so i can do this idiotic
// group op silliness
static void wrapBrushDraw(editBrush *us) { brushDraw(us); }

// render the current brush list
void gedrendRenderView(int flags, int camera)
{  PROF
   editBrush *us;
   mxs_vector *cam_pos;
   mxs_angvec *cam_ang;

//   mprintf("grv %d w/%x\n",camera,flags);
   vm_get_3d_camera_loc(&cam_pos,&cam_ang);
   if (flags&GEDREND_FLAG_CURSORONLY)
      if (!gedrend_show_current)
         return;
      else
         flags|=brFlag_COLOR_NONE;
   else
      flags|=brFlag_NO_SELECTION;
   brushSetupDraw(flags,camera);
   if (flags&GEDREND_FLAG_CURSORONLY)
   {  // really need a multibrush solution here... eh?
      if (vBrush_inCurGroup(blistGet()))
         vBrush_GroupOp(FALSE,wrapBrushDraw);  // @TODO: remember how to do this cast... argh
      else
         brushDraw(blistGet());
   }
   else
   {
      editBrush *avoid=blistGet();
      int hIter;

      us=blistIterStart(&hIter);
      while (us!=NULL)
      {
         if ( gedrend_show_current ||
             (!vBrush_inCurGroup(us) && (us!=avoid)) )
            brushDraw(us);
         us=blistIterNext(hIter);
      }
      gedLineRenderAll();
   }
   END_PROF;
}

//                     BBGGRR
#define CAMERA_COLOR 0xF020E0 
int gedrend_camera_pos_color=0x1;

// Show the position of the main camera in a 2D view
void gedrendRenderCameraLoc (mxs_vector *cam_pos, mxs_angvec *cam_ang, mxs_real scale)
{
   r3s_point cam_start, cam_end; // begin and end of line representing camera
   mxs_vector origin, unit;      // same, in "camera space"

   // bind the camera color right now
   // @OPTIMIZE: precompute this
   gedrend_camera_pos_color = gr_make_screen_fcolor(CAMERA_COLOR);  

   mx_zero_vec (&origin);
   mx_unit_vec (&unit, 0);
   unit.x = 3.0;                 // ad hoc

   r3_start_object_angles (cam_pos, cam_ang, R3_DEFANG);
   r3_start_block ();
   {
      r3_transform_point (&cam_start, &origin);
      r3_transform_point (&cam_end,   &unit);

      r3_set_color (gedrend_camera_pos_color);
      r3_draw_line (&cam_start, &cam_end);     // this overflows if too close

      gr_set_fcolor (gedrend_camera_pos_color);
      gr_circle (cam_start.grp.sx, cam_start.grp.sy, FIX_UNIT / 80);
   }
   r3_end_block ();
   r3_end_object ();
}
