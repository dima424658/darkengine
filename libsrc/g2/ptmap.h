// $Header: x:/prj/tech/libsrc/g2/RCS/ptmap.h 1.2 1998/03/25 15:42:21 KEVIN Exp $

#ifndef __PTMAP_H
#define __PTMAP_H

#include <lg.h>
#include <fix.h>
#include <dev2d.h>

extern uchar *g2pt_tmap_ptr;
extern ulong g2pt_tmap_mask;
extern fix g2pt_light, g2pt_dlight, g2pt_toggle;
extern int dither;

//extern void g2ptmap_do_8x8_lit(uchar *dest, int row, fix *u_data, fix *v_data);
//extern void g2ptmap_do_8x8_flat(uchar *dest, int row, fix *u_data, fix *v_data, fix color);
//extern void g2ptmap_light_8x8(fix tl, fix tstep, fix bl, fix bstep);

extern void g2ptmap_run(uchar *dest, int n, fix u, fix v);
extern void g2ptmap_affine_duv(fix du, fix dv);
//extern void g2ptmap_flat_run(uchar *dest, int n, fix u, fix v, fix du, fix dv);
extern void g2ptmap_lit_run(uchar *dest, int n, fix u, fix v);

extern void g2ptmap_perspective_run(int n, double *, uchar *dest);

#define g2ptmap_do_run(dest,n,u,v,du,dv)  \
    do { g2ptmap_affine_duv(du,dv); g2ptmap_run(dest, n, u,v); } while (0)

#define g2ptmap_do_flat_run(dest,n,u,v,du,dv,lt)  \
    do { g2pt_light = (lt); g2ptmap_flat_run(dest, n, u,v,du,dv); } while (0)

#define g2ptmap_do_lit_run(dest,n,u,v,du,dv,lt,dlt)  \
    do {                                           \
       g2ptmap_set_lighting(lt, dlt, dest);          \
       g2ptmap_affine_duv(du,dv);                    \
       g2ptmap_lit_run(dest, n, u,v);                \
    } while (0)

#ifdef DITHERING

#define g2ptmap_set_lighting(lt,dlt,dest)    \
   do {                                     \
     if (!dither) {                          \
       g2pt_light = (lt);                       \
       g2pt_dlight = (dlt);                      \
       g2pt_toggle = 0;                           \
     } else {                                    \
       if ((((int) (dest)) ^ (scanline)) & 1) {   \
          g2pt_light = (lt) + 65536;                 \
          g2pt_dlight = (dlt) - 65536;                \
       } else {                                      \
          g2pt_light = (lt);                            \
          g2pt_dlight = (dlt) + 65536;                   \
       }                                                \
       g2pt_toggle = ((dlt) - 65536) ^ ((dlt)+65536);      \
     }                                                    \
   }                                                       \
   while (0)

#else

#define g2ptmap_set_lighting(lt,dlt,dest) \
      (g2pt_light = (lt), g2pt_dlight = (dlt))

#endif

#define g2ptmap_do_lit_run_implicit(dest,n,u,v,du,dv) \
   do { g2ptmap_lit_run(dest, n, u, v, du, dv); } while (0)

#define g2ptmap_do_perspective_lit_run_implicit(dest,n,a,b,c) \
   do { double z[3];z[0]=(a);z[1]=(b);z[2]=(c); g2ptmap_perspective_run(n, z, dest); } while (0)

extern int g2ptmap_setup(grs_bitmap *bm);

#define G2PTC_OK     0
#define G2PTC_FAIL   1

#endif
