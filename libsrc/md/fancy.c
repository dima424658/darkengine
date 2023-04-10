/*
 * $Source: x:/prj/tech/libsrc/md/RCS/fancy.c $
 * $Revision: 1.5 $
 * $Author: JAEMZ $
 * $Date: 1998/02/23 16:56:21 $
 *
 * Simplified Model Rendering Routines
 *
 */

#include <stdlib.h>
#include <stdio.h>

#include <mprintf.h>
#include <r3d.h>
#include <md.h>
#include <mdutil.h>


// linear breakpoint, this seems pretty safe
float mdd_linear_bpoint = 50.0;


// Returns the maximum pixel error in the biggest
// texture map in the model
float md_tmap_err(mds_model *m)
{
   float dist;
   float d; // diameter
   mxs_vector *z;

   // Make the error a sphere of max radius
   // away from the model sphere and compare max and min z values

   // Get distance squared from viewer
   r3_start_block();
   z = r3_get_view_in_obj();
   r3_end_block();

   // Get distance
   dist = mx_mag_vec(z);

   // Get the pgon diameter and
   // closest point to the viewer
   d = 2.0*m->max_pgon_radius;
   dist -= m->radius;

   // pixel size difference
   return d*r3_get_hscale()*(1.0/dist - 1.0/(dist+d));
}
   

// Pixel error below which you use linear texture
// maps used by md_fancy_render_model. 1 means pixel
// perfect.  Defaults to 1

void md_set_linear_breakpoint(float b)
{
   mdd_linear_bpoint = b;
}


// Prepares for rendering the model by first checking the bounding
// box, turning on clipping if it needs to be clipped,
// not if not, does either linear or perspective texturing
// depending on where the linear breakpoint is
// returns FALSE if not visible
BOOL md_fancy_setup_model(mds_model *m,mds_parm parms[])
{
   float err;
   int or_code,and_code;
   mxs_vector cen,z={0,0,0};

   // check bounding sphere
   r3_start_block();
   r3_transform_o2w(&cen,&z);
   r3_end_block();

   and_code = r3_clip_sphere(&cen,m->radius,&or_code);

   // off view, can't see
   if (and_code!=0) {
      return FALSE;
   }

   // clip or not
   if (or_code==0) {
      r3_set_clipmode(R3_NO_CLIP);
   } else {
      r3_set_clipmode(R3_CLIP);
   }

   // linear/perspective check
   // negative error means part is behind you
   err = md_tmap_err(m);

   md_set_tmap_linear((bool) (err > 0 && err < mdd_linear_bpoint));

   return TRUE;
}

// Renders the model by first checking the bounding
// box, turning on clipping if it needs to be clipped,
// not if not, does either linear or perspective texturing
// depending on where the linear breakpoint is
void md_fancy_render_model(mds_model *m,mds_parm parms[])
{
   if (md_fancy_setup_model(m, parms))
      md_render_model(m,parms);
}

// Does the fancy_render_model thing, but definitely clips
void md_fancy_cliphappy_render_model(mds_model *m,mds_parm parms[])
{
   if (md_fancy_setup_model(m, parms))
   {
      r3_set_clipmode(R3_CLIP);
      md_render_model(m,parms);
   }
}

