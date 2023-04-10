///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/res/RCS/pkzip.h $
// $Author: TOML $
// $Date: 1997/01/14 16:37:07 $
// $Revision: 1.1 $
//
// We only define the functions actually used by the resource library
//

#ifndef __PKZIP_H
#define __PKZIP_H

#ifdef _WIN32
//
// Compress memory-to-memory
//
long PkImplodeMemToMem(const void * pSource, long sizeSource,
                       void * pDest, long destMax);

//
// Expand file-to-memory
//
long PkExplodeFileToMem(int fdSource, void * pDest,
                        long destSkip, long destMax);

#else

#define PkImplodeMemToMem(pSource, sizeSource, pDest, destMax) LzwCompressBuff2Buff(pSource, sizeSource, pDest, destMax)
#define PkExplodeFileToMem(fdSource, pDest, destSkip, destMax) (0L)

#endif


#endif /* !__PKZIP_H */
