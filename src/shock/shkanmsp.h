/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkanmsp.h,v 1.4 2000/01/29 13:40:27 adurant Exp $
#pragma once

#ifndef __SHKANMSP_H
#define __SHKANMSP_H

#include <dynarray.h>
#include <hashpp.h>
#include <hshpptem.h>
#include <rect.h>

#include <dev2d.h>

#include <resapi.h>
#include <simtime.h>

class cAnimSprite
{
public:
   cAnimSprite(const char *pBaseName, int numFrames, tSimTime frameTime);
   ~cAnimSprite();

   void Draw(Point pt);
   grs_bitmap *Lock(void);
   void Unlock(void);
   void SetFrame(int frame);
   void SetFrame(void);

private:
   int m_frame;
   int m_numFrames;
   tSimTime m_frameTime;
   tSimTime m_nextFrameTime;
   cDynArray<IRes *> m_frameTable;
};

#endif
