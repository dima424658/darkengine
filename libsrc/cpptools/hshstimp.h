///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/cpptools/RCS/hshstimp.h $
// $Author: TOML $
// $Date: 1997/12/21 14:50:51 $
// $Revision: 1.4 $
//
// (c) Copyright 1993-1996 Tom Leonard. All Rights Reserved. Unlimited license granted to Looking Glass Technologies Inc.
//

#ifndef __HSHSTIMP_H
#define __HSHSTIMP_H

#define __CPPTOOLSAPI

#include <pool.h>

DECLARE_HANDLE(tHashSetKey);
DECLARE_HANDLE(tHashSetNode);

class cHashSetBase;

#define HASHSET_DECLARE_POOL()          DECLARE_POOL()
#define HASHSET_IMPLEMENT_POOL(ofkind)  IMPLEMENT_POOL(ofkind)

struct __CPPTOOLSAPI sHashSetChunk
{
    tHashSetNode node;
    sHashSetChunk *pNext;

    HASHSET_DECLARE_POOL();
};

struct tHashSetHandle
{
private:
   friend class cHashSetBase;

   unsigned        Index;
   sHashSetChunk * pChunk;
   sHashSetChunk * pPrev;
};

#endif /* !__HSHSTIMP_H */
