////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/sound/RCS/sndfmt.cpp $
// $Author: PATMAC $
// $Date: 1998/03/20 12:55:54 $
// $Revision: 1.12 $
//
// (c) 1996 Looking Glass Technologies Inc.
// Pat McElhatton (from JohnB)
//
// Module name: sound formats
// File name: sndfmt.cpp
//
// Description: Implementation of sound file/resource format handling
//
////////////////////////////////////////////////////////////////////////

#include <lg.h>
#include <cstring>

#include <lgsndi.h>
#include <sndfmt.h>

#include <sndvoc.h>
#include <mprintf.h>

namespace
{
const uint16 kWaveFormatPcm = 0x0001;
const uint16 kWaveFormatImaAdpcm = 0x0011;

uint16 ReadU16(const uint8 *data)
{
   return static_cast<uint16>(data[0] | (data[1] << 8));
}

uint32 ReadU32(const uint8 *data)
{
   return static_cast<uint32>(data[0]) |
      (static_cast<uint32>(data[1]) << 8) |
      (static_cast<uint32>(data[2]) << 16) |
      (static_cast<uint32>(data[3]) << 24);
}

bool IsChunk(const uint8 *data, const char *name)
{
   return std::memcmp(data, name, 4) == 0;
}
}


//
// extract useful info from RIFF WAVE file image header
//  return TRUE if an error occurs
//
static BOOL
SndCrackWaveHeader(
                   void          *pRezData,
                   uint32        rezLen,
                   void          **ppData,
                   uint32        *pDataLen,
                   uint32        *pNumSamples,
                   sSndAttribs   *pAttribs )
{
   const uint8 *bytes = static_cast<const uint8 *>(pRezData);
   if (rezLen < 12 || !IsChunk(bytes, "RIFF") || !IsChunk(bytes + 8, "WAVE"))
      return TRUE;

   uint16 format = 0;
   bool haveFormat = false;
   bool haveFact = false;
   bool haveData = false;
   for (uint32 offset = 12; offset <= rezLen - 8; )
   {
      const uint8 *chunk = bytes + offset;
      const uint32 chunkLen = ReadU32(chunk + 4);
      const uint32 dataOffset = offset + 8;

      if (IsChunk(chunk, "fmt "))
      {
         if (chunkLen < 16 || chunkLen > rezLen - dataOffset)
            return TRUE;
         format = ReadU16(bytes + dataOffset);
         pAttribs->nChannels = ReadU16(bytes + dataOffset + 2);
         pAttribs->sampleRate = ReadU32(bytes + dataOffset + 4);
         pAttribs->bytesPerBlock = ReadU16(bytes + dataOffset + 12);
         pAttribs->bitsPerSample = ReadU16(bytes + dataOffset + 14);
         if (format == kWaveFormatImaAdpcm)
         {
            if (chunkLen < 20 || ReadU16(bytes + dataOffset + 16) < 2)
               return TRUE;
            pAttribs->samplesPerBlock = ReadU16(bytes + dataOffset + 18);
         }
         haveFormat = true;
      }
      else if (IsChunk(chunk, "fact"))
      {
         if (chunkLen < 4 || chunkLen > rezLen - dataOffset)
            return TRUE;
         *pNumSamples = ReadU32(bytes + dataOffset);
         haveFact = true;
      }
      else if (IsChunk(chunk, "data"))
      {
         if (chunkLen == 0)
            return TRUE;
         *ppData = const_cast<uint8 *>(bytes + dataOffset);
         *pDataLen = chunkLen;
         haveData = true;
         if (haveFormat && (format != kWaveFormatImaAdpcm || haveFact))
            break;
      }

      const uint32 paddedLen = chunkLen + (chunkLen & 1);
      if (paddedLen < chunkLen || paddedLen > rezLen - dataOffset)
         break;
      offset = dataOffset + paddedLen;
   }

   if (!haveFormat || !haveData || pAttribs->nChannels == 0 ||
       pAttribs->bitsPerSample == 0 || pAttribs->bytesPerBlock == 0)
      return TRUE;

   if (format == kWaveFormatPcm)
   {
      const uint32 bytesPerSample =
         (pAttribs->nChannels * pAttribs->bitsPerSample) / 8;
      if (bytesPerSample == 0)
         return TRUE;
      pAttribs->dataType = kSndDataPCM;
      pAttribs->samplesPerBlock = pAttribs->bytesPerBlock / bytesPerSample;
      *pNumSamples = *pDataLen / bytesPerSample;
   }
   else if (format == kWaveFormatImaAdpcm && haveFact &&
            pAttribs->samplesPerBlock != 0)
   {
      pAttribs->dataType = kSndDataIMAADPCM;
      const uint32 blocks = *pDataLen / pAttribs->bytesPerBlock;
      const uint32 partial = *pDataLen % pAttribs->bytesPerBlock;
      uint32 maxSamples = blocks * pAttribs->samplesPerBlock;
      if (partial >= 4)
         maxSamples += 1 + 2 * (partial - 4);
      if (*pNumSamples > maxSamples)
         *pNumSamples = maxSamples;
   }
   else
   {
      return TRUE;
   }

   pAttribs->numSamples = *pNumSamples;
   return FALSE;
}

