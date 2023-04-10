/*
 * $Source: x:/prj/tech/libsrc/g2/RCS/grcurv.h $
 * $Revision: 1.2 $
 * $Author: PATMAC $
 * $Date: 1998/07/04 13:14:32 $
 *
 * Dispatch macros for curve functions.
 *
 * This file is part of the g2 library.
 *
 */

#ifndef __GRCURV_H
#define __GRCURV_H

/* radii are scaled to FIX_UNIT = full canvas width */

EXTERN int g2_circle(fix x0, fix y0, fix r);
#define gr_circle g2_circle
#define gr_int_circle(x0, y0, r) \
   g2_circle(fix_make(x0,0), fix_make(y0,0), fix_div(r,320))
#define gr_fix_circle(x0, y0, r) \
   g2_circle(x0, y0, (r)/320)

EXTERN int g2_disk(fix x0, fix y0, fix r);
#define gr_disk g2_disk
#define gr_int_disk(x0, y0, r) \
   g2_disk(fix_make(x0,0), fix_make(y0,0), fix_div(r,320))
#define gr_fix_disk(x0, y0, r) \
   g2_disk(x0, y0, (r)/320)

EXTERN int g2_ring(fix x0, fix y0, fix ro, fix ri);
#define gr_ring g2_ring
#define gr_int_ring(x0, y0, ro, ri) \
   g2_ring(fix_make(x0,0), fix_make(y0,0), fix_div(ro,320), fix_div(ri,320))
#define gr_fix_ring(x0, y0, ro, ri) \
   g2_ring(x0, y0, (ro)/320, (ri)/320)

/* a and b are in pixels */
EXTERN int g2_elipse(fix x0, fix y0, fix a, fix b);
#define gr_elipse g2_elipse
#define gr_fix_elipse g2_elipse
#define gr_int_elipse(x0, y0, a, b) \
   g2_elipse(fix_make(x0,0), fix_make(y0,0), fix_make(a,0), fix_make(b,0))

EXTERN int g2_oval(fix x0, fix y0, fix a, fix b);
#define gr_oval g2_oval
#define gr_fix_oval g2_oval
#define gr_int_oval(x0, y0, a, b) \
   g2_oval(fix_make(x0,0), fix_make(y0,0), fix_make(a,0), fix_make(b,0))

#define gr_int_uring gr_int_ring
#define gr_fix_uring gr_fix_ring
#define gr_int_ucircle gr_int_circle
#define gr_fix_ucircle gr_fix_circle
#define gr_int_udisk gr_int_disk
#define gr_fix_udisk gr_fix_disk
#define gr_int_uelipse gr_int_elipse
#define gr_fix_uelipse gr_fix_elipse
#define gr_int_uoval gr_int_oval
#define gr_fix_uoval gr_fix_oval

#endif /* __GRCURV_H */
