/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

/*
 *  for a true planar mapping, we have
 *
 *    P + uS + vT = (x,y,z)
 *    i = x/z*xsc,  j = y/z*ysc
 *
 *  So we want to compute the mapping from (i,j)
 *    to (u,v), knowing the intermediate form is
 *
 *    u = a/c, v = b/c
 *
 *   So first we want to find the point
 *   intersected by i,j in the plane:
 *
 *     i = x/z*xsc
 *     j = y/z*ysc
 *
 *    z*i = x*xsc
 *    z*j = y*ysc
 *
 *    P + uS + vT = (x,y,z)
 *
 *      x = z*i/xsc
 *      y = z*j/ysc
 *
 *    P + uS + vT = (z*i/xsc, z*j/ysc, z)
 *
 *    P0 + uS0 + vT0 = z*i/xsc
 *    P1 + uS1 + vT1 = z*j/ysc
 *    P2 + uS2 + vT2 = z
 *
 *    P0 + uS0 + vT0 = (P2 + uS2 + vT2)/xsc * i
 *    P1 + uS1 + vT1 = (P2 + uS2 + vT2)/ysc * j
 *
 *    (S0 - S2/xsc*i)*u + (T0 - T2/xsc*i)*v = P2/xsc*i - P0
 *    (S1 - S2/ysc*j)*u + (T1 - T2/ysc*j)*v = P2/ysc*j - P1
 *
 *    Now we just solve the above, say, using Cramer's rule:
 *
 *    S20 = S2/xsc    T20 = T2/xsc    P20 = P2/xsc
 *    S21 = S2/ysc    T21 = T2/ysc    P21 = P2/ysc
 *
 *    denom = (S0-S20i)*(T1-T21j) - (t0-t20i)*(s1-s21j)
 *     utop = (P20i-P0)*(t1-t21j) - (t0-t20i)*(p21j-p1)
 *     vtop = (S0-S20i)*(p21j-p1) - (p20i-p0)*(s1-s21j)
 *
 *    u = utop/denom
 *    v = vtop/denom
 *
 *    denom = s0*t1 - s0*t21j - t1*s20i + s20*t21*i*j - (t0*s1 - t0*s21j - s1*t20i + t20*s21*i*j)
 *     utop = t1*p20i - p20*t21*i*j - p0*t1 + p0*t21j - (t0*p21j - t0*p1 - t20*p21*i*j + p1*t20i)
 *     vtop = s0*p21j - s0*p1 - s20*p21*i*j + p1*s20i - (s1*p20i - p20*s21*i*j - p0*s1 + p0*s21j)
 *    
 *    a = (t0*p1 - p0*t1) + (t1*p20 - p1*t20)*i + (p0*t21 - t0*p21)*j
 *    b = (p0*s1 - s0*p1) + (p1*s20 - s1*p20)*i + (s0*p21 - p0*s21)*j
 *    c = (s0*t1 - t0*s1) + (s1*t20 - t1*s20)*i + (t0*s21 - s0*t21)*j
 */

#include <matrix.h>
#include <profile.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

float xsci=320,ysci=240;
double xsc=1.0/320,ysc=1.0/240;
extern bool project_space;

void duv_set_size(int w, int h)
{
   xsci = w/2.0;
   ysci = h/2.0;

   xsc = 2.0 / w;
   ysc = 2.0 / h;
}

double tmap_data[9];

void calc_uvw_deltas(mxs_vector *pt, mxs_vector *u, mxs_vector *v)
{  PROF
   double p0,p1,p2;

#define s0  (u->x)
#define s1  (u->y)
#define s2  (u->z)

#define t0  (v->x)
#define t1  (v->y)
#define t2  (v->z)

   p0 = pt->x;
   p1 = pt->y;
   p2 = pt->z;

   // raw calculation

   tmap_data[0] = t0 * p1 - p0 * t1;
   tmap_data[1] = p0 * s1 - s0 * p1;
   tmap_data[2] = s0 * t1 - t0 * s1;

   tmap_data[3] = t1 * p2 - p1 * t2;
   tmap_data[4] = p1 * s2 - s1 * p2;
   tmap_data[5] = s1 * t2 - t1 * s2;

   tmap_data[6] = p0 * t2 - t0 * p2;
   tmap_data[7] = s0 * p2 - p0 * s2;
   tmap_data[8] = t0 * s2 - s0 * t2;

     // account for screen centering bias

   if (project_space) {
      // in project space, x is prescaled by xsci*65536
      //                   y is prescaled by ysci*65536

      // so tmap_data[0..2] is scaled by xsci*ysci*65536*65536
      //             [3..5] is scaled by ysci*65536
      //             [6..8] is scaled by xsci*65536

      // so we need to pull out the extra 65536 from 0..2

      int i;
      for (i=0; i < 3; ++i) tmap_data[i] /= 65536.0;

      tmap_data[0] = tmap_data[0] - xsci*tmap_data[3] - ysci*tmap_data[6];
      tmap_data[1] = tmap_data[1] - xsci*tmap_data[4] - ysci*tmap_data[7];
      tmap_data[2] = tmap_data[2] - xsci*tmap_data[5] - ysci*tmap_data[8];

        // adjust for half-pixel bias that compensates for fix_cint

      tmap_data[0] += (tmap_data[3] + tmap_data[6])/2;
      tmap_data[1] += (tmap_data[4] + tmap_data[7])/2;
      tmap_data[2] += (tmap_data[5] + tmap_data[8])/2;

        // scale numerators by 2^16 so we get free fixed-point conversion

      tmap_data[0] *= 65536;
      tmap_data[1] *= 65536;

      tmap_data[3] *= 65536;
      tmap_data[4] *= 65536;

      tmap_data[6] *= 65536;
      tmap_data[7] *= 65536;

   } else {
      tmap_data[0] = tmap_data[0] - tmap_data[3] - tmap_data[6];
      tmap_data[1] = tmap_data[1] - tmap_data[4] - tmap_data[7];
      tmap_data[2] = tmap_data[2] - tmap_data[5] - tmap_data[8];

        // scale by screen size because the viewspace vectors aren't
        // actually quite yet what we want

        // this is only needed if we're not in project space?

      tmap_data[3] *= xsc;
      tmap_data[4] *= xsc;
      tmap_data[5] *= xsc;

      tmap_data[6] *= ysc;
      tmap_data[7] *= ysc;
      tmap_data[8] *= ysc;

        // adjust for half-pixel bias that compensates for fix_cint

      tmap_data[0] += (tmap_data[3] + tmap_data[6])/2;
      tmap_data[1] += (tmap_data[4] + tmap_data[7])/2;
      tmap_data[2] += (tmap_data[5] + tmap_data[8])/2;

        // scale numerators by 2^16 so we get free fixed-point conversion

      tmap_data[0] *= 65536;
      tmap_data[1] *= 65536;

      tmap_data[3] *= 65536;
      tmap_data[4] *= 65536;

      tmap_data[6] *= 65536;
      tmap_data[7] *= 65536;
   }

   END_PROF;
}
