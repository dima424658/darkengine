/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/lnktrai_.h,v 1.1 1998/07/01 13:05:19 mahk Exp $
#pragma once  
#ifndef __LNKTRAI__H
#define __LNKTRAI__H

#include <comtools.h>
#include <edtrait.h>
#include <edtraist.h>
#include <linktype.h>

//
// Link trait impl class
//

class cLinkEditTrait : public cCTUnaggregated<IEditTrait,&IID_IEditTrait,kCTU_Default>
{

protected:
   // We use the ID instead of the IRelation so we can support wildcard
   RelationID mID;  
   sEditTraitDesc mDesc; 
   sEditTraitCaps mCaps; 

   static sEditTraitCaps gCaps;

   int LinkCount(ObjID obj);


   virtual void LinkDialog(ObjID obj);


public: 
   cLinkEditTrait(RelationID id, sEditTraitDesc* desc);



   STDMETHOD_(const sEditTraitDesc*,Describe)() 
   {
      return &mDesc; 
   }

   STDMETHOD_(const sEditTraitCaps*,Capabilities)()
   {
      return &mCaps; 
   }
   
   STDMETHOD_(BOOL,IntrinsicTo)(ObjID obj)
   {
      return LinkCount(obj) != 0; 
   }

   STDMETHOD(Add)(ObjID obj)
   {
      LinkDialog(obj);
      return S_OK; 
   }

   STDMETHOD(Remove)(ObjID )
   {
      return E_FAIL; 
   }

   STDMETHOD(Edit)(ObjID obj)
   {
      LinkDialog(obj); 
      return S_OK; 
   }

   STDMETHOD(Parse)(ObjID , const char* )
   {
      return E_FAIL; 
   }

   STDMETHOD(Unparse)(ObjID obj, char* buf, int buflen);


}; 

#endif // __LNKTRAI__H
