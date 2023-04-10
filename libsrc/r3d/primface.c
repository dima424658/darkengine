// $Header: x:/prj/tech/libsrc/r3d/RCS/primface.c 1.7 1996/06/30 03:10:42 buzzard Exp $

//  3D Rendering primitives and context

#include <lg.h>
#include <mprintf.h>

#include <r3ds.h>
#include <ctxts.h>
#include <matrix.h>

#include <primface.h>

  // check if a polygon is forward facing, takes array of 3 phandles
  // automatically deals with left/right-handed space
bool r3_check_facing_3(r3s_phandle *vp)
{
   mxs_vector a,b,c;

   mx_sub_vec(&a, &vp[1]->p, &vp[0]->p);
   mx_sub_vec(&b, &vp[2]->p, &vp[1]->p);

   // compute normal with cross product
   mx_cross_vec(&c, &b, &a);
   
   // compare dot product of one pt with normal

   return mx_dot_vec(&vp[0]->p, &c) < 0;
}

  // check the facing of a polygon that may have
  // colinear points
bool r3_check_facing_n(int n, r3s_phandle *vp)
{
   mxs_vector norm;
   int i, j;

   // Compute normal with Newell's method
   // See graphics gems III V.5

   norm.x = norm.y = norm.z = 0;

   j = n-1; 
   for (i=0; i < n; ++i) {
      norm.x += (vp[j]->p.y - vp[i]->p.y)*(vp[j]->p.z + vp[i]->p.z);
      norm.y += (vp[j]->p.z - vp[i]->p.z)*(vp[j]->p.x + vp[i]->p.x);
      norm.z += (vp[j]->p.x - vp[i]->p.x)*(vp[j]->p.y + vp[i]->p.y);
      j = i;
   }
     // compare dot product of one pt with normal
   return mx_dot_vec(&vp[0]->p, &norm) > 0;
}

  // check if a polygon is forward facing, takes a camera space normal
bool r3_check_v_normal(r3s_phandle vp, mxs_vector *n)
{
   // polygon is facing if    dot(point-camera, normal) < 0
   //   in this space, camera is at 0
   return mx_dot_vec(&vp->p, n) < 0;
}

   // check camera space pt & normal to see if forward facing
bool r3_check_v_normal_vec(mxs_vector *p, mxs_vector *n)
{
   return mx_dot_vec(p, n) < 0;
}

   // check worldspace pt & normal to see if forward facing
extern bool r3_check_w_normal_vec(mxs_vector *p, mxs_vector *n)
{
   mxs_vector temp;
   mx_sub_vec(&temp, p, &r3d_state.cur_con->trans.vino);   // HACK HACK HACK
         // this is wrong--should be  trans.vinw   but it don't exist
   return mx_dot_vec(&temp, n) < 0;
}

   // check object space pt & normal to see if forward facing
extern bool r3_check_o_normal_vec(mxs_vector *p, mxs_vector *n)
{
   mxs_vector temp;
   mx_sub_vec(&temp, p, &r3d_state.cur_con->trans.vino);
   return mx_dot_vec(&temp, n) < 0;
}
