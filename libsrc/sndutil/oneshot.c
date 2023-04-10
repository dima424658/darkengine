////////////////////////////////////////////////////////////////////////
//
// (c) 1996 Looking Glass Technologies Inc.
// Pat McElhatton
//
// Module name: Sound resource one-shot player
// File name: oneshot.c
//
// Description: Play a one-shot sound, without rez locking or extra copies
//
////////////////////////////////////////////////////////////////////////
#include <sndutil.h>
#include <oneshot.h>
#include <timelog.h>
#include <mprintf.h>

//
// endOneShot - free stuff this sound was using
//
static void
endOneShot( ISndSample   *pSample,
            void         *pCBData )
{
   sndOneShotStuff *pInfo;

   //mprintf("end one-shot\n");
   LOG0( "endStream" );
   pInfo = (sndOneShotStuff *) pCBData;

   // first tell the app about samples demise
   if ( pInfo->endCB != NULL ) {
      pInfo->endCB( pSample, pInfo->pEndCBData );
   }

   Free( pInfo );
}


//
// fill the sample buffer when this oneshot is about to be played
//  the real data movement is done by the getData routine
//
static void
fillOneShot( ISndSample    *pSample,
             void          *pCBData,
             uint32        nBytes )
{
   sndOneShotStuff   *pInfo;
   uint32            newPos;

   pInfo = (sndOneShotStuff *) pCBData;

   if ( ISndSample_ResyncNeeded( pSample, &newPos ) ) {
      // need to update play offset to reflect start position
      pInfo->playOffset = pInfo->dataOffset + (newPos * pInfo->bytesPerSample);
   }

   ISndSample_LoadBufferIndirect( pSample, pInfo->getData, pInfo, nBytes );
}


////////////////////////////////////
// CreateSoundOneShot - create a one-shot sound from a sound resource
//
// Takes:
//	       rezId -- Id of resource to play
// Returns:	An ISndSample * that is ready to go.
//
// This avoids the extra data copies inherent in ISndMixer_CreateSampleFromRez, as
//  well as doing the resource handling
//
ISndSample *
CreateSoundOneShot( ISndMixer          *pMixer,
                    SndLoadFunction    getData,
                    sndOneShotStuff    *pInfo,
                    uint32             soundRezLen,
                    SndEndCallback     endCB,
                    void               *pEndCBData )
{
   uint32            nSamples;
   void              *pRawData;
   uint32            rawDataLen;
   ISndSample        *pSample;
   uint32            bytesToRead;
   sSndAttribs       attribs;
   uint8             headerBuffer[1024];

   bytesToRead = sizeof(headerBuffer);
   if ( bytesToRead > soundRezLen ) {
      // don't try to read more than is in resource
      bytesToRead = soundRezLen;
   }

   LOG2("CreateSoundOneShot %ld bytesToRead, %ld dataLen", bytesToRead,
        soundRezLen );

   // Get the rez header, and examine it to find type of sound resource
   // TBD: What if bufferLen is too small to fit entire Rez header in buffer?
   pInfo->playOffset = 0;
   getData( pInfo, headerBuffer, bytesToRead );
   if ( SndCrackRezHeader( headerBuffer, bytesToRead, &pRawData, &rawDataLen,
                           &nSamples, &attribs )  ) {
      Free( pInfo );
      return NULL;
   }

   // create the actual sample...
   // TBD: How to pass offset to raw data area, raw data len to sample::Init
   if ( attribs.dataType != kSndDataPCM ) {
      // we only support PCM data for one-shots currently
      Free( pInfo );
      return NULL;
   }
	pSample = ISndMixer_CreateRawSample( pMixer, kSndSampleNormal, NULL, rawDataLen,
                                        nSamples, &attribs );

   if( pSample == NULL ) {
      Free( pInfo );
   } else {

      pInfo->playOffset = (long) pRawData - (long) headerBuffer;
      pInfo->dataOffset = pInfo->playOffset;
      pInfo->endOffset = pInfo->playOffset + rawDataLen;
      pInfo->bytesPerSample = attribs.bitsPerSample >> 3;

      // set the callback which will fill the buffer with sample data
      pInfo->getData = getData;
      ISndSample_RegisterFillCallback( pSample, fillOneShot, pInfo );

      // setup end-of-sample callback
      pInfo->endCB = endCB;
      pInfo->pEndCBData = pEndCBData;
      ISndSample_RegisterEndCallback( pSample, endOneShot, pInfo );
   }

	return pSample;
}

