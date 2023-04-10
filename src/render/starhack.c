/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/starhack.c,v 1.20 2000/02/19 12:35:48 toml Exp $
// starfield setup/initializers/so on

#include <stdlib.h>

#include <lg.h>
#include <mprintf.h>
#ifdef PLAYTEST
#include <config.h>
#endif
#include <dev2d.h>
#include <lgd3d.h>
#include <r3d.h>
#include <matrix.h>
#include <star.h>
#include <timer.h>

#include <portal.h>
#include <portsky.h>
#include <wr.h>
#include <starhack.h>
#include <texmem.h>
#include <command.h>
#include <status.h>
#include <render.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

extern BOOL g_lgd3d;
extern BOOL g_zbuffer;
////////////
// silly starfield code for now
// is it worth tracking whether we saw any sky or not
// i guess we can time it, eh?

BOOL any_stars;
int star_base_color=1, star_num_colors=4;

int star_rate=0, star_axis=2;

static bool g_d3d_stars = TRUE;

static bool g_span_sky = TRUE;

static float star_z, star_w;

static bool have_set_cnt = FALSE;

static BOOL bDidInit = FALSE;

static void lgd3d_pixfunc(int c, int x, int y)
{
   r3s_point p;
   p.grp.sx = fix_make(x,0);
   p.grp.sy = fix_make(y,0);
   p.grp.w = star_w;
   p.p.z = star_z;
   gr_set_fcolor(c);
   lgd3d_draw_point(&p);
}

typedef struct sfs_point
{
   int c;
   short x,y;
} sfs_point;

static sfs_point *buffer_points=NULL;
static int num_star_pixels;

static void buffer_pixfunc(int c, int x, int y)
{
   sfs_point *p = &(buffer_points[num_star_pixels++]);
   p->c = c;
   p->x = x;
   p->y = y;
}

static void (*pixfunc)(int c, int x, int y);

static void iterate_buffered_points(int (*point_is_visible)(int x, int y))
{
   int i;
   pixpal = grd_pal16_list[0];
   for (i=0; i<num_star_pixels; i++) {
      sfs_point *p = &(buffer_points[i]);
      if (point_is_visible(p->x, p->y))
         pixfunc(p->c, p->x, p->y);
   }
}

void starfield_set_z(double z)
{
   star_z = z;
   star_w = 1.0/z;
}


void starfield_renderframe(Position *campos, float zoom)
{
   fixang star_heading;
   mxs_vector zero = {0,0,0};

   if (any_stars)
   {
      //      ulong start_time=tm_get_millisec(), end_time;
      r3_start_frame();
      r3_set_view_angles(&zero,&campos->fac,R3_DEFANG);
      r3_set_zoom(zoom);
      star_heading=(tm_get_millisec()*star_rate/16)&0xffff;
      r3_start_object_n(&zero,star_heading,star_axis);
      r3_start_block();
      StarSky();    // for now, we always assume we are around...
      StarSetCheckBkgnd(!g_lgd3d && !g_span_sky);
      StarRender();
      r3_end_block();
      r3_end_object();
      r3_end_frame();
      //      end_time=tm_get_millisec();
      //      mprintf("stars took %d\n",end_time-start_time);
   }
}

void starfield_startframe(Position *campos, float zoom)
{
   if ((g_lgd3d) && (g_d3d_stars)) {
      if (g_zbuffer) {
         ptsky_set_type(PTSKY_ZBUFFER);
      } else {
         if (g_span_sky) {
            num_star_pixels = 0;
            pixfunc = lgd3d_pixfunc;
            StarSetPixFunc(buffer_pixfunc);
            ptsky_set_type(PTSKY_SPAN);
         } else {
            lgd3d_clear(0);
            StarSetPixFunc(lgd3d_pixfunc);
            ptsky_set_type(PTSKY_NONE);
         }
         starfield_renderframe(campos, zoom);
      }
   } else {
      if (g_span_sky) {
         num_star_pixels = 0;
         pixfunc = gd_upix8_expose(0,0,0);
         StarSetPixFunc(buffer_pixfunc);
         ptsky_set_type(PTSKY_SPAN);
         starfield_renderframe(campos, zoom);
      } else
         ptsky_set_type(PTSKY_NORMAL);
   }
   any_stars=FALSE;
}

void starfield_endframe(Position *campos, float zoom)
{
   if (g_lgd3d && g_d3d_stars && g_zbuffer)
   {
      StarSetPixFunc(lgd3d_pixfunc);
      starfield_renderframe(campos, zoom);
   }
   if ((!g_lgd3d) && (!g_span_sky))
   {
      StarSetPixFunc(NULL);
      starfield_renderframe(campos, zoom);
   }
}

#define STAR_DEFAULT_COUNT 150

void starfield_set_cnt(int new_cnt)
{
   if (have_set_cnt) {
      StarFree();
      StarSet(0,NULL,NULL);
      Free(buffer_points);
      buffer_points = NULL;
   }

   if (!new_cnt)
      return;

   buffer_points = Malloc(new_cnt * sizeof(sfs_point));
   StarMalloc(new_cnt);
   srand(new_cnt);
   StarRand(star_base_color, star_num_colors);
   have_set_cnt = TRUE;
}

#ifdef PLAYTEST
Command star_keys[] =
{
   { "set_star_count", FUNC_INT, starfield_set_cnt, "how many stars" },
   { "set_star_rate", VAR_INT, &star_rate, "how fast they rotate" },
   { "set_star_axis", VAR_INT, &star_axis, "which axis (0-2) rotates" },
   { "star_base_color", VAR_INT, &star_base_color, "first valid palette " },
   { "star_num_colors", VAR_INT, &star_num_colors, "number of palette colors" },
   { "d3d_stars", TOGGLE_BOOL, &g_d3d_stars, "toggle doing d3d stars" },
   { "span_sky", TOGGLE_BOOL, &g_span_sky, "toggle doing span stars" },
};
#endif

void starfield_enter_mode(void)
{
   // set default sky mode...
   if (g_lgd3d)
      g_span_sky = TRUE;
   else
      g_span_sky = FALSE;

#ifdef PLAYTEST
   if ((grd_bpp > 8) && config_is_defined("star_no_ltab_hack"))
      g_span_sky = TRUE;
#endif
}

void starfield_init(void)
{
   if (bDidInit)
      return;

   have_set_cnt = FALSE;
   starfield_set_cnt(STAR_DEFAULT_COUNT);
   ptsky_set_render_func(iterate_buffered_points);
   StarSetStarRenderCallback(0); // grab it back from newsky.
#ifdef PLAYTEST
   COMMANDS(star_keys, HK_ALL);
#endif
   bDidInit = TRUE;
}

void starfield_term(void)
{
   if (!bDidInit)
      return;

   if (have_set_cnt)
      StarFree();

   StarSet(0,NULL,NULL);
   if (buffer_points != NULL)
   {
      Free(buffer_points);
      buffer_points = 0;
   }
   have_set_cnt = FALSE;
   bDidInit = FALSE;
}
