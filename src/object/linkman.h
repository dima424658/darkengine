/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/linkman.h,v 1.4 2000/01/29 13:23:26 adurant Exp $
#pragma once

#ifndef __LINKMAN_H
#define __LINKMAN_H
#include <comtools.h>
#include <linktype.h>

#undef INTERFACE 
#define INTERFACE ILinkManager

DECLARE_INTERFACE_(ILinkManager, IUnknown)
{
   // IUnknown methods
   DECLARE_UNKNOWN_PURE();

   //
   // Look up a relation
   // 
   STDMETHOD_(IRelation*,GetRelation)(THIS_ RelationID id) PURE;
   STDMETHOD_(IRelation*,GetRelationNamed)(THIS_ const char* name) PURE;

#define ILinkManager_GetRelation(p, a)         COMCall1(p, GetRelation, a)   
#define ILinkManager_GetRelationNamed(p, a)    COMCall1(p, GetRelationNamed, a)   

   //
   // Notify the link manager (and thereby all relations) of a system event
   //
   STDMETHOD(Notify)(THIS_ eRelationNotifyMsg msg, RelationNotifyData data) PURE;

#define ILinkManager_Notify(p, a, b)   COMCall2(p, Notify, a, b)

   //
   // Iterate across all relations
   //
   STDMETHOD(IterStart)(THIS_ sRelationIter* iter) PURE ;
   STDMETHOD_(BOOL,IterNext)(THIS_ sRelationIter* iter, RelationID* rel) PURE; 
   STDMETHOD(IterStop)(THIS_ sRelationIter* iter) PURE;

#define ILinkManager_IterStart(p, a)      COMCall1(p, IterStart, a)   
#define ILinkManager_IterNext(p, a, b)    COMCall2(p, IterNext, a, b)
#define ILinkManager_IterStop(p, a)       COMCall1(p, IterStop, a)   

   //
   // Add/Remove/Access Links
   // 
   STDMETHOD_(LinkID, Add)(THIS_ ObjID source, ObjID dest, RelationID id) PURE;
   STDMETHOD_(LinkID, AddFull)(THIS_ ObjID source, ObjID desc, RelationID id, void* data) PURE;
   STDMETHOD(Remove)(THIS_ LinkID id) PURE; 
   STDMETHOD_(BOOL,Get)(THIS_ LinkID id, sLink* out) CONSTFUNC PURE; 
   STDMETHOD(SetData)(THIS_ LinkID id, void* data) PURE;
   STDMETHOD_(void*,GetData)(THIS_ LinkID id) PURE;

#define ILinkManager_Add(p, a, b, c)            COMCall3(p, Add, a, b, c)
#define ILinkManager_AddFull(p, a, b, c, d)     COMCall4(p, AddFull, a, b, c, d)
#define ILinkManager_Remove(p, a)               COMCall1(p, Remove, a)   
#define ILinkManager_Get(p, a, b)               COMCall2(p, Get, a, b)
#define ILinkManager_SetData(p, a, b)           COMCall2(p, SetData, a, b)
#define ILinkManager_GetData(p, a, b)           COMCall2(p, GetData, a, b)

   // 
   // Query links, objects can be OBJID_WILDCARD, relation can be RELID_WILDCARD
   //
   STDMETHOD_(ILinkQuery*,Query)(THIS_ ObjID source, ObjID dest, RelationID flavor) PURE;

   // 
   // Remap a link id after it has been loaded, in order to account for differences in the relation IDs
   // Call this from any code that loads link IDs from disk
   //
   STDMETHOD_(LinkID,RemapOnLoad)(THIS_ LinkID id) PURE; 

#define ILinkManager_Query(p, a, b, c)    COMCall3(p, Query, a, b, c)
   
   //
   // Look for a singleton link
   //
   STDMETHOD_(LinkID, GetSingleLink)(THIS_ RelationID id, ObjID source, ObjID dest) PURE;

   //
   // Check for any links at all
   //
   STDMETHOD_(BOOL, AnyLinks)(THIS_ RelationID id, ObjID source, ObjID dest) PURE;
};

EXTERN tResult LGAPI LinkManagerCreate(void);


#endif // __LINKMAN_H

