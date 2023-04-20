////////////////////////////////////////////////////////////////////////
//
// (c) 1996 Looking Glass Technologies Inc.
// Pat McElhatton
//
// Module name: Sound resource streamer
// File name: sndrezst.c
//
// Description: Play a sound resource in streaming mode given its rez id
//
////////////////////////////////////////////////////////////////////////
#include <sndrezst.h>
#include <timelog.h>

//
// internal struct created by CreateSoundRezStreamer,
//  automatically destroyed when stream play finishes
//
typedef struct _sndStreamStuff {
   void           *pBuffer;         // stream ring buffer
   uint32         bufferLen;        // ring buffer bytes
   Id             rezId;            // Id of resource to play
   uint32         rezLen;           // bytes in resource
   uint32         playOffset;       // byte offset into resource to get data from
   uint32         endOffset;        // offset into resource of end of data
   SndEndCallback endCB;            // app end-of-sample callback
   void           *pEndCBData;      // app end-of-sample callback data
} sndStreamStuff;


//
// getRezData - function to copy resource data into stream ring buffer
//
static void
getRezData( void     *pCBData,
            void     *pDst,
            uint32   nBytes )
{
   sndStreamStuff *pInfo;

   pInfo = (sndStreamStuff *) pCBData;
   ResExtractPartial( pInfo->rezId, pDst, pInfo->playOffset, nBytes );
}


//
// refillStream - refill stream with data
//
static void
refillStream( ISndSample   *pSample,
              void         *pCBData,
              uint32       bytesNeeded )
{
   sndStreamStuff *pInfo;
   uint32 bytesLeft;

   pInfo = (sndStreamStuff *) pCBData;
   bytesLeft = pInfo->endOffset - pInfo->playOffset;

   TLOG3( "refillStream %d needed, %d left, %d offset", bytesNeeded,
         bytesLeft, pInfo->playOffset);

   if ( bytesNeeded > bytesLeft ) {
      bytesNeeded = bytesLeft;
   }

   ISndSample_LoadBufferIndirect( pSample, getRezData, pInfo, bytesNeeded );
   pInfo->playOffset += bytesNeeded;
   // TBD - will we ever have a long looping resource?
}


//
// endStream - free stuff this stream was using
//
static void
endStream( ISndSample   *pSample,
           void         *pCBData )
{
   sndStreamStuff *pInfo;

mprintf("end stream\n");
   TLOG0( "endStream" );
   pInfo = (sndStreamStuff *) pCBData;

   // first tell the app about samples demise
   if ( pInfo->endCB != NULL ) {
      pInfo->endCB( pSample, pInfo->pEndCBData );
   }

   free( pInfo->pBuffer );
   free( pInfo );

   ISndSample_Release( pSample );
}


