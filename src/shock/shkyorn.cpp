/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkyorn.cpp,v 1.11 2000/02/19 13:26:38 toml Exp $

#include <dev2d.h>
#include <string.h>
#include <appagg.h>
#include <guistyle.h>

#include <resapi.h>

#include <objsys.h>
#include <osysbase.h>

#include <scrptapi.h>

#include <shkscrm.h>
#include <shkyorn.h>
#include <shkovrly.h>
#include <shkovcst.h>
#include <shkutils.h>
#include <shkgame.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

IRes *gYorNBack = NULL;
ObjID g_YorNobj = OBJ_NULL;
char g_YorNtext[255];

static Rect full_rect = {{241,203},{405,275}};
//--------------------------------------------------------------------------------------
void ShockYorNInit(int which)
{
   //gYorNBack = LoadPCX("yorn");
   ShockOverlaySetRect(which, full_rect);
}

//--------------------------------------------------------------------------------------
void ShockYorNTerm(void)
{
   //SafeFreeHnd(&gYorNBack);
}

//--------------------------------------------------------------------------------------
static Rect text_rect = {{8,6},{156,39}};
void ShockYorNDraw(void)
{
   Rect r;
   Point drawpt;
   int w,h,dx,dy;
   char temp[255];

   r = ShockOverlayGetRect(kOverlayYorN);
   drawpt = r.ul;
   DrawByHandle(gYorNBack,drawpt);

   // draw in the text
   strcpy(temp,g_YorNtext);
   //guiStyleSetupFont(NULL, StyleFontNormal); 
   gr_font_string_wrap(gShockFont,temp,RectWidth(&text_rect));
   h = gr_font_string_height(gShockFont,temp);
   w = gr_font_string_width(gShockFont, temp);
   dx = r.ul.x + text_rect.ul.x + (RectWidth(&text_rect) - w) / 2;
   dy = r.ul.y + text_rect.ul.y + (RectHeight(&text_rect) - h) / 2;
   gr_font_string(gShockFont,temp,dx,dy);
   //guiStyleCleanupFont(NULL,StyleFontNormal); 

}
//--------------------------------------------------------------------------------------
static Rect yes_rect = {{5,42},{79,64}};
static Rect no_rect = {{83,42},{158,64}};

bool ShockYorNHandleMouse(uiMouseEvent *mev)
{
   Point pt;
   Rect r = ShockOverlayGetRect(kOverlayYorN);
   bool close;
   bool yes;

   if (mev->action & MOUSE_LUP)
   {
      pt.x = mev->pos.x - r.ul.x;
      pt.y = mev->pos.y - r.ul.y;
      if (RectTestPt(&yes_rect,pt))
      {
         yes = TRUE;
         close = TRUE;
      }
      else if (RectTestPt(&no_rect,pt))
      {
         yes = FALSE;
         close = TRUE;
      }
      if (close)
      {
         // send an appropriate message to the object
         AutoAppIPtr(ScriptMan);
         sYorNMsg message(ShockOverlayGetObj(),yes);

         pScriptMan->SendMessage(&message);
         ShockOverlayChange(kOverlayYorN,kOverlayModeOff);
      }
      return(TRUE);
   }
   return(FALSE);
}

//--------------------------------------------------------------------------------------
void ShockYorNStateChange(int which)
{
   //ToggleMouseStateChange(which);
}
//--------------------------------------------------------------------------------------
// Check for transparency in the source art
//--------------------------------------------------------------------------------------
bool ShockYorNCheckTransp(Point pt)
{
   bool retval;
   retval = ShockOverlayCheckTransp(pt, kOverlayYorN, gYorNBack);
   return(retval);
}
//--------------------------------------------------------------------------------------
void ShockYorNPopup(ObjID o, const char *text)
{
   // don't bring up if one already up
   if (ShockOverlayCheck(kOverlayYorN))
      return;

   g_YorNobj = o;
   strcpy(g_YorNtext, text);
   ShockOverlayChange(kOverlayYorN, kOverlayModeOn);
   ShockOverlaySetObj(kOverlayYorN, o);
}
//--------------------------------------------------------------------------------------
