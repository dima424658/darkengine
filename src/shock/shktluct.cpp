/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shktluct.cpp,v 1.6 2000/02/19 13:26:26 toml Exp $

// translucent "floating" text
#include <2d.h>
#include <appagg.h>

#include <resapi.h>
#include <scrnmode.h>
#include <schema.h>

#include <mprintf.h>

#include <shktluct.h>
#include <shkovrly.h>
#include <shkovcst.h>
#include <shkplayr.h>
#include <shkutils.h>
#include <shkprop.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

IRes *gTextBackHnd;

extern grs_font *gShockFontBlue;
char gTlucText[255];

static BOOL gTlucUpdate;

static Rect text_rect = {{192, 278},{192 + 256, 278 + 64}};

//--------------------------------------------------------------------------------------
void ShockTlucTextInit(int which)
{
   Rect use_rect;
   gTextBackHnd= LoadPCX("tluctext");
   ShockOverlaySetFlags(which, kOverlayFlagBufferTranslucent);

   sScrnMode smode;
   ScrnModeGet(&smode);

   use_rect.ul.x = (smode.w - RectWidth(&text_rect)) / 2;
   use_rect.ul.y = smode.h - (480 - text_rect.ul.y);
   use_rect.lr.x = use_rect.ul.x + RectWidth(&text_rect);
   use_rect.lr.y = use_rect.ul.y + RectHeight(&text_rect);

   //mprintf("setting use_rect to %d, %d -> %d, %d\n",use_rect.ul.x, use_rect.ul.y, use_rect.lr.x, use_rect.lr.y);
   ShockOverlaySetRect(which, use_rect);
}

//--------------------------------------------------------------------------------------
void ShockTlucTextTerm(void)
{
   SafeFreeHnd(&gTextBackHnd);
}

//--------------------------------------------------------------------------------------
void ShockTlucTextDraw(void)
{
   int w,h;
   Point p;
   Rect r = ShockOverlayGetRect(kOverlayTlucText);

   w = gr_font_string_width(gShockFontBlue, gTlucText);
   h = gr_font_string_height(gShockFontBlue, gTlucText);
   p.x = (RectWidth(&r) - w) / 2;
   p.y = (RectHeight(&r) - h) / 2;
   //mprintf("drawing %s at %d, %d (width is %d, %d)(%d, %d)\n",gTlucText,p.x,p.y,w,h,RectWidth(&r),RectHeight(&r));
   //gr_set_fcolor(gShockTextColor);
   gr_font_string(gShockFontBlue, gTlucText, p.x, p.y);
}
//--------------------------------------------------------------------------------------
IRes *ShockTlucTextBitmap(void)
{
   return(gTextBackHnd);
}
//--------------------------------------------------------------------------------------
void ShockTlucTextAdd(char *name, char *table, int offset)
{
   //mprintf("text add %s, %s, %d\n",name,table,offset);
   ShockStringFetch(gTlucText,sizeof(gTlucText), name, table, offset);
   ShockOverlayChange(kOverlayTlucText,kOverlayModeOn);
   gTlucUpdate = TRUE;
}
//--------------------------------------------------------------------------------------

bool ShockTlucUpdate(void)
{
   if (gTlucUpdate)
   {
      gTlucUpdate = FALSE;
      return(TRUE);
   }
   else
      return(FALSE);
}

//--------------------------------------------------------------------------------------
sOverlayFunc OverlayTlucText = { 
   ShockTlucTextDraw, // draw
   ShockTlucTextInit, // init
   ShockTlucTextTerm, // term
   NULL, // mouse
   NULL, // dclick (really use)
   NULL, // dragdrop
   NULL, // key
   ShockTlucTextBitmap, // bitmap
   "", // upschema
   "", // downschema
   NULL, // state
   NULL, // transparency
   0, // distance
   FALSE, // needmouse
   200, // alpha
   ShockTlucUpdate, // update
};

