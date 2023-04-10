////////////////////////////////////////////////////////////////////////
//
// (c) 1996 Looking Glass Technologies Inc.
// Pat McElhatton
//
// Module name: Sound file one-shot player
// File name: foneshot.c
//
// Description: Play a sound file in one-shot mode
//
////////////////////////////////////////////////////////////////////////
#include <sndutil.h>
#include <oneshot.h>
#include <timelog.h>
#include <mprintf.h>
#include <memall.h>
#include <dbg.h>

//
// getFileData - copy file data into stream ring buffer
//
static void
*getFileData( void     *pCBData,
              void     *pDst,
              uint32   nBytes )
{
   sndOneShotStuff *pInfo;
   FILE *inFile;

   pInfo = (sndOneShotStuff *) pCBData;
   inFile = (FILE *) pInfo->extras[0];
   fseek( inFile, pInfo->playOffset, SEEK_SET );
   fread( pDst, 1, nBytes, inFile );
   if ( (pInfo->playOffset + nBytes) >= pInfo->endOffset ) {
      // we have reached end of data
      fclose( inFile );
   }

   return pDst;
}


////////////////////////////////////
// CreateSoundFileStreamer - create a streaming sound from a sound file
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
CreateSoundFileOneShot( ISndMixer        *pMixer,
                        char             *fileName,
                        SndEndCallback   endCB,
                        void             *pEndCBData )
{
   uint32         fileLen;
   ISndSample     *pSample;
   sndOneShotStuff *pInfo;
   char           fname[256];
   FILE           *inFile;

   strcpy( fname, fileName );
   // if filename has no extension, give it a .wav extension
   if ( strchr( fname, '.' ) == NULL ) {
      // append ".wav" to end of filename
      strcat( fname, ".wav" );
   }
   inFile = fopen( fname, "rb" );
   if ( inFile == NULL ) {
      Warning( ("couldn't open %s\n", fname) );
      return NULL;
   } else {
      fseek( inFile, 0L, SEEK_END );
      fileLen = ftell( inFile );
      fseek( inFile, 0L, SEEK_CUR );
   }

   pInfo = (sndOneShotStuff *) Malloc( sizeof( sndOneShotStuff ) );
   pInfo->extras[0] = (uint32) inFile;

   LOG1("CreateSoundFileStreamer %ld fileLen", fileLen );
   pSample = CreateSoundOneShot( pMixer, getFileData, pInfo, fileLen,
                                 endCB, pEndCBData );
	return pSample;
}

