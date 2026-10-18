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

//
// Expand file-to-memory
//
long PkExplodeFileToMem(int fdSource, void * pDest,
                        long destSkip, long destMax);


#endif /* !__PKZIP_H */
