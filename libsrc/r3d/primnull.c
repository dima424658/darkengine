// $Header: x:/prj/tech/libsrc/r3d/RCS/primnull.c 1.2 1997/10/03 13:03:25 KEVIN Exp $

#include <r3ds.h>
#include <dev2d.h>

#include <primfunc.h>

static void setup_null(r3s_texture tm)
{
}

static poly_setup_func poly_null_setup_funcs[] =
{
   // 0
   setup_null,    //      solid unlit
   setup_null,    //      solid unlit clipped
   setup_null,    //      solid gouraud
   setup_null,    //      solid gouraud clipped
   setup_null,    //      solid rgb gouraud
   setup_null,    //      solid rgb gouraud clipped
   setup_null,    //      solid pal gouraud
   setup_null,    //      solid pal gouraud clipped

   // 4
   setup_null,    //      transluc unlit
   setup_null,    //      transluc unlit clipped
   setup_null,    //      transluc gouraud
   setup_null,    //      transluc gouraud clipped
   setup_null,    //      transluc rgb gouraud
   setup_null,    //      transluc rgb gouraud clipped
   setup_null,    //      transluc pal gouraud
   setup_null,    //      transluc pal gouraud clipped

   // 8
   setup_null,    //      texture unlit
   setup_null,    //      texture unlit clipped
   setup_null,    //      texture gouraud
   setup_null,    //      texture gouraud clipped
   setup_null,    //      texture rgb gouraud
   setup_null,    //      texture rgb gouraud clipped
   setup_null,    //      texture pal gouraud
   setup_null,    //      texture pal gouraud clipped

   // 16
   setup_null,    //      texture unlit
   setup_null,    //      texture unlit clipped
   setup_null,    //      texture gouraud
   setup_null,    //      texture gouraud clipped
   setup_null,    //      linear rgb gouraud
   setup_null,    //      linear rgb gouraud clipped
   setup_null,    //      linear pal gouraud
   setup_null     //      linear pal gouraud clipped
};

static int null_convert(int n, r3s_phandle *vp)
{
   return CLIP_ALL;
}

static int null_line(r3s_phandle p0, r3s_phandle p1)
{
   return CLIP_ALL;
}

static int null_point(r3s_phandle p)
{
   return CLIP_ALL;
}


// don't rasterize at all
void r3_use_null(void)
{
   r3_setup_prim_funcs(
      poly_null_setup_funcs,
      null_convert,
      null_line,
      null_point);
}


