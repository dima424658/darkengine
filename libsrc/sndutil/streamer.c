////////////////////////////////////////////////////////////////////////
//
// (c) 1996 Looking Glass Technologies Inc.
// Pat McElhatton
//
// Module name: Sound resource streamer common code
// File name: streamer.c
//
// Description: Play a sound in streaming mode
//
////////////////////////////////////////////////////////////////////////
#include <sndutil.h>
#include <streamer.h>
#include <timelog.h>
#include <mprintf.h>
#include <imaadpcm.h>
#include <memall.h>

//
// getDecompressedData - copy data from decompression buffer into stream ring buffer
//
static void *
getDecompressedData( void     *pCBData,
                     void     *pDst,
                     uint32   nBytes )
{
   sndStreamStuff       *pInfo;
   BOOL                 getHeader;

   pInfo = (sndStreamStuff *) pCBData;

   // decompress input block
   getHeader = (pInfo->decompOffset == 0);
   pInfo->pInData = DecompressIMABlockPartial( pInfo->pInData, pDst,
                                               nBytes >> 1, getHeader,
                                               &(pInfo->decompState) );
   // flag to future calls that we have already gotten the header
   pInfo->decompOffset = 1;

   return pDst;
}


//
// refillPCMStream - refill stream with data from uncompressed PCM source
//
static void
refillPCMStream( ISndSample   *pSample,
                 void         *pCBData,
                 uint32       bytesNeeded )
{
   sndStreamStuff    *pInfo;
   uint32            bytesLeft;
   uint32            newPos;

   pInfo = (sndStreamStuff *) pCBData;
   if ( ISndSample_ResyncNeeded( pSample, &newPos ) ) {
      // we have just been unmuted - we must continue playing at sample newPos

      // convert from sample position to byte position
      newPos *= (pInfo->attribs.nChannels * (pInfo->attribs.bitsPerSample / 8));
      newPos += pInfo->dataOffset;
      if ( newPos > pInfo->endOffset ) {
         newPos = pInfo->endOffset;
      }
      pInfo->playOffset = newPos;
   }
   bytesLeft = pInfo->endOffset - pInfo->playOffset;

   TLOG3( "refillStream %d needed, %d left, %d offset", bytesNeeded,
         bytesLeft, pInfo->playOffset);

   if ( bytesNeeded > bytesLeft ) {
      bytesNeeded = bytesLeft;
   }

   ISndSample_LoadBufferIndirect( pSample, pInfo->getData, pInfo, bytesNeeded );

   // TBD - will we ever have a long looping resource?
}


