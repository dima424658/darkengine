/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCANMSP_H
#define __DPCANMSP_H

#ifndef __DYNARRAY_H
#include <dynarray.h>
#endif // !__DYNARRAY_H

#ifndef HASHPP_H
#include <hashpp.h>
#endif // !HASHPP_H

#ifndef HSHPPTEM_H
#include <hshpptem.h>
#endif // !HSHPPTEM_H

#ifndef RECT_H
#include <rect.h>
#endif // RECT_H

#ifndef __DEV2D_H
#include <dev2d.h>
#endif // !__DEV2D_H

#ifndef __RESAPI_H
#include <resapi.h>
#endif // !__RESAPI_H

#ifndef __SIMTIME_H
#include <simtime.h>
#endif // !__SIMTIME_H

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

#endif  // !__DPCANMSP_H
