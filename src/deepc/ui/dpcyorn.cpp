/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <dev2d.h>
#include <string.h>
#include <appagg.h>
#include <guistyle.h>

#include <resapi.h>

#include <objsys.h>
#include <osysbase.h>

#include <scrptapi.h>

#include <dpcscrm.h>
#include <dpcyorn.h>
#include <dpcovrly.h>
#include <dpcovcst.h>
#include <dpcutils.h>
#include <dpcgame.h>

IRes *gYorNBack = NULL;
ObjID g_YorNobj = OBJ_NULL;
char g_YorNtext[255];

static Rect full_rect = {{241,203},{405,275}};
//--------------------------------------------------------------------------------------
void DPCYorNInit(int which)
{
   //gYorNBack = LoadPCX("yorn");
    DPCOverlaySetRect(which, full_rect);
}

//--------------------------------------------------------------------------------------
void DPCYorNTerm(void)
{
   //SafeFreeHnd(&gYorNBack);
}

//--------------------------------------------------------------------------------------
static Rect text_rect = {{8,6},{156,39}};
void DPCYorNDraw(unsigned long inDeltaTicks)
{
   Point drawpt;
   int w,h,dx,dy;
   char temp[255];

   Rect r = DPCOverlayGetRect(kOverlayYorN);
   drawpt = r.ul;
   DrawByHandle(gYorNBack,drawpt);

   // draw in the text
   strcpy(temp,g_YorNtext);
   //guiStyleSetupFont(NULL, StyleFontNormal); 
   gr_font_string_wrap(gDPCFont,temp,RectWidth(&text_rect));
   h = gr_font_string_height(gDPCFont,temp);
   w = gr_font_string_width(gDPCFont, temp);
   dx = r.ul.x + text_rect.ul.x + (RectWidth(&text_rect) - w) / 2;
   dy = r.ul.y + text_rect.ul.y + (RectHeight(&text_rect) - h) / 2;
   gr_font_string(gDPCFont,temp,dx,dy);
   //guiStyleCleanupFont(NULL,StyleFontNormal); 

}
//--------------------------------------------------------------------------------------
static Rect yes_rect = {{5,42},{79,64}};
static Rect no_rect = {{83,42},{158,64}};

bool DPCYorNHandleMouse(uiMouseEvent *mev)
{
   Point pt;
   Rect r = DPCOverlayGetRect(kOverlayYorN);
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
         sYorNMsg message(DPCOverlayGetObj(),yes);

         pScriptMan->SendMessage(&message);
         DPCOverlayChange(kOverlayYorN,kOverlayModeOff);
      }
      return(TRUE);
   }
   return(FALSE);
}

//--------------------------------------------------------------------------------------
void DPCYorNStateChange(int which)
{
   //ToggleMouseStateChange(which);
}
//--------------------------------------------------------------------------------------
// Check for transparency in the source art
//--------------------------------------------------------------------------------------
bool DPCYorNCheckTransp(Point pt)
{
   bool retval;
   retval = DPCOverlayCheckTransp(pt, kOverlayYorN, gYorNBack);
   return(retval);
}
//--------------------------------------------------------------------------------------
void DPCYorNPopup(ObjID o, const char *text)
{
   // don't bring up if one already up
   if (DPCOverlayCheck(kOverlayYorN))
      return;

   g_YorNobj = o;
   strcpy(g_YorNtext, text);
   DPCOverlayChange(kOverlayYorN, kOverlayModeOn);
   DPCOverlaySetObj(kOverlayYorN, o);
}
//--------------------------------------------------------------------------------------
