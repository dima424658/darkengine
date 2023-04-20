///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/cpptools/RCS/dynarray.h $
// $Author: TOML $
// $Date: 1998/10/05 22:10:53 $
// $Revision: 1.20 $
//
// (c) Copyright 1993-1996 Tom Leonard. All Rights Reserved. Unlimited license granted to Looking Glass Technologies Inc.
//

#ifndef __DYNARRAY_H
#define __DYNARRAY_H

#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <malloc.h>

#include <dynarsrv.h>

#ifdef  _MSC_VER
#include <new>
#endif

#include <lg.h>

#if defined(__WATCOMC__)
#pragma off (unreferenced)
#pragma warning 549 9
#endif

#pragma pack(4)
#pragma once

typedef int (*tVoidCompareFunc)(const void * pLeft, const void * pRight);
// for back compatability with a prior implementation:
typedef unsigned index_t;
typedef int (*tSearchFunc)(const void *pKey, const void *); // Comparison functions for search and sort
typedef int (*tSortFunc)(const void *, const void *);

///////////////////////////////////////////////////////////////////////////////
//
// Dynamic Bounds-checked Arrays
// -----------------------------
//
// This header declares a simple class to encapsulate dynamically allocated
// arrays. Data items must either be simple types, complex types without
// constructors, or complex types using the "class" version.
//
// Items must be safely relocatable, as reallocs may implicitly occur.
// For example, to declare a dynamic array of integers:
// { cDynArray<int> myArray(1); myArray[1] = 1; }
//
// Notes: block size must be a power of 2, destructors not virtual. WARNING:
// MSVC allows its optimizer compiler to evaluate both the left and right 
// sides of an assignment prior to the assignment. So, given myArray[1] = foo()
// where foo changes myArray and causes a realloc will generate probably unexpected 
// results, because the compiler might store (myArray.m_pItems + 1) in a register
// prior to calling foo.

const unsigned BAD_INDEX           = 0xFFFFFFFFL;
#define kDynArrayDefBlockSize 4

#ifdef DEBUG
EXTERN void DumpDynarraySummary();
#endif

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDARaw
//
// Base class of dynamic array
//

template <class T, unsigned BlockSize, class ServiceFuncs>
class cDABase
{
public:

   ///////////////////////////////////
   //
   // Constructors, assignment operator, and destructor
   //
   cDABase();
   cDABase(int size);
   cDABase(const cDABase<T, BlockSize, ServiceFuncs> &);
   cDABase<T, BlockSize, ServiceFuncs> & operator=(const cDABase<T, BlockSize, ServiceFuncs> &);
   ~cDABase(); // note destructor is *not* virtual
   
   ///////////////////////////////////
   //
   // Detach memory from the dynarray
   //

   T * Detach();

   ///////////////////////////////////
   //
   // Access the array using array notation.
   //

   T & operator[] (short);
   T & operator[] (int);
   T & operator[] (long);
   T & operator[] (ushort);
   T & operator[] (uint);
   T & operator[] (ulong);

   const T & operator[] (short) const;
   const T & operator[] (int) const;
   const T & operator[] (long) const;
   const T & operator[] (ushort) const;
   const T & operator[] (uint) const;
   const T & operator[] (ulong) const;

   ///////////////////////////////////
   //
   // Access the array as a pointer (i.e., a C-array)
   //
   // (inline in class because watcom parser has trouble with non-in-class version
   
   operator T *()             { return m_pItems; }
   operator const T *() const { return (const T *)m_pItems; }

   T * AsPointer();
   const T * AsPointer() const;

   ///////////////////////////////////
   //
   // Grow the array, insert an item, shifting items up
   //
   void InsertAtIndex(const T &, unsigned);

   ///////////////////////////////////
   //
   // Grow the array, appending an item
   //
   unsigned Append(const T &);

   ///////////////////////////////////
   //
   // Grow/shrink the array, filling new slots with blanks
   //
   unsigned Grow(unsigned num = 1);
   void Shrink(unsigned num = 1);

