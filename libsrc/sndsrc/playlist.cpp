////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/sndsrc/RCS/playlist.cpp $
// $Author: PATMAC $
// $Date: 1998/01/03 00:51:12 $
// $Revision: 1.7 $
//
// (c) 1997 Looking Glass Technologies Inc.
// Pat McElhatton
//
// Module name: Playlist Interpreter
// File name: playlist.cpp
//
// Description: interpreter for sound source play list
//   includes methods for seeking & reporting position
//
////////////////////////////////////////////////////////////////////////

#include <sndsrci.h>
#include <stdio.h>
#include <res.h>
#include <playlist.h>
#include <sndsegi.h>
#include <mprintf.h>

//
// This module holds the playlist initialization method, and the method for
// seeking within a playlist.
//

//
// size of a playlist element, indexed by opcode
//
static int playlistOpSizes[] = {
   0,                               // there is no element for opcode plNone
   sizeof( SSPLEndList ),
   sizeof( SSPLSilence ),
   sizeof( SSPLRezSingle ),
   sizeof( SSPLFileSingle ),
   sizeof( SSPLMemSingle ),
   sizeof( SSPLRezDual ),
   sizeof( SSPLFileDual ),
   sizeof( SSPLMemDual ),
   sizeof( SSPLCallback ),
   sizeof( SSPLLabel ),
   sizeof( SSPLBranch ),
   sizeof( SSPLSetGate ),
   sizeof( SSPLRawMemSingle ),
   sizeof( SSPLRawMemDual )
};

// the 2 format longwords in segment structs are used to hold attribute info:
// 1st longword:
//    bits 0-3 are data type
//    bit 4 is frequency doubling flag
// 2nd longword:
//    bits 0-15 are samples per block
//    bits 16-31 are bytes per block
#define kSndPlaylistFlagPCM         0
#define kSndPlaylistFlagIMAADPCM    1
#define kSndPlaylistFlagDoDouble    16


///////////////////////////////////
// setFormatWords - save format info 
//
static void
setFormatWords( uint32       *pFormat,
                sSndAttribs  *pSegAttribs,
                sSndAttribs  *pSrcAttribs )
{
   switch( pSegAttribs->dataType ) {
      case kSndDataPCM:          *pFormat = 0;          break;
      case kSndDataIMAADPCM:     *pFormat = 1;          break;
   }

   if ( pSrcAttribs->sampleRate == (2 * pSegAttribs->sampleRate) ) {
      *pFormat |= kSndPlaylistFlagDoDouble;
   }

   pFormat[1] = (pSegAttribs->bytesPerBlock << 16) | pSegAttribs->samplesPerBlock;
}


