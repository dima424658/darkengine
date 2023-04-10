/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//////////////////////////////////////////////////////////////////////////
//
// $Header: x:/prj/tech/libsrc/namedres/rcs/resutil.h 1.1 1998/07/23 11:58:46 JUSTIN Exp $
//
// Utility methods for resources.
//

#ifndef __RESUTIL_H
#define __RESUTIL_H

#include <resapi.h>

struct sExtractData
{
   IRes *            pResource;
   tResBlockCallback Callback;
   void *            pCallbackData;
};

EXTERN long ResBaseExtractCallback(void *pBuf, 
                                   long nNumBytes, 
                                   long nIx, 
                                   void *pData);

//
// Turns a path into "canonical" form. At the moment, that is defined as
// forward slashes, since Dark requires that. We may want to get more
// sophisticated later...
//
EXTERN void CanonicalizePathname(char *pPathname);

#endif // !__RESUTIL_H
