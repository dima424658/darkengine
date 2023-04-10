///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/cpptools/RCS/dynarray.h $
// $Author: TOML $
// $Date: 1998/06/11 13:46:09 $
// $Revision: 1.15 $
//
// (c) Copyright 1993-1996 Tom Leonard. All Rights Reserved. Unlimited license granted to Looking Glass Technologies Inc.
//
// @TBD (toml 03-31-98): should do a more fully inlined version for efficiency
//

#ifndef __DYNARRAY_H
#define __DYNARRAY_H

#include <string.h>
#include <stddef.h>

#ifdef  _MSC_VER
#include <new>
#endif

#include <lg.h>

#if defined(__WATCOMC__)
#pragma warning 549 9
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Dynamic Bounds-checked Arrays
// -----------------------------
//
// This header declares a simple class to encapsulate dynamically allocated
// arrays. Data items must either be simple types, complex types without
// constructors, or complex types with constructors and where the virtual
// functions for object management are overriden.
//
// Items must be safely relocatable, as reallocs may implicitly occur.
//
// For example, to declare a dynamic array of integers:
//
// cDynArray<int> myArray;
//
// myArray[1] = 1;
//
// Note block size must be a power of 2


///////////////////////////////////////////////////////////////////////////////

class cIStore;
class cOStore;

typedef unsigned long index_t; // for indexing into the array
typedef void tDynArrayItem;
typedef void tDynArrayKey;
typedef int (*tSearchFunc)(const tDynArrayKey *pKey, const tDynArrayItem *); // Comparison functions for search and sort
typedef int (*tSortFunc)(const tDynArrayItem *, const tDynArrayItem *);

const index_t   BAD_INDEX           = 0xFFFFFFFFL;
const ushort    kDynArrDefBlockSize = 8;

// @TBD (toml 03-22-96): Make export/importable
#define __CPPTOOLSAPI

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDynArrayBase
//
// Base class of dynamic array
//

#pragma pack(4)

class __CPPTOOLSAPI cDynArrayBase
{
public:

    ///////////////////////////////////
    //
    // Get/Set the block size (the minimum increment by which the array storage
    // grows and shrinks)
    //
    unsigned GetBlockSize() const;
    void     SetBlockSize(ushort);

    ///////////////////////////////////
    //
    // Get and set array size (the number of active elements)
    //
    index_t      Size() const;
    virtual BOOL SetSize(index_t);

    ///////////////////////////////////
    //
    // Query the size of array elements
    //
    size_t GetElementSize() const;

    ///////////////////////////////////
    //
    // Test if a given index is valid
    //
    BOOL IsValidIndex(index_t index) const;

    ///////////////////////////////////
    //
    // Assign to a slot
    //
    virtual void SetItem(const tDynArrayItem* itemPtr, index_t index);

    ///////////////////////////////////
    //
    // Remove an item, shifting items down
    //
    virtual void DeleteItem(index_t index);
    void FastDeleteItem(index_t index);

    ///////////////////////////////////
    //
    // Grow the array, insert an item, shifting items up
    //
    void InsertAtIndex(const tDynArrayItem *itemPtr, index_t index);

    ///////////////////////////////////
    //
    // Move an item, shifting items
    //
    void MoveItemToIndex(index_t currentIndex,index_t newIndex);

    ///////////////////////////////////
    //
    // Streaming protocol
    //
    BOOL ToStream(cOStore &) const;
    BOOL FromStream(cIStore &);

    ///////////////////////////////////
    //
    // Swap the contents of two arrays
    //
    void SwapContents(cDynArrayBase &);

    ///////////////////////////////////
    //
    // Array sorting and searching (qsort, linear search, bninary search)
    //
    void    Sort(tSortFunc);
    index_t LSearch(tDynArrayKey *, tSearchFunc) const;
    index_t BSearch(tDynArrayKey *, tSearchFunc) const;

protected:

