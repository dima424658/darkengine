////////////////////////////////////////////////////////////////////////
//
// (c) 1996 Looking Glass Technologies Inc.
// Pat McElhatton
//
// Module name: Sound resource streamer
// File name: rstremer.c
//
// Description: Play a sound resource in streaming mode given its rez id
//
////////////////////////////////////////////////////////////////////////
#include <sndutil.h>
#include <streamer.h>
#include <timelog.h>
#include <mprintf.h>
#include <memall.h>

//
// getRezData - function to copy resource data into stream ring buffer
//
static void
*getRezData( void     *pCBData,
             void     *pDst,
             uint32   nBytes )
{
   sndStreamStuff *pInfo;

   pInfo = (sndStreamStuff *) pCBData;
   ResExtractPartial( (Id) pInfo->extras[0], pDst, pInfo->playOffset, nBytes );
   pInfo->playOffset += nBytes;
   return pDst;
}


////////////////////////////////////
// CreateSoundRezStreamer - create a streaming sound from a sound resource
//
// Takes:
//    pMixer      audio mixer to play sounds on
//    rezId       Id of sound resource to play
//    pBuffer     pointer to user-allocated ring buffer (optional, pass NULL
//                  and this routine will handle ring buffer alloc/dealloc
//    bufferLen   size in bytes of ring buffer
//    endCB       app callback, invoked when sound is done playing
//    pEndCBData  callback data for endCB
//
// Returns:	An ISndSample * that is ready to go.
//
ISndSample *
CreateSoundRezStreamer( ISndMixer        *pMixer,
                        Id               rezId,
                        void             *pBuffer,
                        uint32           bufferLen,
                        SndEndCallback   endCB,
                        void             *pEndCBData )
{
   uint32 rezLen;
   ISndSample *pSample;
   sndStreamStuff *pInfo;

   pInfo = (sndStreamStuff *) Malloc( sizeof( sndStreamStuff ) );
   pInfo->extras[0] = rezId;
   rezLen = ResSize( rezId );

   TLOG2("CreateSoundRezStreamer %ld rezLen, %ld buffLen",
        rezLen, bufferLen );
   pSample = CreateSoundStreamer( pMixer, getRezData, pInfo, rezLen, pBuffer,
                                  bufferLen, endCB, pEndCBData );
	return pSample;
}

