///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/cpptools/RCS/hashset.cpp $
// $Author: TOML $
// $Date: 1998/02/11 09:30:30 $
// $Revision: 1.15 $
//
// (c) Copyright 1996 Tom Leonard. All Rights Reserved. Unlimited license granted to Looking Glass Technologies Inc.
//

#include <lg.h>
#include <string.h>
#include <limits.h>
#ifdef HASH_STREAMS
//include "storintr.h"
#endif
#include <hashset.h>

#include <pool.h>
#include <hshstimp.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cHashSetBase
//

// Necessary for proper construction order of pool:
#ifdef __WATCOMC__
#pragma initialize library
#else
#pragma init_seg (lib)
#endif

HASHSET_IMPLEMENT_POOL(sHashSetChunk);

#ifndef NO_DB_MEM
// Must be last header, and must be after pool macro!
#include <memall.h>
#include <dbmem.h>
#endif

///////////////////////////////////////

constexpr auto kSlotSize = sizeof(sHashSetChunk *);
// Maximum allowed size if next prime after this value
// @TBD: This value is very conservative (could be slightly less than UINT_MAX)
constexpr auto kMaxSlots = INT_MAX / kSlotSize;


///////////////////////////////////////
//
// Constructor for hash table of specified size:
//
cHashSetBase::cHashSetBase(unsigned n)
{
    m_nItems = 0;
    if (n > kMaxSlots)
        n = kMaxSlots;
    m_nPts = NextPrime(n);                       // Next largest prime number
    m_ResizeThreshold = 2 * m_nPts;                  // Good empirical value.
    m_Table = new sHashSetChunk *[m_nPts];
    memset(m_Table, 0, m_nPts * kSlotSize);
}


///////////////////////////////////////
//
// Copy constructor: makes a shallow copy
//
cHashSetBase::cHashSetBase(const cHashSetBase & s)
{
    m_nPts = 0;
    m_Table = 0;
    CopyOld(s);
}


///////////////////////////////////////
//
// Destructor
//
// WARNING: Destructor only cleans up its memory, not the nodes.
// Derived classes must call DestroyAll in their destructor.
//
cHashSetBase::~cHashSetBase()
{
    if (m_nItems)
    {
        for (register unsigned i = 0; i < m_nPts; i++)
        {
            sHashSetChunk *pNext;
            for (sHashSetChunk * p = m_Table[i]; p; p = pNext)
            {
                pNext = p->pNext;
                delete p;
            }
        }
    }

    delete[] m_Table;
}


///////////////////////////////////////
//
// Clear all entries in the hash table.  The table size remains the same.
//
void cHashSetBase::SetEmpty()
{
    if (!m_nItems)
        return;

    for (register unsigned i = 0; i < m_nPts; i++)
    {
        sHashSetChunk *pNext;
        for (sHashSetChunk * p = m_Table[i]; p; p = pNext)
        {
            pNext = p->pNext;
            delete p;
        }
        m_Table[i] = 0;
    }

    m_nItems = 0;
}



///////////////////////////////////////
//
// ScanNext
// Common implementation for GetFirst & GetNext,
// which just pick the first slot to check for content after validating.
//
tHashSetNode cHashSetBase::ScanNext(tHashSetHandle & h) const
{
    if (h.pChunk)
    {
        h.pPrev = h.pChunk;
        h.pChunk = h.pChunk->pNext;
        if (h.pChunk)
            return h.pChunk->node;
    }

    for (unsigned i = h.Index + 1; i < m_nPts; i++)
    {
        if (m_Table[i])
        {
            h.pChunk = m_Table[i];
            h.Index = i;
            h.pPrev = 0;
            return h.pChunk->node;
        }
    }

    h.pPrev = 0;
    h.pChunk = 0;
    h.Index = m_nPts;
    return 0;
}


#ifdef HASH_STREAMS

///////////////////////////////////////
//
// Stream out
//
BOOL cHashSetBase::ToStream(cOStore & OStore) const
{
    unsigned iElement = 0;
    if (!OStore.WriteHeader("HASHSET"))
        return FALSE;

    for (register unsigned i = 0; i < m_nPts; i++)
    {
        for (sHashSetChunk * p = m_Table[i]; p; p = p->pNext)
        {
            if (!OStore.To(iElement++))
                return FALSE;
            if (!NodeToStream(OStore, p->node))
                return FALSE;
        }
    }

    if (!OStore.To(-1))
        return FALSE;

    return OStore.WriteTrailer();
}


