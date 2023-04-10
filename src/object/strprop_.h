/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/strprop_.h,v 1.5 1998/08/24 20:57:59 mahk Exp $
#pragma once  
#ifndef __STRPROP__H
#define __STRPROP__H

#include <propface.h>
#include <propert_.h>
#include <dataops_.h>
#include <str.h>
#include <label.h>


//////////////////////////////////////////////////////////////
// STRING PROPERTY IMPLEMENTATION CLASS
//

//
// Data ops
//

class cStringDataOps: public cClassDataOps<cStr>
{
public:

   cStringDataOps() : cClassDataOps<cStr>(kNoFlags) {}; 

   STDMETHOD_(int,Version)() { return sizeof(Label)+1; }; 

   STDMETHOD(Read)(sDatum* pdat, IDataOpsFile* file, int version ); 
   STDMETHOD(Write)(sDatum val, IDataOpsFile* file) ;

   static cStringDataOps gOps; 
}; 

//
// Property Class
//

#ifdef EDITOR
EXTERN void CreateStringPropEditor(IStringProperty* prop); 
#else 
#define CreateStringPropEditor(x)
#endif 

//
// TEMPLATE: cStringPropertyExtensions
// Takes a property class with cStr* accessors, and provides const char* accessors
//
// Allows both generic and specific properties to be built 
//


template <class BASECLASS> 
class cStringPropertyExtensions: public BASECLASS
{

public:
   cStringPropertyExtensions(const sPropertyDesc* desc, IPropertyStore* store)
      : BASECLASS(desc,store)
   {
      SetOps(&cStringDataOps::gOps); 
   }

   STDMETHOD_(BOOL,Get)(ObjID obj, const char* (*ptr)) const
   {
      cStr* str; 
      if (BASECLASS::Get(obj,&str))
      {
         *ptr = (const char*)(*str);
         return TRUE;
      }
      return FALSE; 
   }

   STDMETHOD_(BOOL,GetSimple)(ObjID obj, const char* (*ptr)) const 
   {
      cStr* str; 
      if (BASECLASS::GetSimple(obj,&str))
      {
         *ptr = (const char*)(*str);
         return TRUE;
      }
      return FALSE; 
   }

   STDMETHOD(Set)(ObjID obj, const char* value)
   {
      cStr str = value; 
      return BASECLASS::Set(obj,&str); 
   }

   STDMETHOD_(BOOL,IterNextValue)(sPropertyObjIter* iter, ObjID* next, const char* (*ptr)) const 
   {
      cStr* str; 
      if (BASECLASS::IterNextValue(iter,next,&str))
      {
         *ptr = (const char*)(*str); 
         return TRUE; 
      }
      return FALSE; 
   }

   STDMETHOD_(BOOL,TouchValue)(ObjID obj, const char* val)
   {
      cStr str = val; 
      return BASECLASS::TouchValue(obj,&str); 
   }
 
protected:
   virtual void CreateEditor()
   {
       CreateStringPropEditor(this); 
   }

}; 


////////////////////////////////////////////////////////////

typedef cGenericProperty<IStringProperty,&IID_IStringProperty,cStr*> cStringPropBase; 

typedef cStringPropertyExtensions<cStringPropBase> cGenericStringProperty; 


#endif // __STRPROP__H





