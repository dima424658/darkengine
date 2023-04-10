/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/edittul_.h,v 1.2 1997/10/16 18:19:07 mahk Exp $
#pragma once  
#ifndef __EDITTUL__H
#define __EDITTUL__H

#include <edittool.h>

class cEditTools : public IEditTools
{
   DECLARE_AGGREGATION(cEditTools);

public:
   cEditTools(IUnknown* pOuter);
   virtual ~cEditTools();

   //
   // IEditTools Methods
   //

   STDMETHOD_(const char*,ObjName)(ObjID obj);
   STDMETHOD_(ObjID,GetObjNamed)(const char* name) ; 
   STDMETHOD(EditTypedData)(const char* title, const char* type, void* data);


protected:
   // Aggie protocol
   HRESULT Init();
   HRESULT End();

private:
   // As if we had private members

};


#endif // __EDITTUL__H
