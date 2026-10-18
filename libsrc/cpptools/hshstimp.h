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

#include <stdint.h>
#include <pool.h>

typedef uintptr_t tHashSetKey;
typedef uintptr_t tHashSetNode;

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
