/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/auxsnd.cpp,v 1.8 1999/06/28 21:28:12 mwhite Exp $
// memory sound source stuff from Pat

#include <stdio.h>
#include <string.h>

#include <lg.h>
#include <platform_services.h>
#include <sndsrc.h>
#include <playlist.h>

#include <auxsnd.h>  // so we get our own EXTERN 

// must be last header
#include <dbmem.h>

#define kAUXSND_SecondsOfBuffer 1.0F

typedef struct _fakeSplicerThingy {
   ISndSource        *pSource;
   SndEndCallback    userCB;
   void              *pUserCBData;
   uint32            *pPlaylist;
   void              *pOwnedData;
} fakeSplicerThingy;

namespace
{
typedef int64_t sf_count_t;
struct SNDFILE_tag;
typedef SNDFILE_tag SNDFILE;

struct SF_INFO
{
   sf_count_t frames;
   int samplerate;
   int channels;
   int format;
   int sections;
   int seekable;
};

struct SF_VIRTUAL_IO
{
   sf_count_t (*get_filelen)(void *);
   sf_count_t (*seek)(sf_count_t, int, void *);
   sf_count_t (*read)(void *, sf_count_t, void *);
   sf_count_t (*write)(const void *, sf_count_t, void *);
   sf_count_t (*tell)(void *);
};

struct sSndFileApi
{
   void *library;
   SNDFILE *(*open_virtual)(SF_VIRTUAL_IO *, int, SF_INFO *, void *);
   sf_count_t (*readf_short)(SNDFILE *, short *, sf_count_t);
   int (*close)(SNDFILE *);
};

struct sMemorySound
{
   const uint8 *data;
   sf_count_t size;
   sf_count_t position;
};

sf_count_t MemoryLength(void *context)
{
   return static_cast<sMemorySound *>(context)->size;
}

sf_count_t MemorySeek(sf_count_t offset, int whence, void *context)
{
   sMemorySound *memory = static_cast<sMemorySound *>(context);
   sf_count_t base;
   if (whence == SEEK_SET)
      base = 0;
   else if (whence == SEEK_CUR)
      base = memory->position;
   else if (whence == SEEK_END)
      base = memory->size;
   else
      return -1;

   if (offset < -base || offset > memory->size - base)
      return -1;
   memory->position = base + offset;
   return memory->position;
}

sf_count_t MemoryRead(void *destination, sf_count_t count, void *context)
{
   sMemorySound *memory = static_cast<sMemorySound *>(context);
   if (count < 0)
      return 0;
   if (count > memory->size - memory->position)
      count = memory->size - memory->position;
   memcpy(destination, memory->data + memory->position,
          static_cast<size_t>(count));
   memory->position += count;
   return count;
}

sf_count_t MemoryWrite(const void *, sf_count_t, void *) { return 0; }

sf_count_t MemoryTell(void *context)
{
   return static_cast<sMemorySound *>(context)->position;
}

bool LoadSndFile(sSndFileApi *api)
{
   static bool attempted;
   static sSndFileApi loaded;
   if (!attempted)
   {
      const char *names[] = {
#ifdef _WIN32
         "libsndfile-1.dll", "sndfile.dll"
#else
         "libsndfile.so.1", "libsndfile.so"
#endif
      };
      attempted = true;
      for (unsigned i = 0; i < sizeof(names) / sizeof(names[0]); ++i)
      {
         loaded.library = PlatformLoadLibrary(names[i]);
         if (loaded.library)
            break;
      }
      if (loaded.library)
      {
         loaded.open_virtual = reinterpret_cast<SNDFILE *(*)(
            SF_VIRTUAL_IO *, int, SF_INFO *, void *)>(
               PlatformFindSymbol(loaded.library, "sf_open_virtual"));
         loaded.readf_short = reinterpret_cast<sf_count_t (*)(
            SNDFILE *, short *, sf_count_t)>(
               PlatformFindSymbol(loaded.library, "sf_readf_short"));
         loaded.close = reinterpret_cast<int (*)(SNDFILE *)>(
            PlatformFindSymbol(loaded.library, "sf_close"));
         if (!loaded.open_virtual || !loaded.readf_short || !loaded.close)
         {
            PlatformUnloadLibrary(loaded.library);
            memset(&loaded, 0, sizeof(loaded));
         }
      }
   }
   *api = loaded;
   return api->library != NULL;
}

void WriteU16(uint8 *destination, uint16 value)
{
   destination[0] = static_cast<uint8>(value);
   destination[1] = static_cast<uint8>(value >> 8);
}

void WriteU32(uint8 *destination, uint32 value)
{
   destination[0] = static_cast<uint8>(value);
   destination[1] = static_cast<uint8>(value >> 8);
   destination[2] = static_cast<uint8>(value >> 16);
   destination[3] = static_cast<uint8>(value >> 24);
}

void *DecodeOggToWave(const void *source, uint32 sourceLen,
                      uint32 *waveLen)
{
   static bool warnedMissingDecoder;
   if (sourceLen < 4 || memcmp(source, "OggS", 4) != 0)
      return NULL;

   sSndFileApi api;
   if (!LoadSndFile(&api))
   {
      if (!warnedMissingDecoder)
      {
         Warning(("Ogg Vorbis audio requires the libsndfile runtime\n"));
         warnedMissingDecoder = true;
      }
      return NULL;
   }

   SF_VIRTUAL_IO io = { MemoryLength, MemorySeek, MemoryRead,
                        MemoryWrite, MemoryTell };
   sMemorySound memory = { static_cast<const uint8 *>(source),
                           sourceLen, 0 };
   SF_INFO info = {};
   SNDFILE *file = api.open_virtual(&io, 0x10, &info, &memory);
   if (!file || info.frames <= 0 || info.samplerate <= 0 ||
       (info.channels != 1 && info.channels != 2) ||
       static_cast<uint64_t>(info.frames) >
          (UINT32_MAX - 44u) / (static_cast<uint32>(info.channels) * 2u))
   {
      if (file)
         api.close(file);
      Warning(("Unable to decode Ogg Vorbis sound resource\n"));
      return NULL;
   }

   const uint32 frameBytes = static_cast<uint32>(info.channels) * 2u;
   const uint32 capacity = static_cast<uint32>(info.frames) * frameBytes;
   uint8 *wave = static_cast<uint8 *>(Malloc(44u + capacity));
   if (!wave)
   {
      api.close(file);
      return NULL;
   }

   sf_count_t framesRead = 0;
   while (framesRead < info.frames)
   {
      sf_count_t count = api.readf_short(
         file, reinterpret_cast<short *>(wave + 44) +
            framesRead * info.channels,
         info.frames - framesRead);
      if (count <= 0)
         break;
      framesRead += count;
   }
   api.close(file);
   if (framesRead <= 0)
   {
      Free(wave);
      Warning(("Unable to decode Ogg Vorbis sound resource\n"));
      return NULL;
   }

   const uint32 dataBytes = static_cast<uint32>(framesRead) * frameBytes;
   memcpy(wave, "RIFF", 4);
   WriteU32(wave + 4, 36u + dataBytes);
   memcpy(wave + 8, "WAVEfmt ", 8);
   WriteU32(wave + 16, 16);
   WriteU16(wave + 20, 1);
   WriteU16(wave + 22, static_cast<uint16>(info.channels));
   WriteU32(wave + 24, static_cast<uint32>(info.samplerate));
   WriteU32(wave + 28,
            static_cast<uint32>(info.samplerate) * frameBytes);
   WriteU16(wave + 32, static_cast<uint16>(frameBytes));
   WriteU16(wave + 34, 16);
   memcpy(wave + 36, "data", 4);
   WriteU32(wave + 40, dataBytes);
   *waveLen = 44u + dataBytes;
   return wave;
}

bool PrepareSoundBuffer(void *source, uint32 sourceLen, void **prepared,
                        uint32 *preparedLen, void **owned)
{
   *prepared = source;
   *preparedLen = sourceLen;
   *owned = NULL;
   if (sourceLen < 4 || memcmp(source, "OggS", 4) != 0)
      return true;
   *owned = DecodeOggToWave(source, sourceLen, preparedLen);
   if (!*owned)
      return false;
   *prepared = *owned;
   return true;
}
}

