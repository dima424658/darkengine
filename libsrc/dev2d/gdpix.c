// $Header: x:/prj/tech/libsrc/dev2d/RCS/gdpix.c 1.2 1997/02/17 14:49:13 KEVIN Exp $
// Generic pixel drawing primitives

#include <clip.h>
#include <gdpix.h>

void gd_upoint_opt(int x, int y, gdupix_func *upix_func)
{
   upix_func(grd_gc.fcolor, x, y);
}

void gd_upoint(int x, int y)
{
   gd_upix_expose(0,0,0)(grd_gc.fcolor, x, y);
}

int gd_pix_opt(int c, int x, int y, gdupix_func *upix_func)
{
   if ((x<grd_clip.left)||(x>=grd_clip.right)||
       (y<grd_clip.top)||(y>=grd_clip.bot))
      return CLIP_ALL;
   upix_func(c, x, y);
   return CLIP_NONE;
}

int gd_pix8(int c, int x, int y)
{
   return gd_pix_opt(c, x, y, gd_upix8_expose(c, x, y));
}

int gd_pix16(int c, int x, int y)
{
   return gd_pix_opt(c, x, y, gd_upix16_expose(c, x, y));
}

int gd_pix24(int c, int x, int y)
{
   return gd_pix_opt(c, x, y, gd_upix24_expose(c, x, y));
}

int gd_point(int x, int y)
{
   return gd_pix_opt(grd_gc.fcolor, x, y, gd_upix_expose(0, x, y));
}

int gd_gpix_opt(int x, int y, gdgpix_func *ugpix_func)
{
   if ((x<grd_clip.left)||(x>=grd_clip.right)||
       (y<grd_clip.top)||(y>=grd_clip.bot))
      return 0;
   return ugpix_func(x, y);
}

int gd_gpix8(int x, int y)
{
   return gd_gpix_opt(x, y, gd_ugpix8_expose(x, y));
}

int gd_gpix16(int x, int y)
{
   return gd_gpix_opt(x, y, gd_ugpix16_expose(x, y));
}

int gd_gpix24(int x, int y)
{
   return gd_gpix_opt(x, y, gd_ugpix24_expose(x, y));
}
