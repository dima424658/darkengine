/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/linkprop.h,v 1.3 2000/01/29 13:23:29 adurant Exp $
#pragma once

#ifndef __LINKPROP_H
#define __LINKPROP_H
#include <propbase.h>
#include <linksto_.h>

////////////////////////////////////////////////////////////
// PROPERTY LINK STORE
//
// This is a link store that presents a property whose values are 
// ObjIDs as a set of links.
//

F_DECLARE_INTERFACE(IProperty);
F_DECLARE_INTERFACE(IIntProperty);

#define PROPLINK_PART(src) (OBJ_IS_ABSTRACT(src) ? kLinkGameSys : kLinkMission)
#define PROPLINKID_MAKE(rel,src) LINKID_MAKE(rel,PROPLINK_PART(src),src)

class cPropertyLinkStore : public cBaseLinkStore 
{
   IIntProperty* Prop;
   RelationID Rel;

public: 
   cPropertyLinkStore(IUnknown* );
   ~cPropertyLinkStore();
   
   STDMETHOD(QueryInterface)(REFIID id, void** ppI);
   STDMETHOD(AddRelation)(RelationID id);
   STDMETHOD_(LinkID, Add)(sLink* link);
   STDMETHOD(AddAtID)(LinkID id, sLink* link);
   STDMETHOD(Remove)(LinkID id);
   STDMETHOD_(BOOL,Get)(LinkID id, sLink* out);
   STDMETHOD_(ILinkQuery*,GetAll)(RelationID ID);
   STDMETHOD(Save)(RelationID id, IUnknown* file); 
   STDMETHOD(Load)(RelationID id, IUnknown* file); 
   STDMETHOD(Reset)(RelationID id);

};

#endif // __LINKPROP_H






