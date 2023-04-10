////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/sndsrc/RCS/playlist.h $
// $Author: PATMAC $
// $Date: 1998/01/03 00:51:32 $
// $Revision: 1.3 $
//
// (c) 1997 Looking Glass Technologies Inc.
// Pat McElhatton
//
// Module name: Playlist Interpreter Definitions
// File name: playlist.h
//
// Description: sound source play list definitions
//
////////////////////////////////////////////////////////////////////////

#include <sndsrc.h>

typedef enum _SSPlaylistOps {
   plNone            = 0,
   plEndList         = 1,
   plSilence         = 2,
   plRezSingle       = 3,
   plFileSingle      = 4,
   plMemSingle       = 5,
   plRezDual         = 6,
   plFileDual        = 7,
   plMemDual         = 8,
   plCallback        = 9,
   plLabel           = 10,
   plBranch          = 11,
   plSetGate         = 12,
   plRawMemSingle    = 13,
   plRawMemDual      = 14,
   plLASTOp          = 15     // Must be highest valued enum!
} SSPlaylistOps;


//#define kSndPlayListFlag

// end of playlist
typedef struct _SSPLEndList {
   uint32      op;
} SSPLEndList;


// silence
typedef struct _SSPLSilence {
   uint32      op;
   uint32      nSamples;
} SSPLSilence;


// single resource
typedef struct _SSPLRezSingle {
   uint32      op;
   uint32      nSamples;      // LIBRARY WILL FILL IN
   uint32      off;           // LIBRARY WILL FILL IN
   uint32      format1;       // LIBRARY WILL FILL IN
   uint32      format2;       // LIBRARY WILL FILL IN
   uint32      id;
} SSPLRezSingle;


// single file
typedef struct _SSPLFileSingle {
   uint32      op;
   uint32      nSamples;      // LIBRARY WILL FILL IN
   uint32      off;           // LIBRARY WILL FILL IN
   uint32      format1;       // LIBRARY WILL FILL IN
   uint32      format2;       // LIBRARY WILL FILL IN
   char        name[32];
} SSPLFileSingle;


// single memory buffer, with attribs in header
typedef struct _SSPLMemSingle {
   uint32      op;
   uint32      nSamples;
   uint32      off;           // LIBRARY WILL FILL IN
   uint32      format1;       // LIBRARY WILL FILL IN
   uint32      format2;       // LIBRARY WILL FILL IN
   void        *pData;
} SSPLMemSingle;


// single memory buffer of raw data, with no header,
// with same attributes as sound source
typedef struct _SSPLRawMemSingle {
   uint32      op;
   uint32      nSamples;
   void        *pData;
} SSPLRawMemSingle;


// resource dual (splice)
typedef struct _SSPLRezDual {
   uint32      op;
   uint32      nSamples;
   uint32      off;           // LIBRARY WILL FILL IN
   uint32      format1;       // LIBRARY WILL FILL IN
   uint32      format2;       // LIBRARY WILL FILL IN
   uint32      id;            // LIBRARY WILL FILL IN
} SSPLRezDual;


// file dual (splice)
typedef struct _SSPLFileDual {
   uint32      op;
   uint32      nSamples;
   uint32      off;           // LIBRARY WILL FILL IN
   uint32      format1;       // LIBRARY WILL FILL IN
   uint32      format2;       // LIBRARY WILL FILL IN
   char        name[32];
} SSPLFileDual;


// dual memory buffer, with attribs in header
typedef struct _SSPLMemDual {
   uint32      op;
   uint32      nSamples;
   uint32      off;           // LIBRARY WILL FILL IN
   uint32      format1;       // LIBRARY WILL FILL IN
   uint32      format2;       // LIBRARY WILL FILL IN
   void        *pData;
} SSPLMemDual;

// dual memory buffer of raw data, with no header,
// with same attributes as sound source
typedef struct _SSPLRawMemDual {
   uint32      op;
   uint32      nSamples;
   void        *pData;
} SSPLRawMemDual;


typedef void (*SndPlaylistCallback)(ISndSource *, uint32 *pNArgs);

// callback
typedef struct _SSPLCallback {
   uint32               op;
   SndPlaylistCallback  func;
   uint32               nArgs;      // number of following longword args
} SSPLCallback;

// destination for a branch
typedef struct _SSPLLabel {
   uint32               op;
   uint32               labelNum;
} SSPLLabel;

typedef enum {
   SSPLBTBranch,                   // always branch
   SSPLBTBranchZero,               // branch if gate == 0
   SSPLBTBranchNotZero,            // branch if gate != 0
   SSPLBTDecrementBranchNotZero    // branch if (--gate) != 0
} SSPLBranchType;

// conditional branch
typedef struct _SSPLBranch {
   uint32               op;
   uint32               labelNum;
   uint32               gateNum;
   uint32               indirect;
   uint32               branchType;
} SSPLBranch;

// set a gate value - useful for Branch loop counter
typedef struct _SSPLSetGate {
   uint32               op;
   uint32               gateNum;
   uint32               gateValue;
} SSPLSetGate;
