// $Header: x:/prj/tech/libsrc/g2/RCS/grply.h 1.2 1996/11/07 17:29:30 KEVIN Exp $

#ifndef __GRPLY_H
#define __GRPLY_H
#include <g2d.h>
#include <icanvas2.h>
#include <pftype.h>
#include <dev2d.h>
#include <g2spoint.h>

extern int g2_poly_area(int n, g2s_point **ppl);

#define gr_upoly \
   ((g2up_func *)(g2d_canvas_table[G2C_UPOLY]))
#define gr_upoly_expose \
   ((g2up_expose_func *)(g2d_canvas_table[G2C_UPOLY_EXPOSE]))

#define gr_uspoly \
   ((g2up_func *)(g2d_canvas_table[G2C_USPOLY]))
#define gr_uspoly_expose \
   ((g2up_expose_func *)(g2d_canvas_table[G2C_USPOLY_EXPOSE]))

#define gr_ucpoly \
   ((g2up_func *)(g2d_canvas_table[G2C_UCPOLY]))
#define gr_ucpoly_expose \
   ((g2up_expose_func *)(g2d_canvas_table[G2C_UCPOLY_EXPOSE]))

extern int gr_poly(int c, int n, grs_vertex **vpl);
extern int gr_spoly(int c, int n, grs_vertex **vpl);
extern int gr_cpoly(int c, int n, grs_vertex **vpl);
#define gr_wire_poly gr_poly
#define gr_wire_upoly gr_upoly
#define gr_tluc8_upoly(c, n, vpl) \
do {                                      \
   if (gr_get_fill_type() == FILL_NORM) { \
      gr_set_fill_type(FILL_BLEND);       \
      gr_upoly(c, n, vpl);                \
      gr_set_fill_type(FILL_NORM);        \
   }                                      \
} while (0)
#define gr_tluc8_poly(c, n, vpl) \
do {                                      \
   if (gr_get_fill_type() == FILL_NORM) { \
      gr_set_fill_type(FILL_BLEND);       \
      gr_poly(c, n, vpl);                 \
      gr_set_fill_type(FILL_NORM);        \
   }                                      \
} while (0)
#endif /* !__GRPLY_H */
