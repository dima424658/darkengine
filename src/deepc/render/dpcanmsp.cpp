/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// Animated Sprite Management
//
// @NOTE (justin 7-1-98): This code used to have a cAnimSpriteMan under the
// hood, which kept all of the frame data in a single dynarray. Problem with
// that is that these frames were never being released, so the thing was
// effectively a big memory leak. So I've taken that out for now, and put
// all of the data into the individual sprites.
//
// The only real reason to have a common manager is to promote sharing of
// sprite info, if we were to have lots of cAnimSprites that point to the
// same underlying data. However, this is probably better accomplished by
// creating a proper animated sprite resource type -- duplicate resources
// automatically point to the same data. (And as this stands now, the actual
// graphical frame info is automatically shared by the resource system.)
//

#include <resapi.h>

#include <dpcanmsp.h>

#include <lgassert.h>

#include <dpcutils.h>

// Must be last header
#include <dbmem.h>

const int kAnimSpriteMaxFrames = 20;

cAnimSprite::cAnimSprite(const char *pBaseName, int numFrames, tSimTime frameTime):
   m_frameTime(frameTime),
   m_frame(0),
   m_numFrames(numFrames),
   m_nextFrameTime(GetSimTime()+frameTime)
{
   int i;
   char *pFileName;

   Assert_(m_numFrames<kAnimSpriteMaxFrames);

   m_frameTable.SetSize(m_numFrames);

   pFileName = (char*)malloc(sizeof(char)*strlen(pBaseName)+3);
   for (i=0; i<m_numFrames; i++)
   {
      sprintf(pFileName, "%s%d", pBaseName, i);
      m_frameTable[i] = LoadPCX(pFileName);
   }
}

cAnimSprite::~cAnimSprite()
{
   int i;
   for (i=0; i<m_numFrames; i++)
   {
      SafeFreeHnd(&m_frameTable[i]);
   }
}

void cAnimSprite::SetFrame(void)
{
   // don't try and animate "static" animsprites
   if (m_frameTime <= 0)
      return;

   while (m_nextFrameTime<GetSimTime())
   {
      ++m_frame;
      if (m_frame>=m_numFrames)
         m_frame = 0;
      m_nextFrameTime += m_frameTime;
   }
}

void cAnimSprite::SetFrame(int frame)
{
   if ((m_frame < 0) || (m_frame >= m_numFrames))
   {
      Warning(("cAnimSprite::SetFrame: frame %d is invalid\n",m_frame));
      frame = 0;
   }
   m_frame = frame;
}

void cAnimSprite::Draw(Point point)
{
   SetFrame();
   DrawByHandle(m_frameTable[m_frame], point);
}

// Assumption: no one does a SetFrame() between Lock() and Unlock(); ie,
// the time between Lock() and Unlock() is short. If this assumption fails,
// then we should keep track of the Locked frame...
grs_bitmap* cAnimSprite::Lock(void)
{
   SetFrame();
   return (grs_bitmap*) m_frameTable[m_frame]->Lock();
}

void cAnimSprite::Unlock(void)
{
   m_frameTable[m_frame]->Unlock();
}
