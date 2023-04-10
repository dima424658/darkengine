/*
 * $Source: x:/prj/tech/libsrc/r3d/RCS/utils.c $
 * $Revision: 1.3 $
 * $Author: JAEMZ $
 * $Date: 1997/06/13 18:26:35 $
 *
 * View point related functions
 *
 */

#include <math.h>

#include <lg.h>
#include <r3ds.h>
#include <xform.h>
#include <matrix.h>
#include <prim.h>
#include <r3dutil.h>


// Render a wire frame of radius rad.
// The caller is responsible for setting
// object space and color
void r3_wire_cube(float rad)
{
   mxs_vector v[8];
   r3s_point p[8];

   mx_mk_vec(&v[0],-rad,-rad,-rad);
   mx_mk_vec(&v[1],rad,-rad,-rad);
   mx_mk_vec(&v[2],rad,rad,-rad);
   mx_mk_vec(&v[3],-rad,rad,-rad);

   mx_mk_vec(&v[4],-rad,-rad,rad);
   mx_mk_vec(&v[5],rad,-rad,rad);
   mx_mk_vec(&v[6],rad,rad,rad);
   mx_mk_vec(&v[7],-rad,rad,rad);

   r3_transform_block(8,p,v);

   r3_draw_line(&p[0],&p[1]);
   r3_draw_line(&p[1],&p[2]);
   r3_draw_line(&p[2],&p[3]);
   r3_draw_line(&p[3],&p[0]);

   r3_draw_line(&p[4],&p[5]);
   r3_draw_line(&p[5],&p[6]);
   r3_draw_line(&p[6],&p[7]);
   r3_draw_line(&p[7],&p[4]);

   r3_draw_line(&p[0],&p[4]);
   r3_draw_line(&p[1],&p[5]);
   r3_draw_line(&p[2],&p[6]);
   r3_draw_line(&p[3],&p[7]);
}

// Pass it a bounding box
void r3_wire_bbox(mxs_vector *bmin,mxs_vector *bmax)
{
   mxs_vector v[8];
   r3s_point p[8];

   mx_mk_vec(&v[0],bmin->x,bmin->y,bmin->z);
   mx_mk_vec(&v[1],bmax->x,bmin->y,bmin->z);
   mx_mk_vec(&v[2],bmax->x,bmax->y,bmin->z);
   mx_mk_vec(&v[3],bmin->x,bmax->y,bmin->z);

   mx_mk_vec(&v[4],bmin->x,bmin->y,bmax->z);
   mx_mk_vec(&v[5],bmax->x,bmin->y,bmax->z);
   mx_mk_vec(&v[6],bmax->x,bmax->y,bmax->z);
   mx_mk_vec(&v[7],bmin->x,bmax->y,bmax->z);

   r3_transform_block(8,p,v);

   r3_draw_line(&p[0],&p[1]);
   r3_draw_line(&p[1],&p[2]);
   r3_draw_line(&p[2],&p[3]);
   r3_draw_line(&p[3],&p[0]);

   r3_draw_line(&p[4],&p[5]);
   r3_draw_line(&p[5],&p[6]);
   r3_draw_line(&p[6],&p[7]);
   r3_draw_line(&p[7],&p[4]);

   r3_draw_line(&p[0],&p[4]);
   r3_draw_line(&p[1],&p[5]);
   r3_draw_line(&p[2],&p[6]);
   r3_draw_line(&p[3],&p[7]);

}

// The caller is responsible for all that jazz
// even setting object space and color and block
void r3_wire_sphere(float rad)
{
   mxs_vector v[32];
   r3s_point p[32];
   float z,r;
   float ang;
   int i,j;

   for (z=-rad;z<rad;z+=rad/16) {
      r = sqrt(rad*rad - z*z);

      for (i=0;i<32;++i) {
         ang = i*MX_REAL_2PI/32.0;
         v[i].x = cos(ang)*r;
         v[i].y = sin(ang)*r;
         v[i].z = z;
      }

      r3_transform_block(32,p,v);

      j = 31;
      for (i=0;i<32;++i) {
         r3_draw_line(&p[j],&p[i]);
         j = i;
      }
   }
}


// The caller is responsible for all that jazz
// even setting object space and color and block
void r3_wire_cylinder(float hrad,float h)
{
   mxs_vector v[32];
   r3s_point p[32];
   float ang;
   int i,j;

   for (i=0;i<16;++i) {
      ang = i*MX_REAL_2PI/16.0;
      v[i+16].x = v[i].x = cos(ang)*hrad;
      v[i+16].y = v[i].y = sin(ang)*hrad;
      v[i].z = 0;
      v[i+16].z = h;
   }

   r3_transform_block(32,p,v);

   j = 15;
   for (i=0;i<16;++i) {
      r3_draw_line(&p[j],&p[i]);
      r3_draw_line(&p[j+16],&p[i+16]);
      r3_draw_line(&p[i],&p[i+16]);
      j = i;
   }
}



// Draws a vector at position pos, and extending towards vec.
// You can scale it and set the maximum size of the vector
void r3_render_vector(mxs_vector pos,mxs_vector vec,float scale,float maxv)
{
   r3s_point p[2];
   float mag;

   mx_scaleeq_vec(&vec,scale);
   mag = mx_mag_vec(&vec);

   if (mag > maxv) {
      mx_normeq_vec(&vec);
      mx_scaleeq_vec(&vec,maxv);
   }

   mx_addeq_vec(&vec,&pos);

   r3_transform_point(&p[0],&pos);
   r3_transform_point(&p[1],&vec);

   r3_draw_line(&p[0],&p[1]);
}