//
// endFakeSplicer - free stuff this stream was using
//
static void
endFakeSplicer( ISndSample    *pSample,
                void          *pCBData )
{
   fakeSplicerThingy *pFakeSplicer = (fakeSplicerThingy *) pCBData;

   if ( pFakeSplicer->userCB != NULL ) {
      pFakeSplicer->userCB( pSample, pFakeSplicer->pUserCBData );
   }
   pFakeSplicer->pSource->DisconnectFromPlayer();
   pFakeSplicer->pSource->Release();

   Free( pFakeSplicer->pPlaylist );
   if ( pFakeSplicer->pOwnedData != NULL ) {
      Free( pFakeSplicer->pOwnedData );
   }
   Free( pFakeSplicer );
}

//
// create a sound source which will play the data in pBuffer (size in bufferLen)
//
// endCB is the user end-of-play callback, will be called when sample is
//   done playing - this needs to be done here so we can wrap the end
//   callback with a callback which cleans up the sound source
//   and any other crud we create
//
ISndSample *
CreateMemSoundSource( ISndMixer        *pMixer,          // your mixer
                      void             *pBuffer,         // the sound data
                      uint32           bufferLen,        // sizeof the sound data
                      SndEndCallback   endCB,            // your end callback
                      void             *pEndCBData )     // data for endCB
{
   ISndSample           *pSample;
   sSndAttribs          attribs;
   uint32               totalSamples, dummy;
   ISndSource           *pSndSrc;
   uint32               *pPlaylistBase;
   void                 *pRawData;
   void                 *pPreparedBuffer;
   void                 *pOwnedData;
   char                 *pListTmp;
   uint32               preparedLen, rawDataLen, nSamples, finalDataLen;
   fakeSplicerThingy    *pFakeSplicer;
   SSPLMemSingle        *pMemSingle;
   SSPLEndList          *pEndList;

   if ( pMixer == NULL ) {
      return NULL;
   }

   if ( !PrepareSoundBuffer( pBuffer, bufferLen, &pPreparedBuffer,
                             &preparedLen, &pOwnedData ) ) {
      return NULL;
   }

   //
   // get the attribs for source from 1st resource
   //
   if ( SndCrackRezHeader( pPreparedBuffer, preparedLen, &pRawData, &rawDataLen,
                           &nSamples, &attribs )  ) {
      Warning(("error cracking header\n"));
      if ( pOwnedData != NULL ) Free( pOwnedData );
      return NULL;
   } else {
      //
      // cracked the header successfully, if ADPCM, change the
      //  sound attributes to those of the decompressed data
      //
      if ( attribs.dataType == kSndDataIMAADPCM ) {
         attribs.dataType = kSndDataPCM;
         attribs.bitsPerSample = 16;
         finalDataLen = ( attribs.numSamples << 1 ) * attribs.nChannels;
      } else {
         finalDataLen = rawDataLen;
      }
   }

   pSndSrc = SndCreateSource( &attribs );

   //
   // build the playlist
   //  The playlist is just two ops:
   //    1 - plays a memory buffer
   //    2 - ends the playlist
   //
   pPlaylistBase = (uint32 *)Malloc( sizeof(SSPLMemSingle) + sizeof(SSPLEndList) );

   pListTmp = (char *) pPlaylistBase;

   pMemSingle = (SSPLMemSingle *) pListTmp;
   pMemSingle->op = plMemSingle;
   pMemSingle->nSamples = attribs.numSamples;
   pMemSingle->pData = pPreparedBuffer;
   pListTmp += sizeof( SSPLMemSingle );

   pEndList = (SSPLEndList *) pListTmp;
   pEndList->op = plEndList;

   pSndSrc->SetPlaylist( pPlaylistBase );


   // find out total #samples in playlist
   pSndSrc->GetPositions( &dummy, &dummy, &totalSamples );

   //
   // Da Magic:  When we create a raw sample of type "Normal" (a JohnB-ism),
   //  this usually creates the DirectSoundBuffer and copies the data directly
   //  to it - but since we pass in NULL for the sound data address, it calls
   //  the stream refill callback, which in this case is sndsrc code which will read
   //  compressed data from the passed-in memory buffer, and will decompress
   //  directly to the DirectSoundBuffer.
   //
   pSample = ISndMixer_CreateRawSample( pMixer, kSndSampleNormal, NULL,
                                        finalDataLen, totalSamples, &attribs );
   if ( pSample == NULL ) {

      pSndSrc->Release();
      Free( pPlaylistBase );
      if ( pOwnedData != NULL ) Free( pOwnedData );
      pSndSrc = NULL;

   } else {

      // wrap the user end callback in our own
      pFakeSplicer = (fakeSplicerThingy *) Malloc( sizeof(fakeSplicerThingy) );
      pFakeSplicer->userCB = endCB;
      pFakeSplicer->pUserCBData = pEndCBData;
      pFakeSplicer->pSource = pSndSrc;
      pFakeSplicer->pPlaylist = pPlaylistBase;
      pFakeSplicer->pOwnedData = pOwnedData;

      pSample->RegisterEndCallback( endFakeSplicer, pFakeSplicer );
      pSample->SetData((intptr_t)pFakeSplicer);

      // connect the sound source to the sample
      pSndSrc->ConnectToPlayer( pSample );
   }

   return pSample;
}


