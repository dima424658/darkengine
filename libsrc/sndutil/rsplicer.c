////////////////////////////////////////////////////////////////////////
//
// (c) 1996 Looking Glass Technologies Inc.
// Pat McElhatton
//
// Module name: Sound resource splicer
// File name: rsplicer.c
//
// Description: Play a sequence of sound resources in streaming mode
//   with specified gaps/overlaps
//
////////////////////////////////////////////////////////////////////////
#include <sndutil.h>
#include <splicer.h>
#include <timelog.h>
#include <mprintf.h>
#include <memall.h>


//
// get data from a resource
//
static void
*readRezData( void            *pCBData,
              void            *pDst,
              uint32          nBytes,
              uint32          offset )
{
   sndSegInternal  *pSeg;

   //mprintf( "read %ld bytes @ %ld\n", nBytes, offset );
   if ( nBytes != 0 ) {
      pSeg = (sndSegInternal *) pCBData;
      ResExtractPartial( pSeg->extras[0], pDst, offset, nBytes );
   }

   return pDst;
}



////////////////////////////////////
// CreateSoundRezSplicer - create a streaming sound from a list of sound resources
//
// Takes:
//    pMixer      audio mixer to play sounds on
//    pSegList    list of sound resources & offsets
//	   numSegs     number of elements in pSegList
//    pBuffer     pointer to user-allocated ring buffer (optional, pass NULL
//                  and this routine will handle ring buffer alloc/dealloc
//    bufferLen   size in bytes of ring buffer
//    endCB       app callback, invoked when sound is done playing
//    pEndCBData  callback data for endCB
//
// Returns:	An ISndSample * that is ready to go.
// NOTE:
//  Does not handle stereo, or compressed .WAV files
//
ISndSample *
CreateSoundRezSplicer( ISndMixer       *pMixer,
                       SndRezSegment   *pSegList,
                       uint32          numSegs,
                       void            *pBuffer,
                       uint32          bufferLen,
                       SndEndCallback  endCB,
                       void            *pEndCBData )
{
   ISndSample           *pSample;
   sndSegInternal       *pSeg;
   sndSegInternal       *pSeg0;
   uint32               i;

   TLOG2("CreateSoundRezSplicer %ld numRez, %ld buffLen",
        numSegs, bufferLen );

   if ( numSegs < 1 ) {
      return NULL;
   }

   pSeg0 = (sndSegInternal *) Malloc( sizeof(sndSegInternal) * (numSegs));
   pSeg = pSeg0;
   for ( i = 0; i < numSegs; i++ ) {

      //
      // build internal version of segment list
      // just fill in the offset, numBytes & rezId fields
      //

      pSeg->offset = pSegList->offset;

      pSeg->extras[0] = pSegList->rezId;
      pSeg->numBytes = ResSize( pSegList->rezId );

      //mprintf("opened rez %d, %ld bytes\n", pSegList->rezId, pSeg->numBytes );
      pSeg++;
      pSegList++;

   } // end for i == 0...numSegs

   pSample = CreateSoundSplicer( pMixer, readRezData, pSeg0, numSegs, pBuffer,
                                 bufferLen, endCB, pEndCBData );
	return pSample;
}


