/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/status.c,v 1.12 2000/02/19 13:11:27 toml Exp $

// Status line system

#include <string.h>

#include <lg.h>
#include <2d.h>
#include <res.h>
#include <rect.h>
#include <guistyle.h>

#include <status.h>

#include <editor.h>
#include <scrnman.h>

#include <config.h>
#include <cfgdbg.h>
#include <mprintf.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

////  If I were Mahk, this would be
////  data driven and determined by layout
////  But I'm not, so I won't.

////  Start points are expressed as
////  locations from 0..100

  // foreground/background colors
#define STATUS_BG  (guiStyleGetColor(NULL,StyleColorWhite))
#define STATUS_FG  (guiStyleGetColor(NULL,StyleColorBlack))

#define MAX_FIELDS  SF_NUM_FIELDS
#define MAX_FIELD_SIZE  128

static int default_size[MAX_FIELDS] =
{
    0,
    1, 
   20,
   40,
   42,
   75,
   77,
   79,
};

static int default_x[MAX_FIELDS+1];
static int current_x[MAX_FIELDS+1];
static int current_size[MAX_FIELDS];
char status_string[MAX_FIELDS][MAX_FIELD_SIZE];

static Rect rect;
static int chr;        // typical character width
#if 0
static bool overflow;  // do we have any strings that have gone qwazy?
#endif

static int status;         // is the status line enabled, to what nesting?
static bool redraw=TRUE;  // do we have pending stuff to show?

void StatusEnable(void)
{
   if (++status > 2)
      ConfigSpew("status_spew",("StatusEnable called %d times not StatusDisabled\n", status));
}

void StatusDisable(void)
{
   if (--status < 0)
      ConfigSpew("status_spew",("StatusDisable: Called too many times.\n"));
   redraw = TRUE;
}

void StatusSetRect(Rect *r)
{
   int i;
   rect = *r;
   guiStyleSetupFont(NULL,StyleFontNormal); 
   chr = gr_font_char_width(gr_get_font(), 'a');
   guiStyleCleanupFont(NULL,StyleFontNormal); 
   current_x[MAX_FIELDS] = grd_bm.w;
   for (i=0; i < MAX_FIELDS; ++i) {
      default_x[i] = grd_bm.w * default_size[i] / 100;
      current_x[i] = default_x[i];
   }
}

static grs_canvas canv;

void StatusStartDrawing(void)
{
   short w = RectWidth(&rect);
   short h = RectHeight(&rect);
   
   gr_init_sub_canvas(grd_canvas, &canv, rect.ul.x, rect.ul.y,w,h);
   gr_push_canvas(&canv);

}

void StatusStopDrawing(void)
{
   gr_pop_canvas();
   ScrnForceUpdateRect(&rect);
   gr_close_canvas(&canv);
}

void StatusDrawString(int f)
{
   int x0 = current_x[f];
   int x1 = current_x[f+1];
   grs_clip clip = grd_gc.clip;

   gr_set_fcolor(STATUS_BG);
   gr_rect(x0,0,x1,grd_bm.h);
   gr_safe_set_cliprect(x0,0,x1-1,grd_bm.h-1);
   gr_set_fcolor(STATUS_FG);
   guiStyleSetupFont(NULL,StyleFontNormal); 
   gr_string(status_string[f], x0, 0);
   guiStyleCleanupFont(NULL,StyleFontNormal); 

   grd_gc.clip = clip;
}

void StatusDrawStringAll(void)
{
   int i;
   StatusStartDrawing();
   for (i=0; i < MAX_FIELDS; ++i)
       StatusDrawString(i);
   StatusStopDrawing();
}


int str_wid(const char* s)
{
   guiStyleSetupFont(NULL,StyleFontNormal); 
   gr_font_string_width(gr_get_font(),s); 
   guiStyleCleanupFont(NULL,StyleFontNormal); 
}

void StatusField(int x, const char *s)
{
   int cur_sz;

   if (x < 0 || x > MAX_FIELDS)
      Error(1, "Invalid status field parameter %d\n", x);
   if (!strcmp(status_string[x], s))
      return;

   strncpy(status_string[x], s, MAX_FIELD_SIZE);

   if (!status) {
      redraw = TRUE;
      return;
   }

   cur_sz = current_size[x];
   current_size[x] = str_wid(s);

     // if we're not in a wacky overflow state, and
     // this string fits in the default size, then
     // just draw it
#if 0
   if (current_size < cur_sz && !overflow) {
#endif
      StatusStartDrawing();
      StatusDrawString(x);
      StatusStopDrawing();
#if 0
   }
#endif
}

void Status(const char *s)
{
   StatusField(SF_STATUS, s);
#ifndef SHIP   
   if (*s != '\0' && config_is_defined("status"))
      mprintf("STATUS: %s\n",s);
#endif 
}

void StatusRecomputeAll(void)
{
   int i;
   for (i=0; i < MAX_FIELDS; ++i)
      current_size[i] = str_wid(status_string[i]);
   StatusDrawStringAll();
}

void StatusUpdate(void)
{
   if (redraw) {
      StatusRecomputeAll();
      redraw = FALSE;
   }
}
