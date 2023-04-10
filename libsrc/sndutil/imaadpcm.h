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


#ifdef __cplusplus

class IMAADPCM {
public:
   IMAADPCM();
   virtual        ~IMAADPCM();

   // set compressor/decompressor state
   void           Init( int predSample, int index );

   // return # of samples generated
   long           Compress( short *pIn, char *pOut, long nSamps );

   // return # of samples generated (0 or 1)
   long           EndCompress( char *pOut );

   // return # of bytes consumed
   long           Decompress( char *pIn, short *pOut, long nSamps );

   // return packed compressor state
   unsigned long  GetState( void );

   // set compressor state
   void           SetState( unsigned long );

private:
   /* compressor state maintained between blocks */
   /*
    * predictedSample and index are the real compressor state,
    *  nybbleToggle & spareNybble are just used when conversion
    *  stops on an odd sample boundary
    */
   int    mPredictedSample;
   int    mIndex;
   int    mNybbleToggle;
   int    mSpareNybble;   
};

#endif

// decompress a block of IMA ADPCM, which is preceeded with
//   a header which holds the current predicted value & steptable index
// this is used to decode IMA ADPCM from RIFF WAVE files
// return # of samples generated
#ifdef __cplusplus
extern "C" {
#endif

long DecompressIMABlock( char *pIn, short *pOut, long nSamps, int nChans );

//
// decompress nSamps from pIn to pOut, getHeader must be true on first
//  fetch of a new block, pState points to a longword which holds the
//  decompressor state between calls in the same block
//
char *DecompressIMABlockPartial( char *pIn, short *pOut, long nSamps,
                                 BOOL getHeader, unsigned long *pState );

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

