/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/propman_.h,v 1.18 2000/01/29 13:24:39 adurant Exp $
#pragma once

#ifndef PROPMAN__H
#define PROPMAN__H


#include <objtype.h>
#include <lgassert.h>
#include <dynarray.h>
#include <hashset.h>
#include <str.h>
#include <aggmemb.h>

#include <dbasemsg.h>

#include <appagg.h>
#include <propman.h>

class cNullProperty;

typedef cDynArray<IProperty*> PropArray;
typedef cDynArray<PropertyID> PropIDArray;


////////////////////////////////////////////////////////////
//
// cPropertyManager
//
// The cPropertyManager is the one place that knows about all the
// properties.  Only it has the ability to do things like iterate over
// all properties, etc.
//
////////////////////////////////////////////////////////////


class cPropertyManager : public IPropertyManager
{
   // Hash table mapping strings to Properties 
   class cPropertyTable : public cStrIHashSet <IProperty *>
   {
   public:
      cPropertyTable () {};
      tHashSetKey GetKey (tHashSetNode node) const
      {
         return (tHashSetKey) (((IProperty *) (node))->Describe()->name);
      };
   };


   // We're going to be an aggregate member when we grow up
   DECLARE_AGGREGATION(cPropertyManager);

public:
   cPropertyManager (IUnknown* outer);  // constructor
   virtual ~cPropertyManager (); // destructor

   // Find the Property with a given name, can return NULL
   STDMETHOD_(IProperty *,GetProperty) (PropertyID id) const;
   STDMETHOD_(IProperty *,GetPropertyNamed) (const char* name) const; 


   // Notify the property system of an event
   STDMETHOD(Notify)(ePropertyNotifyMsg msg, PropNotifyData data);

   // Iteration functions
   STDMETHOD(BeginIter)(struct sPropertyIter* iter);
   STDMETHOD_(IProperty*,NextIter)(struct sPropertyIter* iter); 

   //------------------------------------------------------------
   // Property system internal methods 
   //

public:
   // Add a Property to the list being managed, return success
   // Sets id to be unique
   STDMETHOD_(BOOL,AddProperty)(IProperty *prop, PropertyID *id);

   // Remove a Property from the list being managed, return success
   STDMETHOD_(BOOL,DelProperty)(IProperty *prop);

   // Lookups that don't bump refcount
   IProperty* ByID (PropertyID id) const;
   IProperty* ByName (const char* name) const; 

private:
   // Called when a property message is recieved.  Will be sent to appropriate prop for handling.
   static void NetworkMessageCallback(const sNetMsg_Generic *pMsg, ulong size, ObjID from, void *pClientData);

   // Code shared by AddProperty and AddMetaProperty
   BOOL AddPropertyBase (IProperty * prop);

   //----------------------------------------
   // Aggregate protocol
   //

   HRESULT Init();
   HRESULT End(); 

   //
   // Data members
   //

   cPropertyTable name_hash_;    // mapping of names to Properties
   PropArray props_;  // mapping of IDs to properties
   PropIDArray sort_order_; // constrained sort order. 
   PropIDArray begin_create_; // props that need begin create
   PropIDArray end_create_; // props that need end create
   PropIDArray load_obj_; // props that need load obj

   cNullProperty* nullprop_;
   BOOL props_sorted_; // is sort_order valid?
};


#endif // PROPMAN__H




