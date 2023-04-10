////////////////////////////////////////////////////////////////////////
//
// (c) 1996 Looking Glass Technologies Inc.
// Pat McElhatton
//
// Module name: Sound streamer internal definitions
// File name: streamer.h
//
// Description: Internal definitions common to all sound streamers
//
////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif

typedef void *  (*SndStreamLoader)(void *pCBData, void *pDst, uint32);

//
// internal struct created by CreateSoundRezStreamer,
//  automatically destroyed when stream play finishes
//
typedef struct _sndStreamStuff {
   void              *pBuffer;         // stream ring buffer
   uint32            bufferLen;        // ring buffer bytes
   uint32            playOffset;       // byte offset into resource to get data from
   uint32            endOffset;        // offset into resource of end of data
   SndEndCallback    endCB;            // app end-of-sample callback
   void              *pEndCBData;      // app end-of-sample callback data
   SndLoadFunction   getData;          // gets data from resource/file/memory
   uint32            extras[4];        // stuff for specialized streamers to use
   uint32            outBytesLeft;     // bytes of output left
   sSndAttribs       attribs;          // attributes of sound data
   char              *pInData;         // input buffer for decompression
   void              *pTmpBuffer;      // output buffer for decompression
   uint32            decompOffset;     // offset into decompression buffer
   uint32            decompState;      // decompressor state
   ISndSample        *pSample;         // back ptr to our owner
   uint32            numSamples;       // total samples
   uint32            skipNSamples;     // number of output samples to skip (ADPCM seeking)
   uint32            dataOffset;       // #bytes in rez/file before audio data
} sndStreamStuff;


//
// play a sound in streaming mode.
//
extern ISndSample *
CreateSoundStreamer( ISndMixer         *pMixer,
                     SndLoadFunction   getData,
                     sndStreamStuff    *pInfo,
                     uint32            dataLen,
                     void              *pBuffer,
                     uint32            bufferLen,
                     SndEndCallback    endCB,
                     void              *pEndCBData );

#ifdef __cplusplus
};
#endif

