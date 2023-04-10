/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <string.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <math.h>

#include <lg.h>
#include <fix.h>
#include <2d.h>
#include <unlit.h>

unsigned char *scr;
int row;

#define MAX_PTS 256   // 100*100 == 20K triangles, but we want e.g. 50*200

grs_vertex row_a[MAX_PTS+1],row_b[MAX_PTS+1];
fix xlist[MAX_PTS+1],ulist[MAX_PTS+1];

#define RAND_LEN 256

static fix rand_list[RAND_LEN];
static int rand_count=0;

#define my_rand()  (rand_list[rand_count = (rand_count+1)&(RAND_LEN-1)]);

#define SWAP(a,b,temp)  ((temp)=(a),(a)=(b),(b)=(temp))

extern grs_bitmap texture;
extern bool swap_uv, wireframe;
int tris;

#if 0
  #define draw_tri(t)    gr_lin_map(&texture, 3, t)
#elif 0
  #define draw_tri(t)    mapper(&texture, 3, t)
#else
  #define draw_tri(t)    render_tri(&texture, t);
#endif

int row_table[1024];

point blah[3];
point *blah_list[3] = { blah,blah+1,blah+2 };

void render_tri(grs_bitmap *b, grs_vertex **vl)
{
   int i;
   for (i=0; i < 3; ++i) {
      blah[i].sx = vl[i]->x;
      blah[i].sy = vl[i]->y;
      blah[i].u = vl[i]->u/65536.0;
      blah[i].v = vl[i]->v/65536.0;
   }
   unlit_triangle(blah_list, b->bits);
}

void draw_grid(int x0, int y0, int x1, int y1, int nx, int ny,
      fix u0, fix v0, fix u1, fix v1)
{
   int i,j;
   fix y,v;

   grs_vertex *prev, *next, *temp;
   g2ut_func *mapper = gr_lin_umap_expose(&texture, 0, 0);

   prev = row_a;
   next = row_b;

   for (i=0; i <= nx; ++i) {
      xlist[i] = fix_make(x0+(x1-x0)*i/nx,0)-0x8000;
      ulist[i] = u0 + fix_mul_div(u1-u0,i,nx);
   }

   y = fix_make(y0,0)-0x8000;
   v = v0;
   for (i=0; i <= nx; ++i) {
      prev[i].x = xlist[i];
      prev[i].y = y;
      if (!swap_uv) {
         prev[i].u = ulist[i];
         prev[i].v = v;
      } else {
         prev[i].v = ulist[i];
         prev[i].u = v;
      }
   }

   for (j=1; j <= ny; ++j) {
      y = fix_make(y0+(y1-y0)*j/ny,0)-0x8000;
      if (j == ny) {
         for (i=0; i <= nx; ++i) {
            next[i].x = xlist[i];
            next[i].y = y;
         }
      } else {
         next[0].x = xlist[0];
         next[0].y = y;
         for (i=1; i < nx; ++i) {
            next[i].x = xlist[i] + my_rand();
            next[i].y = y + my_rand();
         }
         next[i].x = xlist[i];
         next[i].y = y;
      }
      v = v0 + fix_mul_div(v1-v0,j,ny);
      if (!swap_uv) {
         for (i=0; i <= nx; ++i) {
            next[i].u = ulist[i];
            next[i].v = v;
         }
      } else {
         for (i=0; i <= nx; ++i) {
            next[i].v = ulist[i];
            next[i].u = v;
         }
      }

      // ok, now draw all the triangles
      for (i=0; i < nx; ++i) {
         grs_vertex *tri[3];
         tri[0] = &prev[i];
         tri[1] = &prev[i+1];
         tri[2] = &next[i];
         draw_tri(tri);
         tri[0] = &next[i];
         tri[2] = &next[i+1];
         draw_tri(tri);
         tris += 2;
      }
      if (wireframe) {
         gr_set_fcolor(15);
         for (i=0; i < nx; ++i) {
            gr_line(&prev[i], &prev[i+1]);
            gr_line(&next[i], &prev[i+1]);
            gr_line(&prev[i], &next[i]);
         }
      }

      SWAP(next, prev, temp);
   }
}

