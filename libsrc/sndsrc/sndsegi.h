////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/sndsrc/RCS/sndsegi.h $
// $Author: PATMAC $
// $Date: 1998/01/03 00:33:57 $
// $Revision: 1.2 $
//
// (c) 1997 Looking Glass Technologies Inc.
// Pat McElhatton
//
// Module name: sound segment internal classes
// File name: sndsegi.h
//
// Description: definition of sound segment internal objects
//
////////////////////////////////////////////////////////////////////////

#include <sndseg.h>
#include <res.h>

class cRezSegment : public cSndSegment

{
public:

   cRezSegment( void );

   virtual ~cRezSegment( void );

   void *GetRawData( void *pDst, uint32 nBytes );

   // initialize common base class members
   void Init( Id rezId, uint32 offset, sSndAttribs *pAttribs, BOOL doDouble );

private:
   Id             mRezId;
};


class cFileSegment : public cSndSegment
{
public:

   cFileSegment( void );

   virtual ~cFileSegment( void );

   void *GetRawData( void *pDst, uint32 nBytes );

   // initialize common base class members
   void Init( char *pName, uint32 offset, sSndAttribs *pAttribs, BOOL doDouble );
              

private:
   FILE           *mpInFile;

};


class cMemorySegment : public cSndSegment
{
public:

   cMemorySegment( void );

   virtual ~cMemorySegment( void );

   void *GetRawData( void *pDst, uint32 nBytes );

   void Init( void *pData, uint32 offset, sSndAttribs *pAttribs, BOOL doDouble );

private:
   char  *mpData;
};
