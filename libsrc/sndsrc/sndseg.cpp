////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/sndsrc/RCS/sndseg.cpp $
// $Author: PATMAC $
// $Date: 1997/10/01 16:39:25 $
// $Revision: 1.5 $
//
// (c) 1997 Looking Glass Technologies Inc.
// Pat McElhatton
//
// Module name: sound segment
// File name: sndseg.cpp
//
// Description: implementation of sound segment objects
//
////////////////////////////////////////////////////////////////////////

#include <lg.h>
#include <sndseg.h>

// TBD: stereo ADPCM
// TBD: stereo with frequency doubling

// !!! must decide what sample positions mean when frequency
// !!! doubling is in effect


uint32 cSndSegment::mSegsMade = 0;
uint32 cSndSegment::mSegsDestroyed = 0;

EXTERN char *
DecompressIMABlockPartial( char           *pInBuff,
                           short          *pOutBuff,
                           long           nSamples,
                           BOOL           getHeader,
                           unsigned long  *pState );


cSndSegment::cSndSegment( void )
{
   mInited = FALSE;
   mpInBuffer = NULL;
   mpOutBuffer = NULL;
   mSegsMade++;
}


cSndSegment::~cSndSegment( void )
{
   if ( mpInBuffer != NULL ) {
      delete mpInBuffer;
   }
   if ( mpOutBuffer != NULL ) {
      delete mpOutBuffer;
   }
   mSegsDestroyed++;
}


void
cSndSegment::InitBase( uint32       offset,
                       sSndAttribs  *pAttribs,
                       BOOL         doDouble )
{
   mStartOffset = offset;
   mSrcOffset = offset;
   mSrcPos = 0;
   mNeedsSeek = TRUE;
   mNewPos = 0;
   mInited = TRUE;
   mAttribs = *pAttribs;
   mDoDoubling = doDouble;
 
   switch( mAttribs.dataType ) {
      case kSndDataPCM:
         mBytesPerSample = (mAttribs.bitsPerSample * mAttribs.nChannels)/ 8;
         break;
      case kSndDataIMAADPCM:
         mBytesPerSample = 2 * mAttribs.nChannels;
         break;
   }

   mBytesLeft = mAttribs.numSamples * mBytesPerSample;

   //
   // create temp input buffer for compressed datatypes
   //
   if ( mAttribs.dataType == kSndDataIMAADPCM ) {
      mpInBuffer = new char[mAttribs.bytesPerBlock];
   }

   //
   // create temp output buffer if frequency doubling is needed
   //
   if ( doDouble ) {
      mBytesLeft <<= 1;
      mpOutBuffer = new char[mBytesLeft];
   }
}

//
// reset # of bytes left in segment when a segment is to be used by
//  two consecutive playlist ops
//
void
cSndSegment::ReInit( uint32   nSamples )
{
   mAttribs.numSamples = nSamples;
   mBytesLeft = mAttribs.numSamples * mBytesPerSample;
   if ( mDoDoubling ) {
      mBytesLeft <<= 1;
   }
}

// ARGH! - when frequency doubling, are SetPosition positions in
// terms of the doubled or original sample numbers?
void
cSndSegment::SetPosition( uint32 newPos )
{
   if ( mDoDoubling ) {
      // force positions to even boundaries when doubling samples
      mNewPos = newPos / 2;
   } else {
      mNewPos = newPos;
   }
   mNeedsSeek = TRUE;
   mBytesLeft = (mAttribs.numSamples - newPos) * mBytesPerSample;
}


//
// how many bytes of output are left in this segment
//
uint32
cSndSegment::BytesLeft( void )
{
   return mBytesLeft;
}


//
// position in source
//
uint32
cSndSegment::GetPosition( void )
{
   return mSrcPos;
}



// NOTE: This only works (currently) for mono streams!
//
// This routine creates a frequency-doubled copy of its source samples.
// The doubling is done by inserting a sample in between the original
// samples which is the average of its neighbor samples.
// Returns the last original sample value, which must be passed back in
// the next time this routine is called
//
static int
doubleSamples( void     *pSrc,
               void     *pDst,
               uint32   nSamples,
               int      bytesPerSample,
               int      oldValue ) 
{
   int i, value;
   short *pSrcW, *pDstW;
   unsigned char  *pSrcB, *pDstB;

   if ( bytesPerSample == 2 ) {
      pSrcW = (short *) pSrc;
      pDstW = (short *) pDst;
      for ( i = 0; i < nSamples; i++ ) {
         value = *pSrcW++;
         *pDstW++ = (short) ((value + oldValue) / 2);
         *pDstW++ = (short) value;
         oldValue = value;
      }
   } else {
      pSrcB = (unsigned char *) pSrc;
      pDstB = (unsigned char *) pDst;
      for ( i = 0; i < nSamples; i++ ) {
         value = (*pSrcB++) - 128;
         *pDstB++ = (unsigned char) ( ((value + oldValue) / 2) + 128 );
         *pDstB++ = (unsigned char) (value + 128);
         oldValue = value;
      }
   }

   return oldValue;
}