   ///////////////////////////////////
   //
   // Do a verified memcpy into the array
   //
   void MemCpy(const T *, unsigned nItems);
   void AppendMemCpy(const T *, unsigned nItems);

   ///////////////////////////////////
   //
   // Get the block size (the minimum increment by which the array storage
   // grows and shrinks)
   //
   unsigned GetBlockSize() const;

   ///////////////////////////////////
   //
   // Get and set array size (the number of active elements)
   //
   unsigned Size() const;
   BOOL     SetSize(unsigned);

   ///////////////////////////////////
   //
   // Query the size of array elements
   //
   size_t GetElementSize() const;

   ///////////////////////////////////
   //
   // Test if a given index is valid
   //
   BOOL IsValidIndex(unsigned index) const;

   ///////////////////////////////////
   //
   // Assign to a slot
   //
   void SetItem(const T * pItem, unsigned index);

   ///////////////////////////////////
   //
   // Remove an item, shifting items down
   //
   void DeleteItem(unsigned index);

   ///////////////////////////////////
   //
   // Remove an item, swapping last into deleted slot
   //
   void FastDeleteItem(unsigned index);
   
   ///////////////////////////////////
   //
   // Swap two items
   //
   void Swap(unsigned index1, unsigned index2);

   ///////////////////////////////////
   //
   // Move an item, shifting items
   //
   void MoveItemToIndex(unsigned currentIndex, unsigned newIndex);

   ///////////////////////////////////
   //
   // Swap the contents of two arrays
   //
   void SwapContents(cDABase<T, BlockSize, ServiceFuncs> &);

   ///////////////////////////////////
   //
   // Array sorting and searching (qsort, linear search, bninary search)
   //
   using tCompareFunc = int (*)(const T * pLeft, const T * pRight); // Comparison functions for sort
   using tSearchFunc = int (*)(const void * pKey, const T * pRight); // Comparison functions for search

   void     Sort(tCompareFunc);
   unsigned LSearch(const void *, tSearchFunc) const;
   unsigned BSearch(const void *, tSearchFunc) const;

protected:
   ///////////////////////////////////
   //
   // Raw resize
   //
   
   BOOL Resize(unsigned numSlots);

private:
   T *      m_pItems;
   unsigned m_nItems;
};


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDynArray_
//
// Non-class type dynarray with settable block size
//

