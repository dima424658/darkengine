/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/propnull.cpp,v 1.8 1998/10/08 16:35:34 MROWLEY Exp $

#include <propnull.h>
#include <propbase.h>

#include <config.h>
#include <cfgdbg.h>

// Must be last header 
#include <dbmem.h>

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cNullProperty,IProperty);

static sPropertyDesc NullDesc = 
{
   "NULL",
};


STDMETHODIMP_(const sPropertyDesc*) cNullProperty::Describe() const
{
   return &NullDesc;
}


STDMETHODIMP_(const sPropertyTypeDesc*) cNullProperty::DescribeType() const
{
   static sPropertyTypeDesc desc = { "None", 0};
   return &desc;
}

////////////////////////////////////////

STDMETHODIMP_(PropertyID) cNullProperty::GetID() const 
{
   return PROPID_NULL;
}

STDMETHODIMP cNullProperty::Create(ObjID) 
{
   ConfigSpew("null_prop",("cNullProperty::Create() was called\n"));
   return E_FAIL;
} 

STDMETHODIMP cNullProperty::Copy(ObjID,ObjID) 
{
   ConfigSpew("null_prop",("cNullProperty::CreateByExample() was called\n"));
   return E_FAIL;
} 

STDMETHODIMP cNullProperty::Delete(ObjID) 
{
   ConfigSpew("null_prop",("cNullProperty::Delete() was called\n"));
   return E_FAIL;
} 


STDMETHODIMP_(void) cNullProperty::Notify(ePropertyNotifyMsg , PropNotifyData )
{
   ConfigSpew("null_prop",("cNullProperty::Notify() was called\n"));
} 

STDMETHODIMP_(BOOL) cNullProperty::IsRelevant(ObjID) const 
{ 
   ConfigSpew("null_prop",("cNullProperty::IsRelevant() was called\n"));
   return FALSE; 
}  

STDMETHODIMP_(BOOL) cNullProperty::IsSimplyRelevant(ObjID) const 
{ 
   ConfigSpew("null_prop",("cNullProperty::IsSimplyRelevant() was called\n"));
   return FALSE; 
}  

STDMETHODIMP_(BOOL) cNullProperty::Touch(ObjID) 
{ 
   ConfigSpew("null_prop",("cNullProperty::Touch() was called\n"));
   return FALSE; 
}  


STDMETHODIMP_(PropListenerHandle) cNullProperty::Listen(PropertyListenMsgSet, PropertyListenFunc, PropListenerData ) 
{
   ConfigSpew("null_prop",("cNullProperty::Listen() was called\n"));
   return 0; 
} 

STDMETHODIMP_(void) cNullProperty::IterStart(sPropertyObjIter* ) const 
{
   ConfigSpew("null_prop",("cNullProperty::IterStart() was called\n"));
}

STDMETHODIMP_(BOOL) cNullProperty::IterNext(sPropertyObjIter* , ObjID* ) const 
{ 
   ConfigSpew("null_prop",("cNullProperty::IterNext() was called\n"));
   return FALSE; 
} 

STDMETHODIMP_(void) cNullProperty::IterStop(sPropertyObjIter* ) const 
{
   ConfigSpew("null_prop",("cNullProperty::IterStop() was called\n"));
}


