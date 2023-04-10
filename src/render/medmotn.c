/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/medmotn.c,v 1.4 2000/02/19 12:35:26 toml Exp $

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   Here's the function we install as the callback for
   portal_anim_medium_callback().

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#include <math.h>

#include <matrix.h>
#include <mprintf.h>
#include <r3d.h>

#include <portal.h>
#include <wr.h>
#include <medmotns.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


// Each of these corresponds to one element of portal_cell_motion[].
sMedMoCellMotion g_aMedMoCellMotion[MAX_CELL_MOTION];

// Here's how we keep rivers, which flow straight forever, from losing
// precision as their texture anchors head for the middle distance.
// We convert the position to an int, bit mask the result, and add the
// fraction back in.  We only do this if the texture is axis-aligned.
#define TRUNCATE_BITMASK 63

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   We have deltas for x, y, and theta, and update them in proportion
   to the time of the previous frame.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
void MedMoUpdateCellMotion(long frame_time_ms)
{
   int i;
   mxs_real frame_time;         // fraction of sec. since last frame

   frame_time = ((float) (frame_time_ms)) / 1000.0;

   for (i = 1; i < MAX_CELL_MOTION; i++) {
      if (portal_cell_motion[i].in_motion == FALSE)
         continue;

      mx_scale_addeq_vec(&(portal_cell_motion[i].center),
                         &(g_aMedMoCellMotion[i].center_change), frame_time);

      portal_cell_motion[i].angle
         += ((float)((short)(g_aMedMoCellMotion[i].angle_change)) * frame_time);

      // We can prevent sliding, surfaces from getting away,
      // provided they are axis-aligned and not rotating.
      if (portal_cell_motion[i].angle == 0
       && g_aMedMoCellMotion[i].angle_change == 0) {
         if (portal_cell_motion[i].center.x < -TRUNCATE_BITMASK
          || portal_cell_motion[i].center.x > TRUNCATE_BITMASK) {
            int int_x = (int) portal_cell_motion[i].center.x;
            int truncate = int_x & TRUNCATE_BITMASK;

            portal_cell_motion[i].center.x = (float) truncate
               + (portal_cell_motion[i].center.x - (float) int_x);
         }

         if (portal_cell_motion[i].center.y < -TRUNCATE_BITMASK
          || portal_cell_motion[i].center.y > TRUNCATE_BITMASK) {
            int int_y = (int) portal_cell_motion[i].center.y;
            int truncate = int_y & TRUNCATE_BITMASK;

            portal_cell_motion[i].center.y = (float) truncate
               + (portal_cell_motion[i].center.y - (float) int_y);
         }

         if (portal_cell_motion[i].center.z < -TRUNCATE_BITMASK
          || portal_cell_motion[i].center.z > TRUNCATE_BITMASK) {
            int int_z = (int) portal_cell_motion[i].center.z;
            int truncate = int_z & TRUNCATE_BITMASK;

            portal_cell_motion[i].center.z = (float) truncate
               + (portal_cell_motion[i].center.z - (float) int_z);
         }
      }
   }
}
