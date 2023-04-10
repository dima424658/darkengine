/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// translucent "floating" text
#include <2d.h>
#include <appagg.h>

#include <resapi.h>
#include <scrnmode.h>
#include <schema.h>

#include <mprintf.h>

#include <dpctluct.h>
#include <dpcovrly.h>
#include <dpcovcst.h>
#include <dpcplayr.h>
#include <dpcutils.h>
#include <dpcprop.h>

IRes *gTextBackHnd;

extern grs_font *gDPCFontBlue;
char gTlucText[255];

static BOOL gTlucUpdate;

static Rect text_rect = {{192, 278},{192 + 256, 278 + 64}};

//--------------------------------------------------------------------------------------
void DPCTlucTextInit(int which)
{
   Rect use_rect;
   gTextBackHnd= LoadPCX("tluctext");
   DPCOverlaySetFlags(which, kOverlayFlagBufferTranslucent);

   sScrnMode smode;
   ScrnModeGet(&smode);

   use_rect.ul.x = (smode.w - RectWidth(&text_rect)) / 2;
   use_rect.ul.y = smode.h - (480 - text_rect.ul.y);
   use_rect.lr.x = use_rect.ul.x + RectWidth(&text_rect);
   use_rect.lr.y = use_rect.ul.y + RectHeight(&text_rect);

   //mprintf("setting use_rect to %d, %d -> %d, %d\n",use_rect.ul.x, use_rect.ul.y, use_rect.lr.x, use_rect.lr.y);
   DPCOverlaySetRect(which, use_rect);
}

//--------------------------------------------------------------------------------------
void DPCTlucTextTerm(void)
{
   SafeFreeHnd(&gTextBackHnd);
}

//--------------------------------------------------------------------------------------
void DPCTlucTextDraw(unsigned long inDeltaTicks)
{
   int w,h;
   Point p;
   Rect r = DPCOverlayGetRect(kOverlayTlucText);

   w = gr_font_string_width(gDPCFontBlue, gTlucText);
   h = gr_font_string_height(gDPCFontBlue, gTlucText);
   p.x = (RectWidth(&r) - w) / 2;
   p.y = (RectHeight(&r) - h) / 2;
   //mprintf("drawing %s at %d, %d (width is %d, %d)(%d, %d)\n",gTlucText,p.x,p.y,w,h,RectWidth(&r),RectHeight(&r));
   //gr_set_fcolor(gDPCTextColor);
   gr_font_string(gDPCFontBlue, gTlucText, p.x, p.y);
}
//--------------------------------------------------------------------------------------
IRes *DPCTlucTextBitmap(void)
{
   return(gTextBackHnd);
}
//--------------------------------------------------------------------------------------
void DPCTlucTextAdd(char *name, char *table, int offset)
{
   //mprintf("text add %s, %s, %d\n",name,table,offset);
   DPCStringFetch(gTlucText,sizeof(gTlucText), name, table, offset);
   DPCOverlayChange(kOverlayTlucText,kOverlayModeOn);
   gTlucUpdate = TRUE;
}
//--------------------------------------------------------------------------------------

bool DPCTlucUpdate(void)
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
sOverlayFunc OverlayTlucText = 
{ 
   DPCTlucTextDraw,     // draw
   DPCTlucTextInit,     // init
   DPCTlucTextTerm,     // term
   NULL,                // mouse
   NULL,                // dclick (really use)
   NULL,                // dragdrop
   NULL,                // key
   DPCTlucTextBitmap,   // bitmap
   "",                  // upschema
   "",                  // downschema
   NULL,                // state
   NULL,                // transparency
   0,                   // distance
   FALSE,               // needmouse
   200,                 // alpha
   DPCTlucUpdate,       // update
};