///////////////////////////////////
// SetPlaylist - set the list of sound segments which will be played
//  return TRUE if no problems
//
STDMETHODIMP_(BOOL)
cSndSource::SetPlaylist( SndPlaylist   pList )
{
   SndPlaylistElement   *pLastDataOp, *pOp;
   SndPlaylistElement   op;
   SSPLRezSingle        *pRezSingle;
   SSPLFileSingle       *pFileSingle;
   SSPLMemSingle        *pMemSingle;
   SSPLRawMemSingle     *pRawMemSingle;
   SSPLRezDual          *pRezDual;
   SSPLFileDual         *pFileDual;
   SSPLMemDual          *pMemDual;
   SSPLRawMemDual       *pRawMemDual;
   SSPLLabel            *pLabel;
   SSPLBranch           *pBranch;
   SSPLSetGate          *pSetGate;
   char                 tmpBuff[256];
   sSndAttribs          attribs;
   uint32               rawDataLen, bytesToRead, len, samplesThisSegment;
   void                 *pRawData;
   BOOL                 prevWasDual, isDual, notDone, blewIt;
   FILE                 *inFile;
   Id                   rezId;
   int                  i;

   TLOG1( "SSrc::SetPlaylist [%d]\n", mSerialNum );

   // scan the playlist, filling in fields which need info from
   //   the sound resource (like #samples, offset to 1st sample)
   // NOTE: when a dual segment is found, we have to go to the
   //   previous data segment (if any) and lower its sample count

   pLastDataOp = NULL;
   // if the previous segment was a dual segment, then this XXX
   prevWasDual = FALSE;

   blewIt = FALSE;
   notDone = TRUE;
   mpPlaylist = pList;
   mpPlaylistBase = pList;
   mListBytesTotal = 0;
   mSegNum = 0;

   for ( i = 0; i < SNDSRC_MAX_LABELS; i++ ) {
      mpLabels[i] = NULL;
   }
   for ( i = 0; i < SNDSRC_MAX_GATES; i++ ) {
      mGates[i] = 0;
   }

   while ( notDone ) {
      pOp = mpPlaylist;
      isDual = FALSE;
      op = *pOp;
      samplesThisSegment = 0;

      switch( op ) {


         case plEndList:
            notDone = FALSE;
            break;


         case plSilence:
            samplesThisSegment = pOp[1];
            break;


         case plRezSingle:
            pRezSingle = (SSPLRezSingle *) pOp;
            rezId = (Id) pRezSingle->id;
            len = ResSize( rezId );
            bytesToRead = ( len < sizeof(tmpBuff) ) ? len : sizeof(tmpBuff);
            ResExtractPartial( rezId, tmpBuff, 0, bytesToRead );
            blewIt = SndCrackRezHeader( tmpBuff, bytesToRead,
                                        &pRawData, &rawDataLen,
                                        &(pRezSingle->nSamples), &attribs );
            assert( blewIt == FALSE );
            pRezSingle->off = ((char *) pRawData) - tmpBuff;
            setFormatWords( &(pRezSingle->format1), &attribs, &mAttribs );
            if ( prevWasDual ) {
               pRezDual = (SSPLRezDual *) pLastDataOp;
               assert( pRezDual->nSamples < pRezSingle->nSamples );
               pRezSingle->nSamples -= pRezDual->nSamples;
               pRezDual->off = pRezSingle->off;
               pRezDual->id = rezId;
               pRezDual->format1 = pRezSingle->format1;
               pRezDual->format2 = pRezSingle->format2;
            }
            pLastDataOp = pOp;
            samplesThisSegment = pRezSingle->nSamples;
            break;


         case plFileSingle:
            pFileSingle = (SSPLFileSingle *) pOp;
            inFile = fopen( pFileSingle->name, "rb" );
            assert ( inFile != NULL );
            fseek( inFile, 0L, SEEK_END );
            len = ftell( inFile );
            fseek( inFile, 0L, SEEK_SET );
            bytesToRead = ( len < sizeof(tmpBuff) ) ? len : sizeof(tmpBuff);
            fread( tmpBuff, 1, bytesToRead, inFile );
            fclose( inFile );
            blewIt = SndCrackRezHeader( tmpBuff, bytesToRead,
                                        &pRawData, &rawDataLen,
                                        &(pFileSingle->nSamples), &attribs );
            assert( blewIt == FALSE );
            pFileSingle->off = ((char *) pRawData) - tmpBuff;
            setFormatWords( &(pFileSingle->format1), &attribs, &mAttribs );
            if ( prevWasDual ) {
               pFileDual = (SSPLFileDual *) pLastDataOp;
               assert( pFileDual->nSamples < pFileSingle->nSamples );
               pFileSingle->nSamples -= pFileDual->nSamples;
               pFileDual->off = pFileSingle->off;
               memcpy( pFileDual->name, pFileSingle->name,
                       sizeof(pFileSingle->name) );
               pFileDual->format1 = pFileSingle->format1;
               pFileDual->format2 = pFileSingle->format2;
            }
            pLastDataOp = pOp;
            samplesThisSegment = pFileSingle->nSamples;
            break;

         case plMemSingle:
            pMemSingle = (SSPLMemSingle *) pOp;
            bytesToRead = 1024;  // ASSume that header is less than this
            blewIt = SndCrackRezHeader( pMemSingle->pData, bytesToRead,
                                        &pRawData, &rawDataLen,
                                        &(pMemSingle->nSamples), &attribs );
            assert( blewIt == FALSE );
            pMemSingle->off = ((char *) pRawData) - ((char *) pMemSingle->pData);
            setFormatWords( &(pMemSingle->format1), &attribs, &mAttribs );
            if ( prevWasDual ) {
               pMemDual = (SSPLMemDual *) pLastDataOp;
               assert( pMemDual->nSamples < pMemSingle->nSamples );
               pMemSingle->nSamples -= pMemDual->nSamples;
               pMemDual->off = pMemSingle->off;
               pMemDual->format1 = pMemSingle->format1;
               pMemDual->format2 = pMemSingle->format2;
            }
            pLastDataOp = pOp;
            samplesThisSegment = pMemSingle->nSamples;
            break;

         case plRawMemSingle:
            pRawMemSingle = (SSPLRawMemSingle *) pOp;
            if ( prevWasDual ) {
               pRawMemDual = (SSPLRawMemDual *) pLastDataOp;
               assert( pRawMemDual->nSamples < pRawMemSingle->nSamples );
               pRawMemSingle->nSamples -= pRawMemDual->nSamples;
               pRawMemDual->pData = pRawMemSingle->pData;
            }
            pLastDataOp = pOp;
            samplesThisSegment = pRawMemSingle->nSamples;
            break;

         case plRezDual:
            isDual = TRUE;
            pRezDual = (SSPLRezDual *) pOp;
            // NOTE: the preceeding data op could be of type rez, file or mem, but
            // we know that the only field we care about is at the same offset for
            // all three types, so pretend it's a rez single
            pRezSingle = (SSPLRezSingle *) pLastDataOp;
            assert( pRezDual->nSamples < pRezSingle->nSamples );
            // decrease size of previous op by #samples of overlap
            pRezSingle->nSamples -= pRezDual->nSamples;
            pLastDataOp = pOp;
            break;

         case plFileDual:
            isDual = TRUE;
            pFileDual = (SSPLFileDual *) pOp;
            // NOTE: the preceeding data op could be of type rez, file or mem, but
            // we know that the only field we care about is at the same offset for
            // all three types, so pretend it's a file single
            pFileSingle = (SSPLFileSingle *) pLastDataOp;
            assert( pFileDual->nSamples < pFileSingle->nSamples );
            // decrease size of previous op by #samples of overlap
            pFileSingle->nSamples -= pFileDual->nSamples;
            pLastDataOp = pOp;
            break;

         case plMemDual:
            isDual = TRUE;
            pMemDual = (SSPLMemDual *) pOp;
            // NOTE: the preceeding data op could be of type rez, file or mem, but
            // we know that the only field we care about is at the same offset for
            // all three types, so pretend it's a Mem single
            pMemSingle = (SSPLMemSingle *) pLastDataOp;
            assert( pMemDual->nSamples < pMemSingle->nSamples );
            // decrease size of previous op by #samples of overlap
            pMemSingle->nSamples -= pMemDual->nSamples;
            pLastDataOp = pOp;
            break;

         case plRawMemDual:
            isDual = TRUE;
            pRawMemDual = (SSPLRawMemDual *) pOp;
            // NOTE: the preceeding data op could be of type rez, file or mem, but
            // we know that the only field we care about is at the same offset for
            // all three types, so pretend it's a raw mem single
            pRawMemSingle = (SSPLRawMemSingle *) pLastDataOp;
            assert( pRawMemDual->nSamples < pRawMemSingle->nSamples );
            // decrease size of previous op by #samples of overlap
            pRawMemSingle->nSamples -= pRawMemDual->nSamples;
            pLastDataOp = pOp;
            break;

         case plCallback:
            break;

         case plLabel:
            pLabel = (SSPLLabel *) pOp;
            if ( pLabel->labelNum < SNDSRC_MAX_LABELS ) {
               mpLabels[pLabel->labelNum] = (SndPlaylistElement *) ( ((uint32) pOp) + sizeof(SSPLLabel) );
            } else {
               Warning( ("Playlist label number out of range!\n") );
            }
            break;

         case plBranch:
            pBranch = (SSPLBranch *) pOp;
            if ( pBranch->labelNum >= SNDSRC_MAX_LABELS ) {
               Warning( ("Playlist branch label out of range!\n") );
            }
            if ( pBranch->gateNum >= SNDSRC_MAX_GATES ) {
               Warning( ("Playlist branch gate out of range!\n") );
            }
            break;

         case plSetGate:
            pSetGate = (SSPLSetGate *) pOp;
            if ( pSetGate->gateNum >= SNDSRC_MAX_GATES ) {
               Warning( ("Playlist gate out of range!\n") );
            }
            break;

         default:
            // TBD - signal an error
            op = plNone;
            notDone = FALSE;
            break;
      }
      prevWasDual = isDual;
      mListBytesTotal += (samplesThisSegment * mBytesPerSample);
      NextSegment( FALSE );
      if ( blewIt ) {
         notDone = FALSE;
      }

      //mprintf("bytes in seg %d\n", samplesThisSegment * mBytesPerSample);
   }  // end while not done
   mpPlaylist = pList;

   mSegBytesLeft = 0;
   mListBytesTaken = 0;

   // TBD - what if there is already a play in progress?
   return ( blewIt ) ? FALSE : TRUE;
}


