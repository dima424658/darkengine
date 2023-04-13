/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/propscpt.h,v 1.2 1999/03/03 13:27:16 Justin Exp $
#pragma once  
#ifndef __PROPSCPT_H
#define __PROPSCPT_H

#include <objscrpt.h>

///////////////////////////////////////
//
// Property system service 
//

DECLARE_SCRIPT_SERVICE(Property, 0xda)
{
   STDMETHOD_(cMultiParm, Get)(object obj, const char * prop, const char * field = NULL) PURE;
   // Set() will ask the owner of the object to set the property. SetLocal()
   // will set the property locally, even on a proxy object.
   STDMETHOD(Set)(object obj, const char * prop, const cMultiParm&) PURE;
   STDMETHOD(Set)(object obj, const char * prop, const char * field, const cMultiParm&) PURE;
   STDMETHOD(SetLocal)(object obj, const char * prop, const char * field, const cMultiParm&) PURE;

   STDMETHOD(Add)(object obj, const char* prop) PURE;
   STDMETHOD(Remove)(object obj, const char* prop) PURE;
   STDMETHOD(CopyFrom)(object targ, const char* prop, object src) PURE;
   STDMETHOD_(BOOL,Possessed)(object obj, const char* prop) PURE;
};

#endif // __PROPSCPT_H
