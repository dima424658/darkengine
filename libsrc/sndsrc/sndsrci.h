////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/sndsrc/RCS/sndsrci.h $
// $Author: PATMAC $
// $Date: 1997/11/01 01:58:14 $
// $Revision: 1.4 $
//
// (c) 1997 Looking Glass Technologies Inc.
// Pat McElhatton
//
// Module name: Sound Source Internals
// File name: sndsrci.h
//
// Description: Internal interface definition for sound source library
//
////////////////////////////////////////////////////////////////////////

#include <windows.h>

#include <sndsrc.h>
#include <stdio.h>

#include <lgassert.h>
#include <thrdtool.h>

#include <timelog.h>

#ifndef _SNDSRC_H
#include <sndsrc.h>
#endif

#include <sndseg.h>

#include <timelog.h>

// size of temp input buffer which holds 2nd source during dual segments
#define TMP_BUFFER_BYTES 4096

void fillSSData(ISndSample *pSample, void *pCBData, uint32 nBytes);

// TBD: does this have to be extern-C?

class cSndSource : public ISndSource
{
	DECLARE_UNAGGREGATABLE();

public:
   cSndSource();
	virtual ~cSndSource();

   STDMETHOD_( uint32, GetSerialNumber ) ( void );

	STDMETHOD_( BOOL, SetPlaylist)(SndPlaylist pList );

	STDMETHOD_( BOOL, ConnectToPlayer )( ISndSample *pPlayer );

	STDMETHOD_( void, DisconnectFromPlayer )( void );

	STDMETHOD_( BOOL, SetGate )( uint32 gateNum, uint32 gateValue );

	STDMETHOD_(uint32, GetGate)(uint32 gateNum);

	STDMETHOD_( void, RegisterEndCallback )( SndSourceEndCallback func, void *pCBData );

	STDMETHOD_( void, GetPositions ) ( uint32 *pPlay, uint32 *pSource, uint32 *pLeft );

	STDMETHOD_( void, SetPosition ) ( uint32 pos );

	STDMETHOD_( void, GetAttribs ) ( sSndAttribs *pAttribs );

	STDMETHOD_( uint32, SamplesToTime ) ( uint32 samples );

	STDMETHOD_( uint32, TimeToSamples ) ( uint32 milliSecs );

	STDMETHOD_(void, BranchToLabel) (uint32 BranchToLabel);

	STDMETHOD_(uint32, GetMostRecentLabel) ();

   // non-COM methods
   void           FillStream( uint32 nBytes );
   void           *ReadData( void *pDst, uint32 nBytes );
   cSndSegment    *CreateSegment( uint32   *pOp );
   void           CheckAttribs( void );
   void           NextSegment( BOOL createSegs );
   void           SetAttribs( sSndAttribs *pAttribs );

private:

   static uint32        mNextSerialNum;
   static uint32        mSourcesMade;
   static uint32        mSourcesDestroyed;

   uint32               mSerialNum;
   ISndSample           *mpPlayer;
   SndPlaylist          mpPlaylist;
   SndPlaylist          mpPlaylistBase;
   uint32               mStartOffset;
   SndSourceEndCallback mfEndCB;
   void                 *mpEndCBData;
   uint32               mGateValue;

   sSndAttribs          mAttribs;

   uint32               mSegBytesLeft;
   uint32               mSegBytesTaken;
   uint32               mListBytesTaken;
   uint32               mListBytesTotal;

   int                  mNumSources;
   cSndSegment          *mpSrc1;
   uint32               mSrc1Offset;
   SndPlaylistElement   mSrc1Type;
   cSndSegment          *mpSrc2;
   uint32               mSrc2Offset;
   SndPlaylistElement   mSrc2Type;
   int                  mBytesPerSample;
   int                  mSegNum;
   BOOL                 mResyncNeeded;
   uint32               mResyncPos;

   char                 *mpTmpBuffer;
   uint32               mTmpBufferBytes;

   SndPlaylistElement   *mpLabels[SNDSRC_DEFAULT_MAX_LABELS];
   uint32               mGates[SNDSRC_DEFAULT_MAX_GATES];
};

