///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lgalloc/RCS/pool.h $
// $Author: TOML $
// $Date: 1997/07/15 21:28:02 $
// $Revision: 1.5 $
//

#ifndef __POOL_H
#define __POOL_H

#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////////

typedef struct {char unused;} *HPOOL;

//
// Create a pool of fixed sized objects
//
EXTERN HPOOL LGAPI PoolCreate(size_t);

//
// Allocate an item from the pool
//
EXTERN void * LGAPI PoolAlloc(HPOOL hPool);

//
// Free an item from the pool
//
EXTERN void LGAPI PoolFree(HPOOL pPool, void *pData);

//
// Destroy the pool
//
EXTERN void LGAPI PoolDestroy(HPOOL pPool);


///////////////////////////////////////////////////////////////////////////////

#if __cplusplus

// Macros to implement class-specific new/delete using a fixed size pool.
//
// In the header:
//     class cMyClass
//         {
//         DECLARE_POOL()
//     public:
//         };
//
// In the source:
//     IMPLEMENT_POOL(cMyClass)
//
// If your pooled class is used as a static anywhere, you will need to
// add this to the source file where IMPLEMENT_POOL() is located:
//
// Watcom:          #pragma initialize library
// MS/Symantec:     #pragma init_seg (lib)

class _cPoolWrapper
    {
public:
    _cPoolWrapper(size_t size)           : m_size(size), hPool(PoolCreate(size)) {}
    ~_cPoolWrapper()                       {PoolDestroy(hPool);}
    void *  PoolAlloc(size_t size)         {return size==m_size ? ::PoolAlloc(hPool) : malloc(size);}
    void    PoolFree(void *p, size_t size) {if (size==m_size) ::PoolFree(hPool, p); else free(p);}
private:
    size_t m_size;
    HPOOL hPool;
    };

#define DECLARE_POOL() \
    public:             \
        void *operator new(size_t );    \
        void *operator new(size_t, const char *, int);    \
        void operator delete(void* , size_t ); \
    private:            \
        static _cPoolWrapper m_Pool;

#define IMPLEMENT_POOL(CLASS) \
    _cPoolWrapper CLASS::m_Pool(sizeof(CLASS)); \
    void *CLASS::operator new(size_t n)             {return m_Pool.PoolAlloc(n);} \
    void *CLASS::operator new(size_t n, const char *, int) {return m_Pool.PoolAlloc(n);} \
    void CLASS::operator delete(void* p, size_t n)  {m_Pool.PoolFree(p,n);}

#endif

///////////////////////////////////////////////////////////////////////////////

#endif /* !__POOL_H */
