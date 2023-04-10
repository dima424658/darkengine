/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkutils.cpp,v 1.36 2000/02/19 13:26:33 toml Exp $

#include <appagg.h>
#include <string.h>

#include <shkutils.h>
#include <string.h>
#include <dev2d.h>
#include <mprintf.h>

#include <resapi.h>
#include <resistr.h>
#include <imgrstyp.h>
#include <palrstyp.h>
#include <strrstyp.h>
#include <fonrstyp.h>

#include <objtype.h>
#include <shkgame.h>
#include <shkcurm.h>
#include <shkfsys.h>
#include <shkinv.h>

#include <palmgr.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

grs_font *gShockFont = NULL;
IRes *gShockFontRes = NULL;

grs_font *gShockFontAA = NULL;
IRes *gShockFontAARes = NULL;

grs_font *gShockFontMono = NULL;
IRes *gShockFontMonoRes = NULL;

grs_font *gShockFontDimmed = NULL;
IRes *gShockFontDimmedRes = NULL;

grs_font *gShockFontBlue = NULL;
IRes *gShockFontBlueRes = NULL;

uint gShockTextColor = 0;

// For the DrawCursor methods:
static IRes *gPendingHnd = NULL;

//--------------------------------------------------------------------------------------
void ShockUtilsInit(void)
{
   AutoAppIPtr(ResMan);
   IRes *fontpal;
   grs_bitmap *fontpalbmp;

   fontpal = LoadPCX("fontpal");
   fontpalbmp = (grs_bitmap *) fontpal->Lock();

   gShockFontMonoRes = pResMan->Bind("mainfont",RESTYPE_FONT, NULL, "fonts\\");
   if (gShockFontMonoRes != NULL)
      gShockFontMono = (grs_font *)gShockFontMonoRes->Lock();

   //gShockFontAA = gShockFont;
   //gShockBoldFontAA = gShockFont;

   gShockFontAARes = pResMan->Bind("mainaa",RESTYPE_FONT, NULL, "fonts\\");
   if (gShockFontAARes != NULL)
   {
      gShockFontAA = (grs_font *)gShockFontAARes->Lock();
      gShockFontAA->pal_id = fontpalbmp->align;
   }

   gShockFontDimmedRes = pResMan->Bind("Dimmed",RESTYPE_FONT, NULL, "fonts\\");
   if (gShockFontDimmedRes != NULL)
   {
      gShockFontDimmed = (grs_font *)gShockFontDimmedRes->Lock();
      gShockFontDimmed->pal_id = fontpalbmp->align;
   }

   gShockFontBlueRes = pResMan->Bind("BlueAA",RESTYPE_FONT, NULL, "fonts\\");
   if (gShockFontBlueRes != NULL)
   {
      gShockFontBlue = (grs_font *)gShockFontBlueRes->Lock();
      gShockFontBlue->pal_id = fontpalbmp->align;
   }

   // what is our default font?
   gShockFont = gShockFontAA;

   fontpal->Unlock();
   SafeFreeHnd(&fontpal);

   gPendingHnd = LoadPCX("hourglas", SHK_INTERFACE_PATH);
}

//--------------------------------------------------------------------------------------
void ShockUtilInitColor()
{
   int color[3] = {0,255,190}; // { 0, 255, 0};
   gShockTextColor = FindColor(color);
}


//--------------------------------------------------------------------------------------
void ShockUtilsTerm(void)
{
   gShockFontMonoRes->Unlock();
   gShockFontMonoRes->Release();
   gShockFontAARes->Unlock();
   gShockFontAARes->Release();
   gShockFontDimmedRes->Unlock();
   gShockFontDimmedRes->Release();
   gShockFontBlueRes->Unlock();
   gShockFontBlueRes->Release();
   SafeFreeHnd(&gPendingHnd);
}