// ppSndSrc    a returned handle so that you can diddle sound source
// nLoops      # of loops, 0 == infinite
// loopCB      the loop callback (NULL is ok)
// pLoopCBData ptr to data which will be available to the loop callback

// to do a semi-infinite loop which exits at next loop end:
//  pSndSrc->SetGate( 0, 1 );   // set loop counter 0 to 1

//
// create a sound source which will play the data in pBuffer (size in bufferLen)
//  a specified number of times
//
// endCB is the user end-of-play callback, will be called when sample is
//   done playing - this needs to be done here so we can wrap the end
//   callback with a callback which cleans up the sound source
//   and any other crud we create
//
ISndSample *
CreateMemSoundSourceLooped( ISndMixer           *pMixer,          // your mixer
                            void                *pBuffer,         // the sound data
                            uint32              bufferLen,        // sizeof the sound data
                            SndEndCallback      endCB,            // your end callback
                            void                *pEndCBData,      // data for endCB
                            ISndSource          **ppSndSrc,       // handle to the sound source
                            int                 nLoops,           // # of times to play it
                            SndPlaylistCallback loopCB,           // your loop callback
                            void                *pLoopCBData )    // data for loopCB
{
   ISndSample           *pSample;
   sSndAttribs          attribs;
   uint32               totalSamples, dummy;
   ISndSource           *pSndSrc;
   uint32               *pPlaylistBase;
   void                 *pRawData;
   void                 *pPreparedBuffer;
   void                 *pOwnedData;
   char                 *pListTmp;
   uint32               preparedLen, rawDataLen, nSamples, finalDataLen;
   fakeSplicerThingy    *pFakeSplicer;
   SSPLMemSingle        *pMemSingle;
   SSPLEndList          *pEndList;
   SSPLLabel            *pLabel;
   SSPLBranch           *pBranch;
   SSPLCallback         *pCallback;
   SSPLSetGate          *pSetGate;
   uint32               *pCallbackData;

   if ( ppSndSrc != NULL ) {
      *ppSndSrc = NULL;
   }

   if ( pMixer == NULL ) {
      return NULL;
   }

   if ( !PrepareSoundBuffer( pBuffer, bufferLen, &pPreparedBuffer,
                             &preparedLen, &pOwnedData ) ) {
      return NULL;
   }

   //
   // get the attribs for source from 1st resource
   //
   if ( SndCrackRezHeader( pPreparedBuffer, preparedLen, &pRawData, &rawDataLen,
                           &nSamples, &attribs )  ) {
      Warning(("error cracking header\n"));
      if ( pOwnedData != NULL ) Free( pOwnedData );
      return NULL;
   } else {
      //
      // cracked the header successfully, if ADPCM, change the
      //  sound attributes to those of the decompressed data
      //
      if ( attribs.dataType == kSndDataIMAADPCM ) {
         attribs.dataType = kSndDataPCM;
         attribs.bitsPerSample = 16;
         finalDataLen = ( attribs.numSamples << 1 ) * attribs.nChannels;
      } else {
         finalDataLen = rawDataLen;
      }
   }

   pSndSrc = SndCreateSource( &attribs );


   //
   // build the playlist
   //
   //  The playlist is just five ops:
   //    1 - loop start label
   //    2 - plays a memory buffer
   //    3 - does app loop callback
   //    4 - branch to label
   //    3 - ends the playlist
   pPlaylistBase = (uint32 *)Malloc( sizeof(SSPLSetGate) +
                                     sizeof(SSPLLabel) +
                                     sizeof(SSPLMemSingle) +
                                     sizeof(SSPLCallback) + sizeof(void *) +
                                     sizeof(SSPLBranch) +
                                     sizeof(SSPLEndList)  + 32 );
         
   pListTmp = (char *) pPlaylistBase;

   // set # loops
   // this should just be pSndSrc->SetGate( 0, nLoops ), but that's broken!
   pSetGate = (SSPLSetGate *) pListTmp;
   pSetGate->op = plSetGate;
   pSetGate->gateNum = 0;
   pSetGate->gateValue = nLoops;
   pListTmp += sizeof( SSPLSetGate );

   pLabel = (SSPLLabel *) pListTmp;
   pLabel->op = plLabel;
   pLabel->labelNum = 0;
   pListTmp += sizeof( SSPLLabel );

   pMemSingle = (SSPLMemSingle *) pListTmp;
   pMemSingle->op = plMemSingle;
   pMemSingle->nSamples = attribs.numSamples;
   pMemSingle->pData = pPreparedBuffer;
   pListTmp += sizeof( SSPLMemSingle );

   pCallback = (SSPLCallback *) pListTmp;
   pCallback->op = plCallback;
   pCallback->func = loopCB;
   pCallback->nArgs = sizeof(void *) / sizeof(uint32);
   pListTmp += sizeof( SSPLCallback );
   // add the single argument - the app callback data ptr
   pCallbackData = (uint32 *)pListTmp;
   memcpy(pCallbackData, &pLoopCBData, sizeof(pLoopCBData));
   pListTmp += sizeof(pLoopCBData);

   // branch to label 0 if gate var 0 is not 0 after decrementing
   pBranch = (SSPLBranch *) pListTmp;
   pBranch->op = plBranch;
   pBranch->labelNum = 0;
   pBranch->gateNum = 0;
   pBranch->indirect = 0;
   pBranch->branchType = SSPLBTDecrementBranchNotZero;
   pListTmp += sizeof( SSPLBranch );

   pEndList = (SSPLEndList *) pListTmp;
   pEndList->op = plEndList;

   pSndSrc->SetPlaylist( pPlaylistBase );

   // set loop counter (gate var 0) 
   //pSndSrc->SetGate( 0, nLoops );

   // find out total #samples in playlist
   pSndSrc->GetPositions( &dummy, &dummy, &totalSamples );

   //
   // Da Magic:  When we create a raw sample of type "Normal" (a JohnB-ism),
   //  this usually creates the DirectSoundBuffer and copies the data directly
   //  to it - but since we pass in NULL for the sound data address, it calls
   //  the stream refill callback, which in this case is sndsrc code which will read
   //  compressed data from the passed-in memory buffer, and will decompress
   //  directly to the DirectSoundBuffer.
   //
   pSample = ISndMixer_CreateRawSample( pMixer, kSndSampleStream, NULL,
                                        finalDataLen, totalSamples, &attribs );
   if ( pSample == NULL ) {

      pSndSrc->Release();
      Free( pPlaylistBase );
      if ( pOwnedData != NULL ) Free( pOwnedData );
      pSndSrc = NULL;

   } else {

      // wrap the user end callback in our own
      pFakeSplicer = (fakeSplicerThingy *) Malloc( sizeof(fakeSplicerThingy) );
      pFakeSplicer->userCB = endCB;
      pFakeSplicer->pUserCBData = pEndCBData;
      pFakeSplicer->pSource = pSndSrc;
      pFakeSplicer->pPlaylist = pPlaylistBase;
      pFakeSplicer->pOwnedData = pOwnedData;

      pSample->RegisterEndCallback( endFakeSplicer, pFakeSplicer );
      pSample->SetData((intptr_t)pFakeSplicer);

      // connect the sound source to the sample
      pSndSrc->ConnectToPlayer( pSample );
   }

   // give app handle to the sound source
   if ( ppSndSrc != NULL ) {
      *ppSndSrc = pSndSrc;
   }

   return pSample;
}




