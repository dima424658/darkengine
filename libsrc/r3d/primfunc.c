// $Header: x:/prj/tech/libsrc/r3d/RCS/primfunc.c 1.22 1997/10/03 13:03:23 KEVIN Exp $

#include <dbg.h>

#include <ctxts.h>
#include <clipcon_.h>
#include <prim.h>
#include <primcon_.h>
#include <primfunc.h>
#include <primpoly.h>
#include <primtab_.h>

// remove debugging macro
#undef r3_compute_prim_funcs

static int (*tmap_3d_funcs[])(int, r3s_phandle *, r3s_texture) =
{
   r3_any,
   r3_clip_any,
   r3_check_any,
   r3_check_clip_any,
};

static int (*poly_3d_funcs[])(int, r3s_phandle *) =
{
   r3_implicit_any,
   r3_clip_implicit_any,
   r3_check_implicit_any,
   r3_check_clip_implicit_any,
};

static int (*prim_line_func)(r3s_phandle p0, r3s_phandle p1);
static int (*prim_point_func)(r3s_phandle p);
static poly_setup_func *poly_setup_funcs;
static int (*convert_func)(int n, r3s_phandle *vl);

// use this to setup alternative drivers (like d3d, for example)
void r3_setup_prim_funcs(
   poly_setup_func *setup_func_list,
   int (*convert)(int n, r3s_phandle *vl),
   int (*line_func)(r3s_phandle p0, r3s_phandle p1),
   int (*point_func)(r3s_phandle p))
{
   poly_setup_funcs = setup_func_list;
   convert_func = convert;
   prim_line_func = line_func;
   prim_point_func = point_func;
}

#if 0 // i think these are pretty obsolete
typedef int (*tmap_func)(grs_bitmap *, int n, grs_vertex **);

int r3_tluc8_poly(int c, int n, grs_vertex **v)
{
   gr_tluc8_poly(c, n, v);
   return 0;
}

void r3_tluc8_upoly(int c, int n, grs_vertex **v)
{
   gr_tluc8_upoly(c, n, v);
}
#endif

// determine which parameters to clip
static int clip_flags[] =
{
   0,
   R3_CLIP_I,
   R3_CLIP_RGB,
   R3_CLIP_I,
   0,
   R3_CLIP_I,
   R3_CLIP_RGB,
   R3_CLIP_I,
   R3_CLIP_UV,
   R3_CLIP_ALL,
   R3_CLIP_RGB | R3_CLIP_UV,
   R3_CLIP_ALL,
   R3_CLIP_UV,
   R3_CLIP_ALL,
   R3_CLIP_RGB | R3_CLIP_UV,
   R3_CLIP_ALL
};

  // internal function to set up function pointers
#ifdef DBG_ON
 void r3_compute_prim_funcs(int funcs, char *name)
#else
 void r3_compute_prim_funcs(int funcs)
#endif
{
   int poly_mode;

   r3_convert_func = convert_func;

     // compute poly index if we do any 3d poly functions
   if (funcs & (R3FUNC_3D_POLY | R3FUNC_3D_TMAP)) {
      if (r3d_poly_flags & R3_PL_CHECK_FACING)
         poly_mode = r3d_clip_mode != R3_NO_CLIP ? 3 : 2;
      else
         poly_mode = r3d_clip_mode != R3_NO_CLIP ? 1 : 0;
   }

   if (funcs & R3FUNC_3D_POLY) {
      switch (r3d_poly_flags & R3_PL_MODE_MASK) {
         case R3_PL_POLYGON:
            r3_3d_poly_func = poly_3d_funcs[poly_mode];
            break;

#ifdef DBG_ON
         case R3_PL_TRISTRIP:
            Warning(("r3d: r3_compute_prim_funcs:\n  TRISTRIP mode not supported in %s\n", name));
            break;

         case R3_PL_TRIFAN:
            Warning(("r3d: r3_compute_prim_funcs:\n  TRIFAN mode not supported in %s\n", name));
            break;

         case R3_PL_QUADSTRIP:
            Warning(("r3d: r3_compute_prim_funcs:\n  QUADSTRIP mode not supported in %s\n", name));
            break;

         default:
            Warning(("r3d: r3_compute_prim_funcs:\n  Unhandled polygon drawing mode in %s\n", name));
#endif
      }
   }
   if (funcs & R3FUNC_3D_TMAP) {
      switch (r3d_poly_flags & R3_PL_MODE_MASK) {
         case R3_PL_POLYGON:
            r3_3d_tmap_func = tmap_3d_funcs[poly_mode];
            break;

#ifdef DBG_ON
         case R3_PL_TRISTRIP:
            Warning(("r3d: r3_compute_prim_funcs:\n  TRISTRIP mode not supported in %s\n", name));
            break;

         case R3_PL_TRIFAN:
            Warning(("r3d: r3_compute_prim_funcs:\n  TRIFAN mode not supported in %s\n", name));
            break;

         case R3_PL_QUADSTRIP:
            Warning(("r3d: r3_compute_prim_funcs:\n  QUADSTRIP mode not supported in %s\n", name));
            break;
         default:
            Warning(("r3d: r3_compute_prim_funcs:\n  Unhandled tmap drawing mode in %s\n", name));
#endif
      }
   }

   if (funcs & R3FUNC_2D_POLY) {
      int index = r3d_poly_flags & (R3_PL_TEXTURE_MASK + R3_PL_LIGHT_MASK);

      if (r3d_clip_2d)
         index+=2;

      r3_2d_poly_setup_func = poly_setup_funcs[index>>1];
      r3d_clip_flags = clip_flags[index>>2];
      r3d_do_setup = TRUE;
   }
   if (funcs & R3FUNC_2D_LINE) {
      r3_2d_line_func  = prim_line_func;
   }
   if (funcs & R3FUNC_2D_POINT) {
      r3_2d_point_func = prim_point_func;
   }
}
