/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/idispat_.h,v 1.1 1998/04/20 13:41:07 mahk Exp $
#pragma once  
#ifndef __IDISPAT__H
#define __IDISPAT__H
#include <idispatc.h>
#include <dspchdef.h>
#include <appagg.h>
#include <aggmemb.h>
#include <listset.h>

//
// We expect that people will want to derive off of IDispatch, so we'll provide 
// an implementation template, rather than just a base class 
//

template<class BASE> 
class cDispatch : public BASE 
{

protected: 

   // Extend listener desc to include equality 
   struct sElem : public sDispatchListenerDesc 
   {
      sElem(const sDispatchListenerDesc& d) { *(sDispatchListenerDesc*)this = d; };
      BOOL operator == (const sElem& e) const { return *pID == *e.pID; }; 
   };

   class cListeners : public cSimpleListSet<sElem> 
   {
   }; 

   // The listeners list 
   cListeners mListeners; 


public:


   STDMETHOD(Listen)(const sDispatchListenerDesc* desc)
   {
      return mListeners.AddElem(*desc) ? S_OK : E_FAIL; 
   }

   STDMETHOD(Unlisten)(const GUID* guid)
   {
      sDispatchListenerDesc desc = { guid}; 
      return mListeners.RemoveElem(desc) ? S_OK : S_FALSE; 
   }

   STDMETHOD(SendMsg)(const sDispatchMsg* msg)
   {
      typename cListeners::cIter iter;
      for (iter = mListeners.Iter(); !iter.Done(); iter.Next())
      {
         sElem& elem = iter.Value(); 
         if (elem.interests & msg->kind)
            elem.func(msg,&elem); 
      }
      return S_OK; 

   }

};

#endif // __IDISPAT__H