//--------------------------------------------------------------------------------------
// Utility routine to load correct .PCX bitmap (based on current res).
// Ideally, in the long run, this should take an ISearchPath instead of
// the current char *path; it'll be more efficient.
//
// For the moment, this is doing a global Lock(), since we have a couple
// of DataPeeks around the system. If we can rid ourselves of the DataPeeks,
// then we can eliminate this Lock, and the Unlock in SafeFreeHnd.
//
// The returned resource should be freed with SafeFreeHnd().
//--------------------------------------------------------------------------------------
IRes *LoadPCX(const char *name, char *path, eShockLoadFlags flags)
{
   /*
   char hires[16], lores[16], name[16];

   sscanf(str, "%s %s", hires, lores);
   if (gHires)
      strcpy(name, hires);
   else
      strcpy(name, lores);
   */
   // We are not currently assuming that image files are .pcx. This is more
   // flexible, but possibly slower. We should keep an eye on the time
   // impact of this:
   // strcat(name, ".PCX");

   AutoAppIPtr(ResMan);
   IRes *pRes = pResMan->Bind(name, RESTYPE_IMAGE, NULL, path);
   if (!pRes) {
      return NULL;
   }
   // @NOTE: We are holding a permanent Lock on all images here (which is
   // eventually Unlocked by SafeFreeHnd). This *may* want to go away later,
   // but we need to think about the Palette ramifications...
   grs_bitmap *pbm = (grs_bitmap *) pRes->Lock();

   // Now that we have the Image, load the Palette as well.
   // @TBD: We should create an ImageAndPalette resource type, which will
   // load both of them at a shot, instead of having to do two disk hits.
   if ((pbm->align == 0) && !(flags & ShockLoadNoPalette))
   {
      IRes *pPallRes = pResMan->Retype(pRes, RESTYPE_PALETTE, 0);
      if (pPallRes) {
         uchar *pPall = (uchar *) pPallRes->Lock();
         pbm->align = palmgr_alloc_pal(pPall);
         pPallRes->Unlock();
         // keep it in memory, so don't drop it.
         //pPallRes->Drop();
         SafeRelease(pPallRes);
      }
   }

   return pRes;
}

//--------------------------------------------------------------------------------------
// Draw some art, specified by handle
//--------------------------------------------------------------------------------------
BOOL DrawByHandle(IRes *drawhand, Point pt)
{
   grs_bitmap *bm;
   if (drawhand != NULL)
   {
      bm = (grs_bitmap *) drawhand->Lock();
      gr_bitmap(bm, pt.x, pt.y);
      drawhand->Unlock();
      return(TRUE);
   }
   return(FALSE);
}

BOOL DrawByHandleCenter(IRes *drawhand, Point pt)
{
   grs_bitmap *bm;
   if (drawhand != NULL)
   {
      bm = (grs_bitmap *) drawhand->Lock();
      gr_bitmap(bm, pt.x - (bm->w / 2), pt.y - (bm->h / 2));
      drawhand->Unlock();
      return(TRUE);
   }
   return(FALSE);
}

BOOL DrawByHandleCenterRotate(IRes *drawhand, Point pt, fixang theta)
{
   grs_bitmap *bm;
   if (drawhand != NULL)
   {
      bm = (grs_bitmap *) drawhand->Lock();
      gr_rotate_bitmap(bm, theta, fix_make(pt.x,0), fix_make(pt.y,0));
      drawhand->Unlock();

      return(TRUE);
   }
   return(FALSE);
}
//--------------------------------------------------------------------------------------
// Similar to the above, but will do any additional manipulation appropriate
// for all cursors. At the moment, this just means that it will tack on the
// "pending" icon if a frob request is pending.
// This may be getting a little high-level for shkutils; should we break
// it out?
//--------------------------------------------------------------------------------------
BOOL DrawCursorByHandle(IRes *drawhand, Point pt)
{
   grs_bitmap *bm;
   if (drawhand != NULL)
   {
      bm = (grs_bitmap *) drawhand->Lock();
      gr_bitmap(bm, pt.x, pt.y);
      if (ShockFrobPending() && gPendingHnd) {
         grs_bitmap *bm2;
         bm2 = (grs_bitmap *) gPendingHnd->Lock();
         gr_bitmap(bm2, pt.x + bm->w, pt.y + bm->h);
         gPendingHnd->Unlock();
      }
      drawhand->Unlock();
      return(TRUE);
   }
   return(FALSE);
}

BOOL DrawCursorByHandleCenter(IRes *drawhand, Point pt)
{
   grs_bitmap *bm;
   int dx,dy;
   if (drawhand != NULL)
   {
      bm = (grs_bitmap *) drawhand->Lock();
      dx = pt.x - (bm->w / 2);
      dy = pt.y - (bm->h / 2);
      gr_bitmap(bm, dx, dy);
      if (ShockFrobPending() && gPendingHnd) {
         grs_bitmap *bm2;
         bm2 = (grs_bitmap *) gPendingHnd->Lock();
         gr_bitmap(bm2, pt.x + (bm->w / 2), pt.y + (bm->h / 2));
         gPendingHnd->Unlock();
      }
      drawhand->Unlock();

      // add in a stack count, potentially
      if (shock_cursor_mode == SCM_DRAGOBJ)
      {
         char temp[32];
         if (ShockObjGetQuantity(drag_obj,temp))
         {
            gr_set_fcolor(gShockTextColor);
            gr_font_string(gShockFontMono, temp, dx + 3 , dy + 3);
         }
      }
      return(TRUE);
   }
   return(FALSE);
}

