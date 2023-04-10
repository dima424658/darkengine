////////////////////////////////////////////////////////////////////////
// $Header: x:/prj/tech/libsrc/sndutil/RCS/splicer.h 1.4 1997/02/04 18:38:50 PATMAC Exp $
//
// (c) 1996 Looking Glass Technologies Inc.
// Pat McElhatton
//
// Module name: Sound splicer internal definitions
// File name: splicer.h
//
// Description: Internal definitions common to all sound splicers
//
////////////////////////////////////////////////////////////////////////

//
// possible states of a splicer
//
typedef enum _sndSplicerState {
   kSpliceEmpty,           // no samples left, open next segment
   kSpliceSingle,          // take samples from current segment
   kSpliceDual,            // mix samples from current & next segments
   kSpliceSilence          // silent gap following a segment
} sndSplicerState;


//
// internal version of user sound segment list
//
typedef struct _sndSegInternal {
   int32             offset;        // src start offset of seg 1
   int32             endGap;        // in bytes, silence or overlap at end of segment
   uint32            numBytes;      // bytes before next state change
   uint32            startSample;   // sample position of seg 1st sample
   uint32            endSample;     // sample position of seg last sample
   uint32            extras[2];     // data for use of specialized splicers
   BOOL              loopMe;        // TRUE if this is an infinite loop
} sndSegInternal;


//
// internal struct created by CreateSoundRezSplicer,
//  automatically destroyed when stream play finishes
//
typedef struct _sndSplicerStuff {
   sndSplicerState   state;         // splicer state
   void              *pBuffer;      // stream ring buffer
   uint32            bufferLen;     // ring buffer bytes
   void              *pTmpBuffer;   // tmp buffer used while mixing
   uint32            tmpBufferLen;  // tmp buffer bytes
   sndSegInternal    *pSeg;         // sound segment currently playing
   sndSegInternal    *pSeg0;        // ptr to 1st segment
   uint32            seg1Offset;    // byte offset into resource to get data from
   uint32            seg2Offset;    // byte offset into 2nd resource to get data from
   SndEndCallback    endCB;         // app end-of-sample callback
   void              *pEndCBData;   // app end-of-sample callback data
   uint32            bitsPerSample; // 8 or 16 bit data?
   uint32            bytesAvail;    // bytes left until next state change
   SndFetchFunction  getData;       // gets data from resource/file/memory
   uint32            numSegs;       // total segments in splice
   sndSegInternal    *pSegN;        // ptr to just after last segment

   //
   // NOTE: the getData routine will be called with nBytes==0 when the
   // splicer is finished with the current segment.  The getData routine
   // should do any cleanup/deallocation/closing it needs to do for the
   // segment at this time
   //
} sndSplicerStuff;

// size of temporary buffer used while splicer is mixing 2 segments
#define TMP_BUFFER_LEN 2048


//
// the REAL splicer!
//
extern ISndSample *
CreateSoundSplicer( ISndMixer          *pMixer,
                    SndFetchFunction   getData,
                    sndSegInternal     *pSegList,
                    uint32             numSegs,
                    void               *pBuffer,
                    uint32             ringBufferLen,
                    SndEndCallback     endCB,
                    void               *pEndCBData );