//
// extract useful info from VOC file image header
//  return TRUE if an error occurs
//
static BOOL
SndCrackVocHeader(
                  void          *pRezData,
                  uint32        rezLen,
                  void          **ppData,
                  uint32        *pDataLen,
                  uint32        *pNumSamples,
                  sSndAttribs   *pAttribs )
{
   uint8 blockType;
   uint8 *pVoc = (uint8 *) pRezData;
   uint8 *pEndRez = pVoc + rezLen;
   BOOL voiceBlockFound = FALSE;
	uint32 sampleRate;
	int16 bits;
	int16 channels;

   sVocHeader *hdr = (sVocHeader *) pVoc;
   
   sVocDataBlock       *dataBlock;
//   voc_continue_block   *contBlock;
   sVocStereoBlock     *stereoBlock;
   sVocExtendedBlock  *extendedBlock;
   
   pVoc += hdr->offData; // fast forward to data

   while ( !voiceBlockFound ) {
      if ( pVoc >= pEndRez ) {
         // got to the end of VOC without finding a data block
         break;
      }

      blockType = *pVoc; // just read the id off the top.

      switch(blockType)
      {
         case VOC_BLOCK_TERM:
            return TRUE;
         case VOC_BLOCK_DATA:
            dataBlock = (sVocDataBlock *)pVoc;
				
				channels = 1;
				bits = 8;
				sampleRate = 1000000L / (256 - dataBlock->timeConstant);

            *ppData = pVoc + sizeof(sVocDataBlock);
				*pDataLen = VOC_BLOCK_LEN(pVoc) - 2;
				voiceBlockFound = TRUE;
            break;
         case VOC_BLOCK_STEREO:
            stereoBlock = (sVocStereoBlock *)pVoc;
            if(stereoBlock->voiceMode)
            {
               bits = 8;
					channels = 2;
					sampleRate = 
                  128000000L / (65536L - stereoBlock->timeConstant);
            }
            else
            {
               bits = 8;
					channels = 1;
					sampleRate = 
                  256000000L / (65536L - stereoBlock->timeConstant);
            }
            pVoc += VOC_BLOCK_LEN(pVoc) + 4; // go to the end of the block

            *ppData = pVoc + sizeof(sVocDataBlock);
				*pDataLen = VOC_BLOCK_LEN(pVoc) - 2;
            voiceBlockFound = TRUE;
            break;

		 case VOC_BLOCK_EXTENDED:
            extendedBlock = (sVocExtendedBlock *)pVoc;
            *ppData = pVoc + sizeof(sVocExtendedBlock);
				*pDataLen = VOC_BLOCK_LEN(pVoc) - 12;
				sampleRate = extendedBlock->sampleRate;
            channels = extendedBlock->channels;

            if(extendedBlock->format == 0)
					bits = 8;
            else if(extendedBlock->format == 4)
					bits = 16;
            voiceBlockFound = TRUE;
            break;
         default:
            pVoc += VOC_BLOCK_LEN(pVoc) + 4;
            break;
      }
   }  // end while !voiceBlockFound
	
   if ( voiceBlockFound == TRUE ) {
      // fill in sound attributes & samples-in-resource
      pAttribs->nChannels = channels;
      pAttribs->bitsPerSample = bits;
      pAttribs->sampleRate = sampleRate;
      pAttribs->bytesPerBlock = channels * (bits / 8);
      pAttribs->dataType = kSndDataPCM;
      pAttribs->samplesPerBlock = 1;
      *pNumSamples = *pDataLen / pAttribs->bytesPerBlock;
   }
   return !voiceBlockFound;
}

//
// get header info from sound resource (mem-resident sound file image)
// return TRUE if failure occurs
//
BOOL
SndCrackRezHeader(
                  void          *pRezData,
                  uint32        rezLen,
                  void          **ppData,
                  uint32        *pDataLen,
                  uint32        *pNumSamples,
                  sSndAttribs   *pAttribs )
{
   char *buf = (char *) pRezData;
   BOOL bad;

   // determine the file image type (WAVE or VOC)
   // extract sound attribs from rez header
   // find start of audio data
   if ( (strncmp( buf, "RIFF", 4) == 0 )
        && (strncmp( buf + 8, "WAVE", 4) == 0) ) {
      // sound resource is a wave file image
      bad = SndCrackWaveHeader( pRezData, rezLen, ppData, pDataLen,
                                 pNumSamples, pAttribs );
      if ( bad ) mprintf( "SndCrackWaveHeader returned error\n");
      TLOG3("SndCrackWaveHeader %ld bytes, %ld samples %d badFlag",
           rezLen, *pNumSamples, bad );
   } else if ( strncmp( buf, "Creative Voice File", 19 ) == 0 ) {
      // sound resource is a voc file image
      //TBD!
      bad = SndCrackVocHeader( pRezData, rezLen, ppData, pDataLen,
                               pNumSamples, pAttribs );
      if ( bad ) mprintf( "SndCrackVocHeader returned error\n");
      TLOG3("SndCrackVocHeader %ld bytes, %ld samples %d badFlag",
           rezLen, *pNumSamples, bad );
   } else {
      // Error - Rez is not a recognized sound file image type
      mprintf("Unrecognizable sound file type\n");
      bad = TRUE;
   }

   return bad;
}

