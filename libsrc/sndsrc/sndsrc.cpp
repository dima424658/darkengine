////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/sndsrc/RCS/sndsrc.cpp $
// $Author: PATMAC $
// $Date: 1997/06/24 16:41:50 $
// $Revision: 1.2 $
//
// (c) 1997 Looking Glass Technologies Inc.
// Pat McElhatton
//
// Module name: Sound Sources
// File name: sndsrc.cpp
//
// Description: Sound Source core routines
//
////////////////////////////////////////////////////////////////////////

#include <sndsrci.h>
#include <playlist.h>
#include <comtools.h>


uint32 cSndSource::mNextSerialNum = 0;
uint32 cSndSource::mSourcesMade = 0;
uint32 cSndSource::mSourcesDestroyed = 0;

////////////////////////////////////
// IMPLEMENT_UNAGGREGATABLE
//
// This does all the implementations of IUnknown methods
//

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cSndSource, ISndSource);


ISndSource *
SndCreateSource( sSndAttribs *pAttribs )
{
   cSndSource *pSrc = new cSndSource( SNDSRC_DEFAULT_MAX_LABELS,
                                      SNDSRC_DEFAULT_MAX_GATES );

   pSrc->SetAttribs( pAttribs );

   return (ISndSource *) pSrc;
}

ISndSource *
SndCreateSourceEx( sSndAttribs* pAttribs, uint32 maxLabels, uint32 maxGates )
{
   cSndSource *pSrc = new cSndSource( maxLabels, maxGates );

   pSrc->SetAttribs( pAttribs );

   return (ISndSource *) pSrc;
}

///////////////////////////////////
// cSndSource constructor
//

cSndSource::cSndSource( uint32 maxLabels, uint32 maxGates )
{
   mSourcesMade++;
   mSerialNum = mNextSerialNum++;
   TLOG3( "SSrc::Constructor [%d] %ld made %ld destroyed", mSerialNum,
         mSourcesMade, mSourcesDestroyed );

   mpPlayer = NULL;
   mpPlaylist = NULL;
   mpPlaylistBase = NULL;
   mStartOffset = 0;
   mfEndCB = NULL;
   mpEndCBData = NULL;
   mMaxLabels = maxLabels;
   mMaxGates = maxGates;
   mpLabels = new SndPlaylistElement *[mMaxLabels];
   mGates = new uint32[mMaxGates];
   mLastLabelPassed = (uint32) ~0;

   // our default source format will be 22kHz mono 8-bit
   mAttribs.dataType = kSndDataPCM;
   mAttribs.sampleRate = 22050;
   mAttribs.bitsPerSample = 8;
   mAttribs.nChannels = 1;
   mAttribs.bytesPerBlock = 1;
   mAttribs.samplesPerBlock = 1;
   mAttribs.numSamples = (uint32) ~0;
   CheckAttribs();

   // current seg info
   mSrc1Type = plNone;
   mSrc2Type = plNone;
   mSegBytesLeft = 0;
   mSegBytesTaken = 0;

   mListBytesTotal = 0;
   mListBytesTaken = 0;

   mpSrc1 = NULL;
   mpSrc2 = NULL;

   mpTmpBuffer = NULL;
   mTmpBufferBytes = TMP_BUFFER_BYTES;

   mResyncNeeded = FALSE;
   mResyncPos = 0;
}


///////////////////////////////////
// cSndSource destructor
//

cSndSource::~cSndSource()
{
   mSourcesDestroyed++;
   TLOG3( "SSrc::Destructor [%d] %ld made %ld destroyed", mSerialNum,
         mSourcesMade, mSourcesDestroyed );
   // TBD - what if a player is still connected?
   if ( mpSrc1 != NULL ) {
      delete mpSrc1;
   }
   if ( mpSrc2 != NULL ) {
      delete mpSrc2;
   }
   if ( mpTmpBuffer != NULL ) {
      delete [] mpTmpBuffer;
   }
   delete [] mpLabels;
   delete [] mGates;
   if ( mpPlayer != NULL ) {
      mpPlayer->RegisterFillCallback( NULL, NULL );
      Release();     // player no longer has a ref to us
      mpPlayer->Release();
   }
}