//--------------------------------------------------------------------------------------
// Free a IRes *, making sure not to free a NULL, and clearing the value afterwards
//--------------------------------------------------------------------------------------
void SafeFreeHnd(IRes **hndPtr)
{
   if (*hndPtr != NULL)
   {
      (*hndPtr)->Unlock();
      (*hndPtr)->Release();
      *hndPtr = NULL;
   }
}

//--------------------------------------------------------------------------------------
// Return the pixel value at x,y within the bitmap specified by the handle
//--------------------------------------------------------------------------------------
DWORD HandleGetPix(IRes *handle, Point loc)
{
   DWORD retval = 0;
   grs_bitmap *bm;
   if (handle != NULL)
   {
      bm = (grs_bitmap *) handle->Lock();
      if ((loc.x >= bm->w) || (loc.y >= bm->h) || (loc.x < 0) || (loc.y < 0))
         retval = 0;
      else
         retval = gr_get_pixel_bm(bm,loc.x,loc.y);
      handle->Unlock();
   }
   return(retval);
}

//--------------------------------------------------------------------------------------
// just does the mechanical UI elements, no state changes
IRes *gCursorHnd = NULL;
extern IRes *gDefaultHnd;

void RemoveCursor()
{
   gCursorHnd = gDefaultHnd;
}

//--------------------------------------------------------------------------------------
// Returns the cursor to it's default state
//--------------------------------------------------------------------------------------
// hmm, should this check to make sure that isn't going to pop down to nothing?
void ClearCursor(void)
{
   if (shock_cursor_mode != SCM_NORMAL)
   {
      //mprintf("Clearing cursor\n");

      RemoveCursor();
      shock_cursor_mode = SCM_NORMAL;
      drag_obj = OBJ_NULL;
   }
}

//--------------------------------------------------------------------------------------
// Given a bitmap handle, makes that bitmap the current cursor
//--------------------------------------------------------------------------------------
// do we also want a grs_bitmap version of this?
bool SetCursorByHandle(IRes *hnd) // , Cursor *cursorp)
{
   RemoveCursor();

   if (hnd != NULL)
   {
      gCursorHnd = hnd;
      return(TRUE);
   }
   return(FALSE);
}
//--------------------------------------------------------------------------------------
// takes in a color triplet
int FindColor(int *color)
{
   // Lookup the correct color
   int icol = 0; 
   for (int i = 2; i >= 0; i--)
   {
      icol <<= 8; 
      icol |= color[i] & 0xFF; 
   }
   return(gr_make_screen_fcolor(icol));
}
//--------------------------------------------------------------------------------------
char *string_basepath = "strings/";
BOOL ShockStringFetch(char *temp,int bufsize, const char *name, const char *table,int offset)
{
   AutoAppIPtr(ResMan);
   char usename[255];
   if (strlen(name) == 0)
      return(FALSE);

   if (offset == -1)
      strcpy(usename,name);
   else
      sprintf(usename,"%s%d",name,offset);

   cAutoIPtr<IRes> res = pResMan->Bind(table,
                                       RESTYPE_STRING,
                                       NULL,
                                       string_basepath); 
   cAutoIPtr<IStringRes> strres (IID_IStringRes,res); 
   if (strres == NULL)
      return(FALSE);
   const char* s = strres->StringLock(usename); 

   if (s)
   {
      strncpy(temp,s,bufsize); 
      strres->StringUnlock(usename);
      return(TRUE);
   }
   else
   {
      strcpy(temp,"");
      return(FALSE);
   }
}
//--------------------------------------------------------------------------------------
void DrawVerticalString(char *text, int x, int y, int dy)
{
   int j;
   char s[2];
   Point drawpt;
   //int w;

   drawpt.x = x;
   drawpt.y = y;

   s[1] = '\0';

   gr_set_fcolor(gShockTextColor);

   for (j=0; j < strlen(text); j++)
   {
      s[0] = text[j];            
      //w = gr_font_string_width(gShockFont, s);
      gr_font_string(gShockFont, s, drawpt.x, drawpt.y);
      drawpt.y = drawpt.y + dy;
   }
}
//--------------------------------------------------------------------------------------