////////////////////////////////////
// CreateSoundRezStreamer - create a streaming sound from a sound resource
//
// Takes:
//	       rezId -- Id of resource to play
//        len   -- length of the ring buffer
// Returns:	An ISndSample * that is ready to go.
//
ISndSample *
CreateSoundRezStreamer( ISndMixer        *pMixer,
                        Id               rezId,
                        uint32           bufferLen,
                        SndEndCallback   endCB,
                        void             *pEndCBData )
{
   sSndAttribs attribs;
   uint32 rezLen;
   uint32 nSamples;
   void *pRawData;
   uint32 rawDataLen;
   ISndSample *pSample;
   void *pBuffer;
   sndStreamStuff *pInfo;
   uint32 bytesToRead, bytesToWrite;

   bytesToRead = bufferLen;
   rezLen = ResSize( rezId );
   if ( bytesToRead > rezLen ) {
      // don't try to read more than is in resource
      bytesToRead = rezLen;
   }
   pBuffer = malloc( bufferLen );
   
   LOG3("CreateSoundRezStreamer %ld bytesToRead, %ld rezLen, %ld buffLen", bytesToRead,
        rezLen, bufferLen );

   // Get the rez header, and examine it to find type of sound resource
   // TBD: What if bufferLen is too small to fit entire Rez header in buffer?
   ResExtractPartial( rezId, pBuffer, 0, bytesToRead );
   if ( SndCrackRezHeader( pBuffer, bytesToRead, &pRawData, &rawDataLen,
                           &nSamples, &attribs )  ) {
      free( pBuffer );
      return NULL;
   }

   // create the actual sample...
   // TBD: How to pass offset to raw data area, raw data len to sample::Init
	pSample = ISndMixer_CreateRawSample( pMixer, kSndSampleStream, pBuffer, bufferLen,
                                        0, &attribs );

   if( pSample == NULL ) {
      // release the ring buffer
      free( pBuffer );
   } else {
      // create our stream info struct & init it
      pInfo = (sndStreamStuff *) malloc( sizeof( sndStreamStuff ) );
      pInfo->pBuffer = pBuffer;
      pInfo->bufferLen = bufferLen;
      pInfo->rezId = rezId;
      pInfo->rezLen = rezLen;
      pInfo->playOffset = (long) pRawData - (long) pBuffer;
      pInfo->endOffset = pInfo->playOffset + rawDataLen;

      // now fill the ring buffer with sample data
      bytesToWrite = ISndSample_AvailToWrite( pSample );
      if ( rawDataLen < bytesToWrite ) {
         // copy all data to ring buffer
         refillStream( pSample, pInfo, rawDataLen );
         // and force a silence fill to end of buffer
         refillStream( pSample, pInfo, 0 );
      } else {
         refillStream( pSample, pInfo, bytesToWrite );
      }

      // setup stream-fill and end-of-sample callbacks
      ISndSample_RegisterFillCallback( pSample, refillStream, pInfo );
      pInfo->endCB = endCB;
      pInfo->pEndCBData = pEndCBData;
      ISndSample_RegisterEndCallback( pSample, endStream, pInfo );
   }
	return pSample;
}


#define REZ_HDR_MAX 256
////////////////////////////////////
// CreateSoundRezOneShot - create a one-shot sound from a sound resource
//
// Takes:
//	       rezId -- Id of resource to play
// Returns:	An ISndSample * that is ready to go.
//
// This avoids the extra data copies inherent in ISndMixer_CreateSampleFromRez, as
//  well as doing the resource handling
ISndSample *
CreateSoundRezOneShot( ISndMixer        *pMixer,
                       Id               rezId,
                       SndEndCallback   endCB,
                       void             *pEndCBData )
{
   sSndAttribs attribs;
   uint32 rezLen;
   uint32 nSamples;
   void *pRawData;
   uint32 rawDataLen;
   ISndSample *pSample;
   void *pBuffer;
   sndStreamStuff *pInfo;
   uint32 bytesToRead, bytesToWrite;

   rezLen = ResSize( rezId );
   pBuffer = malloc( rezLen );   // actually, the buffer could be shorter
mprintf("start one-shot\n");
   
   //LOG3("CreateSoundRezStreamer %ld bytesToRead, %ld rezLen, %ld buffLen", bytesToRead,
   //rezLen, bufferLen );

   // Get the rez header, and examine it to find type of sound resource
   // TBD: What if bufferLen is too small to fit entire Rez header in buffer?
   ResExtractPartial( rezId, pBuffer, 0, REZ_HDR_MAX );
   if ( SndCrackRezHeader( pBuffer, REZ_HDR_MAX, &pRawData, &rawDataLen,
                           &nSamples, &attribs )  ) {
      return NULL;
   }

   // create the actual sample...
   // TBD: How to pass offset to raw data area, raw data len to sample::Init
	pSample = ISndMixer_CreateRawSample( pMixer, kSndSampleNormal, pBuffer,
                                        rawDataLen, 0, &attribs );
    if( pSample == NULL ) {
      // release the ring buffer
      free( pBuffer );
   } else {
      // create our stream info struct & init it
      pInfo = (sndStreamStuff *) malloc( sizeof( sndStreamStuff ) );
      pInfo->pBuffer = pBuffer;
      pInfo->playOffset = (long) pRawData - (long) pBuffer;

      // now fill the buffer with sample data
      ISndSample_LoadBufferIndirect( pSample, getRezData, pInfo, rawDataLen );

      // setup end-of-sample callbacks
      pInfo->endCB = endCB;
      pInfo->pEndCBData = pEndCBData;
      ISndSample_RegisterEndCallback( pSample, endStream, pInfo );
   }
	return pSample;
}