///////////////////////////////////
// GetSerialNumber - return the serial number
//

STDMETHODIMP_(uint32)
cSndSource::GetSerialNumber( void )
{
   return mSerialNum;
}


///////////////////////////////////
// CheckAttribs - update all member vars which are derived from mAttribs
//
void
cSndSource::CheckAttribs( void )
{
   switch( mAttribs.dataType ) {
      case kSndDataPCM:
         mBytesPerSample = mAttribs.bitsPerSample >> 3;
         break;

      case kSndDataIMAADPCM:
         mBytesPerSample = 2;
         break;
   }
   mBytesPerSample *= mAttribs.nChannels;
}


///////////////////////////////////
// SetAttribs - set source attribs
//
void
cSndSource::SetAttribs( sSndAttribs *pAttribs )
{
   if ( pAttribs != NULL ) {
      mAttribs = *pAttribs;
   }
}


///////////////////////////////////
// SetGate - set loop control variable
//
STDMETHODIMP_(BOOL)
cSndSource::SetGate( uint32   gateNum,
                     uint32   gateValue )
{
   assert( gateNum < mMaxGates );
   if ( gateNum >= mMaxGates )
      return FALSE;
   mGates[gateNum] = gateValue;

   return TRUE;
}

STDMETHODIMP_(uint32) cSndSource::GetGate(uint32 gateNum)
{
   assert( gateNum < mMaxGates );
   return ( gateNum < mMaxGates ) ? mGates[gateNum] : 0;
}


///////////////////////////////////
// RegisterEndCallback - set callback which is invoked when playlist
//  is exhausted
//
STDMETHODIMP_(void)
cSndSource::RegisterEndCallback( SndSourceEndCallback    newCB,
                                 void                    *pCBData )
{
   mfEndCB = newCB;
   mpEndCBData = pCBData;
}


///////////////////////////////////
// GetAttribs - return source attributes
//
STDMETHODIMP_(void)
cSndSource::GetAttribs( sSndAttribs    *pAttribs )
{
   *pAttribs = mAttribs;
}


///////////////////////////////////
// SamplesToTime - return play time in milliseconds for specified # of samples
//
STDMETHODIMP_(uint32)
cSndSource::SamplesToTime( uint32   nSamples )
{
   float    msecs, sampleRate;
   uint32   imsecs;

   if ( mpPlayer != NULL ) {
      // get sample rate from connected player
      sampleRate = (float) mpPlayer->GetFrequency();
   } else {
      sampleRate = (float)mAttribs.sampleRate;
   }

   msecs = 1000.0f * ((float) nSamples / sampleRate);

   imsecs = (uint32)msecs;
   return imsecs;
}


///////////////////////////////////
// TimeToSamples - return number of samples for a given # of milliseconds
//
STDMETHODIMP_(uint32)
cSndSource::TimeToSamples( uint32   milliseconds )
{
   float    samples, sampleRate;
   uint32   nSamples;

   if ( mpPlayer != NULL ) {
      // get sample rate from connected player
      sampleRate = (float) mpPlayer->GetFrequency();
   } else {
      sampleRate = (float)mAttribs.sampleRate;
   }

   samples = (float) sampleRate * ((float) milliseconds / 1000.0f);

   nSamples = (uint32)samples;
   return nSamples;
}

STDMETHODIMP_(void) cSndSource::BranchToLabel(uint32 labelNum)
{
   if ( (labelNum < mMaxLabels) && (mpLabels[labelNum] != NULL) ) {
      mpPlaylist = mpLabels[labelNum];
      mLastLabelPassed = labelNum;
   }
}

STDMETHODIMP_(uint32) cSndSource::GetMostRecentLabel()
{
   return mLastLabelPassed;
}



#ifdef XXX
///////////////////////////////////
//
//
STDMETHODIMP_(
cSndSource::
{
   // TBD
}


#endif
