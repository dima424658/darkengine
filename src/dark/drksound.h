/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drksound.h,v 1.3 2000/02/03 17:37:44 patmac Exp $
#pragma once
#ifndef __DRKSOUND_H
#define __DRKSOUND_H


////////////////////////////////////////////////////////////
// DARK-SPECFIC SOUND STUFF
//

EXTERN void DarkSoundInit(void); 
EXTERN void DarkSoundTerm(void); 

// run ms of a dark specific sound frame
// really, for now, this is just the "am i stopping to listen" code
EXTERN void DrkSoundFrame(void);

// ambient volume offset in millibels
EXTERN void DarkSoundSetAmbientVolume( int vol );
EXTERN int  DarkSoundGetAmbientVolume( void );

#endif // __DRKSOUND_H
