/*
 *  $Source: x:/prj/tech/libsrc/g2/RCS/grlin.h $
 *  $Revision: 1.4 $
 *  $Author: PATMAC $
 *  $Date: 1998/07/04 13:14:37 $
 */

#ifndef __GRLIN_H
#define __GRLIN_H
#include <g2d.h>
#include <icanvas2.h>
#include <lftype.h>

#define gr_uline \
   ((g2ul_func *)(g2d_canvas_table[G2C_ULINE]))
#define gr_uline_expose \
   ((g2ul_expose_func *)(g2d_canvas_table[G2C_ULINE_EXPOSE]))
#define gr_usline \
   ((g2ul_func *)(g2d_canvas_table[G2C_USLINE]))
#define gr_usline_expose \
   ((g2ul_expose_func *)(g2d_canvas_table[G2C_USLINE_EXPOSE]))
#define gr_ucline \
   ((g2ul_func *)(g2d_canvas_table[G2C_UCLINE]))
#define gr_ucline_expose \
   ((g2ul_expose_func *)(g2d_canvas_table[G2C_UCLINE_EXPOSE]))

EXTERN int gr_line_opt_type(grs_vertex *v0, grs_vertex *v1, g2ul_func *lfunc, int type);
#define gr_line(_v0, _v1) \
   gr_line_opt_type(_v0, _v1, gr_uline_expose(_v0, _v1), G2C_CLIP_LINE)
#define gr_sline(_v0, _v1) \
   gr_line_opt_type(_v0, _v1, gr_usline_expose(_v0, _v1), G2C_CLIP_SLINE)
#define gr_cline(_v0, _v1) \
   gr_line_opt_type(_v0, _v1, gr_ucline_expose(_v0, _v1), G2C_CLIP_CLINE)

#define gr_fix_line(_x0,_y0,_x1,_y1) \
do {                                \
   grs_vertex _v0, _v1;             \
   _v0.x = (_x0); _v0.y = (_y0);    \
   _v1.x = (_x1); _v1.y = (_y1);    \
   gr_line(&_v0, &_v1);             \
} while (0)

#define gr_fix_uline(_x0,_y0,_x1,_y1) \
do {                                \
   grs_vertex _v0, _v1;             \
   _v0.x = (_x0); _v0.y = (_y0);    \
   _v1.x = (_x1); _v1.y = (_y1);    \
   gr_uline(&_v0, &_v1);            \
} while (0)

#define gr_fix_sline(_x0,_y0,_x1,_y1) \
do {                                \
   grs_vertex _v0, _v1;             \
   _v0.x = (_x0); _v0.y = (_y0);    \
   _v1.x = (_x1); _v1.y = (_y1);    \
   gr_sline(&_v0, &_v1);            \
} while (0)

#define gr_fix_usline(_x0,_y0,_x1,_y1) \
do {                                \
   grs_vertex _v0, _v1;             \
   _v0.x = (_x0); _v0.y = (_y0);    \
   _v1.x = (_x1); _v1.y = (_y1);    \
   gr_usline(&_v0, &_v1);           \
} while (0)

#define gr_fix_cline(_x0,_y0,_x1,_y1) \
do {                                \
   grs_vertex _v0, _v1;             \
   _v0.x = (_x0); _v0.y = (_y0);    \
   _v1.x = (_x1); _v1.y = (_y1);    \
   gr_cline(&_v0, &_v1);            \
} while (0)

#define gr_fix_ucline(_x0,_y0,_x1,_y1) \
do {                                \
   grs_vertex _v0, _v1;             \
   _v0.x = (_x0); _v0.y = (_y0);    \
   _v1.x = (_x1); _v1.y = (_y1);    \
   gr_ucline(&_v0, &_v1);           \
} while (0)

#define gr_int_line(x0, y0, x1, y1) \
   gr_fix_line(fix_make(x0,0), fix_make(y0,0),  \
               fix_make(x1,0), fix_make(y1,0))

#define gr_int_uline(x0, y0, x1, y1) \
   gr_fix_uline(fix_make(x0,0), fix_make(y0,0), \
               fix_make(x1,0), fix_make(y1,0))

#define gr_int_sline(x0, y0, x1, y1) \
   gr_fix_sline(fix_make(x0,0), fix_make(y0,0), \
               fix_make(x1,0), fix_make(y1,0))

#define gr_int_usline(x0, y0, x1, y1) \
   gr_fix_usline(fix_make(x0,0), fix_make(y0,0), \
               fix_make(x1,0), fix_make(y1,0))

#define gr_int_cline(x0, y0, x1, y1) \
   gr_fix_cline(fix_make(x0,0), fix_make(y0,0), \
               fix_make(x1,0), fix_make(y1,0))

#define gr_int_ucline(x0, y0, x1, y1) \
   gr_fix_ucline(fix_make(x0,0), fix_make(y0,0), \
               fix_make(x1,0), fix_make(y1,0))

#endif