// ppSndSrc    a returned handle so that you can diddle sound source
// nLoops      # of loops, 0 == infinite
// loopCB      the loop callback (NULL is ok)
// pLoopCBData ptr to data which will be available to the loop callback

// to do a semi-infinite loop which exits at next loop end:
//  pSndSrc->SetGate( 0, 1 );   // set loop counter 0 to 1

//
// create a sound source which will play the data in pBuffer (size in bufferLen)
//  a specified number of times
//
// endCB is the user end-of-play callback, will be called when sample is
//   done playing - this needs to be done here so we can wrap the end
//   callback with a callback which cleans up the sound source
//   and any other crud we create
//
ISndSample *
CreateNRezStreamedSoundSource( ISndMixer           *pMixer,          // your mixer
                               IRes                *pRez,
                               char                **ppStreamBuffer, // the stream buffer
//                            uint32              bufferLen,         // sizeof the sound data
                               SndEndCallback      endCB,            // your end callback
                               void                *pEndCBData,      // data for endCB
                               ISndSource          **ppSndSrc )       // handle to the sound source
//                            int                 nLoops,           // # of times to play it
//                            SndPlaylistCallback loopCB,           // your loop callback
//                            void                *pLoopCBData )    // data for loopCB
{
   ISndSample           *pSample;
   sSndAttribs          attribs;
   uint32               totalSamples, dummy;
   ISndSource           *pSndSrc;
   uint32               *pPlaylistBase;
   void                 *pRawData;
   char                 *pListTmp;
   uint32               rawDataLen, nSamples;
   fakeSplicerThingy    *pFakeSplicer;
   SSPLNRezSingle       *pNRezSingle;
   SSPLEndList          *pEndList;
   /*
   SSPLEndList          *pEndList;
   SSPLLabel            *pLabel;
   SSPLBranch           *pBranch;
   SSPLCallback         *pCallback;
   SSPLSetGate          *pSetGate;
   long                 *pWoof;
   */

   if ( ppSndSrc != NULL )
   {
      *ppSndSrc = NULL;
   }

   if ( pMixer == NULL )
   {
      return NULL;
   }

   //
   // get the attribs for source from 1st resource
   //

   // Hack to extract header info from the .wav.  Eventually I want to replace "crackheader"
   // raw memory .wav processing with IStream stuff so that memory/file/nres stuff is all
   // handled through a common interface (and so looping info can be extracted without
   // reading in a whole file).
   int len = pRez->GetSize();
   char tmpBuff[256];
   int bytesToRead = ( len < sizeof(tmpBuff) ) ? len : sizeof(tmpBuff);
   //   int nSamples;
   pRez->ExtractPartial( 0, bytesToRead - 1, tmpBuff );
   if ( SndCrackRezHeader( tmpBuff, bytesToRead,
                               &pRawData, &rawDataLen,
                               &nSamples, &attribs ) )
   {
      Warning(("error cracking header\n"));
      return NULL;
   } 
   else
   {
      //
      // cracked the header successfully, if ADPCM, change the
      //  sound attributes to those of the decompressed data
      //
      if ( attribs.dataType == kSndDataIMAADPCM )
      {
         attribs.dataType = kSndDataPCM;
         attribs.bitsPerSample = 16;
      }
   }

   pSndSrc = SndCreateSource( &attribs );


   //
   // build the playlist
   //
   //  The playlist is just two ops:
   //    1 - plays an nrez buffer
   //    2 - ends the playlist
   pPlaylistBase = (uint32 *)Malloc( sizeof(SSPLNRezSingle) +
                                     sizeof(SSPLEndList) +
                                     32 );
   pListTmp = (char *) pPlaylistBase;

   pNRezSingle = (SSPLNRezSingle *) pListTmp;
   pNRezSingle->op = plNRezSingle;
   pNRezSingle->pRes = pRez;
   pListTmp += sizeof ( SSPLNRezSingle );

   pEndList = (SSPLEndList *) pListTmp;
   pEndList->op = plEndList;

   pSndSrc->SetPlaylist( pPlaylistBase );

   // find out total #samples in playlist
   pSndSrc->GetPositions( &dummy, &dummy, &totalSamples );

   //
   // Da Magic:  When we create a raw sample of type "Normal" (a JohnB-ism),
   //  this usually creates the DirectSoundBuffer and copies the data directly
   //  to it - but since we pass in NULL for the sound data address, it calls
   //  the stream refill callback, which in this case is sndsrc code which will read
   //  compressed data from the passed-in memory buffer, and will decompress
   //  directly to the DirectSoundBuffer.
   //
   int streamBufSize = (int)(attribs.sampleRate * kAUXSND_SecondsOfBuffer * (attribs.bitsPerSample >> 3));
   *ppStreamBuffer = new char[streamBufSize];
   pSample = ISndMixer_CreateRawSample( pMixer, kSndSampleStream, *ppStreamBuffer,
                                        streamBufSize, totalSamples, &attribs );
   if ( pSample == NULL )
   {
      pSndSrc->Release();
      Free( pPlaylistBase );
      pSndSrc = NULL;
   }
   else
   {
      // wrap the user end callback in our own
      pFakeSplicer = (fakeSplicerThingy *) Malloc( sizeof(fakeSplicerThingy) );
      pFakeSplicer->userCB = endCB;
      pFakeSplicer->pUserCBData = pEndCBData;
      pFakeSplicer->pSource = pSndSrc;
      pFakeSplicer->pPlaylist = pPlaylistBase;
      pFakeSplicer->pOwnedData = NULL;

      pSample->RegisterEndCallback( endFakeSplicer, pFakeSplicer );
      pSample->SetData((intptr_t)pFakeSplicer);

      // connect the sound source to the sample
      pSndSrc->ConnectToPlayer( pSample );
   }

   // give app handle to the sound source
   if ( ppSndSrc != NULL ) {
      *ppSndSrc = pSndSrc;
   }

   return pSample;
}

