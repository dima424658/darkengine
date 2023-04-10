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


IMAADPCM::IMAADPCM( void )
{
   mPredictedSample = 0;
   mNybbleToggle = 0;
   mSpareNybble = 0;
   mIndex = 0;
}


IMAADPCM::~IMAADPCM( void )
{
}


/* 
 * initialize compressor state
 */
void
IMAADPCM::Init(
               int   predSample,
               int   index
               )
{
   mPredictedSample = predSample;
   mIndex = index;
   mNybbleToggle = 0;
   mSpareNybble = 0;
}


/*
 * get compressor state packed into a longword
 */
unsigned long 
IMAADPCM::GetState( void )
{
   unsigned short hiWord;
   unsigned long val;

   val = mPredictedSample & 0xFFFF;
   hiWord = (unsigned short) ((mIndex & 0xFF) | ((mSpareNybble & 0xF0) << 4)
      | ((mNybbleToggle & 1) << 12));
   return val | (hiWord << 16);
}


/*
 * set compressor state from packed longword
 */
void
IMAADPCM::SetState( unsigned long state )
{
   short val = (short) (state & 0xFFFF);

   mPredictedSample = val;
   state >>= 16;
   mIndex = state & 0xFF;
   mSpareNybble = (state >> 4) & 0xF0;
   mNybbleToggle = (state >> 12) & 1;
}


/*
 * compress a short block of size numSamples
 *  returns # of bytes written to outBlock
 */
long
IMAADPCM::Compress(
                   short      *inBlock,
                   char       *outBlock,
                   long       numSamples
                   )
{
   int   inSampleIndex, outSampleIndex;
   int	tempstep, mask;

   int   code;           /* 4-bit quantized difference */ 
   long  diff;           /* difference between 2 consecutive samples */
   long  pdiff;          /* difference between 2 consecutive samples */
   int step;             /* step sizes at encoding and decoding portion */

   outSampleIndex = 0;

   /* process buffer */
   for (inSampleIndex = 0; inSampleIndex < numSamples; inSampleIndex++)    {

      step = steptab[mIndex];
      /* difference between actual sample & predicted value */
      diff = (long)inBlock[ inSampleIndex] - mPredictedSample;

      if( diff >= 0 ) {
         code=0; 			/* set sign bit */
      } else {
         code=8;
         diff = -diff;
      }
      mask = 4;
      pdiff = 0;
      tempstep = step;

      if( diff >= tempstep ){
         code |= 4;
         diff -= tempstep;
         pdiff += step;
      }
      tempstep >>=1;
      if( diff >= tempstep ){
         code |= 2;
         diff -= tempstep;
         pdiff += (step >> 1);
      }
      tempstep >>=1;
      if( diff >= tempstep ){
         code |= 1;
         diff -= tempstep;
         pdiff += (step >> 2);
      }
      pdiff += (step >> 3);
      if ( code & 8 ) {
         pdiff = -pdiff;
      }

      /* lower 4-bit of code can be sent out or stored at this point */

      if ( mNybbleToggle ) {
         outBlock[ outSampleIndex ] = (char) (mSpareNybble + (code & 0xF));
         outSampleIndex++;
      } else {
         /* hiNybble is even sample */
         mSpareNybble = code << 4;
      }
      mNybbleToggle ^= 1;

      /* compute new sample estimate mPredictedSample */
      mPredictedSample += pdiff;

      if (mPredictedSample > 32767)				/* check for overflow */
         mPredictedSample = 32767;
      else if (mPredictedSample < -32768)
         mPredictedSample = -32768;

      /* compute new stepsize step */
      mIndex += indextab[code];
      if(mIndex < 0)mIndex = 0;
      else if(mIndex > 88)mIndex = 88;
   }

   return outSampleIndex;
}

/*
 * end compression - flush last nybble, if any to buffer
 * return # of bytes written to buffer (0 or 1)
 */
long
IMAADPCM::EndCompress(
                      char *outBlock
                      )
{
   if ( mNybbleToggle ) {
      *outBlock = (char) mSpareNybble;
      return 1;
   } else {
      return 0;
   }
}


/*
 * decompress a short block of size numSamples
 *  returns # of bytes consumed from inBlock
 */
