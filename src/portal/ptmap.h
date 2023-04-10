/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once

#include <lg.h>
#include <fix.h>
#include <dev2d.h>

extern uchar *pt_tmap_ptr;
extern ulong pt_tmap_mask;
extern fix pt_light, pt_dlight, pt_toggle;
extern int dither;

//extern void ptmap_do_8x8_lit(uchar *dest, int row, fix *u_data, fix *v_data);
//extern void ptmap_do_8x8_flat(uchar *dest, int row, fix *u_data, fix *v_data, fix color);
//extern void ptmap_light_8x8(fix tl, fix tstep, fix bl, fix bstep);

extern void ptmap_run(uchar *dest, int n, fix u, fix v);
extern void ptmap_affine_duv(fix du, fix dv);
//extern void ptmap_flat_run(uchar *dest, int n, fix u, fix v, fix du, fix dv);
extern void ptmap_lit_run(uchar *dest, int n, fix u, fix v);

extern void ptmap_perspective_run(int n, double *, uchar *dest);

#define ptmap_do_run(dest,n,u,v,du,dv)  \
    do { ptmap_affine_duv(du,dv); ptmap_run(dest, n, u,v); } while (0)

#define ptmap_do_flat_run(dest,n,u,v,du,dv,lt)  \
    do { pt_light = (lt); ptmap_flat_run(dest, n, u,v,du,dv); } while (0)

#define ptmap_do_lit_run(dest,n,u,v,du,dv,lt,dlt)  \
    do {                                           \
       ptmap_set_lighting(lt, dlt, dest);          \
       ptmap_affine_duv(du,dv);                    \
       ptmap_lit_run(dest, n, u,v);                \
    } while (0)

#ifdef DITHERING

#define ptmap_set_lighting(lt,dlt,dest)    \
   do {                                     \
     if (!dither) {                          \
       pt_light = (lt);                       \
       pt_dlight = (dlt);                      \
       pt_toggle = 0;                           \
     } else {                                    \
       if ((((int) (dest)) ^ (scanline)) & 1) {   \
          pt_light = (lt) + 65536;                 \
          pt_dlight = (dlt) - 65536;                \
       } else {                                      \
          pt_light = (lt);                            \
          pt_dlight = (dlt) + 65536;                   \
       }                                                \
       pt_toggle = ((dlt) - 65536) ^ ((dlt)+65536);      \
     }                                                    \
   }                                                       \
   while (0)

#else

#define ptmap_set_lighting(lt,dlt,dest) \
      (pt_light = (lt), pt_dlight = (dlt))

#endif

#define ptmap_do_lit_run_implicit(dest,n,u,v,du,dv) \
   do { ptmap_lit_run(dest, n, u, v, du, dv); } while (0)

#define ptmap_do_perspective_lit_run_implicit(dest,n,a,b,c) \
   do { double z[3];z[0]=(a);z[1]=(b);z[2]=(c); ptmap_perspective_run(n, z, dest); } while (0)

extern void ptmap_setup(grs_bitmap *bm);
