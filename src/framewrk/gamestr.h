/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/gamestr.h,v 1.3 1998/09/07 20:47:08 mahk Exp $
#pragma once  
#ifndef __GAMESTR_H
#define __GAMESTR_H
#include <str.h>
#include <comtools.h>
#include <objtype.h>

////////////////////////////////////////////////////////////
// SIMPLE "GAME STRING" API 
//
// Basically, a special string fetcher that allows strings to 
// have "default" values for rapid prototyping by designers 
//
// This is a C++-only com interface 
//
// All the "tables" used here are string names of .str files found in the 
// "strings" subdir of resname_base. 
//

F_DECLARE_INTERFACE(IStringProperty); 
F_DECLARE_INTERFACE(IGameStrings); 

#undef INTERFACE
#define INTERFACE IGameStrings
DECLARE_INTERFACE_(IGameStrings, IUnknown)
{
   DECLARE_UNKNOWN_PURE(); 

   //
   // Fetch a game string from a string res, given a default 
   //

   STDMETHOD_(cStr,FetchString)(const char* table, const char* name, const char* default_val = "", const char* rel_path = "strings") PURE; 

   //
   // Register a "game string" property. 
   // Such a property has string values of the form 
   // <name>
   // or 
   // <name>: "<text>"  
   // <name> is the name used to look up in the string table. 
   // <text> is the default value if none is found.
   // There can only be one property per table, and vice versa
   //
   STDMETHOD(RegisterProp)(const char* table, IStringProperty* prop) PURE; 

   // 
   // Lookup a string in a game string prop, specified by table name 
   // 
   STDMETHOD_(cStr,FetchObjString)(ObjID obj, const char* table) PURE; 

   //
   // Write out the specified string table in the current directory, 
   // based on the values of the property.  
   //
   // If the table is NULL, all registered tables are dumped.
   //
   STDMETHOD(DumpProp)(const char* table) PURE; 

   //
   // Given a game string property, mutate the <text> of every value 
   // so that it matches the resource.  
   //
   // Similarly, NULL means all registered tables
   // 
   STDMETHOD(LoadProp)(const char* table) PURE; 

   //
   // Mutates the <text> part of each value of the property to be empty.
   //
   // NULL means all registerd properties.  
   //
   STDMETHOD(StripProp)(const char* table) PURE; 

   // Assumes that the value of a property is current in the "old" format
   // of just plain <text>, and transforms it into the happier format of
   // <name>:<text> using the default name passed it, and appending the
   // object ID.
   STDMETHOD(ModernizeProp)(const char *table) PURE;

};

#undef INTERFACE

EXTERN void GameStringsCreate(); 





#endif // __GAMESTR_H





