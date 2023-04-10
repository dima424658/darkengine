////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/sndsrc/RCS/fileseg.cpp $
// $Author: PATMAC $
// $Date: 1997/06/24 16:36:48 $
// $Revision: 1.2 $
//
// (c) 1997 Looking Glass Technologies Inc.
// Pat McElhatton
//
// Module name: file sound segment reader
// File name: fileseg.cpp
//
// Description: low-level fetch of sound data from a .WAV file
//
////////////////////////////////////////////////////////////////////////

#include <sndsegi.h>
#include <mprintf.h>

cFileSegment::cFileSegment()
{
   mpInFile = NULL;
}

cFileSegment::~cFileSegment()
{
   if ( mpInFile != NULL ) {
      fclose( mpInFile );
   }
}

void
cFileSegment::Init( char         *pName,
                    uint32       offset,
                    sSndAttribs  *pAttribs,
                    BOOL         doDouble )
{
   mpInFile = fopen( pName, "rb" );
   assert( mpInFile != NULL );

   InitBase( offset, pAttribs, doDouble );
}

void *
cFileSegment::GetRawData( void    *pDst,
                          uint32  nBytes )
{
   int nRead;
   if ( mNeedsSeek ) {
      fseek( mpInFile, mSrcOffset, SEEK_SET );
   }
   //mprintf("fileseg %x   %d bytes @ %d seek: %d\n", this, nBytes, mSrcOffset, mNeedsSeek);
   nRead = fread( pDst, 1, nBytes, mpInFile );
   assert( nRead == nBytes );

   return pDst;
}

