/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/ui/uianim.h,v 1.2 1998/11/04 22:39:17 jaemz Exp $

// this is for little anims on the screen that
// get updated every so often

#ifndef _UIANIM_H
#define _UIANIM_H
#pragma once

#include <dynarray.h>
#include <looptype.h>
#include <resapi.h>
#include <resistr.h>


// Little screen animations
// is derived from one string file named <panel_name>m.str
// Here's a sample file:
/*
list: "frank beans sppooo"
frank: "17,69 0.1 r"
beans: "60,151 2.0 r"
sppooo: "491,174 6.0 l"
*/
// The format is x,y fps [r|l]
// r means reverse, l means loop
// the files are frank_1.tga, frank_2.tga etc...
// in the same directory

// I suggest making the framerates all different and not multiples
// of each other so you get the cpu time smoothed out.

// Pass in as a flag to not lock all the bitmaps up front
// I suggest leaving it at the default though, since you can get
// stuttering as the memory sorts itself out.
// 
const int kAnimNoLock=0;
const int kAnimLock=1;

struct sUiAnimElem;


class cUiAnim {
public:
   cUiAnim(const char *panel_name,const char *res_path,int flags=kAnimLock) :
      mPanelName(panel_name),mUiAnimResPath(res_path),mFlags(flags) {}
protected:
   // You can basically use this all by itself
   // This does all the real work.
   void InitUI();
   void TermUI();
   void OnLoopMsg(eLoopMessage msg, tLoopMessageData data);
private:
   const char *mPanelName;
   const char *mUiAnimResPath;
   long mLastClock;
   const int mFlags;
   // Dynamic array of pointers to the anims
   cDynArray<sUiAnimElem *> mAnims;
};

// This is private to the class above
struct sUiAnimElem
{
   // Contains array of the resource pointer
   cDynArray<IRes *> mFrames;
   int mX,mY;     // offset to render at on screen
   float mSpf;   // frame rate at which to render this anim
   int mFrame;   // which frame we are on
   float mDt;    // running delta to determine when to change frames
   char mType;   // whether to loop "l" or reverse "r", def loop

   // Construct from string resource and name of bitmap
   sUiAnimElem(IStringRes *pStr,const char *name,const char *res_path,int flags);
   ~sUiAnimElem();
   // update the frame, shows next frame if must be
   void Update(float dt);
private:
   const int mFlags;
   // Blit the current frame to the current canvas
   void mBlit();
};

#endif






