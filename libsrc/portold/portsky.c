/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/libsrc/portold/portsky.c,v 1.1 1998/03/03 12:33:22 KEVIN Exp $

#include <stdlib.h>

#include <dbg.h>
#include <lgassert.h>
#include <g2.h>
#include <r3ds.h>

#include <portsky.h>

#define MAX_SPANS 6*G2C_MAX_HEIGHT

int ptsky_type = PTSKY_NORMAL;

static int y_min, y_max;
static int (*xdata)[G2C_MAX_HEIGHT][2];

typedef struct SkySpanInfo SkySpanInfo;

struct SkySpanInfo {
   short x0;
   short x1;
   SkySpanInfo *next;
};

static int span_next=0;
static SkySpanInfo span_data[MAX_SPANS];
static SkySpanInfo *span_ptr[G2C_MAX_HEIGHT];
#define span_alloc() (&span_data[span_next++])


static void add_span_info(int y, int x0, int x1)
{
   SkySpanInfo *si = span_ptr[y];
   if (si==NULL) {
      span_ptr[y] = si = span_alloc();
      si->x0 = x0;
      si->x1 = x1;
      si->next = NULL;
      return;
   }
   // note the following fails to remerge spans in a case like
   // s0x0....s0x1        s1x0....s1x1
   //       x0.................x1
   // but we don't really care....
   do {
      // overlap left?
      if ((si->x0 <= x1) && (x1 <= si->x1)) {
         si->x0 = min(x0, si->x0);
         return;
      }
      // overlap right?
      if ((si->x0 <= x0) && (x0 <= si->x1)) {
         si->x1 = max(x1, si->x1);
         return;
      }
      // complete overlap?
      if ((x0 < si->x0) && (si->x1 < x1)) {
         si->x0 = x0;
         si->x1 = x1;
         return;
      }
      // no overlap, try next span...
      if (si->next == NULL)
         break;
      si = si->next;
   } while (TRUE);
   // add new span
   si->next = span_alloc();
   si = si->next;
   si->next = NULL;
   si->x0 = x0;
   si->x1 = x1;
}


static int sky_point_is_visible(int x, int y)
{
   SkySpanInfo *si;

   if ((y<0)||(y>=G2C_MAX_HEIGHT))
      return FALSE;

   si = span_ptr[y];
   while (si!=NULL) {
      if ((x >= si->x0) && (x < si->x1))
         return TRUE;
      si = si->next;
   }
   return FALSE;
}


static void reset_sky_spans(void)
{
   int i;
   span_next = 0;
   for (i=0; i<G2C_MAX_HEIGHT; i++)
      span_ptr[i] = NULL;
}


// called from portdraw.c in d3d rendering mode only
int ptsky_calc_spans(int n, r3s_phandle *vp)
{
   int i,j;

   g2_reset_scan_buffer();

   j = n-1;
   for (i=0; i < n; ++i) {
      g2_scan_convert((g2s_point *)&(vp[i]->grp), (g2s_point *)&(vp[j]->grp));
      j = i;
   }
   g2_get_scan_conversion(&y_min, &y_max, &xdata);

   // 0-height poly
   if (y_min >= y_max)
      return 0;

   if (y_min < 0) {
      Warning(("Polygon off top of screen.\n"));
      return 0;
   }

   if (y_max >= G2C_MAX_HEIGHT) {
      Warning(("Polygon off bottom of max-sized screen.\n"));
      return 0;
   }

   for (i = y_min; i<y_max; ++i)
      add_span_info(i, (*xdata)[i][0], (*xdata)[i][1]);

   AssertMsg(span_next <= MAX_SPANS, "Too many sky spans!");
   return (y_max - y_min);
}

// how to setup.
static void (*star_render_func)(int (*point_test_func)(int x, int y)) = NULL;
void ptsky_set_render_func(void (*render_func)(int (*test_func)(int x, int y)))
{
   star_render_func = render_func;
   reset_sky_spans();
}


void ptsky_set_type(int type)
{
   ptsky_type = type;
}


// this needs to be called after sky polys are drawn and before any other
// polys are drawn.
// currently called from portdraw.c and portal.c
void ptsky_render_stars(void)
{
   if ((span_next == 0)||(star_render_func==NULL)||(ptsky_type!=PTSKY_SPAN))
      return;

   star_render_func(sky_point_is_visible);
   reset_sky_spans();
}
