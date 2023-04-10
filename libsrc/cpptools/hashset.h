///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/cpptools/RCS/hashset.h $
// $Author: TOML $
// $Date: 1998/08/13 09:37:05 $
// $Revision: 1.18 $
//
// (c) Copyright 1993-1996 Tom Leonard. All Rights Reserved. Unlimited license granted to Looking Glass Technologies Inc.
//
// @TBD (toml 05-01-96): Some day, should be able to do default params in templ-spec using Watcom
//

#if defined(__SC__) || defined(__RCC__)
#pragma once
#endif

#ifndef __HASHSET_H
#define __HASHSET_H

#define __CPPTOOLSAPI

#include <lg.h>
#include <hashfunc.h>

///////////////////////////////////////////////////////////////////////////////
//
// ====================
// Container Hash Table
// ====================
//
// This header defines a hash table designed for quick storage and retrieval
// of complex types. It is useful for sparse data sets as well.
//
// To use the table, one should derive from one of the template classes,
// defining at least GetKey() or GetKey()/IsEqual() depending on the base
// class chosen.
//
// If the contained object is greater than 4 bytes, you must store pointers
// to the data.
//
// In general, these tables are extremely easy to use if the key is one of the
// values supported by the class cHashFunctions
//
// For example, in the case of strings:
//
//  struct sMyStruct
//  {
//      int i;
//      float f;
//      char name[20];
//  }
//
//  class cMyTable : public cStrHashSet<sMyStruct *>
//  {
//      virtual tHashSetKey GetKey(tHashSetNode p) const
//      {
//          return ((sMyStruct *)p)->name;
//      }
//  };
//
//  void bar()
//  {
//      cMyTable myTable;
//      sMyStruct * pMyStruct = new sMyStruct;
//
//      strcpy(pMyStruct->name, "blah");
//      myTable.Insert(pMyStruct);
//      assert(myTable.Search("blah"));
//
//  }
//
// Expansion of Non-inline template functions
// ------------------------------------------
// Clients must create a single module that includes both a declaration
// of their own template type, the header "hshsttem.h", and in the
// case of MSVC, an explicit template instantiation statement. E.g.:
//
//  #include <mytable.h>
//  #include <hshsttem.h>
//
//  #ifdef _MSC_VER
//  template cMyTable;
//  #endif
//
// Iteration protocol for cHashSet
// -------------------------------
//
// The hash set supports the ability to interate over the set of elements.
// These rules apply:
//
// 1) The order is random.  Due to the dynamics of hashing, order can
//    change unpredicatably.
//
// 2) The caller begins iteration by calling GetFirst with a handle.
//    This gives the caller the first element.  To get to the next element
//    call GetNext with the handle.  The handle is a magic cookie.
//
// 3) The returned pointer will be NULL if there are no more elements.
//    Calling GetNext after NULL has been returned has unpredictable
//    results.  Callers should NOT check the handle value to detect
//    termination.
//

class cOStore;
class cIStore;

///////////////////////////////////////////////////////////////////////////////

#include <hshstimp.h> // sad but needed include (toml 12-21-97)

///////////////////////////////////////////////////////////////////////////////

// A hash set can be any size, but here's a selection of
// standard ones

enum eHashSetSizes
{
   kHS_Empty   = 1,
   kHS_Tiny    = 17,
   kHS_Small   = 53,
   kHS_Medium  = 101,
   kHS_Large   = 251,
   kHS_Huge    = 997
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cHashSetBase
//
// This base class implements most of the capability of the hash set.  Use the templates
// for convenience.
//

class __CPPTOOLSAPI cHashSetBase
{
public:
    enum { kDefaultSize = kHS_Medium };

    ///////////////////////////////////
    //
    // Constructors and destructor
    //
    cHashSetBase(unsigned n = kDefaultSize);
    cHashSetBase(const cHashSetBase &);
    virtual ~cHashSetBase();

    cHashSetBase & operator = (const cHashSetBase &);

    ///////////////////////////////////
    //
    // Streaming protocol
    //
    BOOL FromStream(cIStore &);
    BOOL ToStream(cOStore &) const;

    ///////////////////////////////////
    //
    // Comparisons
    //
    BOOL operator <= (const cHashSetBase &) const;
    BOOL operator == (const cHashSetBase &) const;
    BOOL operator != (const cHashSetBase &) const;

    ///////////////////////////////////
    //
    // Destroy all of the items in the table
    //
    virtual void DestroyAll() = 0;

    ///////////////////////////////////
    //
    // Query the size of the table
    //
    unsigned GetCount() const;

    ///////////////////////////////////
    //
    // Query if the table is empty
    //
    BOOL IsEmpty() const;

