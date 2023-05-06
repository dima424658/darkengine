////////////////////////////////////////////////////////////////////////
// $Header: x:/prj/tech/libsrc/sndutil/RCS/splicer.c 1.12 1997/06/19 14:32:39 PATMAC Exp $
//
// (c) 1996 Looking Glass Technologies Inc.
// Pat McElhatton
//
// Module name: Sound resource splicer common code
// File name: splicer.c
//
// Description: Play a sequence of sounds in streaming mode
//   with specified gaps/overlaps
//
////////////////////////////////////////////////////////////////////////
#include <sndutil.h>
#include <splicer.h>
#include <timelog.h>
#include <mprintf.h>
#include <memall.h>

//
// getSpliceSingleData - function to copy resource data into stream ring buffer
//
static void
*getSpliceSingleData( void     *pCBData,
                      void     *pDst,
                      uint32   nBytes )
{
   sndSplicerStuff *pInfo;
   sndSegInternal *pSeg;
   void *pData;

   //mprintf( "read single %d bytes\n", nBytes );
   pInfo = (sndSplicerStuff *) pCBData;
   pSeg = pInfo->pSeg;
   pData = pInfo->getData( pSeg, pDst, nBytes, pInfo->seg1Offset );
   pInfo->seg1Offset += nBytes;

   return pData;
}


//
// getSpliceDualData - function to copy & mix resource data into stream ring buffer
//
static void
*getSpliceDualData( void     *pCBData,
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
   void *pSData, *pDData;

   //mprintf( "read dual %d bytes\n", nBytes );
   pInfo = (sndSplicerStuff *) pCBData;
   pSeg = pInfo->pSeg;
   pSeg2 = pSeg + 1;
   pSData = pInfo->getData( pSeg, pDst, nBytes, pInfo->seg1Offset );
   pDData = pInfo->getData( pSeg2, pInfo->pTmpBuffer, nBytes, pInfo->seg2Offset );
   pInfo->seg1Offset += nBytes;
   pInfo->seg2Offset += nBytes;
   if ( pInfo->bitsPerSample == 8 ) {
      pSB = (uint8 *) pSData;
      pDB = (uint8 *) pDData;
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
      pSW = (int16 *) pSData;
      pDW = (int16 *) pDData;
      for ( i = 0; i < nBytes; i += 2 ) {
         a = (*pSW++) + (*pDW);
         if ( a < -32768L ) a = -32768L;     // clamp over/underflow
         if ( a > 32767 )   a = 32767;
         *pDW++ = (int16) a;
      }
   }

   return pDData;
}