//
// refillADPCMStream - refill stream with data from ADPCM source
//
static void
refillADPCMStream( ISndSample    *pSample,
                   void          *pCBData,
                   uint32        bytesNeeded )
{
   sndStreamStuff    *pInfo;
   uint32            bytesToWrite, bytesOutPerBlock;
   sSndAttribs       *pAttribs;
   uint32            newPos, blockPos, blockNum;

   pInfo = (sndStreamStuff *) pCBData;
   TLOG3( "refillADPCMStream %d needed, %d left, %d offset", bytesNeeded,
         pInfo->outBytesLeft, pInfo->playOffset);

   pAttribs = &(pInfo->attribs);
   if ( ISndSample_ResyncNeeded( pSample, &newPos ) ) {
      // we have just been unmuted - we must continue playing at sample newPos

      // 1. Handle position past end of data
      // 2. Seek to nearest lower block boundary
      // 3. Skip samples to get to new sample position

      if ( newPos >= pInfo->numSamples ) {
         // position is past end of data
         bytesNeeded = 0;
         pInfo->outBytesLeft = 0;
      } else {
         // see how many samples to skip after block start to get
         //   to new position
         blockNum = newPos / pAttribs->samplesPerBlock;
         blockPos = blockNum * pAttribs->samplesPerBlock;
         pInfo->skipNSamples = newPos - blockPos;
         // convert from sample position to byte position in input stream
         blockPos = (blockNum * pAttribs->bytesPerBlock) + pInfo->dataOffset;
         if ( blockPos > pInfo->endOffset ) {
            blockPos = pInfo->endOffset;
         }
         pInfo->playOffset = blockPos;
         // this is the # of output bytes that will be left after
         //  sample skipping is done
         pInfo->outBytesLeft = (pInfo->numSamples - newPos)
            * (2 * pAttribs->nChannels);
      }
   }

   if ( bytesNeeded == 0 ) {
      // this is the apps way of telling the stream to shutdown
      ISndSample_LoadBufferIndirect( pSample, getDecompressedData, pInfo, 0 );
      return;
   }

   bytesOutPerBlock = (pAttribs->samplesPerBlock << 1) * pAttribs->nChannels;
   while ( bytesNeeded > bytesOutPerBlock ) {
      // get an input block
      // the getData func will return where the input data can be found.
      // the getData is allowed to change pInData, for example getDecompressedData
      // changes pInData to point to the next position in input block to
      // continue decompression from
      pInfo->pInData = pInfo->pTmpBuffer;

      pInfo->pInData = pInfo->getData( pInfo, pInfo->pTmpBuffer, pAttribs->bytesPerBlock );

      pInfo->decompOffset = 0;
      bytesToWrite = bytesOutPerBlock;

      if ( pInfo->skipNSamples ) {
         // this stream has just been unmuted, so we have to "seek" to
         // a new position in the source stream - at this point we have to
         // decode and discard the number of samples in skipNSamples to
         // complete the seek
         void *pDst;
         uint32 nDiscardBytes;

         //TBD: avoid creating temporary buffer for discarding samples
         // make a temporary output buffer & decode samples to discard into it
         nDiscardBytes = 2 * pInfo->skipNSamples * pAttribs->nChannels;
         pDst = Malloc( nDiscardBytes );
         pInfo->pInData = DecompressIMABlockPartial( pInfo->pInData, pDst,
                                                     nDiscardBytes >> 1, 1,
                                                     &(pInfo->decompState) );
         pInfo->skipNSamples = 0;
         bytesToWrite -= nDiscardBytes;
         // flag that we have already decoded header
         pInfo->decompOffset = 1;
         Free( pDst );
      }

      // pass output block to sound system
      if ( bytesToWrite > pInfo->outBytesLeft ) {
         bytesToWrite = pInfo->outBytesLeft;
      }

      // playOffset here means offset into decomp output buffer
      ISndSample_LoadBufferIndirect( pSample, getDecompressedData, pInfo, bytesToWrite );
      if ( bytesToWrite == 0 ) {

         // this was our last pass thru
         break;
      } else {
         bytesNeeded -= bytesToWrite;
         pInfo->outBytesLeft -= bytesToWrite;
      }
   }

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

   //mprintf("end stream\n");
   TLOG0( "endStream" );
   pInfo = (sndStreamStuff *) pCBData;

   // first tell the app about samples demise
   if ( pInfo->endCB != NULL ) {
      pInfo->endCB( pSample, pInfo->pEndCBData );
   }

   if ( pInfo->pBuffer != NULL ) {
      Free( pInfo->pBuffer );
   }
   if ( pInfo->pTmpBuffer != NULL ) {
      Free( pInfo->pTmpBuffer );
   }
   Free( pInfo );
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
CreateSoundStreamer( ISndMixer         *pMixer,
                     SndLoadFunction   getData,
                     sndStreamStuff    *pInfo,
                     uint32            dataLen,
                     void              *pBuffer,
                     uint32            bufferLen,
                     SndEndCallback    endCB,
                     void              *pEndCBData )
{
   uint32            nSamples;
   void              *pRawData;
   uint32            rawDataLen;
   ISndSample        *pSample;
   uint32            bytesToRead;
   BOOL              useExternalBuffer;
   sSndAttribs       attribs;
   SndFillCallback   refillFunc;
   void              *pInData;

   bytesToRead = bufferLen;
   if ( bytesToRead > dataLen ) {
      // don't try to read more than is in resource
      bytesToRead = dataLen;
   }

   if ( pBuffer == NULL ) {
      // user wants us to handle memory allocation/deallocation
      pBuffer = Malloc( bufferLen );
      useExternalBuffer = FALSE;
   } else {
      // user is doing memory allocation/deallocation
      useExternalBuffer = TRUE;
   }

   TLOG3("CreateSoundStreamer %ld bytesToRead, %ld dataLen, %ld buffLen", bytesToRead,
        dataLen, bufferLen );

   // Get the rez header, and examine it to find type of sound resource
   // TBD: What if bufferLen is too small to fit entire Rez header in buffer?
   pInfo->playOffset = 0;
   pInData = getData( pInfo, pBuffer, bytesToRead );

   assert( pInData != NULL );

   // NOTE: at this point, the data is at pInfo->pInData, which may or
   //  may not be the same as pBuffer
   if ( SndCrackRezHeader( pInData, bytesToRead,
                           &pRawData, &rawDataLen,
                           &nSamples, &(pInfo->attribs) )  ) {
      if ( !useExternalBuffer ) {
         Free( pBuffer );
      }
      Free( pInfo );
      mprintf("SoundCreateStreamer - sndfmt returned error!\n");
      return NULL;
   }

   // create the actual sample...
   // TBD: How to pass offset to raw data area, raw data len to sample::Init
   attribs = pInfo->attribs;
   if ( attribs.dataType == kSndDataIMAADPCM ) {
      // input type is IMA ADPCM, output type is 16-bit PCM
      attribs.dataType = kSndDataPCM;
      attribs.bitsPerSample = 16;
      attribs.bytesPerBlock = 2;
      attribs.samplesPerBlock = 1;
   }
	pSample = ISndMixer_CreateRawSample( pMixer, kSndSampleStream, pBuffer, bufferLen,
                                        nSamples, &attribs );

   if( pSample == NULL ) {
      // release the ring buffer
      if ( !useExternalBuffer ) {
         Free( pBuffer );
      }
      Free( pInfo );
      mprintf("SoundCreateStreamer - create sample returned NULL!\n");
   } else {
      // create our stream info struct & init it
      pInfo->getData = getData;
      pInfo->pBuffer = (useExternalBuffer) ? NULL : pBuffer;
      pInfo->bufferLen = bufferLen;
      pInfo->playOffset = (long) pRawData - (long) (pInData);
      pInfo->dataOffset = pInfo->playOffset;
      pInfo->endOffset = pInfo->playOffset + rawDataLen;
      pInfo->numSamples = nSamples;
      pInfo->skipNSamples = 0;
      if ( pInfo->attribs.dataType == kSndDataIMAADPCM ) {
         // calculate output bytes
         pInfo->outBytesLeft = nSamples * sizeof(short)
            * pInfo->attribs.nChannels;
         // allocate decompression buffers
         pInfo->pTmpBuffer = Malloc( pInfo->attribs.bytesPerBlock );
         refillFunc = refillADPCMStream;
      } else {
         // for non-compressed formats, #output bytes == #input bytes
         pInfo->outBytesLeft = rawDataLen;
         // no decompression buffers needed
         pInfo->pTmpBuffer = NULL;
         refillFunc = refillPCMStream;
      }

#ifdef NOT_ANYMORE
      // now fill the ring buffer with sample data
      bytesToWrite = ISndSample_AvailToWrite( pSample );
      
      if ( pInfo->outBytesLeft < bytesToWrite ) {
         // copy all data to ring buffer
         refillFunc( pSample, pInfo, pInfo->outBytesLeft );
         // and force a silence fill to end of buffer
         refillFunc( pSample, pInfo, 0 );
      } else {
         refillFunc( pSample, pInfo, bytesToWrite );
      }
#endif
      // setup stream-fill and end-of-sample callbacks
      ISndSample_RegisterFillCallback( pSample, refillFunc, pInfo );
      pInfo->endCB = endCB;
      pInfo->pEndCBData = pEndCBData;
      ISndSample_RegisterEndCallback( pSample, endStream, pInfo );
   }
	return pSample;
}