///////////////////////////////////////
//
// Stream in
//
BOOL cHashSetBase::FromStream(cIStore & IStore)
{
    if (!IStore.ReadHeader("HASHSET"))
        return FALSE;

    unsigned i = 0;
    unsigned j;
    for (;;)
    {
        if (!IStore.From(j))
            return FALSE;

        if (j != i++)
            break;

        // Stream in and allocate the node
        tHashSetNode p = NodeFromStream(IStore);
        if (!p)
            return FALSE;

        Insert(p);
    }

    if ((short) j != -1)
        return IStore.Fail();

    return IStore.ReadTrailer();
}

#endif

///////////////////////////////////////
//
// Assignment operator: sets self to a shallow copy of the
// right hand side.
//
cHashSetBase & cHashSetBase::operator=(const cHashSetBase & s)
{
    if (&s != this)
    {
        DestroyAll();
        CopyOld(s);
    }

    return *this;
}


///////////////////////////////////////
//
// Here's where the dirty work is done.
//
void cHashSetBase::CopyOld(const cHashSetBase & s)
{
    m_nItems = s.m_nItems;
    m_nPts = s.m_nPts;
    m_ResizeThreshold = s.m_ResizeThreshold;

// Allocate proper space for new tables
    m_Table = new sHashSetChunk *[m_nPts];

    for (register unsigned i = 0; i < m_nPts; i++)
    {
        m_Table[i] = 0;
        for (sHashSetChunk * p = s.m_Table[i]; p; p = p->pNext)
        {
            sHashSetChunk *p1 = new sHashSetChunk;
            p1->node = CopyNode(p->node);
            p1->pNext = m_Table[i];
            m_Table[i] = p1;
        }
    }
}


///////////////////////////////////////
//
// Generate secondary hash values.  Takes a primary hash value
// to get a secondary value between one and eight.
//
unsigned cHashSetBase::SecondaryHash(unsigned primary)
{
    return 8 - (primary & 0x7);
}


///////////////////////////////////////
//
// Add an item to this table.  If it is already in the
// table, then return the old instance.  Make the table
// larger if necessary.
//
tHashSetNode cHashSetBase::Insert(tHashSetNode a)
{
    BEGIN_DEBUG_MSG1("cHashSetBase::Insert(%p)", a);

// Find where it should go
    tHashSetHandle h = FindIndex(GetKey(a));
// Anything there?
    if (h.pChunk)
    {
        DebugMsg("....already there");
        return h.pChunk->node;
    }

    a = CopyNode(a);
    sHashSetChunk *pChunk = new sHashSetChunk;
    pChunk->node = a;
    pChunk->pNext = m_Table[h.Index];
    m_Table[h.Index] = pChunk;

    m_nItems++;

// Resize if the table is getting full:
    if (m_ResizeThreshold && m_nItems > m_ResizeThreshold)
        Resize();

#ifdef DEBUG_HASHSET
    DumpTable();
#endif

    DebugMsg1("Returning %p", a);

    return a;

    END_DEBUG;
}


///////////////////////////////////////
//
// Remove and return one occurrence of a;
//   if a is not in table return 0.
//
tHashSetNode cHashSetBase::RemoveByHandle(const tHashSetHandle & h)
{
    // If we found it...
    if (h.pChunk)
    {
        // ... then remove it
        m_nItems--;
        if (h.pPrev)
            h.pPrev->pNext = h.pChunk->pNext;
        else
            m_Table[h.Index] = h.pChunk->pNext;

        tHashSetNode node = h.pChunk->node;
        delete h.pChunk;

        return node;
    }

    return 0;
}

///////////////////////////////////////
//
// Rehash this entry
//
void cHashSetBase::Rehash(tHashSetHandle OldHandle, tHashSetNode a)
{
    AssertMsg(OldHandle.pChunk, "Invalid rehash");

// Unlink old chunk
    if (OldHandle.pPrev)
        OldHandle.pPrev->pNext = OldHandle.pChunk->pNext;
    else
        m_Table[OldHandle.Index] = OldHandle.pChunk->pNext;

// Find where it should go
    tHashSetHandle NewHandle = FindIndex(GetKey(a));
// Anything there?
    AssertMsg(NewHandle.pChunk, "Rehash collision");

    OldHandle.pChunk->pNext = m_Table[NewHandle.Index];
    m_Table[NewHandle.Index] = OldHandle.pChunk;
}


