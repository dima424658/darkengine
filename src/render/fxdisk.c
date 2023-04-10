/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $ Header: $

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   fxdisk.c

   This is descended directly from Sean's disk effects code in TN.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */

#include <stdlib.h>
#include <string.h>

#include <lg.h>
#include <2d.h>
#include <r3d.h>

#include <camera.h>
#include <playrobj.h>

#include <fxspans.h>
#include <fxspan.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


#define kUnclippedDiskMaxSpans 768

static sFXSpan span_list[kUnclippedDiskMaxSpans];
static sFXSpanSet disk_set = { &span_list[0], 0, 0, 0, 0 };
static sFXSpanSnarf disk_snarf = { 0, 0, 0, 0, 0 };


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   This generates the span list for one disk, unclipped.

   Foley and van Dam: the wizards of the DDA!

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static void MakeDiskSpans(sFXSpanSet *set, fix radius)
{
   fix x, y, d, delta_e, delta_se;
   int height = fix_int(radius * 2);
   // The upper span is for the top two octants.
   sFXSpan *upper_span = set->span_list;
   // The lower span is for the next two octants down (the lower two
   // in the top semicircle).
   sFXSpan *lower_span = upper_span + fix_int(radius);
   // These two are used at the end when we make the bottom half of
   // the circle a mirror image of the top half.
   sFXSpan *top_span = upper_span;
   sFXSpan *bottom_span = upper_span + (height - 1);

   x = 0;
   y = radius;
   d = FIX_UNIT - radius;
   delta_e = fix_make(3, 0);
   delta_se = -2 * radius + fix_make(5, 0);

   // This gives us the two top quadrants.  We only advance y in the
   // vertical case (the else clause).
   for (y = radius; y > x; ) {
      lower_span->left = fix_int(radius - y);
      lower_span->length = fix_int(2 * y);
      --lower_span;

      if (d < 0) {
         d += delta_e;
         delta_e += fix_make(2, 0);
         delta_se += fix_make(2, 0);
         x += FIX_UNIT;
      } else {
         upper_span->left = fix_int(radius - x);
         upper_span->length = fix_int(2 * x);
         ++upper_span;

         d += delta_se;
         delta_e += fix_make(2, 0);
         delta_se += fix_make(4, 0);
         x += FIX_UNIT;
         y -= FIX_UNIT;
      }
   }

   // This cleans up where the octants meet.
   upper_span->left = fix_int(radius - x);
   upper_span->length = fix_int(2 * x);

   // The bottom half is the mirror image of the top half.
   for (y = fix_int(radius); y >= 0; --y) {
      *bottom_span = *top_span;
      --bottom_span;
      ++top_span;
   }

   set->first_span_used = 0;
   set->num_spans_used = height;
   set->bounding_width = height;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   This distorts a roundish part of the screen in that wonderful
   heat-disk way.  The x and y arguments refer to the centers of the
   disks, and we copy concentric sections of the first disk onto the
   rest.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
void FXDiskDrawHeatDisks(fix first_x, fix first_y, fix first_radius, 
                         fix last_x, fix last_y, fix last_radius,
                         int num_disks)
{
   int i;
   fix fix_num_disks = fix_make(num_disks, 0);
   fix delta_radius = fix_div(last_radius - first_radius, fix_num_disks);
   fix delta_x_in_canvas = fix_div(last_x - first_x, fix_num_disks);
   fix delta_y_in_canvas = fix_div(last_y - first_y, fix_num_disks);
   fix x_in_canvas = first_x - first_radius;
   fix y_in_canvas = first_y - first_radius;
   fix x_in_snarf = fix_make(0, 0);
   fix y_in_snarf = fix_make(0, 0);
   fix radius = first_radius;
   int size = fix_int(radius * 2) + 1;

   MakeDiskSpans(&disk_set, radius);
   if (!FXSpanClipToCanvas(&disk_set, grd_canvas, x_in_canvas, y_in_canvas))
      return;
   FXSpanSetBoundingWidth(&disk_set);

   // snarf the part of the screen we'll be writing from
   FXSpanSnarfSetup(&disk_snarf, size, size, grd_canvas);
   FXSpanSnarf(&disk_snarf, &disk_set, grd_canvas, x_in_canvas, y_in_canvas);

   for (i = 0; i < num_disks; ++i) {
      x_in_canvas += delta_x_in_canvas;
      y_in_canvas += delta_y_in_canvas;

      // The coordinates in the canvas are for the upper-left corner
      // of the disk's bounding box.
      if (FXSpanClipToCanvas(&disk_set, grd_canvas, 
                             x_in_canvas + x_in_snarf, 
                             y_in_canvas + y_in_snarf))
         FXSpanDrawSetFromSnarf(&disk_set, &disk_snarf, grd_canvas, 
                                x_in_canvas + x_in_snarf,
                                y_in_canvas + y_in_snarf,
                                x_in_snarf, y_in_snarf);

      x_in_snarf -= delta_radius;
      y_in_snarf -= delta_radius;
      radius += delta_radius;

      MakeDiskSpans(&disk_set, radius);
   }

   // free memory used in snarfing
   FXSpanUnsnarf(&disk_snarf);
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   This is more of a hack than a routine, but if the shapes are small
   enough we should get away with it.  There's also probably some
   inefficiency from makeing this through cut-and-paste.

   Ha, ha!  The world will never know.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
void FXDiskDrawHeatDisksUnclipped(fix first_x, fix first_y, fix first_radius, 
                                  fix last_x, fix last_y, fix last_radius,
                                  int num_disks)
{
   int i;
   fix x_source, y_source, x_destination, y_destination;

   fix fix_num_disks = fix_make(num_disks, 0);
   fix fix_num_deltas = max(fix_num_disks - FIX_UNIT, FIX_UNIT);
   fix delta_radius = fix_div(last_radius - first_radius, fix_num_deltas);
   fix delta_x_in_canvas = fix_div(last_x - first_x, fix_num_deltas);
   fix delta_y_in_canvas = fix_div(last_y - first_y, fix_num_deltas);
   fix x_in_canvas = first_x - first_radius;
   fix y_in_canvas = first_y - first_radius;
   fix x_in_square = fix_make(0, 0);
   fix y_in_square = fix_make(0, 0);
   fix radius = first_radius;

   for (i = 0; i < num_disks; ++i) {

      if (fix_int(radius * 2) > kUnclippedDiskMaxSpans)
         continue;

      MakeDiskSpans(&disk_set, radius);

      // The coordinates in the canvas are for the upper-left corner
      // of the disk's bounding box.
      x_in_canvas += delta_x_in_canvas;
      y_in_canvas += delta_y_in_canvas;

      x_source = x_in_canvas + x_in_square;
      y_source = y_in_canvas + y_in_square;
      x_destination = x_source + delta_x_in_canvas;
      y_destination = y_source + delta_y_in_canvas;

      // This is what we do instead of clipping.
      if (((x_source)              <= grd_canvas->gc.clip.f.left)
       || ((x_source + 2 * radius) >= grd_canvas->gc.clip.f.right)
       || ((y_source)              <= grd_canvas->gc.clip.f.top)
       || ((y_source + 2 * radius) >= grd_canvas->gc.clip.f.bot)

       || ((x_destination)              <= grd_canvas->gc.clip.f.left)
       || ((x_destination + 2 * radius) >= grd_canvas->gc.clip.f.right)
       || ((y_destination)              <= grd_canvas->gc.clip.f.top)
       || ((y_destination + 2 * radius) >= grd_canvas->gc.clip.f.bot))
         continue;

      FXSpanDrawSetFromCanvas(&disk_set, grd_canvas, 
                              x_source, y_source,
                              x_destination, y_destination);

      x_in_square -= delta_radius;
      y_in_square -= delta_radius;
      radius += delta_radius;
   }
}


#define kMinDiskRadius    2
#define kMaxDiskRadius    200

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   Here's where we convert disks from world coordinates to the screen.
   Mostly it's just projection, though we also decide how many disks
   to draw, based on radii and distance.  We don't bother with
   anything form not totally on the screen.

   num_disks_at_z_1 applies to the first vector.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
void FXDiskDrawHeatDisksInWorld(mxs_vector *center_1, 
                                mxs_real first_radius, 
                                mxs_vector *center_2, 
                                mxs_real last_radius,
                                int num_disks_at_z_1)
{
   mxs_vector world_points[2];
   r3s_point screen_points[2];
   mxs_real real_num_disks;
   mxs_real scaled_first_radius;
   mxs_real scaled_last_radius;
   fix fix_first_radius;
   fix fix_last_radius;
   mxs_vector camera_vector;
   mxs_angvec dummy_angvec;
   Camera *camera;
   mxs_real distance;
   int num_disks;

   world_points[0] = *center_1;
   world_points[1] = *center_2;

   r3_start_block();

   r3_transform_block(2, &screen_points[0], &world_points[0]);

   // We won't draw anything that's even partly behind us.
   if (screen_points[0].grp.w > 25.0
    || screen_points[0].grp.w < 0.0
    || screen_points[1].grp.w > 25.0
    || screen_points[1].grp.w < 0.0)
      goto finished;

   scaled_first_radius
      = min(kMaxDiskRadius, 
            max(kMinDiskRadius, 
                first_radius * screen_points[0].grp.w * grd_canvas->bm.w));
   fix_first_radius = fix_from_float(scaled_first_radius);

   scaled_last_radius
      = min(kMaxDiskRadius, 
            max(kMinDiskRadius, 
                last_radius * screen_points[1].grp.w * grd_canvas->bm.w));
   fix_last_radius = fix_from_float(scaled_last_radius);

   // We use the distance to the first center for finding the number
   // of disks because it's faster than using both.
   camera = PlayerCamera();
   CameraGetLocation(camera, &camera_vector, &dummy_angvec);
   distance = mx_dist_vec(center_1, &camera_vector);

   real_num_disks = num_disks_at_z_1 / distance;
   num_disks = (int) real_num_disks;
   if (num_disks < 2)
      if (num_disks_at_z_1 == 1)
         num_disks = 1;
      else
         num_disks = 2;

   if (screen_points[0].grp.w < screen_points[1].grp.w)
      FXDiskDrawHeatDisksUnclipped(screen_points[0].grp.sx, 
                                   screen_points[0].grp.sy, 
                                   fix_first_radius, 
                                   screen_points[1].grp.sx, 
                                   screen_points[1].grp.sy, 
                                   fix_last_radius, 
                                   num_disks);
   else
      FXDiskDrawHeatDisksUnclipped(screen_points[0].grp.sx, 
                                   screen_points[0].grp.sy, 
                                   fix_first_radius, 
                                   screen_points[1].grp.sx, 
                                   screen_points[1].grp.sy, 
                                   fix_last_radius, 
                                   num_disks);

finished:
   r3_end_block();
}