    ///////////////////////////////////
    //
    // Empty the table (does not affect things in table, other than leaving
    // them potentially unreferenced)
    //
    void SetEmpty();

    ///////////////////////////////////
    //
    // Set the number of buckets
    //
    void Resize(unsigned n = 0);

#ifdef DEBUG
    ///////////////////////////////////
    //
    // Print table statistics/contents
    //
    void DumpTable();
#endif

protected:
    ///////////////////////////////////
    //
    // Hashing/Insertion/Removal/Lookup
    //
    tHashSetNode   Search(tHashSetKey) const;
    tHashSetNode   Insert(tHashSetNode);
    tHashSetNode   Remove(tHashSetNode);
    tHashSetNode   RemoveByKey(tHashSetKey);
    tHashSetNode   RemoveByHandle(const tHashSetHandle & h);
    tHashSetHandle GetHandle(tHashSetNode);
    void           Rehash(tHashSetHandle, tHashSetNode);

    ///////////////////////////////////
    //
    // Iteration
    //
    tHashSetNode GetFirst(tHashSetHandle &) const;
    tHashSetNode GetNext(tHashSetHandle &) const;

    ///////////////////////////////////
    //
    // Derived class customizor functions
    //
    virtual BOOL        IsEqual(tHashSetKey, tHashSetKey) const = 0;
    virtual tHashSetKey GetKey(tHashSetNode) const = 0;
    virtual unsigned    Hash(tHashSetKey) const = 0;


    ///////////////////////////////////
    //
    // Copy a node. Default does nothing. Use if the table should do it's
    // own storage (i.e., wants to hold copies of the data, not the original)
    //
    virtual tHashSetNode CopyNode(tHashSetNode) const;

    ///////////////////////////////////
    //
    // Node stream member functions
    //
    virtual tHashSetNode NodeFromStream(cIStore &);
    virtual BOOL         NodeToStream(cOStore &, tHashSetNode) const;

    ///////////////////////////////////

    void            CopyOld(const cHashSetBase &);
    tHashSetHandle  FindIndex(tHashSetKey) const;
    static unsigned SecondaryHash(unsigned);

    static unsigned __fastcall NextPrime(unsigned);             // Used to find the next largest prime

protected:
    // Pointer to an array of chunks
    sHashSetChunk ** m_Table;

    // Size of above array
    unsigned m_nPts;

    // Total number of stored objects
    unsigned m_nItems;

    // Resize if items > resizeThreshold.
    unsigned m_ResizeThreshold;

private:
    tHashSetNode ScanNext(tHashSetHandle &) const;
};



///////////////////////////////////////////////////////////////////////////////
//
// TEMPLATE: cHashSet
//
// Base template for the hash table.  See derived versions
// for tables that handle common keys
//

template <class NODE, class KEY, class FUNCLASS /* = cHashFunctions*/>
class cHashSet : public cHashSetBase
{
public:
    ///////////////////////////////////
    //
    // Constructors and destructor
    //
    cHashSet(unsigned n = cHashSetBase::kDefaultSize)   : cHashSetBase(n) { AssertMsg1(sizeof(NODE) == 4, "cHashSet node must be 32-bits. %s", (sizeof(NODE) < 4) ? "Use int." : "Use pointers"); }
    cHashSet(const cHashSet &)                          : cHashSetBase(*this) {}

    ///////////////////////////////////
    //
    // Insert a node
    //
    // Items must be unique. An item with the same key is already in the
    // table, the old instance is returned and the new item is not added.
    //
    NODE Insert(NODE);

    ///////////////////////////////////
    //
    // Look for a node, given the key
    //
    NODE Search(KEY) const;

    ///////////////////////////////////
    //
    // Remove a node previously found by search or iteration
    //
    NODE Remove(NODE);

    ///////////////////////////////////
    //
    // Remove a node by the key
    //
    NODE RemoveByKey(KEY);

    ///////////////////////////////////
    //
    // Remove a node using a previously acquired handle
    //
    NODE RemoveByHandle(const tHashSetHandle & h);

    ///////////////////////////////////
    //
    // Access a node's representation
    //
    tHashSetHandle GetHandle(NODE);

    ///////////////////////////////////
    //
    // Rehash a previously hashed node
    //
    void Rehash(tHashSetHandle, NODE);

    ///////////////////////////////////
    //
    // Destroy all nodes.  Default version iterates and deletes each node.
    // See note above regarding template instantiation.
    //
    virtual void DestroyAll();

