/*
 * $Source: s:/prj/tech/libsrc/g2/RCS/clplin.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 17:34:13 $
 * 
 * Routines for clipping fixed-point lines to a rectangle.
 *
 * This file is part of the g2 library.
 *
 */ 

#include <mathmac.h>
#include <lftype.h>

static fix left, right, top, bot;

static void sline_clip_func(grs_vertex *v0, grs_vertex *v1, fix delta, fix dx)
{
   v0->i += fix_mul_div(v1->i - v0->i, delta, dx);
}

static void cline_clip_func(grs_vertex *v0, grs_vertex *v1, fix delta, fix dx)
{
   v0->u += fix_mul_div(v1->u - v0->u, delta, dx);
   v0->v += fix_mul_div(v1->v - v0->v, delta, dx);
   v0->w += fix_mul_div(v1->w - v0->w, delta, dx);
}

typedef void (*line_clip_func)(grs_vertex *v0, grs_vertex *v1, fix delta, fix d);

static line_clip_func clip_func;

line_clip_func clip_table[G2C_CLIP_LINE_TYPES] = {
   (line_clip_func )gr_null,
   sline_clip_func,
   cline_clip_func
};

static int code_point_lr (fix x) {
   if (x < left)
      return CLIP_LEFT;
   if (x > right)
      return CLIP_RIGHT;
   return 0;
}

static int code_point_tb (fix y) {
   if (y < top)
      return CLIP_TOP;
   if (y > bot)
      return CLIP_BOT;
   return 0;
}

static int code_point (fix x, fix y)
{
   int code = 0;
   code |= code_point_lr(x);
   code |= code_point_tb(y);
   return code;
}

static int clip_x(grs_vertex *v0, grs_vertex *v1, int x)
{
   fix dx = v1->x - v0->x;
   fix delta = x - v0->x;

   v0->x = x;
   v0->y += fix_mul_div(v1->y - v0->y, delta, dx);
   clip_func (v0, v1, delta, dx);
   return code_point_tb(v0->y);
}

static int clip_y(grs_vertex *v0, grs_vertex *v1, int y)
{
   fix dy = v1->y - v0->y;
   fix delta = y - v0->y;

   v0->y = y;
   v0->x += fix_mul_div(v1->x - v0->x, delta, dy);
   clip_func (v0, v1, delta, dy);
   return code_point_lr(v0->x);
}

int gr_line_opt_type(grs_vertex *v0, grs_vertex *v1, g2ul_func *ul_func, int type)
{
   int code0, code1;

   left = grd_fix_clip.left - fix_make(0,0x8000);
   right = grd_fix_clip.right - fix_make(0,0x8001);
   top = grd_fix_clip.top - fix_make(0,0x8000);
   bot = grd_fix_clip.bot - fix_make(0,0x8001);

   code0 = code_point(v0->x, v0->y);
   code1 = code_point(v1->x, v1->y);

   if ((code0&code1)!=0)
      return CLIP_ALL;

   if ((code0|code1)==0) {
      ul_func(v0, v1);
      return CLIP_NONE;
   }
   clip_func = clip_table[type];

   if (code0) {
      if (code0&CLIP_LEFT)
         code0=clip_x(v0, v1, left);
      else if (code0&CLIP_RIGHT)
         code0=clip_x(v0, v1, right);
      if (code0&CLIP_TOP)
         code0=clip_y(v0, v1, top);
      else if (code0&CLIP_BOT)
         code0=clip_y(v0, v1, bot);
      if (code0)
         return CLIP_ALL;
   }

   if (code1) {
      if (code1&CLIP_LEFT)
         code1=clip_x(v1, v0, left);
      else if (code1&CLIP_RIGHT)
         code1=clip_x(v1, v0, right);
      if (code1&CLIP_TOP)
         code1=clip_y(v1, v0, top);
      else if (code1&CLIP_BOT)
         code1=clip_y(v1, v0, bot);
      if (code1)
         return CLIP_ALL;
   }
   ul_func(v0, v1);
   return (code0|code1);
}