///////////////////////////////////
// NextSegment - advance to next segment:
//  - set mSegBytesLeft to #bytes of output which segment @ mpPlaylist
//    can produce
//  - advance mpPlaylist to point to next segment
//  - set mpSrc1 & mpSrc2 IFF createSegs is TRUE

void
cSndSource::NextSegment( BOOL    createSegs )
{
   uint32         elementSize, op, argListSize;
   SSPLCallback   *pCallbackOp;
   SSPLRezSingle  *pDataOp;
   SSPLBranch     *pBranch;
   SSPLSetGate    *pSetGate;
   uint32         labelNum, gateValue;
   SndPlaylistElement   *pBranchDest;

   op = *mpPlaylist;
   assert( op < plLASTOp );
   elementSize = playlistOpSizes[ op ] >> 2;    // convert to size in longwords
   argListSize = 0;

   if ( mpPlaylist == mpPlaylistBase ) {
      mSegNum = 0;
   } else {
      mSegNum++;
   }

   pDataOp = (SSPLRezSingle *) mpPlaylist;

   switch( op ) {
      case plEndList:
         // don't advance mpPlaylist past Endlist op
         TLOG1( "SSrc::NextSegment [%d] endList\n", mSerialNum );
         elementSize = 0;
         mSegBytesLeft = 0;
         if ( createSegs ) {
            if ( mpSrc1 ) {
               delete mpSrc1;
               mpSrc1 = NULL;
            }
            if ( mpSrc2 ) {
               delete mpSrc2;
               mpSrc2 = NULL;
            }
         }
         break;

      case plSilence:
         TLOG2( "SSrc::NextSegment [%d] silence %d\n", mSerialNum, pDataOp->nSamples );
         mSegBytesLeft = pDataOp->nSamples * mBytesPerSample;
         if ( createSegs ) {
            if ( mpSrc1 ) {
               delete mpSrc1;
               mpSrc1 = NULL;
            }
            if ( mpSrc2 ) {
               delete mpSrc2;
               mpSrc2 = NULL;
            }
         }
         break;

      case plCallback:
         pCallbackOp = (SSPLCallback *) mpPlaylist;
         TLOG2( "SSrc::NextSegment [%d] callback %d args\n", mSerialNum, pCallbackOp->nArgs );
         argListSize = pCallbackOp->nArgs;
         if ( createSegs && (pCallbackOp->func != NULL) ) {
            // do the callback
            (pCallbackOp->func) ( this, &(pCallbackOp->nArgs) );
         }
         break;

      case plRezSingle:
      case plFileSingle:
      case plMemSingle:
         TLOG3( "SSrc::NextSegment [%d] rez/file/mem single id 0x%x, %d samples\n",
               mSerialNum, pDataOp->id, pDataOp->nSamples );
         mSegBytesLeft = pDataOp->nSamples * mBytesPerSample;
         if ( pDataOp->format1 & kSndPlaylistFlagDoDouble ) {
            mSegBytesLeft <<= 1;
         }
         if ( createSegs ) {
            if ( mpSrc1 ) {
               delete mpSrc1;
               if ( mpSrc2 ) {
                  mpSrc1 = mpSrc2;
                  mpSrc1->ReInit( pDataOp->nSamples );
                  mpSrc2 = NULL;
               } else {
                  mpSrc1 = CreateSegment( mpPlaylist );
               }
            } else {
               mpSrc1 = CreateSegment( mpPlaylist );
            }
         }
         break;

      case plRezDual:
      case plFileDual:
      case plMemDual:
         TLOG2( "SSrc::NextSegment [%d] rez/file/mem dual %d\n",
               mSerialNum, pDataOp->nSamples );
         mSegBytesLeft = pDataOp->nSamples * mBytesPerSample;
         if ( pDataOp->format1 & kSndPlaylistFlagDoDouble ) {
            mSegBytesLeft <<= 1;
         }
         if ( createSegs ) {
            mpSrc1->ReInit( pDataOp->nSamples );
            mpSrc2 = CreateSegment( mpPlaylist );
         }
         break;

      case plRawMemSingle:
         TLOG2( "SSrc::NextSegment [%d] raw mem single %d\n",
               mSerialNum, pDataOp->nSamples );
         mSegBytesLeft = pDataOp->nSamples * mBytesPerSample;
         if ( createSegs ) {
            if ( mpSrc1 ) {
               delete mpSrc1;
               if ( mpSrc2 ) {
                  mpSrc1 = mpSrc2;
                  mpSrc1->ReInit( pDataOp->nSamples );
                  mpSrc2 = NULL;
               } else {
                  mpSrc1 = CreateSegment( mpPlaylist );
               }
            } else {
               mpSrc1 = CreateSegment( mpPlaylist );
            }
         }
         break;

      case plRawMemDual:
         TLOG2( "SSrc::NextSegment [%d] raw mem dual %d\n",
               mSerialNum, pDataOp->nSamples );
         mSegBytesLeft = pDataOp->nSamples * mBytesPerSample;
         if ( createSegs ) {
            mpSrc1->ReInit( pDataOp->nSamples );
            mpSrc2 = CreateSegment( mpPlaylist );
         }
         break;

      case plLabel:
         TLOG2( "SSrc::NextSegment [%d] label %d\n", mSerialNum, pDataOp->nSamples );
         mSegBytesLeft = 0;
         break;

      case plBranch:
         mSegBytesLeft = 0;
         if ( createSegs ) {
            pBranch = (SSPLBranch *) mpPlaylist;

            // point to just after this element in playlist
            mpPlaylist += elementSize;
            elementSize = 0;

            // figure out branch destination address
            labelNum = ~0;
            if ( pBranch->indirect ) {
               if ( pBranch->labelNum < SNDSRC_MAX_GATES ) {
                  labelNum = mGates[ pBranch->labelNum ];
               }
            } else {
               labelNum = pBranch->labelNum;
            }
            pBranchDest = ( labelNum < SNDSRC_MAX_LABELS ) ?
               mpLabels[labelNum] : NULL;

            if ( (pBranchDest != NULL ) &&
                 (pBranch->gateNum < SNDSRC_MAX_GATES) ) {

               gateValue = mGates[ pBranch->gateNum ];
               switch ( pBranch->branchType ) {

                  case SSPLBTBranch:
                     TLOG2( "SSrc::NextSegment [%d] branch to %d\n", mSerialNum, pBranch->labelNum );
                     mpPlaylist = pBranchDest;
                     break;

                  case SSPLBTBranchZero:
                     TLOG3( "SSrc::NextSegment [%d] branchZero to %d, gateValue %d\n",
                           mSerialNum, pBranch->labelNum, gateValue );
                     if ( gateValue == 0 ) {
                        mpPlaylist = pBranchDest;
                     }
                     break;

                  case SSPLBTBranchNotZero:
                     TLOG3( "SSrc::NextSegment [%d] branchNotZero to %d, gateValue %d\n",
                           mSerialNum, pBranch->labelNum, gateValue );
                     if ( gateValue != 0 ) {
                        mpPlaylist = pBranchDest;
                     }
                     break;

                  case SSPLBTDecrementBranchNotZero:
                     TLOG3( "SSrc::NextSegment [%d] DecrementBranchNotZero to %d, gateValue %d\n",
                           mSerialNum, pBranch->labelNum, gateValue );
                     mGates[ pBranch->gateNum ] = (--gateValue);
                     if ( gateValue != 0 ) {
                        mpPlaylist = pBranchDest;
                     }
                     break;
               }
            }     // end if valid branch-dest & gate
         }        // end if createSegs
         break;

      case plSetGate:
         mSegBytesLeft = 0;
         pSetGate = (SSPLSetGate *) mpPlaylist;
         TLOG3( "SSrc::NextSegment [%d] setGate %d to %d\n",
               mSerialNum, pSetGate->gateNum, pSetGate->gateValue );
         if ( createSegs && (pSetGate->gateNum < SNDSRC_MAX_GATES) ) {
            mGates[ pSetGate->gateNum ] = pSetGate->gateValue;
         }
         break;
   }

   mSrc1Type = op;
   mSrc2Type = op;
   mpPlaylist += (elementSize + argListSize);
}