template <class T, unsigned BlockSize>
class cDynArray_ : public cDABase<T, BlockSize, cDARawSrvFns<T> >
{
   typedef cDABase<T, BlockSize, cDARawSrvFns<T> > Base;
public:
   cDynArray_() {}
   cDynArray_(int size) : Base(size) {}
   cDynArray_(const cDynArray_<T, BlockSize> & from)  : Base(from) {}
   cDynArray_<T, BlockSize> & operator=(const cDynArray_<T, BlockSize> & from) { Base::operator=(from); return *this; }
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDynArray
//
// The general purpose non-class type dynarray
//

template <class T>
class cDynArray : public cDynArray_<T, kDynArrayDefBlockSize>
{
   typedef cDynArray_<T, kDynArrayDefBlockSize> Base;
public:
   cDynArray() {}
   cDynArray(int size)  : Base(size) {}
   cDynArray(const cDynArray<T> & from)  : Base(from) {}
   cDynArray<T> & operator=(const cDynArray<T> & from)  { Base::operator=(from); return *this; }
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDynZeroArray_
//
// Non-class type dynarray with settable block size that zeros new slots
//

template <class T, unsigned BlockSize>
class cDynZeroArray_ : public cDABase<T, BlockSize, cDAZeroSrvFns<T> >
{
   typedef cDABase<T, BlockSize, cDAZeroSrvFns<T> > Base;
public:
   cDynZeroArray_() {}
   cDynZeroArray_(int size) : Base(size) {}
   cDynZeroArray_(const cDynZeroArray_<T, BlockSize> & from)  : Base(from) {}
   cDynZeroArray_<T, BlockSize> & operator=(const cDynZeroArray_<T, BlockSize> & from) { Base::operator=(from); return *this; }
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDynZeroArray
//
// The general purpose non-class type dynarray that zeros new slots
//

template <class T>
class cDynZeroArray : public cDynZeroArray_<T, kDynArrayDefBlockSize>
{
   typedef cDynZeroArray_<T, kDynArrayDefBlockSize> Base;
public:
   cDynZeroArray() {}
   cDynZeroArray(int size)  : Base(size) {}
   cDynZeroArray(const cDynZeroArray<T> & from)  : Base(from) {}
   cDynZeroArray<T> & operator=(const cDynZeroArray<T> & from)  { Base::operator=(from); return *this; }
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDynClassArray_
//
// Class type dynarray with settable block size
//

template <class T, unsigned BlockSize>
class cDynClassArray_ : public cDABase<T, BlockSize, cDAClsSrvFns<T> >
{
   typedef cDABase<T, BlockSize, cDAClsSrvFns<T> > Base;
public:
   cDynClassArray_() {}
   cDynClassArray_(int size) :  Base(size) {}
   cDynClassArray_(const cDynClassArray_<T, BlockSize> & from) : Base(from) {}
   cDynClassArray_<T, BlockSize> & operator=(const cDynClassArray_<T, BlockSize> & from)  { Base::operator=(from); return *this; }
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDynClassArray
//
// The general purpose class type dynarray
//

template <class T>
class cDynClassArray : public cDynClassArray_<T, kDynArrayDefBlockSize>
{
   typedef cDynClassArray_<T, kDynArrayDefBlockSize> Base;
public:
   cDynClassArray() {}
   cDynClassArray(int size) : Base(size) {}
   cDynClassArray(const cDynClassArray<T> & from)  : Base(from) {}
   cDynClassArray<T> & operator=(const cDynClassArray<T> & from) { Base::operator=(from); return *this; }
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDABase, inline methods
//

#define DYNARRAY_BASE_TEMPLATE  template <class T, unsigned BlockSize, class ServiceFuncs>
#define DYNARRAY_BASE           cDABase<T, BlockSize, ServiceFuncs>

DYNARRAY_BASE_TEMPLATE
inline DYNARRAY_BASE::cDABase()
 : m_pItems(NULL),
   m_nItems(0)
{
   cDynArray_TrackCreate();
}

///////////////////////////////////////
//
// Get a pointer to the element at the specified index
//

DYNARRAY_BASE_TEMPLATE
inline T & DYNARRAY_BASE::operator[](short index)
{
    DynArrayAssertIndex(index);
    return m_pItems[index];
}

DYNARRAY_BASE_TEMPLATE
inline T & DYNARRAY_BASE::operator[](int index)
{
    DynArrayAssertIndex(index);
    return m_pItems[index];
}

DYNARRAY_BASE_TEMPLATE
inline T & DYNARRAY_BASE::operator[](long index)
{
    DynArrayAssertIndex(index);
    return m_pItems[index];
}

DYNARRAY_BASE_TEMPLATE
inline T & DYNARRAY_BASE::operator[](ushort index)
{
    DynArrayAssertIndex(index);
    return m_pItems[index];
}

DYNARRAY_BASE_TEMPLATE
inline T & DYNARRAY_BASE::operator[](uint index)
{
    DynArrayAssertIndex(index);
    return m_pItems[index];
}

DYNARRAY_BASE_TEMPLATE
inline T & DYNARRAY_BASE::operator[](ulong index)
{
    DynArrayAssertIndex(index);
    return m_pItems[index];
}

///////////////////////////////////////
//
// Get a const pointer to the element at the specified index
//
DYNARRAY_BASE_TEMPLATE
inline const T & DYNARRAY_BASE::operator[](short index) const
{
    DynArrayAssertIndex(index);
    return m_pItems[index];
}

DYNARRAY_BASE_TEMPLATE
inline const T & DYNARRAY_BASE::operator[](int index) const
{
    DynArrayAssertIndex(index);
    return m_pItems[index];
}

DYNARRAY_BASE_TEMPLATE
inline const T & DYNARRAY_BASE::operator[](long index) const
{
    DynArrayAssertIndex(index);
    return m_pItems[index];
}

DYNARRAY_BASE_TEMPLATE
inline const T & DYNARRAY_BASE::operator[](ushort index) const
{
    DynArrayAssertIndex(index);
    return m_pItems[index];
}

DYNARRAY_BASE_TEMPLATE
inline const T & DYNARRAY_BASE::operator[](uint index) const
{
    DynArrayAssertIndex(index);
    return m_pItems[index];
}

DYNARRAY_BASE_TEMPLATE
inline const T & DYNARRAY_BASE::operator[](ulong index) const
{
    DynArrayAssertIndex(index);
    return m_pItems[index];
}

///////////////////////////////////////
//
// Convert array to a pointer to the first element
//
DYNARRAY_BASE_TEMPLATE
inline T * DYNARRAY_BASE::AsPointer()
{
    return m_pItems;
}

///////////////////////////////////////
//
// Convert array to a pointer to the first element
//
DYNARRAY_BASE_TEMPLATE
inline const T * DYNARRAY_BASE::AsPointer() const
{
    return m_pItems;
}

///////////////////////////////////////

DYNARRAY_BASE_TEMPLATE
unsigned DYNARRAY_BASE::GetBlockSize() const
{
    return BlockSize;
}

///////////////////////////////////////
//
// Get the size of the array
//

DYNARRAY_BASE_TEMPLATE
unsigned DYNARRAY_BASE::Size() const
{
    return m_nItems;
}

///////////////////////////////////////
//
// Resize the array buffer to the desired number of slots.
//
// NOTE: The logical size does not change.
//

DYNARRAY_BASE_TEMPLATE
inline BOOL DYNARRAY_BASE::Resize(unsigned newSlotCount)
{
   // Round up to the nearest even block size
   const unsigned curSlots = ((m_nItems + (BlockSize - 1)) & (~(BlockSize - 1)));
   const unsigned evenSlots = ((newSlotCount + (BlockSize - 1)) & (~(BlockSize - 1)));

   // If there is no need to change...
   if (curSlots == evenSlots)
      return TRUE;
      
   return ServiceFuncs::DoResize((void **)&m_pItems, sizeof(T), evenSlots);
}

///////////////////////////////////////
//
// Set the logical size of the array.  
//

DYNARRAY_BASE_TEMPLATE
inline BOOL DYNARRAY_BASE::SetSize(unsigned newSize)
{
   if (m_nItems != newSize)
   {
      ServiceFuncs::PreSetSize(m_pItems, m_nItems, newSize);
      Resize(newSize);
      ServiceFuncs::PostSetSize(m_pItems, m_nItems, newSize);
      m_nItems = newSize;
      return TRUE;
   }
   return FALSE;
}


///////////////////////////////////////

DYNARRAY_BASE_TEMPLATE
inline DYNARRAY_BASE::cDABase(int size) // want after SetSize
 : m_pItems(NULL),
   m_nItems(0)
{
   cDynArray_TrackCreate();
   SetSize(size);
}
   
///////////////////////////////////////

DYNARRAY_BASE_TEMPLATE
inline DYNARRAY_BASE::cDABase(const DYNARRAY_BASE &from)
 : m_pItems(NULL),
   m_nItems(0)
{
   cDynArray_TrackCreate();
   Resize(from.m_nItems);
   m_nItems = from.m_nItems;
   if (!m_nItems)
      return;

   // Call make item for all the items
   for (unsigned i = 0; i < m_nItems; i++)
      ServiceFuncs::ConstructItem(m_pItems + i, (const T *)from.m_pItems + i);
}

///////////////////////////////////////

DYNARRAY_BASE_TEMPLATE
inline DYNARRAY_BASE & DYNARRAY_BASE::operator=(const DYNARRAY_BASE & from)
{
   SetSize(0);
   Resize(from.m_nItems);
   m_nItems = from.m_nItems;
   if (!m_nItems)
      return *this;

   // Call make item for all the items
   for (unsigned i = 0; i < m_nItems; i++)
      ServiceFuncs::ConstructItem(m_pItems + i, (const T*)from.m_pItems + i);

   return *this;
}

///////////////////////////////////////

DYNARRAY_BASE_TEMPLATE
inline DYNARRAY_BASE::~cDABase() // want after SetSize
{
   cDynArray_TrackDestroy();
   if (m_pItems)
   {
      ServiceFuncs::PreSetSize(m_pItems, m_nItems, 0);
      ServiceFuncs::DoResize((void **)&m_pItems, sizeof(T), 0);
   }
}
   
///////////////////////////////////////

DYNARRAY_BASE_TEMPLATE
inline T * DYNARRAY_BASE::Detach()
{
    T * p = m_pItems;
    m_nItems = 0;
    m_pItems = NULL;
    return p;
}

///////////////////////////////////////
//
// Get the size of a array element
//
DYNARRAY_BASE_TEMPLATE
inline size_t DYNARRAY_BASE::GetElementSize() const
{
    return sizeof(T);
}

///////////////////////////////////////
//
// Return TRUE if the index is valid
//

DYNARRAY_BASE_TEMPLATE
inline BOOL DYNARRAY_BASE::IsValidIndex(unsigned index) const
{
    return (index < m_nItems);
}

///////////////////////////////////////
//
// Default routine to set an item.  The slot being copied into
// contains an initialize item which may need to be destroyed.
//

DYNARRAY_BASE_TEMPLATE
inline void DYNARRAY_BASE::SetItem(const T * pItem, unsigned index)
{
   ServiceFuncs::CopyItem(m_pItems + index, pItem);
}

///////////////////////////////////////
//
// Delete an item from the array
//

DYNARRAY_BASE_TEMPLATE
inline void DYNARRAY_BASE::DeleteItem(unsigned index)
{
   DynArrayAssertIndex(index);

   ServiceFuncs::OnDeleteItem(m_pItems + index);

   unsigned nNew = m_nItems - 1;

   // If we aren't deleting the last element...
   if (index < nNew)
      // ...then shift everything down
      memmove(m_pItems + index, m_pItems + (index + 1), (nNew - index) * sizeof(T));

   Resize(nNew);
   m_nItems--;
}

///////////////////////////////////////
//
// Swap two items.
//

DYNARRAY_BASE_TEMPLATE
inline void DYNARRAY_BASE::Swap(unsigned index1, unsigned index2)
{
   DynArrayAssertIndex(index1);
   DynArrayAssertIndex(index2);

   if (index1 != index2)
      ServiceFuncs::Swap(m_pItems + index1, m_pItems + index2);
}

///////////////////////////////////////

DYNARRAY_BASE_TEMPLATE
inline void DYNARRAY_BASE::FastDeleteItem(unsigned index)
{
   DynArrayAssertIndex(index);
    
   const int last = m_nItems - 1;
   if (index != last)
      ServiceFuncs::SwapDelete(m_pItems + index, m_pItems + last);
   else
      ServiceFuncs::OnDeleteItem(m_pItems + last);
   Resize(last);
   m_nItems--;
}

///////////////////////////////////////

DYNARRAY_BASE_TEMPLATE
inline void DYNARRAY_BASE::MoveItemToIndex(unsigned currentIndex, unsigned newIndex)
{
   DynArrayAssertIndex(currentIndex);
   DynArrayAssertIndex(newIndex);

   ServiceFuncs::MoveItem(m_pItems, m_nItems, currentIndex, newIndex);
}

///////////////////////////////////////
//
// Swap the content of two arrays (all state and ownership
// of allocated memory). If information should only go one-way,
// empty one first.
//

DYNARRAY_BASE_TEMPLATE
inline void DYNARRAY_BASE::SwapContents(DYNARRAY_BASE &x)
{
    T * p;
    p = x.m_pItems;
    x.m_pItems = m_pItems;
    m_pItems = p;

    unsigned  ix;
    ix = x.m_nItems;
    x.m_nItems = m_nItems;
    m_nItems = ix;
}


///////////////////////////////////////
//
// Sort the array of elements based on the supplied comparison function.
//

DYNARRAY_BASE_TEMPLATE
inline void DYNARRAY_BASE::Sort(tCompareFunc pfnCompare)
{
    if (m_pItems)
       qsort(m_pItems, m_nItems, sizeof(T), (tVoidCompareFunc)pfnCompare);
}

///////////////////////////////////////
//
// Find an element using a supplied comparison function.  This routine uses
// a binary search algorithm so the array must be sorted.
//

DYNARRAY_BASE_TEMPLATE
inline unsigned DYNARRAY_BASE::BSearch(const void * pKey, tSearchFunc pfnSearch) const
{
   if (m_pItems)
   {
      T * pItem = (T *)bsearch(pKey, m_pItems, m_nItems, sizeof(T), (tVoidCompareFunc)pfnSearch);
      if (pItem)
         return pItem - m_pItems;
   }
   return BAD_INDEX;
}


///////////////////////////////////////
//
// Find an element using a supplied comparison function.  This routine uses
// a straight linear search algorithm.
//

DYNARRAY_BASE_TEMPLATE
inline unsigned DYNARRAY_BASE::LSearch(const void * pKey, tSearchFunc pfnSearch) const
{
   if (m_pItems)
   {
      for (int i = 0; i < m_nItems; i++)
         if ((*pfnSearch)(pKey, &m_pItems[i]) == 0)
            return i;
   }
   return BAD_INDEX;
}


///////////////////////////////////////
//
// InsertAtIndex
//
// Insert an item into the array at the specified index. Any items
// at or above index are moved down. If index is any value greater
// than the current size, then the item is appended to the end
// of the array.
//

DYNARRAY_BASE_TEMPLATE
inline void DYNARRAY_BASE::InsertAtIndex(const T & item, unsigned index)
{
    Resize(m_nItems + 1);

    if (index < m_nItems)
    {
        // Move items at position >= index down one slot, unless it is the last item
        memmove(m_pItems + (index + 1), m_pItems + index, (m_nItems - index) * sizeof(T));
    }
    else
        index = m_nItems;

    // There's another item in the list
    m_nItems++;

    // Create the new object in the empty slot
    ServiceFuncs::ConstructItem(m_pItems + index, &item);
}

///////////////////////////////////////
//
// Append an element to the end of the array
//

DYNARRAY_BASE_TEMPLATE
inline unsigned DYNARRAY_BASE::Append(const T &item)
{
    Resize(m_nItems + 1);
    const unsigned index = m_nItems;
    m_nItems++;
    ServiceFuncs::ConstructItem(m_pItems + index, &item);
    return Size()-1;
}

///////////////////////////////////////
//
// Grow the array
//
DYNARRAY_BASE_TEMPLATE
inline unsigned DYNARRAY_BASE::Grow(unsigned num)
{
   const unsigned prevSize = Size();
   SetSize(prevSize + num);
   return prevSize;
}

///////////////////////////////////////
//
// Shrink the array
//
DYNARRAY_BASE_TEMPLATE
inline void DYNARRAY_BASE::Shrink(unsigned num)
{
   if (!num)
      return;
   DynArrayAssertIndex(num - 1);
   int shrink = (int)Size() - (int)num;
   if (shrink < 0)
      shrink = 0;
   SetSize(shrink);
}

///////////////////////////////////////
//
// Do a verified memcpy into the array
//
DYNARRAY_BASE_TEMPLATE
inline void DYNARRAY_BASE::MemCpy(const T * p, unsigned nItems)
{
   DynArrayAssertIndex(nItems - 1);
   memcpy(m_pItems, p, nItems * sizeof(T));
}

///////////////////////////////////////

DYNARRAY_BASE_TEMPLATE
inline void DYNARRAY_BASE::AppendMemCpy(const T * p, unsigned nItems)
{
   Grow(nItems);
   memcpy(m_pItems + (m_nItems - nItems), p, nItems * sizeof(T));
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()
#if defined(__WATCOMC__)
#pragma on (unreferenced)
#endif

#endif /* !__DYNARRAY_H */
