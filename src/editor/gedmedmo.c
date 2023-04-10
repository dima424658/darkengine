/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/gedmedmo.c,v 1.3 2000/02/19 13:10:42 toml Exp $

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   gedmedmo.c

   Here's the part of water motion editing which hooks into the
   wonderful world of brushes.

   We can set the motion_index for each PortalCell right after we
   portalize, or when a flow brush is moved.

   We do this for one brush by comparing the middle (the average of
   the vertices) of every cell in the world rep to the brush.  This is
   GEdMedMoMarkWaterOneBrush.

   When we've just portalized, we have to traverse the whole brush
   database.  This would be GEdMedMoMarkWaterAllBrushes.  This also
   calls EdMedMoClearMediumMotionInPortals, which sets all of the motion
   indices to 0.

   Note that we expect motion index 0 to be something safe, like water
   which is not moving.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#include <matrix.h>

#include <editbr_.h>
#include <brinfo.h>
#include <brlist.h>
#include <edmedmo.h>
#include <media.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


// For now, we're only expecting six-sided OBBs.
#define MAX_PLANES_IN_BRUSH 6
static mxs_vector brush_plane_vector[MAX_PLANES_IN_BRUSH];
static mxs_real brush_plane_constant[MAX_PLANES_IN_BRUSH];

// This is pretty damn silly.  But it could let us be flexible later.
static int brush_plane_count = 6;


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   Here's the function we pass in to EdMedMoSetCells in edmedmo.c.  It
   tells use whether a given point is within a the bounding box set up
   by SetBrushBounds, below.  Note that planes face outwards.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static bool PointWithinBrush(mxs_vector *point)
{
   int i;

   for (i = 0; i < brush_plane_count; i++) {
      if ((mx_dot_vec(point, &brush_plane_vector[i])
         + brush_plane_constant[i]) > 0.0)
         return FALSE;
   }

   return TRUE;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   This is swiped wholesale from Chris's OBB code.  If we want
   anything besides OBBs, we should generalize this to handle
   arbitraray polyhedra.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static void SetBrushBounds(editBrush *brush)
{
   mxs_matrix R;           // rotation matrix of brush
   mxs_vector u;           // normal before rotation
   mxs_vector point;       // point on plane of brush
   int i;

   // find rotation matrix for brush orientation
   mx_ang2mat(&R, &(brush->ang));

   // iterate through faces of the brush
   for (i = 0; i < 6; ++i) {

      // generate unit vector on given axis
      mx_unit_vec(&u, i % 3);
      if (i >= 3)
         mx_scaleeq_vec(&u, -1.0);

      // rotate to brush orientation
      mx_mat_mul_vec(&brush_plane_vector[i], &R, &u);

      // find point on plane of face 
      mx_scale_add_vec(&point, &(brush->pos), 
                       &brush_plane_vector[i], brush->sz.el[i % 3]);

      // here's our plane constant
      brush_plane_constant[i] = -mx_dot_vec(&brush_plane_vector[i], &point);
   }
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   This spreads the effect of a single brush through the world
   database.

   When we change a flow brush, we want to call it first with the
   brush's previous geometry and medium index 0 to clear things out.
   Then with the new geometry and real index.  But that code's not
   here.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
void GEdMedMoMarkWaterOneBrush(editBrush *brush, mxs_vector *seed)
{
   uchar medium_motion = brFlow_Index(brush);

   sEdMedMoSurface* info = &g_aEdMedMoSurface[medium_motion]; 

   if (!GetWaterTexIDs(info->texture_name_base,
                       &info->texture_above,
                       &info->texture_below))
      Warning(("No such water texture as %s\n",info->texture_name_base)); 

   SetBrushBounds(brush);
   EdMedMoSetCells(seed, MEDIA_WATER, medium_motion, &PointWithinBrush);
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   When we want to set the motion for every cell in the world rep,
   using all the brushes which do that sort of thing, we do this.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
void GEdMedMoMarkWaterAllBrushes(void)
{
   editBrush *b;
   int hIter;

   EdMedMoClearMediumMotionInPortals();

   b = blistIterStart(&hIter);

   while (b != NULL) {
      if (brushGetType(b) == brType_FLOW)
         GEdMedMoMarkWaterOneBrush(b, &(b->pos));
      b = blistIterNext(hIter);
   }
}
