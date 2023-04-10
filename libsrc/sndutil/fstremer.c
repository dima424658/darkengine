////////////////////////////////////////////////////////////////////////
//
// (c) 1996 Looking Glass Technologies Inc.
// Pat McElhatton
//
// Module name: Sound file streamer
// File name: fstremer.c
//
// Description: Play a sound file in streaming mode
//
////////////////////////////////////////////////////////////////////////
#include <sndutil.h>
#include <streamer.h>
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
   sndStreamStuff *pInfo;
   FILE *inFile;

   pInfo = (sndStreamStuff *) pCBData;
   inFile = (FILE *) pInfo->extras[0];
   fseek( inFile, pInfo->playOffset, SEEK_SET );
   fread( pDst, 1, nBytes, inFile );
   pInfo->playOffset += nBytes;

   return pDst;
}


//
// we wrap the app end sample callback with this so we can
//   close the input file
//
static void
endFileStream( ISndSample    *pSample,
               void          *pCBData )
{
   sndStreamStuff    *pInfo;
   FILE              *inFile;
   SndEndCallback    endCB;
   void              *pEndCBData;

   pInfo = (sndStreamStuff *) pCBData;
   inFile = (FILE *) pInfo->extras[0];
   fclose( inFile );

   // now chain to the app end-of-sample callback
   endCB = (SndEndCallback) pInfo->extras[1];
   pEndCBData = (void *) pInfo->extras[2];
   endCB( pSample, pEndCBData );
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
CreateSoundFileStreamer( ISndMixer        *pMixer,
                         char             *fileName,
                         void             *pBuffer,
                         uint32           bufferLen,
                         SndEndCallback   endCB,
                         void             *pEndCBData )
{
   uint32         fileLen;
   ISndSample     *pSample;
   sndStreamStuff *pInfo;
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

   pInfo = (sndStreamStuff *) Malloc( sizeof( sndStreamStuff ) );
   pInfo->extras[0] = (uint32) inFile;
   pInfo->extras[1] = (uint32) endCB;
   pInfo->extras[2] = (uint32) pEndCBData;

   LOG2("CreateSoundFileStreamer %ld fileLen, %ld buffLen",
        fileLen, bufferLen );
   pSample = CreateSoundStreamer( pMixer, getFileData, pInfo, fileLen, pBuffer,
                                  bufferLen, endFileStream, pInfo );
	return pSample;
}