///////////////////////////////////////
//
// Return TRUE if this set is a subset of the given set
//
BOOL cHashSetBase::operator <= (const cHashSetBase & h) const
{
    for (register unsigned i = 0; i < m_nPts; i++)
    {
        for (sHashSetChunk * p = m_Table[i]; p; p = p->pNext)
        {
            if (!h.Search(GetKey(p->node)))
                return FALSE;
        }
    }

    return TRUE;
}


///////////////////////////////////////
//
// Return TRUE if this set is the same as the given set
//
BOOL cHashSetBase::operator == (const cHashSetBase & h) const
{
    return m_nItems == h.m_nItems && (*this) <= h;
}


///////////////////////////////////////
//
// Return TRUE if this set is different from the given set
//
BOOL cHashSetBase::operator != (const cHashSetBase & h) const
{
    return !operator == (h);
}


///////////////////////////////////////
//
// Resize the hash table
//
void cHashSetBase::Resize(unsigned NewSize)
{
    BEGIN_DEBUG_MSG1("cHashSetBase::Resize(%d)", NewSize);

    if (m_nPts >= kMaxSlots)
    {
        // Turn-off resizing
        m_ResizeThreshold = 0;
        return;
    }

    if (NewSize > kMaxSlots)
        NewSize = kMaxSlots;

    else if (!NewSize)
        NewSize = m_nItems < 6 ? 17 : m_nItems < kMaxSlots / 3 ? 3 * m_nItems : kMaxSlots;

    AssertMsg(NewSize <= kMaxSlots, "Invalid resize value");

// Having picked a nominal table size, select the next largest
// "relatively prime" number:
    NewSize = NextPrime(NewSize);

// Remember the old size and tables...
    unsigned OldSize = m_nPts;

    sHashSetChunk **OldTable = m_Table;

    // make sure we dont swap during the Malloc/new
    m_ResizeThreshold=0;
    m_Table = new sHashSetChunk *[NewSize];
// Reset the size and threshold
    m_nPts = NewSize;
    m_ResizeThreshold = m_nPts >= kMaxSlots ? 0 : 2 * m_nPts;

    if (!m_Table)
    {
        CriticalMsg("Hash table resize allocation failed");
        return;
    }

    memset(m_Table, 0, m_nPts * kSlotSize);

    for (register unsigned i = 0; i < OldSize; i++)
    {
        sHashSetChunk *pNext;
        for (sHashSetChunk * pChunk = OldTable[i]; pChunk; pChunk = pNext)
        {
            // Save the next pointer
            pNext = pChunk->pNext;

            // Find where it should go
            tHashSetHandle h = FindIndex(GetKey(pChunk->node));
            AssertMsg(!h.pChunk, "Duplicate item found during resize");

            pChunk->pNext = m_Table[h.Index];
            m_Table[h.Index] = pChunk;
        }
    }

// Delete the old data
    delete[] OldTable;

    END_DEBUG;
}


///////////////////////////////////////
//
// Default read node from stream
//
tHashSetNode cHashSetBase::NodeFromStream(cIStore &)
{
    return 0;
}


///////////////////////////////////////
//
// Default write node to stream
//
BOOL cHashSetBase::NodeToStream(cOStore &, tHashSetNode) const
{
    return FALSE;
}


//
// Return the next "relatively prime" number greater than or equal to i.
// "Relatively prime" means that it is not divisible by 2, 3, 5, and 7,
// but may be divisible by larger prime numbers.
//
unsigned __fastcall cHashSetBase::NextPrime(unsigned i)
{
// Make sure its odd
    if ((i & 0x01) == 0)
        i++;

    for (;; i += 2)
    {
        if (i % 3 != 0 && i % 5 != 0 && i % 7 != 0)
            return i;
    }
}
#ifdef DEBUG

///////////////////////////////////////
//
// Print status of hash table
//
void cHashSetBase::DumpTable()
{
    DebugStr(cFmtStr("Hash table %p of size %d:", this, m_nPts));
    for (unsigned i = 0; i < m_nPts; i++)
    {
        int c = 0;
        for (sHashSetChunk * p = m_Table[i]; p; p = p->pNext)
            c++;
        if (c)
            DebugMsg2("%d has %d entries", i, c);
    }
}
#endif

///////////////////////////////////////////////////////////////////////////////
