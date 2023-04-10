/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: x:/prj/tech/libsrc/cpptools/RCS/dynarsrv.h 1.4 1998/10/03 10:57:41 TOML Exp $
//
// Dynarray service classes
//

#ifndef __DYNARSRV_H
#define __DYNARSRV_H

#pragma once

#if defined(__WATCOMC__)
   #pragma off (unreferenced)
#endif

///////////////////////////////////////////////////////////////////////////////

class cDABaseSrvFns
{
public:
   static BOOL DoResize(void ** ppItems, unsigned nItemSize, unsigned nNewSlots);
   
#ifndef SHIP
   static const char gm_pszOutOfRange[]; // Out of range message
#endif

#ifdef DEBUG
   static void TrackCreate(unsigned nItemSize);
   static void TrackDestroy();
   
   #define cDynArray_TrackCreate() ServiceFuncs::TrackCreate(sizeof(T))
   #define cDynArray_TrackDestroy() ServiceFuncs::TrackDestroy()

#else
   #define cDynArray_TrackCreate()
   #define cDynArray_TrackDestroy()
#endif
};

#define DynArrayAssertIndex(index) AssertMsg1((unsigned)(index) < m_nItems, ServiceFuncs::gm_pszOutOfRange, index)

///////////////////////////////////////////////////////////////////////////////

template <class T>
class cDARawSrvFns : public cDABaseSrvFns
{
public:

   ////////////////////////////////////
   
   inline static void PreSetSize(T * /*pItems*/, unsigned /*oldSize*/, unsigned /*newSize*/)
   {
   }
   
   ////////////////////////////////////
   
   inline static void PostSetSize(T * /*pItems*/, unsigned /*oldSize*/, unsigned /*newSize*/)
   {
   }
   
   ////////////////////////////////////
   
   inline static void ConstructItem(T * pItem, const T * pFrom)
   {
      memcpy(pItem, pFrom, sizeof(T));
   }
   
   ////////////////////////////////////
   
   inline static void CopyItem(T * pTo, const T * pFrom)
   {
      if (pTo != (T *)pFrom)
         memcpy(pTo, pFrom, sizeof(T));
   }
   
   ////////////////////////////////////
   
   inline static void OnDeleteItem(T * /*pItem*/)
   {
   }
   
   ////////////////////////////////////
   
   inline static void Swap(T * pItem1, T * pItem2)
   {
      void * pTemp = alloca(sizeof(T));
      memcpy(pTemp, pItem1, sizeof(T));
      memcpy(pItem1, pItem2, sizeof(T));
      memcpy(pItem2, pTemp, sizeof(T));
   }
   
   ////////////////////////////////////
   
   inline static void SwapDelete(T * pItem1, T * pItem2)
   {
      memcpy(pItem1, pItem2, sizeof(T));
   }
   
   ////////////////////////////////////
   
   inline static void MoveItem(T * pItems, unsigned /*nItems*/, unsigned currentIndex, unsigned newIndex)
   {
      void * pTemp = alloca(sizeof(T));
      if (currentIndex == newIndex)
        return;

      // In order to move the item, we must first make a copy of it.
      memcpy(pTemp, pItems + currentIndex, sizeof(T));

      // If the element is before target location...
      if (currentIndex < newIndex)
      {
        // Shift items between current and target locations down one
        memmove(pItems + currentIndex, pItems + (currentIndex + 1), (newIndex - currentIndex) * sizeof(T));
      }

      // ...else the element is after target location...
      {
        // Shift items between target and current locations up one.
        memmove(pItems + (newIndex + 1), pItems + newIndex, (currentIndex - newIndex) * sizeof(T));
      }

      // Copy element into new position
      memcpy(pItems + newIndex, pTemp, sizeof(T));
   }
};

///////////////////////////////////////////////////////////////////////////////

template <class T>
class cDAClsSrvFns : public cDARawSrvFns<T>
{
public:

   ////////////////////////////////////
   
   inline static void PreSetSize(T * pItems, unsigned oldSize, unsigned newSize)
   {
      if (newSize < oldSize)
      {
         do
         {
            oldSize--;
            (pItems + oldSize)->~T();
         } while (oldSize != newSize);
      }
   }
   
   ////////////////////////////////////
   
   inline static void PostSetSize(T * pItems, unsigned oldSize, unsigned newSize)
   {
      for (; oldSize < newSize; oldSize++)
         new (pItems + oldSize) T;
   }
   
   ////////////////////////////////////
   
   inline static void ConstructItem(T * pItem, const T * pFrom)
   {
      new (pItem) T(*pFrom);
   }
   
   ////////////////////////////////////
   
   inline static void CopyItem(T * pTo, const T * pFrom)
   {
      if (pTo != pFrom)
         (pTo)->operator=(*pFrom);
   }
   
   ////////////////////////////////////
   
   inline static void OnDeleteItem(T * pItem)
   {
      (pItem)->~T();
   }
   
   ////////////////////////////////////
   
   inline static void SwapDelete(T * pItem1, T * pItem2)
   {
      (pItem1)->~T();
      memcpy(pItem1, pItem2, sizeof(T));
   }
   
};

///////////////////////////////////////////////////////////////////////////////

template <class T>
class cDAZeroSrvFns : public cDARawSrvFns<T>
{
public:   
   inline static void PostSetSize(T * pItems, unsigned oldSize, unsigned newSize)
   {
      for (; oldSize < newSize; oldSize++)
         memset(pItems + oldSize, 0, sizeof(T));
   }
};

///////////////////////////////////////////////////////////////////////////////

#if defined(__WATCOMC__)
   #pragma on (unreferenced)
#endif

#endif /* !__DYNARSRV_H */
