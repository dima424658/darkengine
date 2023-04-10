////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/sndsrc/RCS/sndsrc.h $
// $Author: mwhite $
// $Date: 1999/02/22 23:06:13 $
// $Revision: 1.6 $
//
// (c) 1997 Looking Glass Technologies Inc.
// Pat McElhatton
//
// Module name: Sound Sources
// File name: sndsrc.h
//
// Description: External interface definition for sound source library
//
////////////////////////////////////////////////////////////////////////

#ifndef _SNDSRC_H
#define _SNDSRC_H

#ifndef _INTTYPE_H
//#include <inttype.h>
#endif

#include <comtools.h>

#ifndef _SNDSRCID_H
#include <sndsrcid.h>
#endif

#ifndef _LG_SOUND_H
#include <lgsound.h>
#endif

F_DECLARE_INTERFACE( ISndSource );

#ifdef __cplusplus
extern "C"
{
#endif

// A playlist is a sequence of longwords
typedef unsigned long SndPlaylistElement;
typedef unsigned long *SndPlaylist;

// Default number of gate variables
#define SNDSRC_DEFAULT_MAX_GATES 8

// Default number of labels
#define SNDSRC_DEFAULT_MAX_LABELS 8

ISndSource *
SndCreateSource( sSndAttribs *pAttribs);

ISndSource *
SndCreateSourceEx( sSndAttribs *pAttribs, uint32 maxLabels, uint32 maxGates );


typedef void (*SndSourceEndCallback)(ISndSource *, void *);

#undef INTERFACE
#define INTERFACE ISndSource

DECLARE_INTERFACE_(ISndSource, IUnknown)
{
	// Comtools macro to declare IUnknown methods
	DECLARE_UNKNOWN_PURE();


   STDMETHOD_( uint32, GetSerialNumber ) ( THIS ) PURE;

	STDMETHOD_(BOOL, SetPlaylist)(THIS_ SndPlaylist pList ) PURE;

	STDMETHOD_( BOOL, ConnectToPlayer )( THIS_ ISndSample *pPlayer ) PURE;

	STDMETHOD_( void, DisconnectFromPlayer )( THIS ) PURE;

	STDMETHOD_( BOOL, SetGate )( THIS_ uint32 gateNum, uint32 gateValue ) PURE;

	STDMETHOD_( uint32, GetGate )( THIS_ uint32 gateNum ) PURE;

	STDMETHOD_( void, RegisterEndCallback )( THIS_ SndSourceEndCallback func, void *pCBData ) PURE;

	STDMETHOD_( void, GetPositions ) ( THIS_ uint32 *pPlay, uint32 *pSource, uint32 *pLeft ) PURE;

	STDMETHOD_( void, SetPosition ) ( THIS_ uint32 pos ) PURE;

	STDMETHOD_( void, GetAttribs ) ( THIS_ sSndAttribs *pAttribs ) PURE;

	STDMETHOD_( uint32, SamplesToTime ) ( THIS_ uint32 samples ) PURE;

	STDMETHOD_( uint32, TimeToSamples ) ( THIS_ uint32 milliSecs ) PURE;

};

#define ISndSource_GetSerialNumber( p )            COMCall0( p, GetSerialNumber )
#define ISndSource_SetPlaylist( p, a )             COMCall1( p, SetPlaylist, a )
#define ISndSource_ConnectToPlayer( p, a )         COMCall1( p, ConnectToPlayer, a )
#define ISndSource_DisconnectFromPlayer( p )       COMCall0( p, DisconnectFromPlayer )
#define ISndSource_SetGate( p, a, b )              COMCall2( p, SetGate, a, b )
#define ISndSource_GetGate( p, a )                 COMCall1( p, GetGate, a )
#define ISndSource_RegisterEndCallback( p, a, b )  COMCall2( p, RegisterEndCallback, a, b )
#define ISndSource_GetPositions( p, a, b, c )      COMCall3( p, GetPositions, a, b, c )
#define ISndSource_SetPosition( p, a )             COMCall1( p, SetPosition, a )
#define ISndSource_GetAttribs( p, a )              COMCall1( p, GetAttribs, a )
#define ISndSource_SamplesToTime( p, a )           COMCall1( p, SamplesToTime, a )
#define ISndSource_TimeToSamples( p, a )           COMCall1( p, TimeToSamples, a )
#define ISndSource_Release( p )                    COMCall0(p, Release)


// Needed:
// How should the following things be dealt with:
//  - source type - file/resource/memory buffer
//  - sequencing of sources

// source examples:
// - simple file reader
// - simple rez reader
// - simple memory buffer reader
// - sequence of simple sources

// modifiers for a simple source:
// - start offset
// - sample count
// ? loop count

// methods
// - return the current source position
//   - samples played, samples left to play
//   - time played, time left to play
// - return the source sound attribs - sample rate, #channels, encoding
// - return the source size in samples
// - attach to a sound player
// - turn on/off looping
// - set the current source position (with optional dumping of the
//      data which is queued in the sound player)
// - get/set the "source window" - start offset and #samples
// - get/set the playback direction
//

#ifdef __cplusplus
	};
#endif

#endif //_SNDSRC_H