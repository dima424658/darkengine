/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/propval_.h,v 1.3 2000/01/29 13:24:47 adurant Exp $
#pragma once

#ifndef PROPVAL__H
#define PROPVAL__H
#include <propimp.h>

////////////////////////////////////////////////////////////
// DEFAULT VALUE MANIPULATORS
//
// This is the default "value manipulator" for the proprerty system.
// It should work for scalars and pointer-free structures.
//

class cDefaultPropValueFuncs : public cPropValueFuncs
{
   int size_;
public:
   cDefaultPropValueFuncs(int size) :size_(size) {};
   ~cDefaultPropValueFuncs();
   
   //
   // cPropValueFuncs members
   // 
   uint Version(void); // returns size
   void Init(ObjID obj, void* value);   // no-op
   void New(ObjID obj, void* value, void *sug = NULL); // copy sug or zero out
   void Term(ObjID obj, void* value);  // no-op
   void Copy(ObjID obj, void* targ, void* src);  // memcpy
   void Read(ObjID obj, void* value, uint ver, PropReadFunc read, PropFile* file); // read
   void Write(ObjID obj, void* value, PropWriteFunc write, PropFile* file); // write
};

////////////////////////////////////////////////////////////
// TEMPLATE VALUE FUNCS
//
// For when your property happens to be a class
//
// You probably want to overload read and write.
//

template <class VALUE> class cPropClassValueFuncs : public cPropValueFuncs
{
public:
   void Init(ObjID , void* ) { }; 
   void New(ObjID obj, void* value, void* sug) 
   {
      if (sug == NULL)
         *(VALUE*)value = VALUE(); // use default constructor
      else
         Copy(obj,value,sug);
   }
   void Term(ObjID , void* value) { ((VALUE*)value)->~VALUE(); } // call destructor
   void Copy(ObjID , void* t, void* s) { *(VALUE*)t = *(VALUE*)s;}; 
   void Read(ObjID , void* value, uint , PropReadFunc read, PropFile* file)
   { read(file,value,sizeof(VALUE));};
   void Write(ObjID , void* value, PropWriteFunc write, PropFile* file)
   { write(file,value,sizeof(VALUE));};
};


#endif // PROPVAL__H