///////////////////////////////////
// GetPositions - return source positions
//

STDMETHODIMP_(void)
cSndSource::GetPositions( uint32    *pPlay,
                          uint32    *pSource,
                          uint32    *pLeft )
{
   if ( (pPlay != NULL) && (mpPlayer != NULL) ) {
      *pPlay = mpPlayer->GetPosition();
   }
   *pSource = mListBytesTaken / mBytesPerSample;
   *pLeft = (mListBytesTotal - mListBytesTaken) / mBytesPerSample;
}


///////////////////////////////////
// SetPosition - set the position within a playlist
//
STDMETHODIMP_(void)
cSndSource::SetPosition( uint32     pos )
{
   uint32      basePos = 0;
   uint32      base1Pos = 0;
   uint32      base2Pos = 0;
   uint32      nextBasePos = 0;
   uint32      bytesLeft = mSegBytesLeft;

   uint32      *pOp;

   uint32      *pOp1 = NULL;
   uint32      *pOp2 = NULL;
   BOOL        notDone = TRUE;

   TLOG2( "SSrc::SetPosition [%d] pos %d\n", mSerialNum, pos );

   mResyncNeeded = TRUE;
   mResyncPos = pos;

   // TBD: avoid releasing seg 1 & 2 if seeking within segment?
   if ( mpSrc1 != NULL ) {
      delete mpSrc1;
      mpSrc1 = NULL;
   }
   if ( mpSrc2 != NULL ) {
      delete mpSrc2;
      mpSrc2 = NULL;
   }
   mNumSources = 0;

   mpPlaylist = mpPlaylistBase;

   while ( notDone ) {
      pOp = mpPlaylist;

      switch( *pOp ) {

         case plEndList:
            // position is past end of playlist!
            // TBD
            notDone = FALSE;
            bytesLeft = 0;
            break;

         case plSilence:
            pOp1 = NULL;
            pOp2 = NULL;
            nextBasePos = basePos + pOp[1];
            break;

         case plRezSingle:
         case plFileSingle:
         case plMemSingle:
         case plRawMemSingle:
            if ( pOp2 == NULL ) {
               pOp1 = pOp;
               base1Pos = basePos;
            } else {
               pOp1 = pOp2;
               base1Pos = base2Pos;
            }
            pOp2 = NULL;
            nextBasePos = basePos + pOp[1];
            break;

         case plRezDual:
         case plFileDual:
         case plMemDual:
         case plRawMemDual:
            if ( pOp2 != NULL ) {
               pOp1 = pOp2;
               base1Pos = base2Pos;
            }
            pOp2 = pOp;
            base2Pos = basePos;
            nextBasePos = basePos + pOp[1];
            break;

         case plCallback:
         case plLabel:
         case plBranch:
         case plSetGate:
            // just ignore branches/labels/gates/callbacks
            pOp1 = NULL;
            pOp2 = NULL;
            nextBasePos = basePos;
            break;

         default:
            // TBD - signal an error
            assert(FALSE);
            notDone = FALSE;
            break;
      }

      if ( pos < nextBasePos ) {

         // seek position is somewhere inside the current segment

         if ( pOp1 != NULL ) {
            mpSrc1 = CreateSegment( pOp1 );
            mSrc1Offset = base1Pos;
            if ( pOp2 != NULL ) {
               mpSrc2 = CreateSegment( pOp2 );
               mpSrc2->SetPosition( pos - base2Pos );
               mSrc2Offset = base2Pos;
               mSrc2Type = *pOp1;
               mNumSources = 2;
               // need to extend src1 to include spliced section
               mpSrc1->ReInit( pOp1[1] + pOp2[1] );
            } else {
               mNumSources = 1;
            }
            mpSrc1->SetPosition( pos - base1Pos );
         }


         notDone = FALSE;
         bytesLeft = (nextBasePos - pos) * mBytesPerSample;
      }

      basePos = nextBasePos;

      NextSegment( FALSE );
   }

   //
   // we have to re-set the bytes left count because NextSegment would reset the
   //  count to the total bytes in the current segment
   //
   mSegBytesLeft = bytesLeft;
}


