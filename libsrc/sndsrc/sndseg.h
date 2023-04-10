////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/sndsrc/RCS/sndseg.h $
// $Author: PATMAC $
// $Date: 1997/09/24 14:18:01 $
// $Revision: 1.3 $
//
// (c) 1997 Looking Glass Technologies Inc.
// Pat McElhatton
//
// Module name: sound segment
// File name: sndseg.h
//
// Description: definition of sound segment objects
//
////////////////////////////////////////////////////////////////////////

#ifndef _SNDSEG_H
#define _SNDSEG_H 1

#ifndef _LG_SOUND_H
#include <lgsound.h>
#endif

class cSndSource;

class cSndSegment
{
public:

   cSndSegment( void );

   virtual ~cSndSegment( void );

   virtual void *GetData( void *pDst, uint32 nBytes );

   virtual void *GetRawData( void *pDst, uint32 nBytes ) = 0;

   // set sample position, relative to startPos
   virtual void SetPosition( uint32 nSamples );

   // initialize common base class members
   virtual void InitBase( uint32 offset, sSndAttribs *pAttribs, BOOL doDouble );

   virtual void ReInit( uint32 nSamples );

   virtual uint32 GetPosition( void );

   virtual uint32 BytesLeft();


private:
   static uint32  mSegsMade;
   static uint32  mSegsDestroyed;

protected:

   int            mDoDoubling;      // do frequency doubling
   sSndAttribs    mAttribs;         // our attribs (including numSamples)

   uint32         mStartOffset;     // byte offset into source media of sample 0
   uint32         mSrcOffset;       // current byte offset into source
   uint32         mSrcPos;          // sample position in src
   BOOL           mNeedsSeek;       // TRUE if a seek occured since last GetData call
   uint32         mNewPos;          // sample position to seek to if mNeedsSeek is TRUE
                                    // NOTE: this is NOT doubled

   BOOL           mNeedsBuffer;     // TRUE if data out isn't exactly same as data in
   char           *mpInBuffer;      // conversion buffer, if needed
   char           *mpOutBuffer;     // frequency doubling buffer, if needed
   char           *mpIn;            // current position in mpInBuffer
   uint32         mInSamplesLeft;   // bytes left in mpInBuffer

   uint32         mBytesLeft;       // # of output bytes this segment can still produce

   int            mOldValue;        // last sample value returned by previous GetData call
   BOOL           mInited;          // TRUE iff Init has been done
   int            mBytesPerSample;  // size of output samples

   uint32         mDecompState;     // IMA ADPCM decompressor state
};

#endif
