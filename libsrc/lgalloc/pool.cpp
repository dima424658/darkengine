///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lgalloc/RCS/pool.cpp $
// $Author: TOML $
// $Date: 1997/07/10 17:13:15 $
// $Revision: 1.6 $
//

#include <lg.h>
#undef Free
#include <pool.h>
#include <poolimp.h>

///////////////////////////////////////////////////////////////////////////////

HPOOL LGAPI PoolCreate(size_t size)
{
    return (HPOOL)(new cPoolAllocator(size));
}

///////////////////////////////////////

void * LGAPI PoolAlloc(HPOOL hPool)
{
    return ((cPoolAllocator * )hPool)->Alloc();
}

///////////////////////////////////////

void LGAPI PoolFree(HPOOL hPool, void * pData)
{
    ((cPoolAllocator *)hPool)->Free(pData);
}

///////////////////////////////////////

void LGAPI PoolDestroy(HPOOL hPool)
{
    delete ((cPoolAllocator *)hPool);
}

///////////////////////////////////////////////////////////////////////////////
