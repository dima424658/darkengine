///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/cpptools/RCS/dynarray.cpp $
// $Author: TOML $
// $Date: 1998/05/23 07:08:04 $
// $Revision: 1.11 $
//

#include <lg.h>
#include <dynarray.h>

#include <stdlib.h>

#ifndef NO_DB_MEM
// Must be last header
#include <memall.h>
#include <dbmem.h>
#endif

//include "storintr.h"

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDynArrayBase
//

#ifndef SHIP
char BASED_CODE cDynArrayBase::gm_pszOutOfRange[] = "Index out of range";
#endif

///////////////////////////////////////
//
// Assignment operator
//

cDynArrayBase& cDynArrayBase::operator=(const cDynArrayBase &Array)
{
    // Blow the current array away
    SetSize(0);

    // Copy the values
    m_nBlockSizeLessOne = Array.m_nBlockSizeLessOne;
    m_nItemSize = Array.m_nItemSize;
    m_nItems = Array.m_nItems;
    VerifyMsg(Resize(m_nItems), "Array allocation failed");

    AssertMsg(!m_nItems || m_pItems, "Expected items in dynarray");

    // Call set item for all the items
    for (index_t i = 0; i<m_nItems; i++)
        SetItem(Array.ItemPtr(i), i);

    return *this;
}

///////////////////////////////////////
//
// Resize the array buffer to the desired number of slots.
//
// NOTE: The logical size does not change.
//

BOOL cDynArrayBase::DoResize(index_t evenSlots)
{
    if (evenSlots)
    {
        tDynArrayItem *newP;

        // Note: Allow for scratch block at the end
        if (m_pItems)
            newP = realloc(m_pItems, (evenSlots+1) * m_nItemSize);
        else
            newP = malloc((evenSlots+1) * m_nItemSize);

        AssertMsg(newP, "Dynamic array resize failed");

        if (!newP)
            return FALSE;

        m_pItems = (BYTE *) newP;
    }

    else if (m_pItems)
    {
        free(m_pItems);
        m_pItems = 0;
    }

    m_nSlots = evenSlots;

    return TRUE;
}


///////////////////////////////////////
//
// MoveItemToIndex
//
// Move an element to a specified position in the array, shifting
// other element in the list appropriately.
//

void cDynArrayBase::MoveItemToIndex(index_t currentIndex, index_t newIndex)
{
    AssertMsg(currentIndex < m_nItems && newIndex < m_nItems, gm_pszOutOfRange);

    if (currentIndex == newIndex)
        return;

    // In order to move the item, we must first make a copy of it.
    // We always keep extra space at the end of the item data block
    // for this purpose.
    CopyToTemporary(currentIndex);

    // If the element is before target location...
    if (currentIndex < newIndex)
    {
        // Shift items between current and target locations down one
        memmove(UnsafeItemPtr( currentIndex), UnsafeItemPtr( currentIndex+1),
                        (newIndex - currentIndex) * m_nItemSize);

    }

    // ...else the element is after target location...
    {
        // Shift items between target and current locations up one.
        memmove(UnsafeItemPtr( newIndex+1), UnsafeItemPtr( newIndex),
                        (currentIndex - newIndex) * m_nItemSize);

    }

    // Copy element into new position
    CopyFromTemporary(newIndex);
}



///////////////////////////////////////
//
// Write the array to the stream
//

BOOL cDynArrayBase::ToStream(cOStore &) const
{
#if 0
    int iElement = 0;

    index_t numElems = Size();

    if (!OStore.WriteHeader("DYNARRAY"))
        return FALSE;

    OStore.To(numElems);

    for (index_t i = 0; i < numElems; i++)
    {
        if (!OStore.To(i))
            return FALSE;

        if (!ItemToStream(OStore, UnsafeItemPtr(i)))
            return FALSE;
    }

    return OStore.WriteTrailer();
#else
    return FALSE;
#endif
}

///////////////////////////////////////
//
// Read the array from a stream
//