    ///////////////////////////////////
    //
    // Constructors, assignment operator, and destructor
    //
    cDynArrayBase(size_t ElementSize);
    cDynArrayBase(const cDynArrayBase &);
    virtual ~cDynArrayBase();

    cDynArrayBase & operator= (const cDynArrayBase&);


    ///////////////////////////////////
    //
    // Functions for moving, creating, and copying items.  Along with
    // DeleteItem(), derived arrays that contain complex types with
    // constructors or destructors should override these
    //
    virtual void MakeItem(const tDynArrayItem *, index_t);
    virtual void Swap(index_t, index_t);

    virtual void CopyToTemporary(index_t);
    virtual void CopyFromTemporary(index_t);

    virtual BOOL ItemToStream(cOStore &, const tDynArrayItem *) const;
    virtual BOOL ItemFromStream(cIStore &, tDynArrayItem *);

    BOOL Resize(index_t numSlots);

    tDynArrayItem *ItemPtr(index_t index) const;
    tDynArrayItem *UnsafeItemPtr(index_t index) const;

    tDynArrayItem *GetItemArray() const;

    tDynArrayItem *Detach();

    ///////////////////////////////////

#ifndef SHIP
    void InitExaminePointer(tDynArrayItem *** pppExaminePointer);

    static char BASED_CODE gm_pszOutOfRange[]; // Out of range message
#endif

private:

    BOOL DoResize(index_t numSlots);
    
    ///////////////////////////////////

    BYTE *  m_pItems;                            // Items in the array
    size_t  m_nItemSize;                         // Size of each element in bytes

    index_t m_nItems;                            // Number of items in the array
    index_t m_nSlots;                            // Total number of slots allocated

    unsigned m_nBlockSizeLessOne;
};

#pragma pack()

////////////////////////////////////////////////////////////
//
// TEMPLATE: cDynArray
//
// A dynamic array, templatized
//

template <class T>
class cDynArray : public cDynArrayBase
{
public:
    ///////////////////////////////////
    //
    // Constructors, assignment operator, and destructor
    //
    cDynArray();
    cDynArray(int size, int blockSize = 0);
    cDynArray(const cDynArray<T> &);
    cDynArray<T> & operator=(const cDynArray<T> &);

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
    operator T *();
    operator const T *() const;

    T * AsPointer();
    const T * AsPointer() const;

    ///////////////////////////////////
    //
    // Grow the array, insert an item, shifting items up
    //
    void InsertAtIndex(const T &, index_t);

    ///////////////////////////////////
    //
    // Grow the array, appending an item
    //
    index_t Append(const T &);

    ///////////////////////////////////
    //
    // Grow/shrink the array, filling new slots with blanks
    //
    index_t Grow(unsigned num = 1);
    void Shrink(unsigned num = 1);

    ///////////////////////////////////
    //
    // Do a verified memcpy into the array
    //
    void MemCpy(const T *, unsigned nItems);
    
private:

#ifndef SHIP
    // Use this to view base items in source level debugger
    const T ** m_ppExaminePointer;
#endif
};

////////////////////////////////////////////////////////////
//
// TEMPLATE: cDynClassArray
//
// A dynamic array, templatized, with functions for
// dealing with constructors and destructors implemented
//

template <class T>
class cDynClassArray : public cDynArray<T>
{
public:
    ///////////////////////////////////
    //
    // Constructors, assignment operator, and destructor
    //
    cDynClassArray();
    cDynClassArray(int size, int blockSize = 0);
    cDynClassArray(const cDynClassArray<T> &);
    cDynClassArray<T> & operator=(const cDynClassArray<T> &);

