////////////////////////////////////////////////////////////////////////
//
// (c) 1996 Looking Glass Technologies Inc.
// Pat McElhatton
//
// Module name: Sound utility definitions
// File name: sndutil.h
//
// Description: Utilities for making lgsound easier to use
//
////////////////////////////////////////////////////////////////////////
#ifndef __SNDUTIL_H__
#define __SNDUTIL_H__

#include <lgsound.h>
#include <res.h>
#include <sndfmt.h>

#ifdef __cplusplus
extern "C" {
#endif


//
// this describes one sound segment, multiple sound segments are connected by
//  the splicer
//
typedef struct _SndFileSegment {
   char     *fileName;     // name of sound file
   int32    offset;        // in milliseconds, positive means silent gap, negative is overlap
} SndFileSegment;

//
// this describes one sound segment, multiple sound segments are connected by
//  the splicer
//
typedef struct _SndRezSegment {
   Id       rezId;         // resource Id for segment
   int32    offset;        // in milliseconds, positive means silent gap, negative is overlap
} SndRezSegment;


//
// this structure contains cue info extracted from a wave file
//


typedef struct _SndCue {
   uint32      id;            // cue id
   uint32      samplePos;     // position of cue
   char        *pName;        // name of cue - null-terminated string
} SndCue;

typedef struct _SndCueSet {
   SndCue      *pCues;        // array of cues
   uint32      maxCues;       // max number of cues
   uint32      numCues;       // number of cues @pCues
   uint32      fileCues;      // number of cues in file
   char        *pNames;       // cue name buffer
   uint32      maxNameBytes;  // max length of name buffer
   uint32      numNameBytes;  // used part of name buffer
} SndCueSet;


//
// this is a generic data fetcher, which apps supply to the sound
//  utilities.  It allows the same sound utilities to be used with
//  resources, files, and memory images.
// The position parameter is the byte position relative to the start
// of the sound resource/file/image
//
typedef       void * (*SndFetchFunction)(void    *pCallbackData,
                                         void    *pDataDestination,
                                         uint32  nBytes,
                                         uint32  position );

//
// play a sound resource in streaming mode.
//
extern ISndSample *
CreateSoundRezStreamer( ISndMixer      *pMixer,
                        Id             rezId,
                        void           *pBuffer,
                        uint32         ringBufferLen,
                        SndEndCallback endCB,
                        void           *pEndCBData );

extern ISndSample *
CreateSoundFileStreamer( ISndMixer        *pMixer,
                         char             *fname,
                         void             *pBuffer,
                         uint32           ringBufferLen,
                         SndEndCallback   endCB,
                         void             *pEndCBData );

ISndSample *
CreateSoundMemoryStreamer( ISndMixer        *pMixer,
                           void             *pAudioData,
                           uint32            audioDataLen,
                           void             *pBuffer,
                           uint32           bufferLen,
                           SndEndCallback   endCB,
                           void             *pEndCBData );
//
// play a sound resource in one-shot mode.
//
extern ISndSample *
CreateSoundRezOneShot( ISndMixer      *pMixer,
                       Id             rezId,
                       SndEndCallback endCB,
                       void           *pEndCBData );

extern ISndSample *
CreateSoundFileOneShot( ISndMixer        *pMixer,
                        char             *fname,
                        SndEndCallback   endCB,
                        void             *pEndCBData );

//
// play a sequence of sound resources
//
extern ISndSample *
CreateSoundRezSplicer( ISndMixer       *pMixer,
                       SndRezSegment   *pRezList,
                       uint32          numRez,
                       void            *pBuffer,
                       uint32          ringBufferLen,
                       SndEndCallback  endCB,
                       void            *pEndCBData );

extern ISndSample *
CreateSoundFileSplicer( ISndMixer       *pMixer,
                        SndFileSegment  *pRezList,
                        uint32          numRez,
                        void            *pBuffer,
                        uint32          ringBufferLen,
                        SndEndCallback  endCB,
                        void            *pEndCBData );

#ifdef NOT_DONE_YET
extern ISndSample *
CreateSoundOneShot( ISndMixer        *pMixer,
                    SndFetchFunction getData,
                    void             *pGetData,
                    SndEndCallback   endCB,
                    void             *pEndCBData );



extern ISndSample *
CreateSoundRezOneShot( ISndMixer        *pMixer,
                       Id               rezId,
                       SndEndCallback   endCB,
                       void             *pEndCBData );
#endif

#ifdef __cplusplus
};
#endif

#endif