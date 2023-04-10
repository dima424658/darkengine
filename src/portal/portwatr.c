/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   portwatr.c

   All Portal knows about animated medium boundaries is where they are
   at the moment.  The timing of the motion is handles externally.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#include <math.h>
#include <matrix.h>
#include <mprintf.h>
#include <r3d.h>

#include <port.h>
#include <wrdb.h>
#include <wrdbrend.h>
#include <wrfunc.h>

#include <portwatr.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   Given a vector and a plane, we treat the vector as a point and put
   in in the plane by deriving its z from its x and y.  If the plane
   is vertical, we leave the vector alone.

   If Ax + By + Cz + D = 0, and C != 0, then z = (-Ax - By - D) / C.

   The ones for the other axis planes are equivalent.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static void project_vector_x_onto_yz(mxs_vector *v, PortalPlane *plane)
{
   if (plane->normal.x == 0.0)
      return;

   v->x = (-(plane->normal.y * v->y)
          - (plane->normal.z * v->z)
          - (plane->plane_constant)) / plane->normal.x;
}

static void project_vector_y_onto_zx(mxs_vector *v, PortalPlane *plane)
{
   if (plane->normal.y == 0.0)
      return;

   v->y = (-(plane->normal.z * v->z)
          - (plane->normal.x * v->x)
          - (plane->plane_constant)) / plane->normal.y;
}

static void project_vector_z_onto_xy(mxs_vector *v, PortalPlane *plane)
{
   if (plane->normal.z == 0.0)
      return;

   v->z = (-(plane->normal.x * v->x)
          - (plane->normal.y * v->y)
          - (plane->plane_constant)) / plane->normal.z;
}


// If we want texture scaling on water later, this should be replaced
// with a variable.
#define TEXTURE_UNIT_LENGTH 4.0

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   We apply our textures to the water by ignoring almost all the usual
   rendering information.  The anchor point is just the center of our
   rotation, and the (u, v) vectors are built from stratch.  The
   texture is projected straight down so that it'll roughly match up
   between surfaces which are almost horizontal.

   All the calculation is in world space.  At the end we transform the
   changed versions into camera space and store them via the dest_*
   pointers.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
void portal_position_portal_texture(mxs_vector *dest_u, mxs_vector *dest_v, 
                                    mxs_vector *dest_anchor,
                                    mxs_vector *anchor,
                                    PortalPolygonRenderInfo *render,
                                    PortalPlane *plane,
                                    PortalCellMotion *motion)
{
   mxs_vector u, v;
   mxs_real u_mag, v_mag;
   mxs_vector transformed_anchor;
   r3s_point anchor_point;

   // So finding the new anchor point and offsets into the texture is
   // a complex and interesting problem.  We'll blow it off and count
   // on the texture mapping to tile from the center point of our
   // rotation.
   transformed_anchor = motion->center;

   switch(motion->major_axis)
   {
      // The u and v vectors are built from scratch in (y, z).  The x
      // coordinate comes from projecting the (y, z) onto the plane.  We
      // build from the anchor point, then subtract that out once we've
      // got our x.  In 2d we can make v from u by rotating 90 degrees
      // using y2=z1, z2=-y1.
      case MEDIUM_AXIS_X:
      {
         project_vector_x_onto_yz(&transformed_anchor, plane);

         u.y = mx_cos(motion->angle) * TEXTURE_UNIT_LENGTH;
         v.z = -u.y;
         u.y += transformed_anchor.y;
         v.z += transformed_anchor.z;

         u.z = mx_sin(motion->angle) * TEXTURE_UNIT_LENGTH;
         v.y = u.z;
         u.z += transformed_anchor.z;
         v.y += transformed_anchor.y;

         project_vector_x_onto_yz(&u, plane);
         project_vector_x_onto_yz(&v, plane);
         break;
      }
      // as above, but for the y plane
      case MEDIUM_AXIS_Y:
      {
         project_vector_y_onto_zx(&transformed_anchor, plane);

         u.z = mx_cos(motion->angle) * TEXTURE_UNIT_LENGTH;
         v.x = -u.z;
         u.z += transformed_anchor.z;
         v.x += transformed_anchor.x;

         u.x = mx_sin(motion->angle) * TEXTURE_UNIT_LENGTH;
         v.z = u.x;
         u.x += transformed_anchor.x;
         v.z += transformed_anchor.z;

         project_vector_y_onto_zx(&u, plane);
         project_vector_y_onto_zx(&v, plane);
         break;
      }
      // and for that other plane
      case MEDIUM_AXIS_Z:
      {
         project_vector_z_onto_xy(&transformed_anchor, plane);

         u.x = mx_cos(motion->angle) * TEXTURE_UNIT_LENGTH;
         v.y = -u.x;
         u.x += transformed_anchor.x;
         v.y += transformed_anchor.y;

         u.y = mx_sin(motion->angle) * TEXTURE_UNIT_LENGTH;
         v.x = u.y;
         u.y += transformed_anchor.y;
         v.x += transformed_anchor.x;

         project_vector_z_onto_xy(&u, plane);
         project_vector_z_onto_xy(&v, plane);
         break;
      }
   }

   // We've been in world space, so it's time for camera space.
   mx_subeq_vec(&u, &transformed_anchor);
   mx_subeq_vec(&v, &transformed_anchor);

   u_mag = mx_mag_vec(&u);
   v_mag = mx_mag_vec(&v);

   render->texture_mag = (u_mag > v_mag)? u_mag : v_mag;

   r3_rotate_o2c(dest_u, &u);
   r3_rotate_o2c(dest_v, &v);
   r3_transform_point(&anchor_point, &transformed_anchor);
   *dest_anchor = anchor_point.p;
}

