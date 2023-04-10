/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/linkint.h,v 1.6 2000/01/29 13:23:20 adurant Exp $
#pragma once

#ifndef __LINKINT_H
#define __LINKINT_H
#include <linkman.h>

////////////////////////////////////////////////////////////
// LINK MANAGER INTERNAL INTERFACE
// 
// This is an auxiliary interface to the link manager for use 
// exclusively by Relations and Queries. 
//

#undef INTERFACE 
#define INTERFACE ILinkManagerInternal

DECLARE_INTERFACE_(ILinkManagerInternal, ILinkManager)
{
   // IUnknown methods
   DECLARE_UNKNOWN_PURE();

   //
   // EXTERNAL METHODS
   //

   STDMETHOD_(IRelation*,GetRelation)(THIS_ RelationID id) PURE;
   STDMETHOD_(IRelation*,GetRelationNamed)(THIS_ const char* name) PURE;
   STDMETHOD(Notify)(THIS_ eRelationNotifyMsg msg, RelationNotifyData data) PURE;
   STDMETHOD(IterStart)(THIS_ sRelationIter* iter) PURE ;
   STDMETHOD_(BOOL,IterNext)(THIS_ sRelationIter* iter, RelationID* rel) PURE; 
   STDMETHOD(IterStop)(THIS_ sRelationIter* iter) PURE;
   STDMETHOD_(LinkID, Add)(THIS_ ObjID source, ObjID dest, RelationID id) PURE;
   STDMETHOD_(LinkID, AddFull)(THIS_ ObjID source, ObjID desc, RelationID id, void* data) PURE;
   STDMETHOD(Remove)(THIS_ LinkID id) PURE; 
   STDMETHOD_(BOOL,Get)(THIS_ LinkID id, sLink* out) CONSTFUNC PURE; 
   STDMETHOD(SetData)(THIS_ LinkID id, void* data) PURE;
   STDMETHOD_(void*,GetData)(THIS_ LinkID id) PURE;
   STDMETHOD_(ILinkQuery*,Query)(THIS_ ObjID source, ObjID dest, RelationID flavor) PURE;

   //------------------------------------------------------------
   // Methods for use by queries/relations
   // Ordinary citizens should not use these. 
   //

   //
   // Add a relation.  Relation implementations should call this in their 
   // factory functions or constructors.  
   //
   STDMETHOD_(RelationID,AddRelation)(THIS_ IRelation* relation) PURE;

   // Remove a relation
   STDMETHOD(RemoveRelation)(THIS_ RelationID relid) PURE;

   // Replace an existing relation
   STDMETHOD(ReplaceRelation)(THIS_ RelationID relid, IRelation* relation) PURE;

   // 
   // Add/Remove A query database for a set of query cases.  
   // Used only for wildcard-relation queries
   // 
   STDMETHOD(AddQueryDB)(THIS_ ILinkQueryDatabase* db, RelationQueryCaseSet cases) PURE;
   STDMETHOD(RemoveQueryDB)(THIS_ ILinkQueryDatabase* db, RelationQueryCaseSet cases) PURE;

   //
   // Lock the database during a query.  Prevents deletions until you unlock
   //
   STDMETHOD_(tQueryDate,Lock)(THIS) PURE;
   STDMETHOD(Unlock)(THIS_ tQueryDate lockdate) PURE;
   STDMETHOD_(long,GetLockCount)(THIS) PURE;


   // 
   // Was this link existent at the time a query was started?
   // 
   STDMETHOD_(BOOL,LinkValid)(THIS_ LinkID link, tQueryDate age) PURE;

   //
   // Notify the link manager that a link has been created
   // 
   STDMETHOD(LinkBirth)(THIS_ LinkID id) PURE; 
   
   //
   // Notify the link manager of a link's death.  If TRUE is returned, the
   // link should not be deleted. 
   //
   STDMETHOD_(BOOL,LinkDeath)(THIS_ LinkID id) PURE;
   

   

};
#undef INTERFACE




#endif // __LINKINT_H
