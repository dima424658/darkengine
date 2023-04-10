/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/linkstor.h,v 1.6 2000/01/29 13:23:34 adurant Exp $
#pragma once

#ifndef __LINKSTOR_H
#define __LINKSTOR_H
#include <comtools.h>
#include <linktype.h>

typedef ulong eLinkStorageMethod;

////////////////////////////////////////////////////////////
// LINK STORE INTERFACE
//
// A link store stores the endpoints and relation id of a link in
// any way it chooses.  Does NOT store satellite data.
//

#undef INTERFACE
#define INTERFACE ILinkStore
DECLARE_INTERFACE_(ILinkStore, IUnknown)
{
   DECLARE_UNKNOWN_PURE();

   //
   // Add a new relation to the set of relations we store
   //
   STDMETHOD(AddRelation)(THIS_ RelationID id) PURE;
   STDMETHOD(RemoveRelation)(THIS_ RelationID id) PURE;


   //
   // Add a link to storage
   //
   STDMETHOD_(LinkID, Add)(THIS_ sLink* link) PURE;

   //
   // Add a link, specifying it's id. 
   // 
   STDMETHOD(AddAtID)(THIS_ LinkID id, sLink* link) PURE;

 
   //
   // Remove a link from storage
   //
   STDMETHOD(Remove)(THIS_ LinkID id) PURE;

   //
   // Fetch a link from storage.  LEAVES THE DATA FIELD UNCHANGED
   //
   STDMETHOD_(BOOL,Get)(THIS_ LinkID id, sLink* out) PURE;

   //
   // Get all the links of a particular relation from storage, 
   // in the form of a query
   //
   STDMETHOD_(ILinkQuery*,GetAll)(THIS_ RelationID ID) PURE;

   //
   // Save/Load/Reset the database.
   // Currently, ITagFile is the only supported file interface
   //
   STDMETHOD(Save)(THIS_  RelationID id, IUnknown* file, int filetype) PURE; 
   STDMETHOD(Load)(THIS_ RelationID id, IUnknown* file, int filetype) PURE; 
   STDMETHOD(Reset)(THIS_ RelationID id) PURE;

};

//
// Factory function
//
EXTERN ILinkStore* CreateStandardLinkStore(eLinkStorageMethod method);



////////////////////////////////////////////////////////////
// LINK DATA STORE
// 
// Link link store, but stores that pesky satellite data
//

#undef INTERFACE
#define INTERFACE ILinkDataStore
DECLARE_INTERFACE_(ILinkDataStore, IUnknown)
{
   DECLARE_UNKNOWN_PURE();

   //
   // Add a new relation to the set of relations we store
   //
   STDMETHOD(AddRelation)(THIS_ RelationID id) PURE;
   STDMETHOD(RemoveRelation)(THIS_ RelationID id) PURE;

   //
   // Add a link, generate default data
   //
   STDMETHOD(Add)(THIS_ LinkID id) PURE;

   //
   // Remove a link
   //
   STDMETHOD(Remove)(THIS_ LinkID id) PURE;

   //
   // Set Data
   //
   STDMETHOD(Set)(THIS_ LinkID id, void* data) PURE;

   //
   // Get Data
   //
   STDMETHOD_(void*,Get)(THIS_ LinkID id) PURE;

   //
   // Size of the data
   //
   STDMETHOD_(ulong,DataSize)(THIS) PURE;

   //
   // Save/Load/Reset the database.
   // Currently, ITagFile is the only supported file interface
   //
   STDMETHOD(Save)(THIS_  RelationID id, IUnknown* file, int filetype) PURE; 
   STDMETHOD(Load)(THIS_ RelationID id, IUnknown* file, int filetype) PURE; 
   STDMETHOD(Reset)(THIS_ RelationID id) PURE;

};

//
// Factory function
//

// Data store manages the memory of the data, copies on set.
EXTERN ILinkDataStore* CreateManagedLinkDataStore(ulong data_size);


#endif // __LINKSTOR_H




