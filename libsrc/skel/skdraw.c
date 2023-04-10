/*
 * $Source: x:/prj/tech/libsrc/skel/RCS/skdraw.c $
 * $Revision: 1.5 $
 * $Author: kate $
 * $Date: 1996/08/23 18:16:20 $
 *
 * Skeleton drawing routines
 *
 * $Log: skdraw.c $
 * Revision 1.5  1996/08/23  18:16:20  kate
 * ported to new 3d.  also supports decals.
 * 
 * Revision 1.1  1996/06/10  17:15:48  kate
 * Initial revision
 * 
 * Revision 1.4  1995/09/12  18:30:44  JAEMZ
 * Fixed bug
 * 
 * Revision 1.3  1994/04/21  23:46:50  jaemz
 * Made cylinder drawer more robust
 * 
 * Revision 1.2  1994/04/21  15:22:29  jaemz
 * Fixed cylinder drawer
 * 
 * Revision 1.1  1994/04/21  13:28:11  jaemz
 * Initial revision
 * 
 * 
 */

#include <2d.h>
#include <r3d.h>
#include <mprintf.h>
#include <math.h>

void ff_cyl(float x0,float y0,float z0,float x1,float y1,float z1,float rad,long c)
{
   // approximates with a capped octagonal prism
   mxs_vector a;  // cylinder vector;
   float ma;
   float sqrt2;
   int i;
   r3s_point v[16];
   r3s_phandle tmp[4];
   float ax,ay,az;

   // this is moronic, forgive me, must be a better way to generate an octagon
   a.x = x1-x0;
   a.y = y1-y0;
   a.z = z1-z0;

   ax = fabs(a.x);
   ay = fabs(a.y);
   az = fabs(a.z);

   // inverse magnitudes 
   ma = 1.0/sqrt((a.x*a.x) + (a.y*a.y) + (a.z*a.z));
   sqrt2 = 1.0/sqrt(2);

   // One has to work okay without exploding
   if ( (ay > az) && (ay > ax) ) {
      v[0].p.z = a.y*rad/sqrt(2*(a.y*a.y + (a.z+a.x)*(a.z+a.x)));
      v[0].p.x = v[0].p.z;
      v[0].p.y = -v[0].p.z*(a.z+a.x)/a.y;
   } else if ( az > ax) {
      v[0].p.x = a.z*rad/sqrt(2*(a.z*a.z + (a.x+a.y)*(a.x+a.y)));
      v[0].p.y = v[0].p.x;
      v[0].p.z = -(v[0].p.x*(a.x+a.y)/a.z);
   } else {
      v[0].p.y = a.x*rad/sqrt(2*(a.x*a.x + (a.y+a.z)*(a.y+a.z)));
      v[0].p.z = v[0].p.y;
      v[0].p.x = -v[0].p.y*(a.y+a.z)/a.x;
   }

   v[2].p.x =  ((a.y*v[0].p.z) - (a.z*v[0].p.y))*ma;
   v[2].p.y =  ((a.z*v[0].p.x) - (a.x*v[0].p.z))*ma;
   v[2].p.z =  ((a.x*v[0].p.y) - (a.y*v[0].p.x))*ma;

   v[1].p.x =  (v[0].p.x + v[2].p.x)*sqrt2;
   v[1].p.y =  (v[0].p.y + v[2].p.y)*sqrt2;
   v[1].p.z =  (v[0].p.z + v[2].p.z)*sqrt2;

   v[3].p.x =  (v[2].p.x - v[0].p.x)*sqrt2;
   v[3].p.y =  (v[2].p.y - v[0].p.y)*sqrt2;
   v[3].p.z =  (v[2].p.z - v[0].p.z)*sqrt2;

   for (i=4;i<8;++i) {
      v[i].p.x = -v[i-4].p.x;
      v[i].p.y = -v[i-4].p.y;
      v[i].p.z = -v[i-4].p.z;
   }

   // create vectors for real
   for (i=0;i<8;++i) {
      v[i+8].ccodes = 0;
      v[i+8].p.x = v[i].p.x + x1;
      v[i+8].p.y = v[i].p.y + y1;
      v[i+8].p.z = v[i].p.z + z1;
      v[i].ccodes = 0;
      v[i].p.x = v[i].p.x + x0;
      v[i].p.y = v[i].p.y + y0;
      v[i].p.z = v[i].p.z + z0;
   }

   // these have been transformed, have merely to project them
   r3_start_block();
   r3_project_block(16,v);

   r3_set_color(c);

   for (i=0;i<8;++i) {
      tmp[0] = &v[i];
      tmp[1] = &v[(i+1)%8];
      tmp[2] = &v[((i+1)%8)+8];
      tmp[3] = &v[i+8];

      r3_draw_poly(4,tmp);
   }
   r3_end_block();
}