int scale;
void init_random(void)
{
   int i;
   for (i=0; i < RAND_LEN; ++i)
      rand_list[i] = ((((rand()) & 0x3fff) - 0x2000) << 2) * (scale+1);
}

int grid_size_x = 2, grid_size_y = 2;
int multi_grid_poly=0, multi_grid_split=8;

void draw_scene(void)
{
   int i;
   scr = grd_bm.bits;
   row = grd_bm.row;
   for (i=0; i < grd_bm.h; ++i)
      row_table[i] = row * i;
   init_random();
   if (!multi_grid_poly) {
      draw_grid(0,0,grd_bm.w,grd_bm.h, grid_size_x, grid_size_y,
           0,0, fix_make(240,0), fix_make(240,0));
   } else {
      // compute number of quads
      int poly = (grid_size_x-1) * (grid_size_y-1);
      // distribute them between the two sides
      int poly_left = multi_grid_poly * poly / 100 + 1;
      int poly_right = poly - poly_left;

      int left_x,left_y;
      int right_x,right_y, best,x,y;
      fix uv;

      // left size is  multi_grid_split/16 of whole x thing
      // so to maintain same ratio:
      //     (left_x/(multi_grid_split/16))/left_y = grid_x/grid_y
      //   grid_y*left_x*16 = grid_x*left_y*multi_grid_split

      // and we want left_x * left_y = poly_left
      //      (grid_x*left_y*multi_grid_split/16/grid_y) * left_y = poly_left
      //  left_y^2 = poly_left * 16 * grid_y / multi_grid_split / grid_x

      if (poly_right < 1) poly_right = 1;

      left_y = sqrt(poly_left * 16 * grid_size_y / multi_grid_split / grid_size_x);
      if (left_y == 0) left_y = 1;
      left_x = poly_left / left_y;
      if (left_x == 0) { left_x = 0; if (left_y > 1) --left_y; }

      x = left_x; y = left_y;
      best = x * y;
      ++x;
      if (abs(poly_left - best) > abs(poly_left - x*y))
         left_x = x, left_y = y, best = x*y;
      ++y;
      if (abs(poly_left - best) > abs(poly_left - x*y))
         left_x = x, left_y = y, best = x*y;
      --x;
      if (abs(poly_left - best) > abs(poly_left - x*y))
         left_x = x, left_y = y, best = x*y;

      poly_left = best;

      // ok, we've got poly_left as close as we can
      if (poly - poly_left > 1) poly_right = poly - poly_left;

      right_y = sqrt(poly_right * 16 * grid_size_y / (16-multi_grid_split) / grid_size_x);
      if (right_y == 0) right_y = 1;
      right_x = poly_right / right_y;
      if (right_x == 0) { right_x = 0; if (right_y > 1) --right_y; }

      x = right_x; y = right_y;
      best = x * y;
      ++x;
      if (abs(poly_right - best) > abs(poly_right - x*y))
         right_x = x, right_y = y, best = x*y;
      ++y;
      if (abs(poly_right - best) > abs(poly_right - x*y))
         right_x = x, right_y = y, best = x*y;
      --x;
      if (abs(poly_right - best) > abs(poly_right - x*y))
         right_x = x, right_y = y, best = x*y;

      // ok, we're all set
      x = grd_bm.w * multi_grid_split / 16;
      uv = fix_make(240 * multi_grid_split/16,0);

      draw_grid(0,0,x,grd_bm.h, left_x, left_y,
           0,0, uv, fix_make(240,0));
      draw_grid(x,0,grd_bm.w,grd_bm.h, right_x, right_y,
           uv,0, fix_make(240,0), fix_make(240,0));
   }
}
