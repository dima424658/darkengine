/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/relation.h,v 1.6 2000/01/29 13:24:50 adurant Exp $
#pragma once
#ifndef __RELATION_H
#define __RELATION_H

#include <comtools.h>
#include <linktype.h>
#include <objtype.h>



////////////////////////////////////////////////////////////
// RELATION INTERFACE
// 
// A Relation represents a relationship between objects.  
// It is a multiset of ordered triples (Obj1,Obj2,Data) where the meaning of the
// data is determined by the relation.  Each ordered triple is called a _link_, and 
// has a unique integer ID.  Link ID's are unique across all relations.
//

#undef INTERFACE
#define INTERFACE IRelation

DECLARE_INTERFACE_(IRelation,IUnknown)
{
   //
   // IUnknown Methos
   //
   DECLARE_UNKNOWN_PURE(); 

#define IRelation_QueryInterface(p, a, b) COMQueryInterface(p, a, b)
#define IRelation_AddRef(p)               COMAddRef(p)
#define IRelation_Release(p)              COMRelease(p)

   //
   // Return a description of this relation
   //
   STDMETHOD_(const sRelationDesc*, Describe)(THIS) CONSTFUNC PURE;

#define IRelation_Describe(p)       COMCall0(p, Describe)

   //
   // Get my unique relation ID
   // 
   STDMETHOD_(RelationID,GetID)(THIS) CONSTFUNC PURE; 

#define IRelation_GetID(p)       COMCall0(p, GetID)


   //
   // Set my unique relation ID.
   // DO NOT USE THIS UNLESS YOU ARE THE LINK MANAGER. 
   // 
   STDMETHOD(SetID)(THIS_ RelationID id) PURE; 

   //
   // Get a descripton of my data
   //
   STDMETHOD_(const sRelationDataDesc*, DescribeData)(THIS) CONSTFUNC PURE;

#define IRelation_DescribeData(p)       COMCall0(p, DescribeData)

   //
   // Return the "inverse relation" which has an isomorphic set of links pointing in the
   // opposite direction.
   //
   STDMETHOD_(IRelation*, Inverse)(THIS) PURE;

#define IRelation_Inverse(p)        COMCall0(p, Inverse)

   //
   // Add a new link
   // 
   STDMETHOD_(LinkID, Add)(THIS_ ObjID source, ObjID dest) PURE;
   STDMETHOD_(LinkID, AddFull)(THIS_ ObjID source, ObjID desc, void* data) PURE;

#define IRelation_Add(p, a, b)      COMCall2(p, Add, a, b)
#define IRelation_AddFull(p, a, b, c)   COMCall3(p, AddFull, a, b, c)

   // 
   // Remove a link by ID
   //
   STDMETHOD(Remove)(THIS_ LinkID id) PURE; 

#define IRelation_Remove(p, a)      COMCall1(p, Remove, a)

   // 
   // Fill in the fields of link structure with the value of a link.
   // 
   STDMETHOD_(BOOL,Get)(THIS_ LinkID id, sLink* out) CONSTFUNC PURE; 

#define IRelation_Get(p, a, b)      COMCall2(p, Get, a, b)

   //
   // Set/Get the data for a link  
   //
   STDMETHOD(SetData)(THIS_ LinkID id, void* data) PURE;
   STDMETHOD_(void*,GetData)(THIS_ LinkID id) PURE;

#define IRelation_SetData(p, a, b)  COMCall2(p, SetData, a, b)
#define IRelation_GetData(p, a, b)  COMCall2(p, GetData, a, b)

   //
   // Run a query over some set of links.  Args can be LINKOBJ_WILDCARD
   // 
   STDMETHOD_(ILinkQuery*,Query)(THIS_ ObjID source, ObjID dest) CONSTFUNC PURE;

#define IRelation_Query(p, a, b)    COMCall2(p, Query, a, b)

   //
   // Notify a relation about system events. Primarily for use by relation manager
   //
   STDMETHOD(Notify)(THIS_ eRelationNotifyMsg msg, RelationNotifyData data) PURE;

#define IRelation_Notify(p, a, b)   COMCall2(p, Notify, a, b)

   //
   // Listen to changes in the relation 
   //
   STDMETHOD(Listen)(THIS_ RelationListenMsgSet interests, RelationListenFunc func, RelationListenerData data) PURE;

#define IRelation_Listen(p, a, b, c)   COMCall3(p, Listen, a, b, c)

   //
   // Look for a singleton link
   //
   STDMETHOD_(LinkID, GetSingleLink)(THIS_ ObjID source, ObjID dest) PURE;

   //
   // Check for any links at all
   //
   STDMETHOD_(BOOL, AnyLinks)(THIS_ ObjID source, ObjID dest) PURE;
};

//------------------------------------------------------------
// FIND A RELATION BY NAME OR ID
//

EXTERN IRelation* GetRelation(RelationID id);
EXTERN IRelation* GetRelationNamed(const char* name);

//------------------------------------------------------------
// Create A Standard Relation
//

EXTERN IRelation* CreateStandardRelation(const sRelationDesc* desc, const sRelationDataDesc* datadesc, RelationQueryCaseSet optimize); 


//------------------------------------------------------------
// Create a relation from a link store. 
//

EXTERN IRelation* CreateRelationFromLinkStore(const sRelationDesc* desc, const sRelationDataDesc* datadesc, ILinkStore* store, RelationQueryCaseSet optimize);

//------------------------------------------------------------
// Create a customized relation, specifying all components. 
//

typedef struct sCustomRelationDesc sCustomRelationDesc; 

EXTERN IRelation* CreateCustomRelation(const sRelationDesc* desc, const sRelationDataDesc* datadesc, const sCustomRelationDesc* cdesc);

#endif // __RELATION_H







