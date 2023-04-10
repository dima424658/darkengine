// $Header: x:/prj/tech/libsrc/r3d/RCS/cliptest.c 1.4 1997/08/21 20:42:25 JAEMZ Exp $   Internal clipping of primitives

#include <clipcon_.h>
#include <clipprim.h>
#include <clipu.h>
#include <matrix.h>

#define cclip   (r3d_state.cur_con->clip)

// Clip test primitives for bbox and sphere

// pass in *float
// returns 1 if negative
#define FSIGN(flt) ((*(unsigned int *)(flt))>>31)

// Used to avoid branching
static float x_val[2];
static float y_val[2];
static float z_val[2];


// Clip a world space bounding box against the planes.  
// including user installed planes
// returns the and of all the clip codes
// So if non-zero, the bbox is invisible.  Stuffs the
// or or code
// Now we could of course reduce these to a bit of accuracy and
// save some more time
int r3_clip_bbox(mxs_vector *bmin,mxs_vector *bmax,int *ret_or_code)
{
   mxs_plane *plane;
   int num_planes = 5;
   int and_code=0,or_code=0;
   float dot_val;

   // load up globals so we don't have to branch
   x_val[0] = bmin->x;
   y_val[0] = bmin->y;
   z_val[0] = bmin->z;

   x_val[1] = bmax->x;
   y_val[1] = bmax->y;
   z_val[1] = bmax->z;

   // count from top down, that way you get to eliminate a shifting
   // variable for the clip codes.
   plane = &(r3d_state.cur_con->clip.world_clip[num_planes]);

   // Iterate over the planes
   while(num_planes--) {
      plane--;

      and_code <<= 1;
      or_code  <<= 1;

      // find dot product of near point
      dot_val = plane->d;
      dot_val += x_val[FSIGN(&plane->x)]*plane->x;
      dot_val += y_val[FSIGN(&plane->y)]*plane->y;
      dot_val += z_val[FSIGN(&plane->z)]*plane->z;

      // if near_val negative or_code is set
      // and you need to compute far
      if (dot_val<0) {
         or_code |= 1;

         // Find dot prod of far point, assert and code if present
         dot_val = plane->d;
         dot_val += x_val[1-FSIGN(&plane->x)]*plane->x;
         dot_val += y_val[1-FSIGN(&plane->y)]*plane->y;
         dot_val += z_val[1-FSIGN(&plane->z)]*plane->z;
         and_code |= FSIGN(&dot_val);
         // We could at this point return if we wanted trivial reject
      }
   }

   // user installed clip planes woo hoo
   num_planes = cclip.num_user_planes;
   if (num_planes) {
      r3s_clip_plane *uplane; //user installed
      uplane = ((r3s_clip_plane *)cclip.clip_data) + num_planes;

      // Iterate over the planes
      while(num_planes--) {
         uplane--;
         plane = &(uplane->world_plane);

         and_code <<= 1;
         or_code  <<= 1;

         // find dot product of near point
         dot_val = plane->d;
         dot_val += x_val[FSIGN(&plane->x)]*plane->x;
         dot_val += y_val[FSIGN(&plane->y)]*plane->y;
         dot_val += z_val[FSIGN(&plane->z)]*plane->z;

         // if near_val negative or_code is set
         // and you need to compute far
         if (dot_val<0) {
            or_code |= 1;

            // Find dot prod of far point, assert and code if present
            dot_val = plane->d;
            dot_val += x_val[1-FSIGN(&plane->x)]*plane->x;
            dot_val += y_val[1-FSIGN(&plane->y)]*plane->y;
            dot_val += z_val[1-FSIGN(&plane->z)]*plane->z;
            and_code |= FSIGN(&dot_val);
            // We could at this point return
         }
      }
   }

   *ret_or_code = or_code;
   return and_code;
}


// This routine clips a bounding sphere in world space.
// It returns the and_ccode of all the clip codes and
// stuffs the or_code.  Since it returns and_ccode you can use it as a
// branch for trivial reject if non-zero.  
int r3_clip_sphere(mxs_vector *cen,float rad,int *ret_or_code)
{
   mxs_plane *plane = r3d_state.cur_con->clip.world_clip;
   int num_planes = 5;
   int and_code=0,or_code=0;
   float dot_val;

   plane = &(r3d_state.cur_con->clip.world_clip[num_planes]);

   // Iterate over the planes
   // Pity these aren't all together
   while(num_planes--) {

      --plane;
      or_code <<= 1;
      and_code <<= 1;
 
      // find dot product of near point
      dot_val = plane->d + mx_dot_vec(cen,&plane->v);

      // Look, no branching for us!
      dot_val -= rad;
      or_code  |= FSIGN(&dot_val);
      dot_val += rad*2;
      and_code |= FSIGN(&dot_val);
   }

   // user installed clip planes woo hoo
   num_planes = cclip.num_user_planes;
   if (num_planes) {
      r3s_clip_plane *uplane; //user installed
      uplane = ((r3s_clip_plane *)cclip.clip_data) + num_planes;

      // Iterate over the planes
      while(num_planes--) {
         uplane--;
         plane = &(uplane->world_plane);

         or_code <<= 1;
         and_code <<= 1;
 
         // find dot product of near point
         dot_val = plane->d + mx_dot_vec(cen,&plane->v);

         // Look, no branching for us!
         dot_val -= rad;
         or_code  |= FSIGN(&dot_val);
         dot_val += rad*2;
         and_code |= FSIGN(&dot_val);
      }
   }

   *ret_or_code = or_code;
   return and_code;
}


// This routine clips codes a point in world space.
// It returns the code
int r3_clip_point(mxs_vector *cen)
{
   mxs_plane *plane = r3d_state.cur_con->clip.world_clip;
   int num_planes = 5;
   int code=0;
   float dot_val;

   plane = &(r3d_state.cur_con->clip.world_clip[num_planes]);

   // Iterate over the planes
   // Pity these aren't all together
   while(num_planes--) {

      --plane;
      code <<= 1;
 
      // find dot product of near point
      dot_val = plane->d + mx_dot_vec(cen,&plane->v);

      // Look, no branching for us!
      code  |= FSIGN(&dot_val); 
   }

   // user installed clip planes woo hoo
   num_planes = cclip.num_user_planes;
   if (num_planes) {
      r3s_clip_plane *uplane; //user installed
      uplane = ((r3s_clip_plane *)cclip.clip_data) + num_planes;

      // Iterate over the planes
      while(num_planes--) {
         uplane--;
         plane = &(uplane->world_plane);

         code <<= 1;
 
         // find dot product of near point
         dot_val = plane->d + mx_dot_vec(cen,&plane->v);

         // Look, no branching for us!
         code  |= FSIGN(&dot_val);
      }
   }
   return code;
}
