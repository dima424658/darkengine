///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/cpptools/RCS/relocptr.cpp $
// $Author: TOML $
// $Date: 1997/10/15 12:10:21 $
// $Revision: 1.10 $
//

#include <lg.h>
#include <mprintf.h>
#include <relocptr.h>

#ifndef NO_DB_MEM
// Must be last header
#include <memall.h>
#include <dbmem.h>
#endif

///////////////////////////////////////
//
// Add a pointer to the relocation set
//

int cRelocationSetBase::Find(void **ptr)
{
    for (int i = m_nPtrs; --i >= 0;)
        if (m_PtrList[i] == (uint8**)ptr)
            return i;

    return kNotFound;
}


///////////////////////////////////////
//
// Add a pointer to the relocation set
//

void cRelocationSetBase::Insert(void **ptr)
{
#if DEBUG
    if (Find(ptr) != kNotFound)
    {
        CriticalMsg2("Relocation set must only have one of any given pointer (0x%x, 0x%x)", ptr, *ptr);
        return;
    }
#endif
    AssertMsg2(*ptr >= m_pBaseAddr, "Pointers in relocation set should be above the base", ptr, *ptr);

    // Now we need to add the pointer to the monitored pointer list,
    // which means allocating more space for the list if the list is full.
    if (m_nPtrs == m_nAllocedPtrs)
    {
        m_nAllocedPtrs += kIncrBlockPtrs;
        if (m_PtrList)
            m_PtrList = (uint8 ***) realloc(m_PtrList, m_nAllocedPtrs * sizeof(uint8 **));
        else
            m_PtrList = (uint8 ***) malloc(m_nAllocedPtrs * sizeof(uint8 **));

    }

    // Now that we've got a valid space to put the pointer, let's put it in.
    m_PtrList[m_nPtrs++] = (uint8 **) ptr;
}


///////////////////////////////////////
//
// Remove a pointer from the set
//

void cRelocationSetBase::Remove(void **ptr)
{
    // Try to find the pointer in the pointer list.
    int i;

    if ((i = Find(ptr)) != kNotFound)
    {
        // Now that we've found the pointer, let's remove it
        // by replacing the deleted pointer with the last pointer in the list
        m_PtrList[i] = m_PtrList[--m_nPtrs];
    }
    else
        DebugMsg2("Failed to remove pointer from relocation set (0x%x, 0x%x)", ptr, *ptr);
}


///////////////////////////////////////
//
// Reset base and fixup set members
//

void cRelocationSetBase::SetBaseAddress(const void *newBaseAddr)
{
    // Let's calculate the difference between the
    // actual address and the spoofed address. Storing it as a difference
    // simplifies the calculation further down.

    const int ptrDifference = (uint8 *) newBaseAddr - m_pBaseAddr;
    // We are basically subtracting the old base address from each
    // pointer in the selected block and adding the new base address here.

    if (ptrDifference)
    {
        for (int i = m_nPtrs; --i >= 0;)
            *(m_PtrList[i]) += ptrDifference;

        // Finally, lets set the new base address of the block
        m_pBaseAddr = (uint8 *) newBaseAddr;
    }
}