#ifndef NOT_YET

//
// possible states of a splicer
//
typedef enum _sndSplicerState {
   kSpliceEmpty,           // no samples left, open next segment
   kSpliceSingle,          // take samples from current segment
   kSpliceDual,            // mix samples from current & next segments
   kSpliceSilence          // silent gap following a segment
} sndSplicerState;


//
// internal version of user sound segment list
//
typedef struct _sndSegInternal {
   Id       rezId;         // resource Id
   uint32   offset;        // in bytes, current offset into rez
   int32    endGap;        // in bytes, silence or overlap at end of segment
   uint32   numBytes;      // bytes before next state change
} sndSegInternal;

//
// internal struct created by CreateSoundRezSplicer,
//  automatically destroyed when stream play finishes
//
typedef struct _sndSplicerStuff {
   sndSplicerState   state;         // splicer state
   void              *pBuffer;      // stream ring buffer
   uint32            bufferLen;     // ring buffer bytes
   void              *pTmpBuffer;   // tmp buffer used while mixing
   uint32            tmpBufferLen;  // tmp buffer bytes
   sndSegInternal    *pSeg;         // sound segment currently playing
   sndSegInternal    *pSeg0;        // ptr to 1st segment
   uint32            segsLeft;      // unused sound segments left in list (not counting current)
   uint32            seg1Offset;    // byte offset into resource to get data from
   uint32            seg2Offset;    // byte offset into 2nd resource to get data from
   SndEndCallback    endCB;         // app end-of-sample callback
   void              *pEndCBData;   // app end-of-sample callback data
   uint32            bitsPerSample; // 8 or 16 bit data?
   uint32            bytesAvail;    // bytes left until next state change
} sndSplicerStuff;


// size of temporary buffer used while splicer is mixing 2 segments
#define TMP_BUFFER_LEN 2048

//
// getSpliceSingleData - function to copy resource data into stream ring buffer
//
static void
getSpliceSingleData( void     *pCBData,
                     void     *pDst,
                     uint32   nBytes )
{
   sndSplicerStuff *pInfo;
   sndSegInternal *pSeg;

   pInfo = (sndSplicerStuff *) pCBData;
   pSeg = pInfo->pSeg;
   ResExtractPartial( pSeg->rezId, pDst, pInfo->seg1Offset, nBytes );
}


//
// getSpliceDualData - function to copy & mix resource data into stream ring buffer
//
static void
getSpliceDualData( void     *pCBData,
                   void     *pDst,
                   uint32   nBytes )
{
   sndSplicerStuff *pInfo;
   sndSegInternal *pSeg;
   sndSegInternal *pSeg2;
   uint8 *pSB, *pDB;  // src & dst, 8 bits
   long a;
   int16 *pSW, *pDW;  // src & dst, 16 bits
   uint32 i;

   pInfo = (sndSplicerStuff *) pCBData;
   pSeg = pInfo->pSeg;
   pSeg2 = pSeg + 1;
   ResExtractPartial( pSeg->rezId, pDst, pInfo->seg1Offset, nBytes );
   ResExtractPartial( pSeg2->rezId, pInfo->pTmpBuffer, pInfo->seg2Offset, nBytes );
   if ( pInfo->bitsPerSample == 8 ) {
      pSB = (uint8 *) pInfo->pTmpBuffer;
      pDB = (uint8 *) pDst;
      for ( i = 0; i < nBytes; i++ ) {
         // the bum who decided to make 8-bit samples unsigned should fry!
         // remove them from the gene pool!

         // the source range is 0...255, silence is 128,
         // but a's range is 0...511, silence is 256        
         a = (*pSB++) + (*pDB);
         a -= 128;                  // get silence to be 128 again
         if ( a < 0 )   a = 0;      // clamp over/underflows
         if ( a > 255 ) a = 255;
         *pDB++ = (uint8) a;
      }
   } else {
      // assume 16-bits per sample
      pSW = (int16 *) pInfo->pTmpBuffer;
      pDW = (int16 *) pDst;
      for ( i = 0; i < nBytes; i += 2 ) {
         a = (*pSW++) + (*pDW);
         if ( a < -32768L ) a = -32768L;     // clamp over/underflow
         if ( a > 32767 )   a = 32767;
         *pDW++ = (int16) a;
      }
   }
}