    ///////////////////////////////////
    //
    // Iteration
    //
    NODE GetFirst(tHashSetHandle &) const;
    NODE GetNext(tHashSetHandle &) const;

protected:
    ///////////////////////////////////
    //
    // Default hash/equivalence functions.  These use class specified in by
    // template argument FUNCLASS.
    // See note above regarding template instantiation.
    //
    virtual BOOL IsEqual(tHashSetKey, tHashSetKey) const;
    virtual unsigned Hash(tHashSetKey) const;
};

////////////////////////////////////////////////////////////
//
// TEMPLATE: cStrHashSet, cStrIHashSet
//
// This implements a hash set based on a string key.  Just override GetKey()
// to return a pointer to the string and your all set.
//

//
// Case-sensitive version
//
template <class NODE>
class cStrHashSet : public cHashSet <NODE, const char *, cHashFunctions>
{
public:
    cStrHashSet(unsigned n = cHashSetBase::kDefaultSize)              : cHashSet<NODE, const char *, cHashFunctions>(n) { }
    cStrHashSet(const cHashSet<NODE, const char *, cHashFunctions> &) : cHashSet<NODE, const char *, cHashFunctions>(*this) {}
};

//
// Case-insensitive version
//
template <class NODE>
class cStrIHashSet : public cHashSet <NODE, const char *, cCaselessStringHashFuncs>
{
public:
    cStrIHashSet(unsigned n = cHashSetBase::kDefaultSize)              : cHashSet<NODE, const char *, cCaselessStringHashFuncs>(n) { }
    cStrIHashSet(const cHashSet<NODE, const char *, cCaselessStringHashFuncs> &) : cHashSet<NODE, const char *, cCaselessStringHashFuncs>(*this) {}
};

////////////////////////////////////////////////////////////
//
// TEMPLATE: cGuidHashSet
//
// This implements a hash set based on a GUID key.  Just override GetKey()
// to return a pointer to the GUID and your all set.
//
template <class NODE>
class cGuidHashSet : public cHashSet <NODE, const GUID *, cHashFunctions>
{
};

////////////////////////////////////////////////////////////
//
// TEMPLATE: cPtrHashSet
//
// This implements a hash set based on a pointer key.  Just override GetKey()
// to return the pointer and your all set.
//
template <class NODE>
class cPtrHashSet : public cHashSet <NODE, const void *, cHashFunctions>
{
};

/////////////////////////////////////////////////////////////////////////
//
// TEMPLATE: cSerialHashSet
//
template <class NODE, class KEY, class FUNCLASS /* = cHashFunctions*/>
class cSerialHashSet : public cHashSet<NODE, KEY, FUNCLASS >
{
 protected:
    // Node stream member functions
    virtual tHashSetNode * NodeFromStream(cIStore &);
    virtual BOOL NodeToStream(cOStore &, tHashSetNode *) const;
};

/////////////////////////////////////////////////////////////////////////
//
// TEMPLATE: cSerialStrHashSet
//
template <class NODE, class FUNCLASS /* = cHashFunctions*/>
class cSerialStrHashSet : public cSerialHashSet <NODE, char *, FUNCLASS>
{
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cHashSetBase, inline functions
//

///////////////////////////////////////
//
// Copy a node (default does nothing)
//
inline tHashSetNode cHashSetBase::CopyNode(tHashSetNode a) const
{
    return a;
}

///////////////////////////////////////

inline unsigned cHashSetBase::GetCount() const
{
    return m_nItems;
}

///////////////////////////////////////

inline BOOL cHashSetBase::IsEmpty() const
{
    return m_nItems == 0;
}

///////////////////////////////////////
//
// Find the index of an object given a key.  If the object is not
// found, return the index of the appropriate empty slot
//

inline tHashSetHandle cHashSetBase::FindIndex(tHashSetKey a) const
{
    tHashSetHandle h;
    h.Index = Hash(a) % m_nPts;

    h.pPrev = 0;
    h.pChunk = m_Table[h.Index];
    
    while (h.pChunk)
    {
       if (IsEqual(GetKey(h.pChunk->node), a))
          return h;
        
       h.pPrev = h.pChunk;
       h.pChunk = h.pChunk->pNext;
    }
    
    // Not Found: return index of the empty slot
    h.pChunk = 0;
    h.pPrev = 0;
    return h;
}

///////////////////////////////////////
//
// Find the first occurrence of the hash key
//
inline tHashSetNode cHashSetBase::Search(tHashSetKey a) const
{
    tHashSetHandle h = FindIndex(a);
    return h.pChunk ? h.pChunk->node : 0;
}


///////////////////////////////////////
//
// Get the handle for the given entry
//
inline tHashSetHandle cHashSetBase::GetHandle(tHashSetNode a)
{
    return FindIndex(GetKey(a));
}

///////////////////////////////////////
//
// Remove and return one occurrence of a;
//   if a is not in table return 0.
//
inline tHashSetNode cHashSetBase::RemoveByKey(tHashSetKey k)
{
    // Find where it should be...
    return RemoveByHandle(FindIndex(k));
}

///////////////////////////////////////
//
// Remove and return one occurrence of a;
//   if a is not in table return 0.
//
inline tHashSetNode cHashSetBase::Remove(tHashSetNode a)
{
   return RemoveByKey(GetKey(a));
}

///////////////////////////////////////
//
// Get the first element in the hash set
//
inline tHashSetNode cHashSetBase::GetFirst(tHashSetHandle & h) const
{
    h.Index = (unsigned)-1;
    h.pChunk = 0;
    return ScanNext(h);
}


///////////////////////////////////////
//
// Get the next element in the hash set
//
inline tHashSetNode cHashSetBase::GetNext(tHashSetHandle & h) const
{
    return ScanNext(h);
}

///////////////////////////////////////////////////////////////////////////////
//
// TEMPLATE: cHashSet, inline functions
//

///////////////////////////////////////
//
// Call the hash function
//

template <class NODE, class KEY, class FUNCLASS>
inline unsigned cHashSet<NODE, KEY, FUNCLASS>::Hash(tHashSetKey k) const
{
    return FUNCLASS::Hash((const KEY)(k));
}

///////////////////////////////////////
//
// Call the compare function
//

template <class NODE, class KEY, class FUNCLASS>
inline BOOL cHashSet<NODE, KEY, FUNCLASS>::IsEqual(tHashSetKey k1, tHashSetKey k2) const
{
    return FUNCLASS::IsEqual((const KEY)(k1), (const KEY)(k2));
}

///////////////////////////////////////

template <class NODE, class KEY, class FUNCLASS /* = cHashFunctions*/>
inline NODE cHashSet<NODE, KEY, FUNCLASS>::Search(KEY key) const
{
    return (NODE) cHashSetBase::Search((tHashSetKey) key);
}

///////////////////////////////////////

template <class NODE, class KEY, class FUNCLASS /* = cHashFunctions*/>
inline NODE cHashSet<NODE, KEY, FUNCLASS>::Insert(NODE node)
{
    return (NODE) cHashSetBase::Insert((tHashSetNode) node);
}

///////////////////////////////////////

template <class NODE, class KEY, class FUNCLASS /* = cHashFunctions*/>
inline NODE cHashSet<NODE, KEY, FUNCLASS>::Remove(NODE node)
{
    return (NODE) cHashSetBase::Remove((tHashSetNode) node);
}

///////////////////////////////////////

template <class NODE, class KEY, class FUNCLASS /* = cHashFunctions*/>
inline NODE cHashSet<NODE, KEY, FUNCLASS>::RemoveByKey(KEY k)
{
    return (NODE) cHashSetBase::RemoveByKey((tHashSetKey) k);
}

///////////////////////////////////////

template <class NODE, class KEY, class FUNCLASS /* = cHashFunctions*/>
inline NODE cHashSet<NODE, KEY, FUNCLASS>::RemoveByHandle(const tHashSetHandle & h)
{
    return (NODE) cHashSetBase::RemoveByHandle(h);
}

///////////////////////////////////////

template <class NODE, class KEY, class FUNCLASS /* = cHashFunctions*/>
void cHashSet<NODE, KEY, FUNCLASS>::Rehash(tHashSetHandle Handle, NODE node)
{
    cHashSetBase::Rehash(Handle, (tHashSetNode) node);
}

///////////////////////////////////////

template <class NODE, class KEY, class FUNCLASS /* = cHashFunctions*/>
tHashSetHandle cHashSet<NODE, KEY, FUNCLASS>::GetHandle(NODE node)
{
    return cHashSetBase::GetHandle((tHashSetNode) node);
}

///////////////////////////////////////

template <class NODE, class KEY, class FUNCLASS /* = cHashFunctions*/>
inline NODE cHashSet<NODE, KEY, FUNCLASS>::GetFirst(tHashSetHandle & Handle) const
{
    return (NODE) cHashSetBase::GetFirst(Handle);
}

///////////////////////////////////////

template <class NODE, class KEY, class FUNCLASS /* = cHashFunctions*/>
inline NODE cHashSet<NODE, KEY, FUNCLASS>::GetNext(tHashSetHandle & Handle) const
{
    return (NODE) cHashSetBase::GetNext(Handle);
}

///////////////////////////////////////////////////////////////////////////////

#endif /* !__HASHSET_H */