struct sSongSrcSegment
{
   SSPLLabel         label;
   SSPLCallback      callback;
   uint32            cbData[sizeof(void *) / sizeof(uint32)];
   SSPLNRezSingle    nresSingle;
   SSPLBranch        loopBranch;
   SSPLBranch        segmentBranch;
};


ISndSample*
CreateSongSoundSource ( ISndMixer           *pMixer,
                        uint32              numSegments,
                        IRes*               *pResArray,     // An array of resources, size is numSegments.
                        char                **ppStreamBuffer,
                        SndEndCallback      endCB,
                        void                *pEndCBData,
                        ISndSource          **ppSndSrc,
                        SndPlaylistCallback segmentCB,         // segment callback
                        void                *pSegmentCBData )  // data for CB
{
   ISndSample           *pSample;
   sSndAttribs          attribs;
   ISndSource           *pSndSrc;
   uint32               *pPlaylistBase;
   uint32               totalSamples, dummy;
   void                 *pRawData;
   uint32               rawDataLen, nSamples;
   sSongSrcSegment      *pCurrSeg;
   SSPLCallback         *pCallback;
   uint32               *pCallbackData;
   SSPLLabel            *pLabel;
   SSPLNRezSingle       *pNRezSingle;
   SSPLBranch           *pBranch;
   SSPLEndList          *pEndList;
   fakeSplicerThingy    *pFakeSplicer;

//   SSPLCallback         *pCallback; Hmmm... callbacks....

   //
   // get the attribs for source from 1st resource
   //
   // Hack to extract header info from the .wav.  Eventually I want to replace "crackheader"
   // raw memory .wav processing with IStream stuff so that memory/file/nres stuff is all
   // handled through a common interface (and so looping info, which usually resides after
   // sample data, can be easily extracted without reading in a whole file).
   int len = pResArray[0]->GetSize();
   char tmpBuff[256];
   int bytesToRead = ( len < sizeof(tmpBuff) ) ? len : sizeof(tmpBuff);
   //   int nSamples;
   pResArray[0]->ExtractPartial( 0, bytesToRead - 1, tmpBuff );
   if ( SndCrackRezHeader( tmpBuff, bytesToRead,
                               &pRawData, &rawDataLen,
                               &nSamples, &attribs ) )
   {
      Warning(("error cracking header\n"));
      return NULL;
   } 
   else
   {
      //
      // cracked the header successfully, if ADPCM, change the
      //  sound attributes to those of the decompressed data
      //
      if ( attribs.dataType == kSndDataIMAADPCM )
      {
         attribs.dataType = kSndDataPCM;
         attribs.bitsPerSample = 16;
      }
   }

   // Create the sound source.  The "<<1" is because each segment has 2 gates:
   // one for looping and one for branching to other segments.
   pSndSrc = SndCreateSourceEx( &attribs,
                                numSegments + 1,        // Number of labels (+1 for "end song" label)
                                3 );                    // current seg, next seg, loop count.
   //                                (numSegments << 1) );   // Number of branches (2 branches per segment)

   // Allocate the play list.
   pPlaylistBase = (uint32 *)Malloc( (sizeof(sSongSrcSegment) * numSegments) +
                                     sizeof(SSPLEndList) +
                                     sizeof (SSPLLabel) +
                                     32 );

   // Point at the start of the play list.
   //   pListTmp = (char *) pPlaylistBase;
   pCurrSeg = (sSongSrcSegment*) pPlaylistBase;

   // Fill in the segments of the play list.
   for (uint32 i = 0; i < numSegments; i++)
   {
      // Add label for this section.
      pLabel                  = &(pCurrSeg->label);
      pLabel->op              = plLabel;
      pLabel->labelNum        = i;

      // Add the callback
      pCallback                 = &(pCurrSeg->callback);
      pCallback->op             = plCallback;
      pCallback->func           = segmentCB;
      pCallback->nArgs          = sizeof(void *) / sizeof(uint32);

      // Add the callback data.
      pCallbackData             = pCurrSeg->cbData;
      memcpy(pCallbackData, &pSegmentCBData, sizeof(pSegmentCBData));

      // Add sample.
      pNRezSingle             = &(pCurrSeg->nresSingle);
      pNRezSingle->op         = plNRezSingle;
      pNRezSingle->pRes       = pResArray[i];

      // Add loop branch.
      pBranch                 = &(pCurrSeg->loopBranch);
      pBranch->op             = plBranch;
      pBranch->labelNum       = i;
      pBranch->gateNum        = 2; // 2 is the loopCount gate.
      //      pBranch->gateNum        = i << 1;
      pBranch->indirect       = 0;
      pBranch->branchType     = SSPLBTDecrementBranchNotZero;

      // Add goto branch.
      // Branch to the label whose value is stored in the gate referenced by pBranch->labelNum
      // i.e. "branch gate[pBranch->labelNum]"
      pBranch                 = &(pCurrSeg->segmentBranch);
      pBranch->op             = plBranch;
      pBranch->labelNum       = 1; // 1 is the next segment gate.
      //      pBranch->labelNum       = (i << 1) + 1;
      pBranch->gateNum        = 0; // Unused on branch unconditional.
      pBranch->indirect       = 1;
      pBranch->branchType     = SSPLBTBranch;

      pCurrSeg++;
   }
   
   // Add "end of song" label.
   pLabel               = (SSPLLabel*) pCurrSeg;
   pLabel->op           = plLabel;
   pLabel->labelNum     = numSegments;
   pLabel++;

   // Add "end of playlist".
   pEndList             = (SSPLEndList*) pLabel;
   pEndList->op         = plEndList;

   pSndSrc->SetPlaylist( pPlaylistBase );

      // find out total #samples in playlist
   pSndSrc->GetPositions( &dummy, &dummy, &totalSamples );

   //
   // Da Magic:  When we create a raw sample of type "Normal" (a JohnB-ism),
   //  this usually creates the DirectSoundBuffer and copies the data directly
   //  to it - but since we pass in NULL for the sound data address, it calls
   //  the stream refill callback, which in this case is sndsrc code which will read
   //  compressed data from the passed-in memory buffer, and will decompress
   //  directly to the DirectSoundBuffer.
   //
   int streamBufSize = (int)(attribs.sampleRate * kAUXSND_SecondsOfBuffer * (attribs.bitsPerSample >> 3));
   *ppStreamBuffer = new char[streamBufSize];
   pSample = ISndMixer_CreateRawSample( pMixer, kSndSampleStream, *ppStreamBuffer,
                                        streamBufSize, totalSamples, &attribs );
   if ( pSample == NULL )
   {
      pSndSrc->Release();
      Free( pPlaylistBase );
      pSndSrc = NULL;
   }
   else
   {
      // wrap the user end callback in our own
      pFakeSplicer = (fakeSplicerThingy *) Malloc( sizeof(fakeSplicerThingy) );
      pFakeSplicer->userCB = endCB;
      pFakeSplicer->pUserCBData = pEndCBData;
      pFakeSplicer->pSource = pSndSrc;
      pFakeSplicer->pPlaylist = pPlaylistBase;
      pFakeSplicer->pOwnedData = NULL;

      pSample->RegisterEndCallback( endFakeSplicer, pFakeSplicer );
      pSample->SetData((intptr_t)pFakeSplicer);

      // connect the sound source to the sample
      pSndSrc->ConnectToPlayer( pSample );
   }

   // give app handle to the sound source
   if ( ppSndSrc != NULL ) {
      *ppSndSrc = pSndSrc;
   }

   return pSample;
}