//TBD
#define EXPLODE LOG0

//
// refillSplicedStream - refill stream with spliced data
//
static void
refillSplicedStream( ISndSample  *pSample,
                     void        *pCBData,
                     uint32      bytesNeeded )
{
   sndSplicerStuff   *pInfo;
   uint32            bytesXfer;
   BOOL              notDone;
   int32             bytesAvail;    // bytes left before next state change
   sndSegInternal    *pSeg;
   int32             endGap;
   int32             oldEndGap;
   sndSplicerState   state;

   pInfo = (sndSplicerStuff *) pCBData;
   pSeg = pInfo->pSeg;
   bytesAvail = pInfo->bytesAvail;
   endGap = pSeg->endGap;
   state = pInfo->state;
      
   LOG3( "refillSplicer %d needed, %d available, %d offset", bytesNeeded,
         bytesAvail, pInfo->seg1Offset);

   notDone = TRUE;
   while( notDone ) {
      switch( state ) {

         case kSpliceEmpty:
            //
            // open the next segment if one is available.  NOTE: pSeg already points to
            // the next segment
            //
            if ( pInfo->segsLeft != 0 ) {
               pInfo->segsLeft--;
               bytesAvail = pSeg->numBytes;
               endGap = pSeg->endGap;
               if ( endGap < 0 ) {
                  // dual splice at end - don't count spliced bytes here
                  bytesAvail += endGap;         // add since gap is negative
                  if ( bytesAvail < 0 ) {
                     EXPLODE("bytesAvail < 0");
                  }
               }
               if ( (endGap > 0) && (bytesAvail == 0) ) {
                  state = kSpliceSilence;
                  LOG3( "splice empty->silence %d segsLeft, %d bytesAvail, %d endGap",
                        pInfo->segsLeft, bytesAvail, endGap );
               } else {
                  state = kSpliceSingle;
                  pInfo->seg1Offset = pSeg->offset;
                  LOG3( "splice empty->single: %d segsLeft, %d bytesAvail, %d endGap",
                        pInfo->segsLeft, bytesAvail, endGap );
               }
            } else {
               notDone = FALSE;
               LOG0("splice empty - no segments left");
               // feed no-data to stream to start stream close-down process
               ISndSample_LoadBufferIndirect( pSample, getSpliceSingleData, pInfo, 0 );
            }
            break;

         case kSpliceSingle:
            //
            // just get bytes from a sound rez and copy them to ring buffer
            //
            if ( bytesNeeded < bytesAvail ) {
               bytesXfer = bytesNeeded;
            } else {
               bytesXfer = bytesAvail;
            }
            // xfer bytesXfer from segPtr->rezId starting at seg1Offset
            ISndSample_LoadBufferIndirect( pSample, getSpliceSingleData, pInfo, bytesXfer );
            
            bytesAvail -= bytesXfer;
            bytesNeeded -= bytesXfer;
            pInfo->seg1Offset += bytesXfer;
            if ( bytesAvail == 0 ) {
               // need to switch state, either to dual splice or empty
               if ( endGap == 0 ) {
                  state = kSpliceEmpty;
                  LOG3( "splice single->empty %d segsLeft, %d bytesAvail, %d endGap",
                        pInfo->segsLeft, bytesAvail, endGap );
                  pSeg++;
               } else if ( endGap < 0 ) {
                  state = kSpliceDual;
                  bytesAvail = (-endGap);
                  pInfo->seg2Offset = pSeg[1].offset;
                  LOG3( "splice single->dual %d segsLeft, %d bytesAvail, %d endGap",
                        pInfo->segsLeft, bytesAvail, endGap );
               } else {
                  state = kSpliceSilence;
                  bytesAvail = endGap;
                  LOG3( "splice single->silence %d segsLeft, %d bytesAvail, %d endGap",
                        pInfo->segsLeft, bytesAvail, endGap );
               }
            }
            break;

         case kSpliceDual:
            //
            // get data from 2 sound resources, mix them and write them to ring buffer
            //
            if ( bytesNeeded < bytesAvail ) {
               bytesXfer = bytesNeeded;
            } else {
               bytesXfer = bytesAvail;
            }
            if ( bytesXfer > TMP_BUFFER_LEN ) {
               bytesXfer = TMP_BUFFER_LEN;
            }
            // get data from both resources, mix & xfer to stream ring buffer
            ISndSample_LoadBufferIndirect( pSample, getSpliceDualData, pInfo, bytesXfer );
            
            bytesAvail -= bytesXfer;
            bytesNeeded -= bytesXfer;
            pInfo->seg1Offset += bytesXfer;
            pInfo->seg2Offset += bytesXfer;
            if ( bytesAvail == 0 ) {
               // need to switch state to single splice
               state = kSpliceSingle;
               pInfo->seg1Offset = pInfo->seg2Offset;
               pInfo->segsLeft--;
               oldEndGap = endGap;
               pSeg++;
               endGap = pSeg->endGap;
               if ( endGap < 0 ) {
                  // segment just finished & next segment both have negative endGaps,
                  //   so both endGaps decrease number of bytes in single splice
                  bytesAvail = pSeg->numBytes + oldEndGap + endGap;
                  if ( bytesAvail <= 0 ) {
                     EXPLODE( "0 bytes left after splice!" );
                  }
               } else {
                  // decrease bytes available by size of splice just completed
                  bytesAvail = pSeg->numBytes + oldEndGap;
               }
               LOG3( "splice dual->single %d segsLeft, %d bytesAvail, %d endGap",
                     pInfo->segsLeft, bytesAvail, endGap );
            }
            break;

         case kSpliceSilence:
            //
            // add silence to ring buffer
            //
            if ( bytesNeeded < bytesAvail ) {
               bytesXfer = bytesNeeded;
            } else {
               bytesXfer = bytesAvail;
               state = kSpliceEmpty;
               pSeg++;
               LOG3( "splice silence->empty %d segsLeft, %d bytesAvail, %d endGap",
                     pInfo->segsLeft, bytesAvail, endGap );
            }
            // fill portion of ring buffer with silence
            ISndSample_SilenceFill( pSample, bytesXfer );
            bytesAvail -= bytesXfer;
            bytesNeeded -= bytesXfer;
            break;
      } // end switch ( state )
      // update seg ptr in info struct, since that is where splicing functions called
      //  by LoadBufferIndirect get seg ptr from
      pInfo->pSeg = pSeg;

      if ( bytesNeeded == 0 ) {
         notDone = FALSE;
      }
   }  // end while notDone

   // NOTE: pSeg may be pointing past the last real segment (when finished entire splice)

   pInfo->bytesAvail = bytesAvail;
   pInfo->state = state;

   // TBD - will we ever have a long looping resource?
}