long
IMAADPCM::Decompress(
                     char        *inBlock,
                     short       *outBlock,
                     long        numSamples
                     )
{
   int inByteIndex, outSampleIndex;
   int code;                /* 4-bit quantized difference */ 
   long diff;               /* difference between 2 consecutive samples */
   int step;                /* step sizes at encoding and decoding portion */

   inByteIndex = 0;

   /* process buffer */
   for (outSampleIndex = 0; outSampleIndex < numSamples; outSampleIndex++)    {

      // NOTE: The nybble order for PCs is opposite from Unix
      // Unix has high nybble first, PCs have low nybble first
      /* fetch a nybble code to decompress */
      if ( mNybbleToggle ) {
         //      code = mSpareNybble & 0xF;
         code = (mSpareNybble & 0xF0) >> 4;
         inByteIndex++;
      } else {
         mSpareNybble = inBlock[ inByteIndex ];
         code = mSpareNybble & 0xF;
         //      code = (mSpareNybble & 0xF0) >> 4;
      }
      mNybbleToggle ^= 1;

      /* compute new sample estimate mPredictedSample */
      diff = 0;
      step = steptab[mIndex];
      if (code & 4)       diff += step;
      if (code & 2)       diff += (step >> 1);
      if (code & 1)       diff += (step >> 2);
      diff += step >> 3;
      if (code & 8)       diff = -diff;
      mPredictedSample += diff;

      if (mPredictedSample > 32767)				/* check for overflow */
         mPredictedSample = 32767;
      else if (mPredictedSample < -32768)
         mPredictedSample = -32768;

      /* compute new stepsize step */
      mIndex += indextab[code];
      if (mIndex < 0) mIndex = 0;
      else if (mIndex > 88) mIndex = 88;

      /* output predicted sample */
      outBlock[outSampleIndex] = (short) mPredictedSample;
   }

   // return # of bytes consumed (not including partially consumed)
   return inByteIndex;
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
   IMAADPCM       leftDecomp, rightDecomp;
   IMAHeaderWord  *pIMAHdr;

   // Each block starts with a IMAHeaderWord, which contains
   // the predicted first sample of the block and the stepsize
   // index.  Note that 
   if ( nSamples > 1 ) {
      switch( nChannels ) {
         case 1:
            // decompress a block
            pIMAHdr = (IMAHeaderWord *) pInBuff;
            pOutBuff[0] = pIMAHdr->curValue;
            leftDecomp.Init( pIMAHdr->curValue, pIMAHdr->stepIndex );
            leftDecomp.Decompress( pInBuff + sizeof(IMAHeaderWord),
                                   pOutBuff + 1, nSamples - 1 );
            break;
         case 2:
            // stereo - channels are interleaved every 8 samples
            // TBD!
            leftDecomp.Init( pIMAHdr->curValue, pIMAHdr->stepIndex );
            rightDecomp.Init( pIMAHdr->curValue, pIMAHdr->stepIndex );
            break;
         default:
            return -1;
      }
   }
   return nSamples;
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
   IMAADPCM       decomp;
   IMAHeaderWord  *pIMAHdr;
   long bytesEaten;

   // decompress a block
   if ( getHeader ) {
      // This is first fetch in this block, so we must set
      // the decompressor state from the header word at the
      // start of the block
      pIMAHdr = (IMAHeaderWord *) pInBuff;
      pOutBuff[0] = pIMAHdr->curValue;
      if ( nSamples > 1 ) {
         decomp.Init( pIMAHdr->curValue, pIMAHdr->stepIndex );
         bytesEaten = decomp.Decompress( pInBuff + sizeof(IMAHeaderWord),
                                         pOutBuff + 1, nSamples - 1 );
      }
      // save the decompressor state for future calls
      *pState = decomp.GetState();
      pInBuff += ( sizeof(IMAHeaderWord) + bytesEaten );
   } else {
      // this is not the first fetch in the block, so we must
      // set the state from our saved packed state
      decomp.SetState( *pState );
      bytesEaten = decomp.Decompress( pInBuff, pOutBuff, nSamples );
      // save the decompressor state for future calls
      *pState = decomp.GetState();
      pInBuff += bytesEaten;
   }

   return pInBuff;
}

