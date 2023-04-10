// $Header: x:/prj/tech/libsrc/g2/RCS/genrast.c 1.3 1997/11/21 14:00:29 KEVIN Exp $
//
// Generic edge and rasterization functions.

#include <g2spoint.h>
#include <plyparam.h>
#include <trirast.h>
#include <mathmac.h>

#include <lgassert.h>

#ifdef FLOAT_RAST

static bool use_inverse_table = FALSE;
static float inverse_table[512];

bool edge_inverse_table_inuse()
{
   return use_inverse_table;
}

void edge_enable_inverse_table()
{
   use_inverse_table = TRUE;
}

void edge_disable_inverse_table()
{
   use_inverse_table = FALSE;
}

void edge_init_inverse_table()
{
   int i;
   for (i=0; i<512; i++)
      inverse_table[i] = 16.0 / i;
}

// call once for each left edge.
void gen_left_edge(g2s_point *prev, g2s_point *next, g2s_poly_params *tp)
{
   fix x, dy, dx_fix;
   float dx, delta_x, delta_y;
   int i;
   ulong mask, flags;

   dy = next->sy - prev->sy;
   dx = next->sx - prev->sx;

   AssertMsg(dy > 0, "gen_left_edge(): Hey... edge not valid!\n");
   if ((dy < fix_make(32,0)) && use_inverse_table)
      dx_fix = dx = dx * inverse_table[dy>>12];
   else
      dx_fix = dx = (dx / dy) * 65536.0;

   delta_y = fix_float(fix_ceil(prev->sy) - prev->sy);
   // add 0xffff because we're actually doing ceil's...
   x = (prev->sx + 0xffff) + dx * delta_y;
   delta_x = fix_float(fix_floor(x) - prev->sx);
   tp->left.dx = fix_int(dx_fix);
   tp->left.dx_frac = dx_fix<<16;
   tp->left.x = fix_int(x);
   tp->left.x_frac = x<<16;

   flags = tp->flags&PPF_MASK;
   if (flags == 0) return;

   for (i=0, mask=1; ; mask+=mask, i++) {
      if (!(mask&flags))
         continue;
      tp->left.coord_val[i] = tp->scale[i]*prev->coord[i]*65536.0 + delta_y*tp->dcy[i] + delta_x*tp->dcx[i];
      tp->left.coord_delta[i] = tp->dcy[i] + tp->left.dx * tp->dcx[i];
      if ((flags -= mask)==0)
         break;
   }
}

// call once for each right edge.
void gen_right_edge(g2s_point *prev, g2s_point *next, g2s_poly_params *tp)
{
   float dx, delta_y;
   fix dy;

   dx = next->sx - prev->sx;
   dy = next->sy - prev->sy;
   AssertMsg(dy > 0, "gen_right_edge(): Hey... edge not valid!\n");
   if ((dy < fix_make(32,0)) && use_inverse_table) {
      tp->right.dx = dx = dx * inverse_table[dy>>12];
   } else
      tp->right.dx = dx = (dx / dy) * 65536.0;

   delta_y = fix_float(fix_ceil(prev->sy) - prev->sy);
   // add 0xffff because we're actually doing ceil's...
   tp->right.x = prev->sx + 0xffff + dx * delta_y;
}
#else
// call once for each left edge.
void gen_left_edge(g2s_point *prev, g2s_point *next, g2s_poly_params *tp)
{
   fix x, dy, dx;
   fix delta_y;
   float fdx, fdy;
   int i;
   ulong mask, flags;

   dy = next->sy - prev->sy;
   dx = next->sx - prev->sx;

   AssertMsg(dy > 0, "gen_left_edge(): Hey... edge not valid!\n");
   dx = fix_div(dx, dy);

   delta_y = fix_ceil(prev->sy) - prev->sy;
   // add 0xffff because we're actually doing ceil's...
   x = (prev->sx + 0xffff) + fix_mul(dx, delta_y);

   tp->left.dx = fix_int(dx);
   tp->left.dx_frac = dx<<16;
   tp->left.x = fix_int(x);
   tp->left.x_frac = x<<16;

   flags = tp->flags&PPF_MASK;
   if (flags == 0) return;

   fdx = fix_float(fix_floor(x) - prev->sx);
   fdy = fix_float(delta_y);
   for (i=0, mask=1; ; mask+=mask, i++) {
      if (!(mask&flags))
         continue;
      tp->left.coord_val[i] = tp->scale[i]*prev->coord[i]*65536.0 + fdy*tp->dcy[i] + fdx*tp->dcx[i];
      tp->left.coord_delta[i] = tp->dcy[i] + tp->left.dx * tp->dcx[i];
      if ((flags -= mask)==0)
         break;
   }
}

// call once for each right edge.
void gen_right_edge(g2s_point *prev, g2s_point *next, g2s_poly_params *tp)
{
   fix dx, dy, delta_y;

   dx = next->sx - prev->sx;
   dy = next->sy - prev->sy;
   AssertMsg(dy > 0, "gen_right_edge(): Hey... edge not valid!\n");
   tp->right.dx = dx = fix_div(dx, dy);

   delta_y = fix_ceil(prev->sy) - prev->sy;
   // add 0xffff because we're actually doing ceil's...
   tp->right.x = prev->sx + 0xffff + fix_mul(dx, delta_y);
}
#endif


// n is the number of scanlines for the shorter edge.
void gen_raster_loop(g2s_poly_params *tp, int n)
{
   g2s_raster r;
   do {
      ulong mask, flags = tp->flags&PPF_MASK;
      int i;
      long carry;

      r.p = tp->p;
      tp->p += tp->canvas_row;
      r.x = tp->left.x;
      r.n = fix_int(tp->right.x) - tp->left.x;
      tp->right.x += tp->right.dx;
      carry = ulong_add_get_carry(&tp->left.x_frac, tp->left.dx_frac); // (0 or -1)
      tp->left.x += tp->left.dx - carry;
      if (flags!=0)
         for (i=0, mask=1; ; mask += mask, i++) {
            if (!(mask&flags))
               continue;
            r.coord[i] = tp->left.coord_val[i];
            tp->left.coord_val[i] += tp->left.coord_delta[i] + (tp->dcx[i]&carry);
            if ((flags -= mask)==0)
               break;
         }
      if (r.n > 0)
         tp->inner_loop(&r, tp);
   } while (--n > 0);
}

