/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/objscrpt.h,v 1.15 2000/01/17 19:43:54 adurant Exp $
#pragma once  
#ifndef __OBJSCRPT_H
#define __OBJSCRPT_H

#include <mprintf.h>
#include <objscrt.h>

///////////////////////////////////////
//
// Object services
//

DECLARE_SCRIPT_SERVICE(Object, 0xdf)
{
   // Start creating an object.  If you are going to put initial properties
   // on the object, you should BeginCreate it, then put the properties on, 
   // then EndCreate it. 
   STDMETHOD_(object,BeginCreate)(object archetype_or_clone) PURE;   

   // Finish creating the object
   STDMETHOD(EndCreate)(object obj) PURE;

   // Create a simple object.  Just like BeginCreate followed by EndCreate
   STDMETHOD_(object,Create)(object archetype_or_clone) PURE; 

   // Destroy an object 
   STDMETHOD(Destroy)(object obj) PURE; 

   // Check to see if an object exists
   STDMETHOD_(boolean,Exists)(object obj) PURE; 

   // Object naming
   STDMETHOD(SetName)(object obj, const char* name) PURE; 
   STDMETHOD_(string,GetName)(object obj) PURE;
   STDMETHOD_(object,Named)(const char* name) PURE; 

   // Metaproperties & inheritance
   STDMETHOD(AddMetaProperty)(object obj, object metaprop) PURE; 
   STDMETHOD(RemoveMetaProperty)(object obj, object metaprop) PURE; 
   STDMETHOD_(boolean,HasMetaProperty)(object obj, object metaprop) PURE; 

   STDMETHOD_(boolean,InheritsFrom)(object obj, object archetype_or_metaprop) PURE; 

   // Transience, i.e. whether the object is saved to disk
   STDMETHOD_(boolean,IsTransient)(object obj) PURE; 
   STDMETHOD(SetTransience)(object obj, boolean is_transient) PURE; 

   // Object Position
   STDMETHOD_(vector,Position)(object obj) PURE; 
   STDMETHOD_(vector,Facing)(object obj) PURE; 
   STDMETHOD(Teleport)(object obj, const vector ref position, const vector ref facing, object ref_frame = 0) PURE;
   STDMETHOD_(boolean,IsPositionValid)(object obj) PURE;

   // Object Searching
   STDMETHOD_(object,FindClosestObjectNamed)(ObjID objId, const char* name) PURE; 

   // Add/remove metaproperty to multiple targets
   STDMETHOD_(integer, AddMetaPropertyToMany)(object metaprop, const string ref ToSet) PURE;
   STDMETHOD_(integer, RemoveMetaPropertyFromMany)(object metaprop, const string ref ToSet) PURE;

   // useful to know for an object
   STDMETHOD_(boolean, RenderedThisFrame)(object scr_obj) PURE;
};

#ifdef SCRIPT

inline object::object(const char* name)
{
   id = Object.Named(name);
#ifdef DGB_ON   
   if (id == 0)
      Debug.MPrint("Hey!  There's no object called '",name,"'"); 
#endif 
}

#endif 
EXTERN ObjID FindTheClosestObjectNamed(ObjID objId, const char* name);

#endif // __OBJSCRPT_H

