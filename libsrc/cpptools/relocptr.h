///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/cpptools/RCS/relocptr.h $
// $Author: TOML $
// $Date: 1996/09/16 15:26:23 $
// $Revision: 1.5 $
//

#ifndef __RELOCPTR_H
#define __RELOCPTR_H

#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cRelocatationSetBase
//

class cRelocationSetBase
{
public:
    ~cRelocationSetBase();
    void RemoveAll();
protected:
    cRelocationSetBase(const void * pInitialBase);

    void Insert(void **);
    void Remove(void **);
    void SetBaseAddress(const void * pNewBase);

private:
    enum
    {
        kIncrBlockPtrs  = 16,
        kNotFound       = -1
    };

    int Find(void **);

    uint8 *   m_pBaseAddr;        // The base address of the block
    int       m_nPtrs;            // number of monitored ptrs in block
    int       m_nAllocedPtrs;     // Space for num pointers allocated
    uint8 *** m_PtrList;          // List of addresses of monitored pointers in block

};

///////////////////////////////////////
//
// Construct a relocation set
//

inline cRelocationSetBase::cRelocationSetBase(const void *baseAddr)
  : m_pBaseAddr((uint8 *) baseAddr),
    m_nPtrs(0),
    m_nAllocedPtrs(0),
    m_PtrList(NULL)
{
}

///////////////////////////////////////
//
// Destruction of a set
//

inline cRelocationSetBase::~cRelocationSetBase(void)
{
    if (m_PtrList != NULL)
        free(m_PtrList);
}

///////////////////////////////////////
//
// Remove all pointers from the set
//

inline void cRelocationSetBase::RemoveAll()
{
    m_nPtrs = 0;
}

///////////////////////////////////////////////////////////////////////////////
//
// TEMPLATE: cRelocatationSet
//

template <class TYPE>
class cRelocationSet : public cRelocationSetBase
{
public:
    cRelocationSet(const TYPE * pInitialBase);

    void Insert(TYPE **);
    void Remove(TYPE **);
    void SetBaseAddress(const TYPE * pNewBase);

};

///////////////////////////////////////

template <class TYPE>
inline cRelocationSet<TYPE>::cRelocationSet(const TYPE * pInitialBase)
  : cRelocationSetBase(pInitialBase)
{
}

///////////////////////////////////////

template <class TYPE>
inline void cRelocationSet<TYPE>::Insert(TYPE ** pp)
{
    cRelocationSetBase::Insert(pp);
}

///////////////////////////////////////

template <class TYPE>
inline void cRelocationSet<TYPE>::Remove(TYPE ** pp)
{
    cRelocationSetBase::Remove(pp);
}

///////////////////////////////////////


template <class TYPE>
inline void cRelocationSet<TYPE>::SetBaseAddress(const TYPE * pNewBase)
{
    cRelocationSetBase::SetBaseAddress(pNewBase);
}

///////////////////////////////////////////////////////////////////////////////

#endif /* !__RELOCPTR_H */