// 2 broad ways to handle decompression:
// 1. Always decompress an entire block to a temporary buffer, then
//    spool data out of that buffer.  tmp buffer is maintained betweeen
//    GetData calls, but buffer for input block is only needed during
//    block decompression.
// 2. Only decompress part of buffer which is needed at the moment
//    directly into the destination buffer.  Input block is stored
//    in tmp buffer and must be kept between calls to GetData.
//    Decompressor must be restartable in the middle of a block.
//
// Method 2 seems to be the way to go.  One potential problem with
// it occurs during seeking - when discarding the unneeded samples
// between the nearest block boundary and the seek position, you
// still need somewhere to temporarily write those samples to.  The
// obvious answer would be to write them to the actual destination
// buffer, then overwrite them with the samples from the seek point
// on, but what if the number of samples to skip is larger than the
// number of samples available in the destination buffer?
//
// ARGH! Do we have to worry about odd number of samples when
//  frequency doubling is being done?  Level above LoadBufferIndirect
//  could force sample count to be even when doubling is on, but
//  can we be sure that there is never going to be an odd sample
//  count because of DirectSound buffer wraparound?

void *
cSndSegment::GetData( void    *pDst,
                      uint32  nBytes )
{
   void *pActualDst;
   uint32 skipNSamples = 0;
   uint32 nSamples, nSamps, nSamplesOutNeeded;
   BOOL getHeader;
   short *pOut;
   int blockNum, blockPos;

   nSamples = nBytes / mBytesPerSample;
   if ( mDoDoubling ) {
      // must be called with an even # of samples if doubling
      assert( (nSamples & 1) == 0 );
   }

   // argh - handle block-oriented formats
   // call GetRawData to access media source
   // do ADPCM decompress if needed
   // do frequency doubling if needed

   if ( mAttribs.dataType == kSndDataPCM ) {


      ///////////////////////////////////////////////////
      //
      //                   PCM
      //
      ///////////////////////////////////////////////////
      if ( mNeedsSeek ) {
         // set mSrcOffset to correspond to sample position mNewPos
         mSrcOffset = mStartOffset + (mNewPos * mBytesPerSample);
         mOldValue = 0;
      }
      if ( mDoDoubling ) {
         //TBD!
         pActualDst = GetRawData( mpOutBuffer, nBytes >> 1 );
         mOldValue = doubleSamples( pActualDst, pDst, nSamples,
                                    mAttribs.bitsPerSample, mOldValue );
         pActualDst = pDst;
      } else {
         pActualDst = GetRawData( pDst, nBytes );
      }
      mSrcOffset += nBytes;


   } else if ( mAttribs.dataType == kSndDataIMAADPCM ) {


      ///////////////////////////////////////////////////
      //
      //               IMA ADPCM
      //
      ///////////////////////////////////////////////////

      // handle seeking
      if ( mNeedsSeek ) {
         // see how many samples to skip after block start to get
         //   to new position
         blockNum = mNewPos / mAttribs.samplesPerBlock;
         blockPos = blockNum * mAttribs.samplesPerBlock;
         skipNSamples = mNewPos - blockPos;
         // convert from sample position to byte position in input stream
         mSrcOffset = mStartOffset + (blockNum * mAttribs.bytesPerBlock);
         mSrcPos = mNewPos;
         mInSamplesLeft = 0;
         mOldValue = 0;
      }     // end if mNeedsSeek

      pActualDst = pDst;
      if ( mDoDoubling ) {
         pOut = (short *) mpOutBuffer;     // decompress to tmp output buffer
      } else {
         pOut = (short *) pDst;         // decompress directly to app output buffer
      }

      nSamplesOutNeeded = nBytes >> 1;
      while ( nSamplesOutNeeded > 0 ) {

         if ( mInSamplesLeft == 0 ) {
            //
            // current block is empty, get new block
            //
            mInSamplesLeft = mAttribs.samplesPerBlock - skipNSamples;
            mpIn = (char *) GetRawData( mpInBuffer, mAttribs.bytesPerBlock );
            if ( skipNSamples != 0 ) {
               mpIn = DecompressIMABlockPartial( mpIn, pOut, skipNSamples, TRUE,
                                                 &mDecompState );
               skipNSamples = 0;
               getHeader = FALSE;
            } else {
               getHeader = TRUE;
            }
            mSrcOffset += mAttribs.bytesPerBlock;
         } else {
            getHeader = FALSE;
         }

         //
         // decompress a block
         //
         if ( mDoDoubling ) {
            nSamps = ((nSamplesOutNeeded>>1) < mInSamplesLeft)
               ? (nSamplesOutNeeded>>1) : mInSamplesLeft;
         } else {
            nSamps = (nSamplesOutNeeded < mInSamplesLeft) ? nSamplesOutNeeded : mInSamplesLeft;
         }
         mpIn = DecompressIMABlockPartial( mpIn, pOut, nSamps, getHeader,
                                          &mDecompState );
         mInSamplesLeft -= nSamps;

         if ( mDoDoubling ) {
            //
            // do frequency doubling
            //
            mOldValue = doubleSamples( pOut, pDst, nSamps,
                                       mBytesPerSample, mOldValue );

            pDst = (void *) ( ((char *) pDst) + (nSamps << 2) );
            nSamplesOutNeeded -= (nSamps << 1);
         } else {
            pOut += nSamps;
            nSamplesOutNeeded -= nSamps;
         }


      } // end while nSamplesOutNeeded > 0

   }
   assert( mBytesLeft >= nBytes );
   mBytesLeft -= nBytes;
   mNeedsSeek = FALSE;

   return pActualDst;
}