//
// given a ptr to an op in a playlist, create the appropriate
//  type of cSndSegment, & init it
//
cSndSegment *
cSndSource::CreateSegment( uint32   *pOp )
{
   cSndSegment       *pSeg = NULL;
   cRezSegment       *pRezSeg = NULL;
   cFileSegment      *pFileSeg = NULL;
   cMemorySegment    *pMemSeg = NULL;
   SSPLRezSingle     *pRezSingle;
   SSPLFileSingle    *pFileSingle;
   SSPLMemSingle     *pMemSingle;
   SSPLRawMemSingle  *pRawMemSingle;
   SSPLRezDual       *pRezDual;
   SSPLFileDual      *pFileDual;
   SSPLMemDual       *pMemDual;
   SSPLRawMemDual    *pRawMemDual;
   sSndAttribs       attribs;
   BOOL              doDouble;

   doDouble = FALSE;
   attribs = mAttribs;

   TLOG2( "SSrc::CreateSegment [%d] op %d\n", mSerialNum, *pOp );

   switch ( *pOp ) {

      case plRezSingle:
         pRezSingle = (SSPLRezSingle *) pOp;
         pRezSeg = new cRezSegment;
         // handle the attribs of segment which may differ from source
         if ( (pRezSingle->format1 & 0xF) == kSndPlaylistFlagIMAADPCM ) {
            attribs.dataType = kSndDataIMAADPCM;
            attribs.bitsPerSample = 4;
         }
         doDouble = (pRezSingle->format1 & kSndPlaylistFlagDoDouble) ? TRUE : FALSE;
         if ( doDouble ) {
            // TBD - is this used? should it be doubled?
            attribs.sampleRate <<= 1;
         }
         attribs.bytesPerBlock = pRezSingle->format2 >> 16;
         attribs.samplesPerBlock = pRezSingle->format2 & 0xFFFF;
         // TBD - frequency doubling - does it change numSamples?
         attribs.numSamples = pRezSingle->nSamples;
         pRezSeg->Init( (Id) pRezSingle->id, pRezSingle->off,
                        &attribs, doDouble);
         pSeg = pRezSeg;
         break;

      case plFileSingle:
         pFileSingle = (SSPLFileSingle *) pOp;
         pFileSeg = new cFileSegment;
         // handle the attribs of segment which may differ from source
         if ( (pFileSingle->format1 & 0xF) == kSndPlaylistFlagIMAADPCM ) {
            attribs.dataType = kSndDataIMAADPCM;
            attribs.bitsPerSample = 4;
         }
         doDouble = (pFileSingle->format1 & kSndPlaylistFlagDoDouble) ? TRUE : FALSE;
         if ( doDouble ) {
            // TBD - is this used? should it be doubled?
            attribs.sampleRate <<= 1;
         }
         attribs.bytesPerBlock = pFileSingle->format2 >> 16;
         attribs.samplesPerBlock = pFileSingle->format2 & 0xFFFF;
         // TBD - frequency doubling - does it change numSamples?
         attribs.numSamples = pFileSingle->nSamples;
         pFileSeg->Init( pFileSingle->name, pFileSingle->off,
                        &attribs, doDouble);
         pSeg = pFileSeg;
         break;

      case plMemSingle:
         pMemSingle = (SSPLMemSingle *) pOp;
         pMemSeg = new cMemorySegment;
         // handle the attribs of segment which may differ from source
         if ( (pMemSingle->format1 & 0xF) == kSndPlaylistFlagIMAADPCM ) {
            attribs.dataType = kSndDataIMAADPCM;
            attribs.bitsPerSample = 4;
         }
         doDouble = (pMemSingle->format1 & kSndPlaylistFlagDoDouble) ? TRUE : FALSE;
         if ( doDouble ) {
            // TBD - is this used? should it be doubled?
            attribs.sampleRate <<= 1;
         }
         attribs.bytesPerBlock = pMemSingle->format2 >> 16;
         attribs.samplesPerBlock = pMemSingle->format2 & 0xFFFF;
         // TBD - frequency doubling - does it change numSamples?
         attribs.numSamples = pMemSingle->nSamples;
         pMemSeg->Init( pMemSingle->pData, pMemSingle->off,
                        &attribs, doDouble);
         pSeg = pMemSeg;
         break;

      case plRawMemSingle:
         pRawMemSingle = (SSPLRawMemSingle *) pOp;
         pMemSeg = new cMemorySegment;
         // TBD - frequency doubling - does it change numSamples?
         attribs.numSamples = pRawMemSingle->nSamples;
         pMemSeg->Init( pRawMemSingle->pData, 0, &attribs, doDouble);
         pSeg = pMemSeg;
         break;

      case plRezDual:
         pRezDual = (SSPLRezDual *) pOp;
         pRezSeg = new cRezSegment;
         // handle the attribs of segment which may differ from source
         if ( (pRezDual->format1 & 0xF) == kSndPlaylistFlagIMAADPCM ) {
            attribs.dataType = kSndDataIMAADPCM;
            attribs.bitsPerSample = 4;
         }
         doDouble = (pRezDual->format1 & kSndPlaylistFlagDoDouble) ? TRUE : FALSE;
         if ( doDouble ) {
            // TBD - is this used? should it be doubled?
            attribs.sampleRate <<= 1;
         }
         attribs.bytesPerBlock = pRezDual->format2 >> 16;
         attribs.samplesPerBlock = pRezDual->format2 & 0xFFFF;
         // TBD - frequency doubling - does it change numSamples?
         attribs.numSamples = pRezDual->nSamples;
         pRezSeg->Init( (Id) pRezDual->id, pRezDual->off,
                        &attribs, doDouble);
         pSeg = pRezSeg;
         break;

      case plFileDual:
         pFileDual = (SSPLFileDual *) pOp;
         pFileSeg = new cFileSegment;
         // handle the attribs of segment which may differ from source
         if ( (pFileDual->format1 & 0xF) == kSndPlaylistFlagIMAADPCM ) {
            attribs.dataType = kSndDataIMAADPCM;
            attribs.bitsPerSample = 4;
         }
         doDouble = (pFileDual->format1 & kSndPlaylistFlagDoDouble) ? TRUE : FALSE;
         if ( doDouble ) {
            // TBD - is this used? should it be doubled?
            attribs.sampleRate <<= 1;
         }
         attribs.bytesPerBlock = pFileDual->format2 >> 16;
         attribs.samplesPerBlock = pFileDual->format2 & 0xFFFF;
         // TBD - frequency doubling - does it change numSamples?
         attribs.numSamples = pFileDual->nSamples;
         pFileSeg->Init( pFileDual->name, pFileDual->off,
                        &attribs, doDouble);
         pSeg = pFileSeg;
         break;

      case plMemDual:
         pMemDual = (SSPLMemDual *) pOp;
         pMemSeg = new cMemorySegment;
         // handle the attribs of segment which may differ from source
         if ( (pMemDual->format1 & 0xF) == kSndPlaylistFlagIMAADPCM ) {
            attribs.dataType = kSndDataIMAADPCM;
            attribs.bitsPerSample = 4;
         }
         doDouble = (pMemDual->format1 & kSndPlaylistFlagDoDouble) ? TRUE : FALSE;
         if ( doDouble ) {
            // TBD - is this used? should it be doubled?
            attribs.sampleRate <<= 1;
         }
         attribs.bytesPerBlock = pMemDual->format2 >> 16;
         attribs.samplesPerBlock = pMemDual->format2 & 0xFFFF;
         // TBD - frequency doubling - does it change numSamples?
         attribs.numSamples = pMemDual->nSamples;
         pMemSeg->Init( pMemDual->pData, pMemDual->off,
                        &attribs, doDouble);
         pSeg = pMemSeg;
         break;

      case plRawMemDual:
         pRawMemDual = (SSPLRawMemDual *) pOp;
         pMemSeg = new cMemorySegment;
         // TBD - frequency doubling - does it change numSamples?
         attribs.numSamples = pRawMemDual->nSamples;
         pMemSeg->Init( pRawMemDual->pData, 0, &attribs, doDouble);
         pSeg = pMemSeg;
         break;

      default:
         assert( FALSE );  // error!
         break;
   }
   return pSeg;
}
