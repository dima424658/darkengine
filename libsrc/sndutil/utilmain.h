////////////////////////////////////////////////////////////////////////
//
// (c) 1996 Looking Glass Technologies Inc.
// Pat McElhatton
//
// Module name: Sound utilities standalone main routines
// File name: utilmain.h
//
// Description: Set of functions for setting up sound library which
//    are used by several standalone programs
//
////////////////////////////////////////////////////////////////////////


// maximum # of simultaneous active sounds
#define MAX_SOUNDS 8

// Function name: SoundInit()
// Description: Initialize the sound module.
extern ISndMixer *
SoundInit ( void );

// Function name: SoundTerm()
// Description: Terminate the sound module.
extern void
SoundTerm ( ISndMixer *pMixer );

// Function name: SoundRecur()
// Description: Recur into the sound module for management purposes.
extern void
SoundRecur( ISndMixer *pMixer );