BOOL cDynArrayBase::FromStream(cIStore &)
{
#if 0
    if (!IStore.ReadHeader("DYNARRAY"))
        return FALSE;

    // Get the number of elements
    index_t numElems;
    if (!IStore.From(numElems))
        return FALSE;

    SetSize(numElems);

    for (index_t i = 0; i<numElems; i++)
    {
        index_t j;
        if (!IStore.From(j))
            return FALSE;

        if (j != i)
            return IStore.Fail();

        // Stream in the node over the item
        if (!ItemFromStream(IStore, UnsafeItemPtr(i)))
            return FALSE;
    }

    return IStore.ReadTrailer();
#else
    return FALSE;
#endif
}


///////////////////////////////////////
//
// Read an item from the stream (default binary mode)
//

BOOL cDynArrayBase::ItemFromStream(cIStore &, tDynArrayItem *)
{
#if 0
    // Copy item from stream, overwriting whatever was in the item before
    return IStore.From((unsigned char *)pItem, GetElementSize());
#else
    return FALSE;
#endif
}


///////////////////////////////////////
//
// Write an item to the stream (default binary mode)
//

BOOL cDynArrayBase::ItemToStream(cOStore &, const tDynArrayItem *) const
{
#if 0
    return OStore.To((const unsigned char *)pItem, GetElementSize());
#else
    return FALSE;
#endif
}


///////////////////////////////////////
//
// Swap the content of two arrays (all state and ownership
// of allocated memory). If information should only go one-way,
// empty one first.
//

void cDynArrayBase::SwapContents(cDynArrayBase &x)
{

    BYTE * p;
    p = x.m_pItems;
    x.m_pItems = m_pItems;
    m_pItems = p;

    size_t sz;
    sz = x.m_nItemSize;
    x.m_nItemSize = m_nItemSize;
    m_nItemSize = sz;

    index_t  ix;
    ix = x.m_nItems;
    x.m_nItems = m_nItems;
    m_nItems = ix;

    ix = x.m_nSlots;
    x.m_nSlots = m_nSlots;
    m_nSlots = ix;
}


///////////////////////////////////////
//
// Find an element using a supplied comparison function.  This routine uses
// a binary search algorithm so the array must be sorted.
//

index_t cDynArrayBase::BSearch(tDynArrayKey *pKey, tSearchFunc SearchFunc) const
{
    if (!m_pItems)
        return BAD_INDEX;

    tDynArrayItem *pNewItem = bsearch(pKey, m_pItems, m_nItems, m_nItemSize, SearchFunc);

    if (!pNewItem)
        return BAD_INDEX;

    return ((BYTE *)pNewItem - m_pItems) / m_nItemSize;
}


///////////////////////////////////////
//
// Find an element using a supplied comparison function.  This routine uses
// a straight linear search algorithm.
//

index_t cDynArrayBase::LSearch(tDynArrayKey *pKey, tSearchFunc SearchFunc) const
{
    if (!m_pItems)
        return BAD_INDEX;

    BYTE * pNewItem;
    #ifdef LSEARCH_IN_RTL
    // @TBD: Is this part of our RTL yet?
    pNewItem = (BYTE *) lsearch(pItem, m_pItems, m_nItems, m_nItemSize, SearchFunc);
    #else
    pNewItem = m_pItems;

    for (index_t Remaining = m_nItems; Remaining; Remaining--)
    {
        if (SearchFunc(pKey, (tDynArrayItem *)pNewItem)==0)
            break;
        pNewItem += m_nItemSize;
    }

    if (!Remaining)
        pNewItem = 0;
    #endif

    if (!pNewItem)
        return BAD_INDEX;

    return (pNewItem - m_pItems) / m_nItemSize;
}


///////////////////////////////////////
//
// Sort the array of elements based on the supplied comparison function.
//

void cDynArrayBase::Sort(tSortFunc SortFunc)
{
    if (!m_pItems)
        return;

    qsort(m_pItems, m_nItems, m_nItemSize, SortFunc);
}

///////////////////////////////////////////////////////////////////////////////

