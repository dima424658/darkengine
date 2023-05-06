////////////////////////////////////////////////////////////////////////
//
// (c) 1996 Looking Glass Technologies Inc.
// Pat McElhatton
//
// Module name: Sound file splicer
// File name: fsplicer.c
//
// Description: Play a sequence of sound files in streaming mode
//   with specified gaps/overlaps
//
////////////////////////////////////////////////////////////////////////

#include <sndutil.h>
#include <splicer.h>
#include <timelog.h>
#include <mprintf.h>
#include <memall.h>
#include <dbg.h>

//
// get data from a file
//
static void
*readFileData( void            *pCBData,
               void            *pDst,
               uint32          nBytes,
               uint32          offset )
{
   sndSegInternal  *pSeg;
   FILE *inFile;
   uint32   nRead;
   //mprintf( "read %ld bytes @ %ld\n", nBytes, offset );
   pSeg = (sndSegInternal *) pCBData;
   inFile = (FILE *) pSeg->extras[0];

   if ( nBytes == 0 ) {
      // this segment is no longer needed
      if ( !pSeg->loopMe ) {
         fclose( inFile );
      }
   } else {
      fseek( inFile, offset, SEEK_SET );
      nRead = fread( pDst, 1, nBytes, inFile );
#ifdef DBG
      if ( nRead != nBytes ) {
         Warning( ("fread returned %ld bytes, wanted %ld bytes\n",
                   nRead, nBytes) );
      }
#endif
   }

   return pDst;
}



////////////////////////////////////
// CreateSoundFileSplicer - create a streaming sound from a list of sound files
//
// Takes:
//    pMixer      audio mixer to play sounds on
//    pSegList    list of sound files & offsets
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
CreateSoundFileSplicer( ISndMixer       *pMixer,
                        SndFileSegment  *pSegList,
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
   char                 fname[256];
   FILE                 *inFile;

   TLOG2("CreateSoundFileSplicer %ld numRez, %ld buffLen",
        numSegs, bufferLen );

   if ( numSegs < 1 ) {
      return NULL;
   }

   pSeg0 = (sndSegInternal *) Malloc( sizeof(sndSegInternal) * (numSegs));
   pSeg = pSeg0;
   for ( i = 0; i < numSegs; i++ ) {

      //
      // build internal version of segment list
      // just fill in the offset & numBytes fields & open file
      //
      pSeg->offset = pSegList->offset;

      strcpy( fname, pSegList->fileName );
      // if filename has no extension, give it a .wav extension
      if ( strchr( fname, '.' ) == NULL ) {
         // append ".wav" to end of filename
         strcat( fname, ".wav" );
      }
      inFile = fopen( fname, "rb" );
      pSeg->extras[0] = (uint32) inFile;
      if ( inFile == NULL ) {
         Warning( ("couldn't open %s\n", fname) );
         Free( pSeg0 );
         return NULL;
      } else {
         fseek( inFile, 0L, SEEK_END );
         pSeg->numBytes = ftell( inFile );
         fseek( inFile, 0L, SEEK_CUR );
      }

      pSeg++;
      pSegList++;

   } // end for i == 0...numSegs

   pSample = CreateSoundSplicer( pMixer, readFileData, pSeg0, numSegs, pBuffer,
                                 bufferLen, endCB, pEndCBData );
	return pSample;
}

