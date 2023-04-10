////////////////////////////////////////////////////////////////////////
//
// (c) 1996 Looking Glass Technologies Inc.
// Pat McElhatton
//
// Module name: Sound resource streamer
// File name: sndrezst.c
//
// Description: Play a sound resource in streaming mode given its rez id
//
////////////////////////////////////////////////////////////////////////
#ifndef __SNDREZST_H__
#define __SNDREZST_H__

#include <windows.h>
#include <lgsound.h>
#include <res.h>
#include <sndfmt.h>

typedef struct _SndSegment {
   Id       rezId;         // resource Id for 
   int32    offset;        // in milliseconds, positive means silent gap, negative is overlap
} SndSegment;


extern ISndSample *
CreateSoundRezStreamer( ISndMixer      *pMixer,
                        Id             rezId,
                        uint32         ringBufferLen,
                        SndEndCallback endCB,
                        void           *pEndCBData );

extern ISndSample *
CreateSoundRezSplicer( ISndMixer       *pMixer,
                       SndSegment      *pRezList,
                       uint32          numRez,
                       uint32          ringBufferLen,
                       SndEndCallback  endCB,
                       void            *pEndCBData );

#endif
