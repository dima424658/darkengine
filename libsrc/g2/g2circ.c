/* 
 * $Source: s:/prj/tech/libsrc/g2/RCS/g2circ.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 17:28:37 $
 */

#include <dev2d.h>
#include <grcurv.h>

/* r=FIX_UNIT means full canvas width.       */
/* height is calculated from aspect ratio.   */

void g2_circle_to_elipse(fix r, fix *a, fix *b)
{
   *a = r * grd_bm.w;

   *b = fix_div (*a, grd_cap->aspect);
}

int g2_circle (fix x0, fix y0, fix r)
{
   fix a, b;
  
   g2_circle_to_elipse(r, &a, &b);
   return g2_elipse (x0, y0, a, b);
}

int g2_disk (fix x0, fix y0, fix r)
{
   fix a, b;
  
   g2_circle_to_elipse (r, &a, &b);
   return g2_oval (x0, y0, a, b);
}
