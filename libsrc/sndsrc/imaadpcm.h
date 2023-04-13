/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

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

#ifndef IMAADPCM_H
#define IMAADPCM_H

#ifdef __cplusplus

#define kIMAADPCM_FlagStereo 1

class IMA_ADPCM {
public:
   IMA_ADPCM();
   virtual        ~IMA_ADPCM();

   // set compressor/decompressor state
   void           Init( BOOL stereo, int predSample1, int index1, int predSample2, int index2 );

   // return # of samples generated
   long           Compress( short *pIn, char *pOut, long nSamps );

   // return # of samples generated (0 or 1)
   long           EndCompress( char *pOut );

   // return # of bytes consumed
   long           Decompress( char *pIn, short *pOut, long nSamps );

   /*
   // return packed compressor state
   unsigned long  GetState( void );

   // set compressor state
   void           SetState( unsigned long );
   */

private:
   /* compressor state maintained between blocks */
   /*
    * predictedSample and index are the real compressor state,
    *  nybbleToggle & spareNybble are just used when conversion
    *  stops on an odd sample boundary
    */
   long   mFlags;
   long   mNybbleToggle;
   long   mStereoChunkIndex;
   long   mCh1PredictedSample;
   short  mCh1Index;
   short  mCh1SpareNybble;
   long   mCh2PredictedSample;
   short  mCh2Index;
   short  mCh2SpareNybble;
};

#endif

// decompress a block of IMA ADPCM, which is preceeded with
//   a header which holds the current predicted value & steptable index
// this is used to decode IMA ADPCM from RIFF WAVE files
// return # of samples generated
#ifdef __cplusplus
extern "C" {
#endif

   //long DecompressIMABlock( char *pIn, short *pOut, long nSamps, int nChans );

//
// decompress nSamps from pIn to pOut, getHeader must be true on first
//  fetch of a new block, pState points to a longword which holds the
//  decompressor state between calls in the same block
//
char *DecompressIMABlkPartial( IMA_ADPCM* pDecoder, BOOL isStereo, char *pIn,
                               short *pOut, long nSamps, BOOL getHeader); //, unsigned long *pState );

// this is the header that starts each ADPCM block
//  it needs to be public because app needs to know its size to
//  detect bogus file conditions (block which has incomplete header)
typedef struct {
   short             curValue;
   unsigned char     stepIndex;
   char              crud;
} IMAHeaderWord;

#ifdef __cplusplus
};
#endif


#endif // IMAADPCM_H

