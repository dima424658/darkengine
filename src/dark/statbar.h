/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/statbar.h,v 1.3 1999/10/11 14:21:46 BFarquha Exp $
#pragma once
#ifndef __STATBAR_H
#define __STATBAR_H

#include <rect.h>

//------------------------------------------------------------
// GOOFY STAT BAR DISPLAY ABSTRACTION
//
// This thing displays a single value as a "bar graph"
//

struct sStatusBarDesc
{
   enum eColor { kFG, kBG, kBorder, kNumColors };
   enum eFlags
   {
      kTransparent   = 1 << 0,
      kDrawBorder    = 1 << 1,
      kRtoL          = 1 << 2, // right to left
      kHideAtMax     = 1 << 3,
   };

   int flags;
   int colors[kNumColors];
   int tolerance;  // How much change you should ignore.
   int refresh;   // ms after which you should redraw anyway
};


class cStatusBar : public sStatusBarDesc
{
public:

   cStatusBar(const sStatusBarDesc& desc);
   enum eValue { kCur, kMax, kNumValues };
   void SetVal(eValue which, int val);

   void Reset(); // invalidate state
   void Update(ulong curtime);

   // set the screen rect
   void SetScreenRect(const Rect& rect);
   // not valid until after "Update"
   const Rect& GetScreenRect();

protected:
   // canvas is pushed, draw within fillme
   virtual void DoDraw(const Rect& fillme);

   // return TRUE if we want canvas created before draw.
   virtual BOOL MakeDrawCanvas();

   int mVal[kNumValues];
   int mLastVal[kNumValues];
   ulong mLastDrawTime;
   Rect mScreenRect;
   struct grs_canvas* mpCanvas;
};




#endif // __STATBAR_H
