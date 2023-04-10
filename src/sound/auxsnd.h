/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/auxsnd.h,v 1.6 1999/03/18 14:33:59 mwhite Exp $
// memory sound source stuff from Pat

#pragma once
#ifndef __AUXSND_H
#define __AUXSND_H

EXTERN ISndSample *
   CreateMemSoundSource( ISndMixer        *pMixer,          // your mixer
                         void             *pBuffer,         // the sound data
                         uint32           bufferLen,        // sizeof the sound data
                         SndEndCallback   endCB,            // your end callback
                         void             *pEndCBData );    // data for endCB

EXTERN ISndSample *
CreateMemSoundSourceLooped(ISndMixer           *pMixer,    // your mixer
                           void                *pBuffer,   // the sound data
                           uint32              bufferLen,  // sizeof pBuffer
                           SndEndCallback      endCB,      // end callback
                           void                *pEndCBData,// data for endCB
                           ISndSource          **ppSndSrc, // handle
                           int                 nLoops,     // times to play
                           SndPlaylistCallback loopCB,     // loop callback
                           void                *pLoopCBData );// for loopCB

ISndSample *
CreateNRezStreamedSoundSource( ISndMixer           *pMixer,          // your mixer
                               IRes                *pRez,
                               char                **ppStreamBuffer, // the stream buffer
                               SndEndCallback      endCB,            // your end callback
                               void                *pEndCBData,      // data for endCB
                               ISndSource          **ppSndSrc );       // handle to the sound source

ISndSample*
CreateSongSoundSource ( ISndMixer           *pMixer,
                        uint32              numSegments,
                        IRes*               *pResArray,     // An array of resources, size is numSegments.
                        char                **ppStreamBuffer,
                        SndEndCallback      endCB,
                        void                *pEndCBData,
                        ISndSource          **ppSndSrc,
                        SndPlaylistCallback segmentCB,         // segment callback
                        void                *pSegmentCBData ); // data for CB

#endif // __AUXSND_H
