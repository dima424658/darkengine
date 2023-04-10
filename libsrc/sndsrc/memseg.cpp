////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/sndsrc/RCS/memseg.cpp $
// $Author: PATMAC $
// $Date: 1998/01/03 00:33:33 $
// $Revision: 1.3 $
//
// (c) 1997 Looking Glass Technologies Inc.
// Pat McElhatton
//
// Module name: memory sound segment reader
// File name: memseg.cpp
//
// Description: low-level fetch of sound data from a buffer
//
////////////////////////////////////////////////////////////////////////

#include <sndsegi.h>

cMemorySegment::cMemorySegment()
{
   mpData = NULL;
}

cMemorySegment::~cMemorySegment()
{
   //TBD: who frees the memory buffer
}

void
cMemorySegment::Init( void          *pData,
                      uint32        offset,
                      sSndAttribs   *pAttribs,
                      BOOL          doDouble )
{
   assert( pData != NULL );
   mpData = (char *) pData;
   InitBase( offset, pAttribs, doDouble );
}


void *
cMemorySegment::GetRawData( void    * /* pDst */,
                            uint32  /* nBytes */ )
{
   char *pMem;

   assert ( mpData != NULL );

   pMem = (char *) mpData;
   pMem += mSrcOffset;

   return (void *) pMem;
}

