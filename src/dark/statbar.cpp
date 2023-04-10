/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/statbar.cpp,v 1.5 2000/02/19 13:09:04 toml Exp $
#include <statbar.h>
#include <2d.h>
#include <imgsrc.h>
#include <stdlib.h>
#include <string.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

////////////////////////////////////////////////////////////
// cStatusBar implementation
//

cStatusBar::cStatusBar(const sStatusBarDesc &desc )
   :sStatusBarDesc(desc),
    mpCanvas(NULL)
{
   Reset();
   memset(&mVal,0,sizeof(mVal));
   RectSetNull(&mScreenRect);
}

void cStatusBar::Reset()
{
   for (int i = 0; i < kNumValues; i++)
      mLastVal[i] = -1;
   if (mpCanvas)
      gr_free_canvas(mpCanvas);
   mpCanvas = NULL;
   mLastDrawTime = 0;
}

const Rect& cStatusBar::GetScreenRect()
{
   return mScreenRect;
}

void cStatusBar::SetScreenRect(const Rect& r)
{
   if (RectWidth(&r) != RectWidth(&mScreenRect)
       || RectHeight(&r) != RectHeight(&mScreenRect))
      Reset();
   mScreenRect = r;
}

void cStatusBar::SetVal(eValue which, int val)
{
   if (abs(val - mLastVal[which]) > tolerance)
      Reset();

   mVal[which] = val;
}

void cStatusBar::DoDraw(const Rect& fillme)
{
   gr_set_fcolor(gr_make_screen_fcolor(colors[kFG]));
   gr_rect(RECT_UNWRAP(&fillme));
}

BOOL cStatusBar::MakeDrawCanvas() { return TRUE;}

void cStatusBar::Update(ulong time)
{
   BOOL expired = refresh >= 0 && time - mLastDrawTime > refresh;

   // HideAtMax logic
   if ((flags & kHideAtMax)  && expired && mVal[kCur] == mVal[kMax])
   {
      memcpy(&mLastVal,&mVal,sizeof(mLastVal));
      return ;
   }

   // No screen rect
   if (RectCheckNull(&mScreenRect))
      return;

   BOOL need_refresh = expired|| mpCanvas == NULL;

   BOOL bDoCanvas = MakeDrawCanvas();

   // if we have changed values, or time has elapsed, refresh
   if (need_refresh)
   {
      // clamp values
      int& max = mVal[kMax];
      if (max <= 0) max = 1;
      int& val = mVal[kCur];
      if (val > max) val = max;
      if (val < 0) val = 0;

      int w = RectWidth(&mScreenRect);
      int h = RectHeight(&mScreenRect);

      // make the canvas
      if (bDoCanvas)
         {
         if (!mpCanvas)
            mpCanvas = gr_alloc_canvas(grd_canvas->bm.type,w,h);
         gr_push_canvas(mpCanvas);
         gr_clear(0);
         }

      // interpolate between left and right edge
      int xval = val*w/max;

      // figure out where the full part of the bar is
      Rect fillme = { 0, 0, w, h };
      if (flags & kRtoL)
         fillme.ul.x = w - xval;
      else
         fillme.lr.x = xval;

      DoDraw(fillme);

      if (flags & kDrawBorder)
      {
         gr_set_fcolor(colors[kBorder]);
         gr_box(0,0,w,h);
      }

      if (bDoCanvas)
         gr_pop_canvas();

      memcpy(&mLastVal,&mVal,sizeof(mLastVal));
      mLastDrawTime = time;
   }

   if (bDoCanvas)
      {
      if (flags & kTransparent)
         mpCanvas->bm.flags |= BMF_TRANS;

      gr_bitmap(&mpCanvas->bm,mScreenRect.ul.x,mScreenRect.ul.y);
      }
}





