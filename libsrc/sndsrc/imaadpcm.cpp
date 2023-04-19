////////////////////////////////////////////////////////////////////////
//
// (c) 1996 Looking Glass Technologies Inc.
// Pat McElhatton
//
// Module name: IMA ADPCM definitions
// File name: imaadpcm.h
//
// Description: IMA ADPCM (aka DVI) compress/decompress definitions
//
////////////////////////////////////////////////////////////////////////

/**************************************************************************

	Program to read in and code digitized stereo audio
  	using Intel/DVI's implementation of the classic ADPCM algorithm
	described in the following references:
	
   N. S. Jayant, "Adaptive Quantization With a One Word Memory,"
   Bell System Tech. J., pp. 119-1144, Sep. 1973

   L. R. Rabiner / R. W. Schafer, Digital Processing of Speech
   Signals, Prentice Hall, 1978

 	Mark Stout, Compaq Computer, 1/92

  4-feb-92     patmc    Broken into routines
  11-jul-96    patmc    converted to C++

****************************************************************************/

#include "imaadpcm.h"

static int indextab[16] = {-1,-1,-1,-1, 2, 4, 6, 8,
			     -1,-1,-1,-1, 2, 4, 6, 8};

static int steptab[89]={
    7,   8,   9,  10,  11,  12,  13, 14, /* DVI exten. */
   16,  17,  19,  21,  23,  25,  28,	/* OKI lookup table */
   31,  34,  37,  41,  45,  50,  55,
   60,  66,  73,  80,  88,  97, 107,
   118, 130, 143, 157, 173, 190, 209,
   230, 253, 279, 307, 337, 371, 408,
   449, 494, 544, 598, 658, 724, 796,
   876, 963,1060,1166,1282,1411,1552,

			   1707,1878,						/* DVI exten. */

			   2066,2272,2499,2749,3024,3327,3660,4026,
			   4428,4871,5358,5894,6484,7132,7845,8630,
			   9493,10442,11487,12635,13899,15289,16818,
			   18500,20350,22385,24623,27086,29794,32767
			   };


IMA_ADPCM::IMA_ADPCM( void )
{
   Init(FALSE, 0, 0, 0, 0);
}


IMA_ADPCM::~IMA_ADPCM( void )
{
}


/* 
 * initialize compressor state
 */
void
IMA_ADPCM::Init(
               BOOL stereo,
               int predSample1,
               int index1,
               int predSample2,
               int index2
               )
{
   mCh1PredictedSample = predSample1;
   mCh1Index = index1;
   mCh1SpareNybble = 0;
   mCh2PredictedSample = predSample2;
   mCh2Index = index2;
   mCh2SpareNybble = 0;
   mNybbleToggle = 0;
}

/*
 * compress a short block of size numSamples
 *  returns # of bytes written to outBlock
 */
long
IMA_ADPCM::Compress(
                   short      *inBlock,
                   char       *outBlock,
                   long       numSamples
                   )
{
    return 0;
}

/*
 * end compression - flush last nybble, if any to buffer
 * return # of bytes written to buffer (0 or 1)
 */
long
IMA_ADPCM::EndCompress(
                      char *outBlock
                      )
{
    return 0;
}


/*
 * decompress a short block of size numSamples
 *  returns # of bytes consumed from inBlock
 */
long
IMA_ADPCM::Decompress(
                     char        *inBlock,
                     short       *outBlock,
                     long        numSamples
                     )
{
    return 0;
}


//
// decompress a block of IMA ADPCM data (RIFF WAVE block format)
//
long
DecompressIMABlock(
                   char          *pInBuff,
                   short         *pOutBuff,
                   long          nSamples,
                   int           nChannels
                   )
{
    return 0;
}


//
// decompress a block of IMA ADPCM data (RIFF WAVE block format)
//  returns ptr to next byte in input buffer
//
char *
DecompressIMABlockPartial( char           *pInBuff,
                           short          *pOutBuff,
                           long           nSamples,
                           BOOL           getHeader,
                           unsigned long  *pState )
{
   return nullptr;
}

