////////////////////////////////////////////////////////////////////////
//
// (c) 1996 Looking Glass Technologies Inc.
// Pat McElhatton
//
// Module name: Sound memory streamer
// File name: mstremer.c
//
// Description: Play a sound resource in streaming mode from a memory image
//
////////////////////////////////////////////////////////////////////////
#include <sndutil.h>
#include <streamer.h>
#include <timelog.h>
#include <mprintf.h>
#include <memall.h>

//
// getMemData - function to copy resource data into stream ring buffer
//
static void
*getMemData( void     *pCBData,
            void     *pDst,
            uint32   nBytes )
{
   sndStreamStuff *pInfo;
   void *pData;

   pInfo = (sndStreamStuff *) pCBData;
   // just set the input pointer to the right spot in memory image
   pData = (void *) (pInfo->extras[0] + pInfo->playOffset);
   pInfo->playOffset += nBytes;
   return pData;
}


////////////////////////////////////
// CreateSoundMemoryStreamer - create a streaming sound from a memory image
//
// Takes:
//    pMixer      audio mixer to play sounds on
//    pAudioData  pointer to wave/voc file memory image
//    pBuffer     pointer to user-allocated ring buffer (optional, pass NULL
//                  and this routine will handle ring buffer alloc/dealloc
//    bufferLen   size in bytes of ring buffer
//    endCB       app callback, invoked when sound is done playing
//    pEndCBData  callback data for endCB
//
// Returns:	An ISndSample * that is ready to go.
//
ISndSample *
CreateSoundMemoryStreamer( ISndMixer        *pMixer,
                           void             *pAudioData,
                           uint32            audioDataLen,
                           void             *pBuffer,
                           uint32           bufferLen,
                           SndEndCallback   endCB,
                           void             *pEndCBData )
{
   ISndSample *pSample;
   sndStreamStuff *pInfo;

   assert( pAudioData != NULL );

   pInfo = (sndStreamStuff *) Malloc( sizeof( sndStreamStuff ) );
   pInfo->extras[0] = (uint32) pAudioData;

   LOG2("CreateSoundMemoryStreamer %ld audioLen, %ld buffLen",
        audioDataLen, bufferLen );
   pSample = CreateSoundStreamer( pMixer, getMemData, pInfo, audioDataLen,
                                  pBuffer, bufferLen, endCB, pEndCBData );
	return pSample;
}

