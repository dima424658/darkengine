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

//
// internal struct created by CreateSoundRezStreamer,
//  automatically destroyed when stream play finishes
//
typedef struct _sndOneShotStuff {
   uint32            playOffset;       // byte offset into resource to get data from
   uint32            endOffset;        // offset into resource of end of data
   SndEndCallback    endCB;            // app end-of-sample callback
   void              *pEndCBData;      // app end-of-sample callback data
   uint32            extras[4];        // stuff for specialized streamers to use
   SndLoadFunction   getData;          // routine for actually getting data from source
   uint32            dataOffset;       // offset of start of audio data
   int               bytesPerSample;   // data bytes per sample
} sndOneShotStuff;


//
// play a sound in streaming mode.
//
extern ISndSample *
CreateSoundOneShot( ISndMixer          *pMixer,
                    SndLoadFunction    getData,
                    sndOneShotStuff    *pInfo,
                    uint32             soundRezLen,
                    SndEndCallback     endCB,
                    void               *pEndCBData );

