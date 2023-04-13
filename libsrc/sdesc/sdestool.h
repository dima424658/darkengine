/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: x:/prj/tech/libsrc/sdesc/RCS/sdestool.h 1.5 1998/01/02 14:43:33 mahk Exp $
#pragma once  
#ifndef __SDESTOOL_H
#define __SDESTOOL_H

#include <comtools.h>
#include <sdestype.h>

////////////////////////////////////////////////////////////
// Struct Desc Tools
//
// For use by editor DLLs
//
F_DECLARE_INTERFACE(IStructDescTools); 

#undef INTERFACE
#define INTERFACE IStructDescTools
DECLARE_INTERFACE_(IStructDescTools,IUnknown)
{
   DECLARE_UNKNOWN_PURE();

   //
   // Lookup a field by name
   //
   STDMETHOD_(const sFieldDesc*, GetFieldNamed)(THIS_ const sStructDesc* desc, const char* field) PURE; 

   //
   // Parse and unparse a field
   //
   STDMETHOD(ParseField)(THIS_ const sFieldDesc* fdesc, const char* string, void* struc) PURE;
   STDMETHOD(UnparseField)(THIS_ const sFieldDesc* fdesc, const void* struc, char* string, int buflen) PURE; 

   // 
   // Parse & Unparse a "simple" structure (e.g. a one-field structure)
   //
   STDMETHOD_(BOOL,IsSimple)(THIS_ const sStructDesc* desc) PURE;
   STDMETHOD_(BOOL,ParseSimple)(THIS_ const sStructDesc* desc, const char* string, void *struc) PURE;
   STDMETHOD_(BOOL,UnparseSimple)(THIS_ const sStructDesc* desc, const void* struc, char* buf, int len) PURE;
   

   //
   // Parse & unparse the "full" representation of a structure
   //

   STDMETHOD_(BOOL,ParseFull)(THIS_ const sStructDesc* sdesc, const char* string, void* struc) PURE; 
   STDMETHOD_(BOOL,UnparseFull)(THIS_ const sStructDesc* sdesc, const void* struc, char* out, int buflen) PURE; 

   //
   // Dump a struct to the mono 
   //
   STDMETHOD(Dump)(THIS_ const sStructDesc* sdesc, const void* struc) PURE; 

   //
   // Set and Get an integral field
   // 
   STDMETHOD(SetIntegral)(THIS_ const sFieldDesc* fdesc, long value, void* struc) PURE;
   STDMETHOD(GetIntegral)(THIS_ const sFieldDesc* fdesc, const void* struc, long* value) PURE;

   //
   // Lookup an sdesc in the registry.  NULL if none exists
   //
   STDMETHOD_(const sStructDesc*, Lookup)(THIS_ const char* name) PURE; 
   STDMETHOD(Register)(THIS_ const sStructDesc* desc) PURE; 
   STDMETHOD(ClearRegistry)(THIS) PURE; 


   
};


EXTERN void SdescToolsCreate(void);

#endif // __SDESTOOL_H