//TBD
#define EXPLODE TLOG0

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
   BOOL              notDone, seekPastEnd;
   int32             bytesAvail;    // bytes left before next state change
   sndSegInternal    *pSeg;
   int32             endGap;
   int32             oldEndGap;
   sndSplicerState   state;
   uint32            bytesPerSample;
   uint32            startOff;
   uint32            resyncSample;

   pInfo = (sndSplicerStuff *) pCBData;
   pSeg = pInfo->pSeg;

   bytesPerSample = pInfo->bitsPerSample >> 3;


   TLOG3( "refillSplicer start %ld end %ld offset %ld", pSeg->startSample,
         pSeg->endSample, pInfo->seg1Offset );

   if ( ISndSample_ResyncNeeded( pSample, &resyncSample ) ) {

      // we have just been unmuted, we must reposition
      // source stream to resyncSample
      // 1. Find correct segment (handle past end of data case)
      // 2. Seek within that segment, & reinit for splice case
      TLOG1("\nsplicer - resyncing to position %d", resyncSample );
      seekPastEnd = FALSE;
      while ( resyncSample > pSeg->endSample ) {
         if ( pSeg < pInfo->pSegN ) {
            TLOG2( "skipping segment from %ld to %ld", pSeg->startSample, pSeg->endSample );
            pSeg++;
         } else {
            // seek past end of splice data - sample is done playing
            seekPastEnd = TRUE;
            break;
         }
      }
      if ( seekPastEnd || (pSeg >= pInfo->pSegN) ) {
         TLOG0("splice empty - no segments left");
         // feed no-data to stream to start stream close-down process
         ISndSample_LoadBufferIndirect( pSample, getSpliceSingleData, pInfo, 0 );
         return;
      }

      // pSeg is the segment new position is in
      pInfo->pSeg = pSeg;

      // determine if new pos is in a single-splice, dual-splice or silence
      startOff = (resyncSample - pSeg->startSample) * bytesPerSample;

      // assume we are in single splice
      pInfo->state = kSpliceSingle;
      pInfo->seg1Offset = pSeg->offset + startOff;
      // calc bytes left as if seg data extended all the way to the next segment,
      //  which is wrongo for dual-splice or silence cases - they are fixed below
      pInfo->bytesAvail = ((pSeg->endSample + 1) - resyncSample) * bytesPerSample;
      if ( pSeg->endGap != 0 ) {
         if ( pSeg->endGap < 0 ) {
            // this segment has a dual splice at end
            if ( startOff >= (pSeg->numBytes + pSeg->endGap) ) {
               // we are in dual splice
               pInfo->state = kSpliceDual;
               pInfo->seg2Offset = pSeg[1].offset + (startOff - (pSeg->numBytes + pSeg->endGap));
               pInfo->bytesAvail = ((pSeg->endSample + 1) - resyncSample) * bytesPerSample;
            } else {
               // we are in single splice part - reduce bytes left by dual splice len
               pInfo->bytesAvail += pSeg->endGap;
            }
         } else {
            // this segment has a silence gap at end
            if ( startOff >= pSeg->numBytes ) {
               // we are in silence gap
               pInfo->state = kSpliceSilence;
               pInfo->bytesAvail = pSeg->endGap - (startOff - pSeg->numBytes);
            } else {
               // we are in single splice part - reduce bytes left by following gap len
               pInfo->bytesAvail -= pSeg->endGap;
            }
         }
      }  // if endGap != 0

   }  // if resyncNeeded


   bytesAvail = pInfo->bytesAvail;
   endGap = pSeg->endGap;
   state = pInfo->state;
      
   //mprintf( "refillSplicer %d needed, %d available, %d offset\n", bytesNeeded,
   //   bytesAvail, pInfo->seg1Offset);
   TLOG3( "refillSplicer %d needed, %d available, %d offset", bytesNeeded,
         bytesAvail, pInfo->seg1Offset);

   notDone = TRUE;
   while( notDone ) {

      switch( state ) {

         case kSpliceEmpty:
            //
            // open the next segment if one is available.  NOTE: pSeg already points to
            // the next segment
            //
            if ( pSeg < pInfo->pSegN ) {
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
                  TLOG3( "splice empty->silence seg[%d], %d bytesAvail, %d endGap",
                        pSeg - pInfo->pSeg0, bytesAvail, endGap );
               } else {
                  //if ( pSeg->loopMe ) {
                  //   mprintf("loop seg bytes %ld off %ld gap 0x%x\n",
                  //           pSeg->numBytes, pSeg->offset, pSeg->endGap );
                  //}

                  TLOG3( "splice empty->single seg[%d], %d bytesAvail, %d endGap",
                        pSeg - pInfo->pSeg0, bytesAvail, endGap );
                  state = kSpliceSingle;
                  pInfo->seg1Offset = pSeg->offset;
               }

            } else {
               notDone = FALSE;
               TLOG0("splice empty - no segments left");
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
            if ( bytesAvail == 0 ) {
               // need to switch state, either to dual splice or empty
               if ( endGap == 0 ) {
                  state = kSpliceEmpty;
                  LOG3( "splice single->empty seg[%d], %d bytesAvail, %d endGap",
                        pSeg - pInfo->pSeg0, bytesAvail, endGap );
                  // tell data read function we are done with segment
                  pInfo->getData( pSeg, NULL, 0, 0 );
                  if ( !pSeg->loopMe ) {
                     // on to next single splice segment
                     pSeg++;
                  }
               } else if ( endGap < 0 ) {
                  state = kSpliceDual;
                  bytesAvail = (-endGap);
                  pInfo->seg2Offset = pSeg[1].offset;
                  LOG3( "splice single->dual seg[%d], %d bytesAvail, %d endGap",
                        pSeg - pInfo->pSeg0, bytesAvail, endGap );
               } else {
                  state = kSpliceSilence;
                  bytesAvail = endGap;
                  LOG3( "splice single->silence seg[%d], %d bytesAvail, %d endGap",
                        pSeg - pInfo->pSeg0, bytesAvail, endGap );
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
            if ( bytesAvail == 0 ) {
               // need to switch state to single splice
               state = kSpliceSingle;
               pInfo->seg1Offset = pInfo->seg2Offset;
               oldEndGap = endGap;
               // tell data read function we are done with segment
               pInfo->getData( pSeg, NULL, 0, 0 );
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
               LOG3( "splice dual->single seg[%d], %d bytesAvail, %d endGap",
                     pSeg - pInfo->pSeg0, bytesAvail, endGap );
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
               // tell data read function we are done with segment
               pInfo->getData( pSeg, NULL, 0, 0 );
               pSeg++;
               LOG3( "splice silence->empty seg[%d], %d bytesAvail, %d endGap",
                     pSeg - pInfo->pSeg0, bytesAvail, endGap );
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

   //mprintf( "endStream\n" );
   TLOG0( "endStream" );
   pInfo = (sndSplicerStuff *) pCBData;

   // first tell the app about samples demise
   if ( pInfo->endCB != NULL ) {
      pInfo->endCB( pSample, pInfo->pEndCBData );
   }

   // release the ring buffer
   if ( pInfo->pBuffer != NULL ) {
      Free( pInfo->pBuffer );
   }
   Free( pInfo->pTmpBuffer );
   Free( pInfo->pSeg0 );
   Free( pInfo );

   //ISndSample_DumpData( pSample, FALSE );
}


////////////////////////////////////
// CreateSoundSplicer - create a streaming sound from a list of sound resources
//
// Takes:
//    pMixer      audio mixer to play sounds on
//    getData     app function which supplies sound image data
//    pSegList    list of sounds & offsets
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
CreateSoundSplicer( ISndMixer          *pMixer,
                    SndFetchFunction   getData,
                    sndSegInternal     *pSegList,
                    uint32             numSegs,
                    void               *pBuffer,
                    uint32             bufferLen,
                    SndEndCallback     endCB,
                    void               *pEndCBData )
{
   sSndAttribs       attribs;
   uint32            rezLen;
   uint32            nSamples;
   void              *pRawData;
   uint32            rawDataLen;
   ISndSample        *pSample;
   sndSplicerStuff   *pInfo;
   uint32            bytesToRead;
   sndSegInternal    *pSeg;
   BOOL              blewIt;
   float             bytesPerMilliSecond;
   uint32            i;
   BOOL              useExternalBuffer;
   uint32            segStartSample;
   int               bytesPerSample;

   TLOG2("CreateSoundSplicer %ld numRez, %ld buffLen",
        numSegs, bufferLen );

   if ( numSegs < 1 ) {
      return NULL;
   }

   if ( pBuffer == NULL ) {
      // user wants us to handle memory allocation/deallocation
      pBuffer = Malloc( bufferLen );
      useExternalBuffer = FALSE;
   } else {
      // user is doing memory allocation/deallocation
      useExternalBuffer = TRUE;
   }
   pSeg = pSegList;
   pInfo = (sndSplicerStuff *) Malloc( sizeof( sndSplicerStuff ) );
   pInfo->pSeg0 = pSeg;
   pInfo->pSegN = pSeg + numSegs;
   pInfo->numSegs = numSegs;
   segStartSample = 0;

   for ( i = 0; i < numSegs; i++ ) {

      //
      // build internal version of segment list - this requires opening each segment and
      //   cracking its sound resource header
      //
      
      rezLen = pSeg->numBytes;
      bytesToRead = 256;
      if ( bytesToRead > rezLen ) {
         // don't try to read more than is in resource
         bytesToRead = rezLen;
      }
      // Get the rez header, and examine it to find type of sound resource
      // TBD: What if bufferLen is too small to fit entire Rez header in buffer?
      getData( pSeg, pBuffer, bytesToRead, 0 );
      blewIt = SndCrackRezHeader( pBuffer, bytesToRead, &pRawData, &rawDataLen,
                                  &nSamples, &attribs );
      if ( !blewIt ) {
         if ( i == (numSegs - 1) ) {
            // create the actual stream after creating last segment
            pSample = ISndMixer_CreateRawSample( pMixer, kSndSampleStream, pBuffer, bufferLen,
                                                 segStartSample + nSamples, &attribs );
            if( pSample == NULL ) {
               blewIt = TRUE;
            }
         }
         if ( i == 0 ) {
            bytesPerMilliSecond = (float) (attribs.sampleRate * (attribs.bitsPerSample >> 3))
               / 1000.0;
         }
      }

      if ( blewIt ) {
         if ( !useExternalBuffer ) {
            Free( pBuffer );
         }
         Free( pInfo->pSeg0 );
         Free( pInfo );
         return NULL;
      }

      // fill in internal segment info
      if ( pSeg->offset == 0x80000000 ) {    // ugly little hack for loop mode
         //mprintf("looping seg %d\n", i );
         pSeg->endGap = 0;
         pSeg->loopMe = TRUE;
         pSeg->offset = 0;
      } else {
         pSeg->endGap = bytesPerMilliSecond * pSeg->offset;
         pSeg->loopMe = FALSE;
      }

      // after this point, offset means offset into sound resource, not
      //   the size of the gap/overlap at end of sample
      pSeg->offset = (long) pRawData - (long) pBuffer;
      if ( attribs.bitsPerSample == 16 ) {
         // force gap length to be integral # of 16-bit samples
         pSeg->endGap &= ~1;
      }
      pSeg->numBytes = rawDataLen;

      // calculate sample position in total spliced sample which corresponds
      // to the first sample position of this segment
      bytesPerSample = attribs.nChannels * (attribs.bitsPerSample / 8);
      pSeg->startSample = segStartSample;
      segStartSample += (nSamples + (pSeg->endGap / bytesPerSample));
      // for purposes of seeking within splice, define last sample of segment
      // as sample just before 1st sample of next segment - this sample will
      // not correspond with the end of segment data if end offset is not 0
      pSeg->endSample = segStartSample - 1;

      LOG3("segment offset %ld, numBytes %ld, endGap %ld", pSeg->offset,
           pSeg->numBytes, pSeg->endGap );
      pSeg++;

   } // end for i == 0...numSegs

   // prevent last end gap from being negative (no following seg to overlap)
   pSeg--;
   if ( pSeg->endGap < 0 ) {
      pSeg->endGap = 0;
   }

   // init splicer info struct
   pInfo->getData = getData;
   pInfo->state = kSpliceEmpty;
   pInfo->pBuffer = (useExternalBuffer) ? NULL : pBuffer;
   pInfo->bufferLen = bufferLen;
   pInfo->pTmpBuffer = Malloc( TMP_BUFFER_LEN );
   pInfo->tmpBufferLen = TMP_BUFFER_LEN;
   pInfo->pSeg = pInfo->pSeg0;
   pInfo->bitsPerSample = attribs.bitsPerSample;

   // setup stream-fill and end-of-sample callbacks
   ISndSample_RegisterFillCallback( pSample, refillSplicedStream, pInfo );
   pInfo->endCB = endCB;
   pInfo->pEndCBData = pEndCBData;
   ISndSample_RegisterEndCallback( pSample, endSplicedStream, pInfo );

   // setup data dumping
   //ISndSample_DumpData( pSample, TRUE );

#ifdef NOT_ANYMORE
   // now fill the ring buffer with sample data
   refillSplicedStream( pSample, pInfo, bufferLen - 4 );
   // TBD: this doesn't handle the case where the total spliced sample is shorter than
   //    the ring buffer...
#endif

	return pSample;
}

