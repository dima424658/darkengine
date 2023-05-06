////////////////////////////////////////////////////////////////////////
//
// (c) 1996 Looking Glass Technologies Inc.
// Pat McElhatton
//
// Module name: Sound resource one-shot player
// File name: roneshot.c
//
// Description: Play a sound resource in one-shot mode given its rez id
//
////////////////////////////////////////////////////////////////////////
#include <sndutil.h>
#include <oneshot.h>
#include <timelog.h>
#include <mprintf.h>
#include <memall.h>

//
// getRezData - function to copy resource data into one-shot buffer
//
static void
*getRezData( void     *pCBData,
             void     *pDst,
             uint32   nBytes )
{
   sndOneShotStuff *pInfo;

   pInfo = (sndOneShotStuff *) pCBData;
   ResExtractPartial( (Id) pInfo->extras[0], pDst, pInfo->playOffset, nBytes );

   return pDst;
}


////////////////////////////////////
// CreateSoundRezOneShot - create a one-shot sound from a sound resource
//
// Takes:
//    pMixer      audio mixer to play sounds on
//    rezId       Id of sound resource to play
//    endCB       app callback, invoked when sound is done playing
//    pEndCBData  callback data for endCB
//
// Returns:	An ISndSample * that is ready to go.
//
ISndSample *
CreateSoundRezOneShot( ISndMixer        *pMixer,
                       Id               rezId,
                       SndEndCallback   endCB,
                       void             *pEndCBData )
{
   uint32 rezLen;
   ISndSample *pSample;
   sndOneShotStuff *pInfo;

   pInfo = (sndOneShotStuff *) Malloc( sizeof( sndOneShotStuff ) );
   pInfo->extras[0] = rezId;
   rezLen = ResSize( rezId );

   TLOG2("CreateSoundRezOneShot %d rezId, %ld rezLen",
        rezId, rezLen );
   pSample = CreateSoundOneShot( pMixer, getRezData, pInfo, rezLen,
                                 endCB, pEndCBData );
	return pSample;
}

