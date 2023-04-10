// $Header: x:/prj/tech/libsrc/g2/RCS/point.c 1.2 1997/02/07 12:54:56 KEVIN Exp $
#include <point.h>
int gr_draw_point(int c, g2s_point *p)
{
   return gd_pix_opt(c, fix_int(p->sx), fix_int(p->sy), gd_upix_expose(0,0,0));
}