    virtual BOOL SetSize(index_t);
    virtual void SetItem(const tDynArrayItem* itemPtr, index_t index);
    virtual void DeleteItem(index_t index);
    virtual void MakeItem(const tDynArrayItem *, index_t);
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDynArrayBase, inline functions
//

///////////////////////////////////////
//
// Resize the array buffer to the desired number of slots.
//
// NOTE: The logical size does not change.
//

inline BOOL cDynArrayBase::Resize(index_t newSlotCount)
{
    // Round up to the nearest even block size
    const index_t evenSlots = ((newSlotCount + m_nBlockSizeLessOne) & (~m_nBlockSizeLessOne));

    // If there is no need to change...
    if (m_nSlots == evenSlots)
        return TRUE;

   return DoResize(evenSlots);
}

///////////////////////////////////////
//
// Initialize an array object, specifying the element size. The initialized
// array is empty.
//

inline cDynArrayBase::cDynArrayBase(size_t elementSize)
  : m_nBlockSizeLessOne(kDynArrDefBlockSize - 1),
    m_nItemSize(elementSize),
    m_nItems(0),
    m_nSlots(0),
    m_pItems(0)
{
}

///////////////////////////////////////
//
// Copy constructor
//

inline cDynArrayBase::cDynArrayBase(const cDynArrayBase& array)
  : m_nBlockSizeLessOne(array.m_nBlockSizeLessOne),
    m_nItemSize(array.m_nItemSize),
    m_nItems(array.m_nItems),
    m_nSlots(0),
    m_pItems(0)
{
    VerifyMsg(Resize(m_nItems), "Array allocation failed");

    AssertMsg(!m_nItems || m_pItems, "Expected items in dynarray");

    // Call make item for all the items
    for (index_t i = 0; i < m_nItems; i++)
        MakeItem(array.ItemPtr(i), i);
}


///////////////////////////////////////
//
// Destroy the dynamic array
//

inline cDynArrayBase::~cDynArrayBase()
{
    if (m_pItems)
        free(m_pItems);
}

///////////////////////////////////////
//
// Get the size of the array
//

inline index_t cDynArrayBase::Size() const
{
    return m_nItems;
}

///////////////////////////////////////
//
// Return TRUE if the index is valid
//

inline BOOL cDynArrayBase::IsValidIndex(index_t index) const
{
    return (index < m_nItems);
}

///////////////////////////////////////
//
// Get a pointer to an item given its index
//

inline tDynArrayItem *cDynArrayBase::ItemPtr(index_t index) const
{
    AssertMsg(m_pItems && index < m_nItems, gm_pszOutOfRange);
    return m_pItems + index * m_nItemSize;
}

///////////////////////////////////////
//
// Get a pointer to an item given its index
//

inline tDynArrayItem *cDynArrayBase::UnsafeItemPtr(index_t index) const
{
    return m_pItems + index * m_nItemSize;
}

///////////////////////////////////////
//
// Default routine to delete an item from the array (virtual)
//

inline void cDynArrayBase::DeleteItem(index_t index)
{
    AssertMsg(index < m_nItems, gm_pszOutOfRange);

    m_nItems--;                         // We're gonna get rid of an item

    // If we aren't deleting the last element...
    if (index < m_nItems)
        // ...then shift everything down
        memmove(UnsafeItemPtr(index), UnsafeItemPtr(index+1), (m_nItems - index) * m_nItemSize);

    Resize(m_nItems);
}

// Macro for obtaining a pointer to temporary storage for one
// element. The temporary storage is an extra slot kept at the
// end of the item handle.
#define TmpItemPtr() UnsafeItemPtr(m_nItems)

///////////////////////////////////////
//
// Place an item into temporary storage
//

inline void cDynArrayBase::CopyToTemporary(index_t index)
{
    memcpy(TmpItemPtr(), UnsafeItemPtr(index), m_nItemSize);
}

///////////////////////////////////////
//
// Get an item from temporary storage
//

inline void cDynArrayBase::CopyFromTemporary(index_t index)
{
    memcpy(UnsafeItemPtr(index), TmpItemPtr(), m_nItemSize);
}

#undef TmpItemPtr

///////////////////////////////////////
//
// Swap two items.
//

inline void cDynArrayBase::Swap(index_t index1, index_t index2)
{
    AssertMsg(index1 < m_nItems && index2 < m_nItems, gm_pszOutOfRange);

    if (index1 != index2)
    {
       CopyToTemporary(index1);
       memcpy(UnsafeItemPtr(index1),UnsafeItemPtr(index2), m_nItemSize);
       CopyFromTemporary(index2);
    }
}

///////////////////////////////////////

inline void cDynArrayBase::FastDeleteItem(index_t index)
{
   int last = m_nItems - 1;
   if (index != last)
      Swap(index, last);
   DeleteItem(last);
}

///////////////////////////////////////
//
// Get the size of a array element
//
inline size_t cDynArrayBase::GetElementSize() const
{
    return m_nItemSize;
}

///////////////////////////////////////
//
// Get the block size
//
inline unsigned cDynArrayBase::GetBlockSize() const
{
    return m_nBlockSizeLessOne + 1;
}

///////////////////////////////////////
//
// Set the size of the allocation unit
//
inline void cDynArrayBase::SetBlockSize(ushort blockSize)
{
   #ifdef DEBUG
   AssertMsg(((blockSize) & (blockSize - 1)) == 0, "cDynArray block size must be a power of 2");
   #endif
   m_nBlockSizeLessOne = (blockSize - 1);
}

///////////////////////////////////////
//
// Get the item array
//
inline tDynArrayItem *cDynArrayBase::GetItemArray() const
{
    return m_pItems;
}

///////////////////////////////////////
//
// Default routine to set an item (virtual).  The slot being copied into
// contains an initialize item which may need to be destroyed.
//

inline void cDynArrayBase::SetItem(const tDynArrayItem *itemPtr, index_t index)
{
    memmove(UnsafeItemPtr(index), itemPtr, m_nItemSize);
}

///////////////////////////////////////
//
// Default routine to create an item (virtual).  The slot being copied into
// contains uninitialized space.
//

inline void cDynArrayBase::MakeItem(const tDynArrayItem *itemPtr, index_t index)
{
    memmove(UnsafeItemPtr(index), itemPtr, m_nItemSize);
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

inline void cDynArrayBase::InsertAtIndex(const tDynArrayItem *itemPtr, index_t index)
{
    Resize(m_nItems + 1);

    if (index < m_nItems)
    {
        // Move items at position >= index down one slot, unless it is the last item
        memmove(UnsafeItemPtr(index+1), UnsafeItemPtr(index), (m_nItems - index + 1) * m_nItemSize);
    }
    else
        index = m_nItems;

    // There's another item in the list
    m_nItems++;

    // Create the new object in the empty slot
    MakeItem(itemPtr, index);
}

///////////////////////////////////////

inline tDynArrayItem *cDynArrayBase::Detach()
{
    void * p = m_pItems;
    m_nItems = 0;
    m_nSlots = 0;
    m_pItems = 0;
    return p;
}

///////////////////////////////////////
//
// Set the logical size of the array (virtual).  A derived class may want to override
// this function to construct default objects or to delete existing objects.
//

inline BOOL cDynArrayBase::SetSize(index_t newSize)
{
    if (m_nItems != newSize && Resize(newSize))
    {
        m_nItems = newSize;
        return TRUE;
    }
    return FALSE;
}

///////////////////////////////////////

#ifndef SHIP
inline void cDynArrayBase::InitExaminePointer(tDynArrayItem *** pppExaminePointer)
{
   *pppExaminePointer = (tDynArrayItem **)&m_pItems;
}
#endif

///////////////////////////////////////////////////////////////////////////////
//
// TEMPLATE: cDynArray, inline functions
//

template <class T>
inline cDynArray<T>::cDynArray()
    : cDynArrayBase(sizeof(T))
{
#ifndef SHIP
   InitExaminePointer((tDynArrayItem ***) &m_ppExaminePointer);
#endif
}

///////////////////////////////////////

template <class T>
inline cDynArray<T>::cDynArray(int size, int blockSize)
    : cDynArrayBase(sizeof(T))
{
#ifndef SHIP
   InitExaminePointer((tDynArrayItem ***) &m_ppExaminePointer);
#endif
   if (blockSize)
      SetBlockSize((ushort)blockSize);
   SetSize(size);
}

///////////////////////////////////////

template <class T>
inline cDynArray<T>::cDynArray(const cDynArray<T>& array)
    : cDynArrayBase(array)
{
#ifndef SHIP
   InitExaminePointer((tDynArrayItem ***) &m_ppExaminePointer);
#endif
}

///////////////////////////////////////

template <class T>
inline cDynArray<T> &cDynArray<T>::operator=(const cDynArray<T>& array)
{
    return (cDynArray<T> &)cDynArrayBase::operator=(array);
}

///////////////////////////////////////

template <class T>
inline T * cDynArray<T>::Detach()
{
    return (T *)cDynArrayBase::Detach();
}

///////////////////////////////////////
//
// Get a pointer to the element at the specified index
//
template <class T>
inline T &cDynArray<T>::operator[](short index)
{
    AssertMsg(GetItemArray() && index < Size(), gm_pszOutOfRange);
    return ((T *)GetItemArray())[index];
}

template <class T>
inline T &cDynArray<T>::operator[](int index)
{
    AssertMsg(GetItemArray() && index < Size(), gm_pszOutOfRange);
    return ((T *)GetItemArray())[index];
}

template <class T>
inline T &cDynArray<T>::operator[](long index)
{
    AssertMsg(GetItemArray() && index < Size(), gm_pszOutOfRange);
    return ((T *)GetItemArray())[index];
}

template <class T>
inline T &cDynArray<T>::operator[](ushort index)
{
    AssertMsg(GetItemArray() && index < Size(), gm_pszOutOfRange);
    return ((T *)GetItemArray())[index];
}

template <class T>
inline T &cDynArray<T>::operator[](uint index)
{
    AssertMsg(GetItemArray() && index < Size(), gm_pszOutOfRange);
    return ((T *)GetItemArray())[index];
}

template <class T>
inline T &cDynArray<T>::operator[](ulong index)
{
    AssertMsg(GetItemArray() && index < Size(), gm_pszOutOfRange);
    return ((T *)GetItemArray())[index];
}

///////////////////////////////////////
//
// Get a const pointer to the element at the specified index
//
template <class T>
inline const T &cDynArray<T>::operator[](short index) const
{
    AssertMsg(GetItemArray() && index < Size(), gm_pszOutOfRange);
    return ((T *)GetItemArray())[index];
}

template <class T>
inline const T &cDynArray<T>::operator[](int index) const
{
    AssertMsg(GetItemArray() && index < Size(), gm_pszOutOfRange);
    return ((T *)GetItemArray())[index];
}

template <class T>
inline const T &cDynArray<T>::operator[](long index) const
{
    AssertMsg(GetItemArray() && index < Size(), gm_pszOutOfRange);
    return ((T *)GetItemArray())[index];
}

template <class T>
inline const T &cDynArray<T>::operator[](ushort index) const
{
    AssertMsg(GetItemArray() && index < Size(), gm_pszOutOfRange);
    return ((T *)GetItemArray())[index];
}

template <class T>
inline const T &cDynArray<T>::operator[](uint index) const
{
    AssertMsg(GetItemArray() && index < Size(), gm_pszOutOfRange);
    return ((T *)GetItemArray())[index];
}

template <class T>
inline const T &cDynArray<T>::operator[](ulong index) const
{
    AssertMsg(GetItemArray() && index < Size(), gm_pszOutOfRange);
    return ((T *)GetItemArray())[index];
}

///////////////////////////////////////
//
// Insert an element at the specified index
//
template <class T>
inline void cDynArray<T>::InsertAtIndex(const T &item, index_t index)
{
    cDynArrayBase::InsertAtIndex(&item, index);
}

///////////////////////////////////////
//
// Append an element to the end of the array
//
template <class T>
inline index_t cDynArray<T>::Append(const T &item)
{
    InsertAtIndex(item, BAD_INDEX);
    return Size()-1;
}


///////////////////////////////////////
//
// Grow the array
//
template <class T>
inline index_t cDynArray<T>::Grow(unsigned num)
{
   const index_t prevSize = Size();
   SetSize(prevSize + num);
   return prevSize;
}

///////////////////////////////////////
//
// Shrink the array
//
template <class T>
inline void cDynArray<T>::Shrink(unsigned num)
{
    int shrink = (int)Size() - (int)num;
    if (shrink < 0)
        shrink = 0;
    SetSize(shrink);
}

///////////////////////////////////////
//
// Do a verified memcpy into the array
//
template <class T>
inline void cDynArray<T>::MemCpy(const T * p, unsigned nItems)
{
   AssertMsg(GetItemArray() && nItems <= Size(), gm_pszOutOfRange);
   memcpy(GetItemArray(), p, nItems * sizeof(T));
}

///////////////////////////////////////
//
// Convert array to a pointer to the first element
//
template <class T>
inline cDynArray<T>::operator T *()
{
    return (T *)GetItemArray();
}

///////////////////////////////////////
//
// Convert array to a pointer to the first element
//
template <class T>
inline cDynArray<T>::operator const T *() const
{
    return (const T *)GetItemArray();
}

///////////////////////////////////////
//
// Convert array to a pointer to the first element
//
template <class T>
inline T * cDynArray<T>::AsPointer()
{
    return (T *)GetItemArray();
}

///////////////////////////////////////
//
// Convert array to a pointer to the first element
//
template <class T>
inline const T * cDynArray<T>::AsPointer() const
{
    return (const T *)GetItemArray();
}

///////////////////////////////////////////////////////////////////////////////
//
// TEMPLATE: cDynClassArray, inline functions
//

template <class T>
inline cDynClassArray<T>::cDynClassArray()
{
}

template <class T>
inline cDynClassArray<T>::cDynClassArray(int size, int blockSize)
 : cDynArray<T>(size, blockSize)
{
}

template <class T>
inline cDynClassArray<T>::cDynClassArray(const cDynClassArray<T> & fromArray)
 : cDynArray<T>(fromArray)
{
}

template <class T>
inline cDynClassArray<T> & cDynClassArray<T>::operator=(const cDynClassArray<T> & fromArray)
{
   cDynArray<T>::operator=(fromArray);
   return *this;
}

template <class T>
inline BOOL cDynClassArray<T>::SetSize(index_t newSize)
{
   BOOL result;
   int oldSize = Size();
   if (newSize > oldSize)
   {
      result = cDynArray<T>::SetSize(newSize);
      for (; oldSize < newSize; oldSize++)
         new ( ((T *)GetItemArray() + oldSize)) T;
   }
   else if (newSize < oldSize)
   {
      do
      {
         oldSize--;
         (((T *)GetItemArray() + oldSize))->~T();
      } while (oldSize != newSize);

      result = cDynArray<T>::SetSize(newSize);
   }
   return result;
}

template <class T>
inline void cDynClassArray<T>::SetItem(const tDynArrayItem* pItem, index_t index)
{
   ((T *)GetItemArray() + index)->operator=(*((T *)pItem));
}

template <class T>
inline void cDynClassArray<T>::DeleteItem(index_t index)
{
   ((T *)GetItemArray() + index)->~T();
   cDynArray<T>::DeleteItem(index);
}

template <class T>
inline void cDynClassArray<T>::MakeItem(const tDynArrayItem * pItem, index_t index)
{
   ((T *)GetItemArray() + index)->operator=(*((T *)pItem));
}

///////////////////////////////////////////////////////////////////////////////

//if defined(__WATCOMC__)
//pragma warning 549 2
//endif

#endif /* !__DYNARRAY_H */
