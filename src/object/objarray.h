/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/object/objarray.h,v 1.3 2000/03/07 19:58:26 toml Exp $
//

#include <ObjType.h>
#include <ObjSpace.h>
#include <string.h>

#include <appagg.h>

#include <dbmem.h>

#ifndef INCLUDED_OBJARRAY_H
#define INCLUDED_OBJARRAY_H

#pragma once

///////////////////////////////////////////////////////////////////////////////
//
// Object Array abstraction.
//
// An array, indexed by cObjIDs; 
//
// This is ported from Siege.


//
// Obj Array flags
// 

enum eObjArrayFlags 
{
   kOAF_NoAutoResize = 1 << 0,   // Don't automatically resize when the object space resizes
   kOAF_NoAbstracts  = 1 << 1,   // No abstract objects
   kOAF_NoConcretes  = 1 << 2,   // No concrete objects 
   kOAF_Static       = 1 << 3,   // We're a static variable.  Don't do work in the constructor

   kOAF_Default = 0, 

}; 


////////////////////////////////////////////////////////////
//
// TEMPLATE: cObjArray
//

template <class T, ulong flags = kOAF_Default>
class cObjArray 
{
   T* m_Vec;
   sObjBounds m_Bounds; 
   IObjIDSink* m_pSink; 


   static inline sObjBounds ZeroBounds()
   {
      sObjBounds retval =  { 0, 0}; 
      return retval;          
   }


public:
   
   //
   // Construction
   //

   cObjArray(const sObjBounds& bounds) 
      : m_Vec(NULL), m_pSink(NULL), m_Bounds(ZeroBounds())
   { 
      Resize(bounds); 
      AssertMsg(flags & kOAF_NoAutoResize, "Auto sizing object array specifies bounds in constructor\n"); 
   }; 

   cObjArray()
      : m_pSink(NULL), m_Vec(NULL), m_Bounds(ZeroBounds())
   {
      if (!(flags & kOAF_Static))
         AutoConnect(); 
   }
 

   ~cObjArray() 
   { 
      if (flags & kOAF_Static)
      {
         SafeRelease(m_pSink);
      }
      else
         AutoDisconnect(); 
      delete [] ( m_Vec + m_Bounds.min); 
   }; 

   //
   // Access an element
   //

   T& operator[] (ObjID idx) const 
   { 
      AssertMsg3((idx >= m_Bounds.min && idx < m_Bounds.max), "Obj ID %d is our of range in cObjArray::operator[] (m_Bounds.min == %d, m_Bounds.max == %d)", idx, m_Bounds.min, m_Bounds.max);
      return m_Vec[idx]; 
   }; 

   //
   // Access range limits 
   //

   const sObjBounds& Bounds() const { return m_Bounds; }; 

   //
   // Clear the array
   //
   tResult Clear(const T& val = T()); 

   //
   // Resize the array
   //
   tResult Resize(const sObjBounds& bounds); 

   //
   // Resize the array to the current size of the objid space
   //
   tResult Resize(); 

      //
   // Auto connect to ObjID man
   //
   void AutoConnect(); 
   void AutoDisconnect(); 

};

////////////////////////////////////////////////////////////

template <class T, ulong flags>
inline tResult cObjArray<T,flags>::Clear(const T& val)
{
   for (int i = m_Bounds.min; i < m_Bounds.max; i++)
      m_Vec[i] = val; 
   return S_OK; 
}

///////////////////////////////////////

template <class T, ulong flags> 
inline tResult cObjArray<T,flags>::Resize(const sObjBounds& newBounds)
{
   sObjBounds bounds = newBounds; 

   if (flags &  kOAF_NoAbstracts)
      bounds.min = 0; 
   if (flags & kOAF_NoConcretes)
      bounds.max = 0; 

   if (bounds.min == m_Bounds.min && bounds.max == m_Bounds.max)
      return S_FALSE; 

   T* oldVec = m_Vec;
   tObjIdx oldMin = m_Bounds.min;
   tObjIdx oldMax = m_Bounds.max; 

   m_Vec = new T[bounds.max-bounds.min] - bounds.min; 
   m_Bounds = bounds; 

   // Clear the "tails" of the new array
   int i; 
   for (i = bounds.min; i < oldMin; i++)
      m_Vec[i] = T(); 
   for (i = oldMax; i < bounds.max; i++)
      m_Vec[i] = T(); 
   
   // Copy the old data, accounting for either shrinking or growing
   tObjIdx start = (bounds.min < oldMin) ? oldMin : bounds.min; 
   tObjIdx end   = (bounds.max > oldMax) ? oldMax : bounds.max; 

   if (start < end) 
      memcpy(&m_Vec[start],&oldVec[start],(end-start)*sizeof(T)); 

   delete [] (oldVec + oldMin); 

   return S_OK; 
   
}

////////////////////////////////////

template <class T, ulong flags>
inline tResult cObjArray<T,flags>::Resize()
{
   AutoAppIPtr(ObjIDManager); 

   // Resize the array based on the current size of the ObjID space
   return Resize(pObjIDManager->GetObjIDBounds()); 
}


//////////////////////////////////////////////////////////////////////////////
//
// TEMPLATE: cArrayObjIDSink
//

template <class T, ulong flags> 
class cArrayObjIDSink: public cObjIDSink
{

public:
   typedef cObjArray<T,flags> cArray; 
   
   cArrayObjIDSink(cArray* pArray) : m_pArray(pArray) {}; 


   virtual void OnObjIDSpaceResize(const sObjBounds& bounds)
   {
      m_pArray->Resize(bounds); 
   }

private:

   cArray* m_pArray; 


};

//////////////////////////////////////////////////////////////////////////////
//
// AutoConnection support for array
//
//

template <class T, ulong flags>
inline void cObjArray<T,flags>::AutoConnect()
{
   AutoAppIPtr(ObjIDManager); 

   // Resize the array based on the current size of the ObjID space
   Resize(pObjIDManager->GetObjIDBounds()); 

   if (!(flags & kOAF_NoAutoResize))
   {
      m_pSink = new cArrayObjIDSink<T,flags>(this);        
      pObjIDManager->Connect(m_pSink); 
   }
}



///////////////////////////////////////

template <class T, ulong flags>
inline void cObjArray<T,flags>::AutoDisconnect()
{
   if (m_pSink)
   {
      AutoAppIPtr(ObjIDManager); 
      pObjIDManager->Disconnect(m_pSink); 

      SafeRelease(m_pSink);       
   }
}

#include <undbmem.h> 

///////////////////////////////////////////////////////////////////////////////

#endif /* !INCLUDED_OBJARRAY_H */






