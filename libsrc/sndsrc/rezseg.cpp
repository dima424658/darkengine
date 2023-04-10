////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/sndsrc/RCS/rezseg.cpp $
// $Author: PATMAC $
// $Date: 1997/06/11 17:40:26 $
// $Revision: 1.1 $
//
// (c) 1997 Looking Glass Technologies Inc.
// Pat McElhatton
//
// Module name: resource sound segment reader
// File name: rezseg.cpp
//
// Description: low-level fetch of sound data from a resource -
//    data is a .WAV file image
//
////////////////////////////////////////////////////////////////////////

#include <sndsegi.h>

cRezSegment::cRezSegment()
{
}

cRezSegment::~cRezSegment()
{
   //TBD: who unlocks the rez (Nobody! we use ResExtractPartial!)
}


void
cRezSegment::Init( Id            rezId,
                   uint32        offset,
                   sSndAttribs   *pAttribs,
                   BOOL          doDouble )
{
   mRezId = rezId;
   InitBase( offset, pAttribs, doDouble );
}


void *
cRezSegment::GetRawData( void    *pDst,
                         uint32  nBytes )
{
   ResExtractPartial( mRezId, pDst, mSrcOffset, nBytes );

   return pDst;
}

