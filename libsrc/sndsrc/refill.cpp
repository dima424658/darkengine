////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/sndsrc/RCS/refill.cpp $
// $Author: PATMAC $
// $Date: 1997/06/24 16:40:16 $
// $Revision: 1.2 $
//
// (c) 1997 Looking Glass Technologies Inc.
// Pat McElhatton
//
// Module name: Sound Source Stream Refill Stuff
// File name: refill.cpp
//
// Description: stream refill callbacks for sound sources
//
////////////////////////////////////////////////////////////////////////

#include <sndsrci.h>
#include <sndseg.h>
#include <playlist.h>
#include <mprintf.h>

//
// This module holds all the stuff which refills an ISndSample stream buffer from
// a playlist.
//

// TBD - stream refill callback

// Notes:
// 1. A single call to RefillStream can result in multiple segments being
//    read, which can mean more than one call to SilenceFill or LoadBufferIndirect.
// 2. A single call to LoadBufferIndirect can result in one or two calls to the
//    getData routine, two if the stream buffer wraps.

// Mix::Update ->
//   Smp::CheckStream ->
//     SSrc::FillStream ->
//       Smp::SilenceFill

// Mix::Update ->
//   Smp::CheckStream ->
//     SSrc::FillStream ->
//       Smp::LoadBufferIndirect ->
//         getSSData ->
//           SSrc::ReadData ->
//             readXXXData
//             convertData (if needed)

// void *getSSData(void *pCBData, void *pDst, uint32);
//  returns where the data was actually returned

// SSrc::GetData will be called twice within a single LoadBufferIndirect
//  if the data passed to DirectSound wraps around the end of the buffer
//  and the source offset must be advanced each time


// ?? Should we allow dual segments where segments have different attribs ?? YES
// ?? Should we allow dual segments where one segment is rez & other file ?? NO
//
// This is just a coupler from the C interface
// ISndSample_LoadBufferIndirect uses to ISndSource_GetData
//  returns where the data was actually stored (may not be the same as pDst)
//
static void
*getSSData( void     *pCBData,
            void     *pDst,
            uint32   nBytes )
{
   void *pData;
   cSndSource *pSrc;

   pSrc = (cSndSource *) pCBData;
   pData = pSrc->ReadData( pDst, nBytes );

   return pData;
}


///////////////////////////////////
// ReadData - generate data & write to specified buffer
//  - this routine must write exactly nBytes to buffer (FillStream
//    has already checked that current segment can generate at least nBytes)

void *
cSndSource::ReadData( void    *pDst,
                      uint32   nBytes )
{
   void *pData = pDst;
   uint8 *pAB, *pBB, *pMixB;  // src, dst, src+dst, 8 bits
   long a;
   int16 *pAW, *pBW, *pMixW;  // src, dst, src+dst, 16 bits
   uint32 i;
   char *pAData, *pBData, *pMixOut;
   uint32 bytesToMix, bytesThisTime;

   if ( mResyncNeeded ) {
      mpSrc1->SetPosition( mResyncPos - mSrc1Offset );
   }
   LOG3( "SSrc::ReadData [%d] %d bytes resync==%d\n", mSerialNum, nBytes, mResyncNeeded );

   // fetch the first source, convert if necessary
   pAData = (char *) mpSrc1->GetData( pDst, nBytes );

   if ( mpSrc2 != NULL ) {
      // fetch the second source, convert if necessary
      // convert second source if necessary
      if ( mResyncNeeded ) {
         mpSrc2->SetPosition( mResyncPos - mSrc2Offset );
      }

      if ( mpTmpBuffer == NULL ) {
         // allocate a temp buffer for 2nd source
         mpTmpBuffer = new char[mTmpBufferBytes];
      }
      bytesToMix = nBytes;
      while ( bytesToMix > 0 ) {
         bytesThisTime = (bytesToMix > mTmpBufferBytes) ? mTmpBufferBytes : bytesToMix;
         pBData = (char *) mpSrc2->GetData( mpTmpBuffer, bytesThisTime );
         pMixOut = (char *) pDst;

         // mix first & second source
         if ( mAttribs.bitsPerSample == 8 ) {
            pAB = (uint8 *) pAData;
            pBB = (uint8 *) pBData;
            pMixB = (uint8 *) pMixOut;
            for ( i = 0; i < bytesThisTime; i++ ) {
               // the bum who decided to make 8-bit samples unsigned should fry!
               // remove them from the gene pool!

               // the source range is 0...255, silence is 128,
               // but a's range is 0...511, silence is 256        
               a = (*pAB++) + (*pBB++);
               a -= 128;                  // get silence to be 128 again
               if ( a < 0 )   a = 0;      // clamp over/underflows
               if ( a > 255 ) a = 255;
               *pMixB++ = (uint8) a;
            }
         } else {
            // assume 16-bits per sample
            pAW = (int16 *) pAData;
            pBW = (int16 *) pBData;
            pMixW = (int16 *) pMixOut;
            for ( i = 0; i < bytesThisTime; i += 2 ) {
               a = (*pAW++) + (*pBW++);
               if ( a < -32768L ) a = -32768L;     // clamp over/underflow
               if ( a > 32767 )   a = 32767;
               *pMixW++ = (int16) a;
            }
         }

         bytesToMix -= bytesThisTime;
         pMixOut += bytesThisTime;
         pAData += bytesThisTime;

      }     // end while bytesToMix

      return pDst;
   } else {
      return pAData;
   }
}


