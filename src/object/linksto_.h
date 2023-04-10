/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/linksto_.h,v 1.5 2000/01/29 13:23:35 adurant Exp $
#pragma once

#ifndef __LINKSTO__H
#define __LINKSTO__H
#include <linkstor.h>
#include <linkknow.h>

////////////////////////////////////////////////////////////
// BASE LINK STORE/DATA STORE CLASSES
//
//

class cBaseLinkStore : public ILinkStore, protected cLinkManagerKnower
{
public:
   DECLARE_UNAGGREGATABLE();

   // Dumb default implementations that just run queries and do the right thing. 

   STDMETHOD(Save)(RelationID id, IUnknown* file, int filetype); 
   STDMETHOD(Load)(RelationID id, IUnknown* file, int filetype);    
   STDMETHOD(Reset)(RelationID id);

   STDMETHOD(RemoveRelation)(RelationID ) { return S_OK; }; 

   
};


class cBaseLinkDataStore : public ILinkDataStore, protected cLinkManagerKnower
{
public:
   DECLARE_UNAGGREGATABLE();

   // Dumb default implementations that just run queries and do the right thing. 

   STDMETHOD(AddRelation)(RelationID id);
   STDMETHOD(RemoveRelation)(RelationID ) { return S_OK; }; 
   STDMETHOD(Add)(LinkID id);
   STDMETHOD(Remove)(LinkID id);
   STDMETHOD(Save)(RelationID id, IUnknown* file, int filetype); 
   STDMETHOD(Load)(RelationID id, IUnknown* file, int filetype);    
   STDMETHOD(Reset)(RelationID id);
   
};

#endif // __LINKSTO__H