//
// endSplicedStream - free stuff this stream was using
//
static void
endSplicedStream( ISndSample  *pSample,
                  void        *pCBData )
{
   sndSplicerStuff *pInfo;

   LOG0( "endStream" );
   pInfo = (sndSplicerStuff *) pCBData;

   // first tell the app about samples demise
   if ( pInfo->endCB != NULL ) {
      pInfo->endCB( pSample, pInfo->pEndCBData );
   }

   free( pInfo->pBuffer );
   free( pInfo->pTmpBuffer );
   free( pInfo->pSeg0 );
   free( pInfo );

   ISndSample_DumpData( pSample, FALSE );
   ISndSample_Release( pSample );
}


////////////////////////////////////
// CreateSoundRezSplicer - create a streaming sound from a list of sound resources
//
// Takes:
//	       rezId -- Id of resource to play
//        len   -- length of the ring buffer
// Returns:	An ISndSample * that is ready to go.
// NOTE:
//  Does not handle stereo, or compressed .WAV files
//
ISndSample *
CreateSoundRezSplicer( ISndMixer       *pMixer,
                       SndSegment      *pSegList,
                       uint32          numSegs,
                       uint32          bufferLen,
                       SndEndCallback  endCB,
                       void            *pEndCBData )
{
   sSndAttribs       attribs;
   Id                rezId;
   uint32            rezLen;
   uint32            nSamples;
   void              *pRawData;
   uint32            rawDataLen;
   ISndSample        *pSample;
   void              *pBuffer;
   sndSplicerStuff   *pInfo;
   uint32            bytesToRead;
   sndSegInternal    *pSeg;
   BOOL              blewIt;
   float             bytesPerMilliSecond;
   uint32            i;

   LOG2("CreateSoundRezSplicer %ld numRez, %ld buffLen",
        numSegs, bufferLen );

   if ( numSegs < 1 ) {
      return NULL;
   }

   pBuffer = malloc( bufferLen + 16 );
   pSeg = (sndSegInternal *) malloc( sizeof(sndSegInternal) * (numSegs));
   pInfo = (sndSplicerStuff *) malloc( sizeof( sndSplicerStuff ) );
   pInfo->pSeg0 = pSeg;

   for ( i = 0; i < numSegs; i++ ) {

      //
      // build internal version of segment list - this requires opening each segment and
      //   cracking its sound resource header
      //
      
      bytesToRead = 256;
      rezId = pSegList->rezId;
      rezLen = ResSize( rezId );
      if ( bytesToRead > rezLen ) {
         // don't try to read more than is in resource
         bytesToRead = rezLen;
      }

      // Get the rez header, and examine it to find type of sound resource
      // TBD: What if bufferLen is too small to fit entire Rez header in buffer?
      ResExtractPartial( rezId, pBuffer, 0, bytesToRead );
      blewIt = SndCrackRezHeader( pBuffer, bytesToRead, &pRawData, &rawDataLen,
                                  &nSamples, &attribs );

      if ( (i == 0) && !blewIt ) {
         // create the actual stream after creating 1st segment
         pSample = ISndMixer_CreateRawSample( pMixer, kSndSampleStream, pBuffer, bufferLen,
                                              0, &attribs );
         if( pSample == NULL ) {
            blewIt = TRUE;
         } else {
            bytesPerMilliSecond = (float) (attribs.sampleRate * (attribs.bitsPerSample >> 3))
               / 1000.0;
         }
      }

      if ( blewIt ) {
         free( pBuffer );
         free( pInfo->pSeg0 );
         free( pInfo );
         return NULL;
      }

      // fill in internal segment info
      pSeg->rezId = rezId;
      pSeg->offset = (long) pRawData - (long) pBuffer;
      pSeg->endGap = bytesPerMilliSecond * pSegList->offset;
      if ( attribs.bitsPerSample == 16 ) {
         // force gap length to be integral # of 16-bit samples
         pSeg->endGap &= ~1;
      }
      pSeg->numBytes = rawDataLen;
      LOG3("segment offset %ld, numBytes %ld, endGap %ld", pSeg->offset,
           pSeg->numBytes, pSeg->endGap );
      pSeg++;
      pSegList++;

   } // end for i == 0...numSegs

   // prevent last end gap from being negative (no following seg to overlap)
   pSeg--;
   if ( pSeg->endGap < 0 ) {
      pSeg->endGap = 0;
   }

   // init splicer info struct
   pInfo->state = kSpliceEmpty;
   pInfo->pBuffer = pBuffer;
   pInfo->bufferLen = bufferLen;
   pInfo->pTmpBuffer = malloc( TMP_BUFFER_LEN );
   pInfo->tmpBufferLen = TMP_BUFFER_LEN;
   pInfo->pSeg = pInfo->pSeg0;
   pInfo->segsLeft = numSegs;
   pInfo->bitsPerSample = attribs.bitsPerSample;

   // setup stream-fill and end-of-sample callbacks
   ISndSample_RegisterFillCallback( pSample, refillSplicedStream, pInfo );
   pInfo->endCB = endCB;
   pInfo->pEndCBData = pEndCBData;
   ISndSample_RegisterEndCallback( pSample, endSplicedStream, pInfo );

   // setup data dumping
   ISndSample_DumpData( pSample, TRUE );

   // now fill the ring buffer with sample data
   refillSplicedStream( pSample, pInfo, bufferLen - 4 );
   // TBD: this doesn't handle the case where the total spliced sample is shorter than
   //    the ring buffer...

	return pSample;
}

#endif