///////////////////////////////////
// FillStream - refill player stream buffer
//  NOTE: this routine can write less than nBytes to buffer
//

void
cSndSource::FillStream( uint32    nBytes )
{
   uint32            bytesToDo, newPos;
   //SSPLCallback      *pCallback;

   LOG3( "SSrc::FillStream [%d] %d bytes, pos %d\n", mSerialNum, nBytes, mListBytesTaken );
   // TBD - callbacks

   // if nBytes is 0 at this point, that is a flag that the stream should shutdown,
   // so just pass nBytes==0 to LoadBufferIndirect, and it will start SilenceFill-ing
   // the buffer after end of real data (to prevent last buffer from repeating) and exit
   if ( nBytes == 0 ) {
      ISndSample_LoadBufferIndirect( mpPlayer, getSSData, (void *) this, 0 );
      return;
   }

   //
   // if the user did a SetPosition on the player since the last call to
   //  FillStream, do a SetPosition on this source to the new position
   //
   if ( mpPlayer->ResyncNeeded( &newPos ) ) {
      SetPosition( newPos );
      // this will leave mResyncNeeded set, we must clear it before exiting FillStream
   }

   while ( nBytes > 0 ) {

      if ( mSegBytesLeft == 0 ) {
         //
         // open next segment if there are any left
         //
         do {        // loop to skip over non-data segments
            NextSegment( TRUE );
         } while ( (*mpPlaylist != plEndList) && (mSegBytesLeft == 0) );

         if ( mSegBytesLeft == 0 ) {
            nBytes = 0;
            if ( mfEndCB != NULL ) {
               // playlist is exhausted, invoke end callback
               (mfEndCB) ( this, mpEndCBData );
            }
         }

      }  // end if mSegBytesLeft == 0

      bytesToDo = (nBytes > mSegBytesLeft) ? mSegBytesLeft : nBytes;
      if ( mpSrc1 ) {
         //mprintf("to do %d seg %d bytes left %d %d\n", bytesToDo, mSegNum, mSegBytesLeft, mpSrc1->BytesLeft() );
      }
      if ( mSrc1Type == plSilence ) {
         mpPlayer->SilenceFill( bytesToDo );
      } else {
         mpPlayer->LoadBufferIndirect( getSSData, (void *) this, bytesToDo );
      }
      mSegBytesLeft -= bytesToDo;
      nBytes -= bytesToDo;
      mListBytesTaken += bytesToDo;

      mResyncNeeded = FALSE;
   }     // end while bytesLeft
}


//
// fill callback for sound sources
//
static void
fillSSData( ISndSample     * /*pSample*/,
            void           *pCBData,
            uint32         nBytes )
{
   cSndSource *pSrc;
   pSrc = (cSndSource *) pCBData;

   pSrc->FillStream( nBytes );
}


///////////////////////////////////
// ConnectToPlayer - connect this source to an ISndSample
//  returns TRUE if this worked
//

STDMETHODIMP_(BOOL)
cSndSource::ConnectToPlayer( ISndSample    *pPlayer )
{
   BOOL result = TRUE;

   if ( mpPlayer != NULL ) {
      mpPlayer->Release();
   }
   mpPlayer = pPlayer;
   mpPlayer->RegisterFillCallback( fillSSData, (void *) this );
   AddRef();            // player now has a reference to us
   pPlayer->AddRef();   // and we have a reference to it
   mpPlayer->GetAttribs( &mAttribs );
   CheckAttribs();

   // TBD - what if this source is already attached to another player?
   return result;
}


///////////////////////////////////
// DisconnectFromPlayer - disconnect this source from an ISndSample
//  returns TRUE if this worked
//

STDMETHODIMP_(void)
cSndSource::DisconnectFromPlayer( void )
{
   // TBD - flag an error if player is not stopped?
   if ( mpPlayer != NULL ) {
      mpPlayer->RegisterFillCallback( NULL, NULL );
      Release();     // player no longer has a ref to us
      mpPlayer->Release();
      mpPlayer = NULL;
   }
}


