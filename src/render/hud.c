/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/hud.c,v 1.3 2000/02/19 12:35:25 toml Exp $

#include <string.h>

#include <2d.h>
#include <mprintf.h>

#include <fixtime.h>
#include <hud.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

typedef struct {
   char str[256];
   int color;
   int kill_time;
} hudString;

int line_mask=0;

#define ONESHOT_HUD

#ifdef ONESHOT_HUD
#define MAX_HUD_LINES 8
static hudString curHud[MAX_HUD_LINES];

// color -1 means use default
// duration 0 means wait for user input to clear
bool hudAddString(char *str, int color, int duration)
{
   int i, msk;

   for (msk=1, i=0; i<MAX_HUD_LINES; i++, msk<<=1)
      if ((msk&line_mask)==0)
         break;
   if (i==MAX_HUD_LINES)
      return FALSE;
   line_mask|=msk;
   strcpy(curHud[i].str,str);
   if (color==-1) color=0x1;  // 0xb4
   curHud[i].color=color;
   curHud[i].kill_time=fixtime()+duration;
   return TRUE;
}

#define hudActive(i,msk)     (msk&line_mask)
#define hudDeactivate(i,msk) (line_mask&=~msk)
#endif

#ifdef LOOPING_HUD
#define MAX_HUD_LINES 4
static hudString curHud[MAX_HUD_LINES];
static int curLine;
// color -1 means use default
// duration 0 means wait for user input to clear
bool hudAddString(char *str, int color, int duration)
{
   int i, msk;

   for (msk=1, i=0; i<MAX_HUD_LINES; i++, msk<<=1)
      if ((msk&line_mask)==0)
         break;
   if (i==MAX_HUD_LINES)
      return FALSE;
   line_mask|=msk;
   strcpy(curHud[i].str,str);
   if (color==-1) color=0x1;  // 0xb4
   curHud[i].color=color;
   curHud[i].kill_time=fixtime()+duration;
   return TRUE;
}
#define hudActive(i,msk)     (msk&line_mask)
#define hudDeactivate(i,msk) (line_mask&=~msk)
#endif

#define SCRX (grd_canvas->bm.w)
#define SCRY (grd_canvas->bm.h)

void hudDraw(void *vfnt)
{
   int curtime=(int)fixtime();
   int i, msk, line=0, h;
   grs_font *fnt=(grs_font *)vfnt;

   if (fnt==NULL) fnt=gr_get_font();
   h=gr_font_string_height(fnt,"w");

   for (msk=1, i=0; i<MAX_HUD_LINES; i++, msk<<=1)
      if (hudActive(i,msk))
         if (curtime>curHud[i].kill_time)
            hudDeactivate(i,msk);
         else
         {  // string=curHud[i].str, color=.color, line is local var line
            int w=gr_font_string_width(fnt,curHud[i].str);
            gr_set_fcolor(curHud[i].color);
            gr_font_string(fnt,curHud[i].str,SCRX-10-w,SCRY-3-h-line*12);
            line++;
         }

#ifdef SILLYCURSOR
   gr_set_fcolor(67);
#define LEN (grd_canvas->bm.w/80)
   gr_int_line(SCRX/2-LEN, SCRY/2-LEN, SCRX/2+LEN, SCRY/2+LEN);
   gr_int_line(SCRX/2+LEN, SCRY/2-LEN, SCRX/2-LEN, SCRY/2+LEN);
#endif
}
