/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/questapi.h,v 1.7 2000/01/29 13:20:00 adurant Exp $
#pragma once

//------------------------------------
// The Quest Data system
// A simple mapping from names to values for mission and campaign
// specific data.

#ifndef __QUESTAPI_H
#define __QUESTAPI_H

#include <comtools.h>
#include <objtype.h>

// Is this mission specific, or over the whole campaign?
// Don't use kQuestDataUnknown
typedef int eQuestDataType;
enum eQuestDataType_ {kQuestDataMission, kQuestDataCampaign, kQuestDataUnknown};

typedef long (*QuestMoveFunc) (void *buf, size_t elsize, size_t nelem);

typedef BOOL (*QuestFilterFunc) (const char *pName, int oldValue, int newValue);

F_DECLARE_INTERFACE(IQuestData); 
F_DECLARE_INTERFACE(IQuestDataIter); 

#undef INTERFACE
#define INTERFACE IQuestData

DECLARE_INTERFACE_(IQuestData,IUnknown)
{
   DECLARE_UNKNOWN_PURE(); 

   // Create a quest data item
   STDMETHOD(Create)(THIS_ const char *pName, int value, eQuestDataType type) PURE; 

   // Set existing data item
   STDMETHOD(Set)(THIS_ const char *pName, int value) PURE; 

   // Get data, returns zero if undefined
   STDMETHOD_(int,Get)(THIS_ const char *pName) PURE;

   // Query for existence
   STDMETHOD_(BOOL, Exists)(THIS_ const char *pName) PURE;

   // Delete
   STDMETHOD(Delete)(THIS_ const char *pName) PURE;

   // Get an iterator
   STDMETHOD_(IQuestDataIter*, Iter)(eQuestDataType type) PURE;

   // Delete all
   STDMETHOD(DeleteAll)(THIS) PURE;

   STDMETHOD(DeleteAllType)(THIS_ eQuestDataType type) PURE;

   // Save all data for a particular type
   STDMETHOD_(BOOL, Save)(THIS_ QuestMoveFunc moveFunc, eQuestDataType type) PURE;

   // Load for a particular type
   STDMETHOD_(BOOL, Load)(THIS_ QuestMoveFunc moveFunc, eQuestDataType type) PURE;

   // Script services
   STDMETHOD(SubscribeMsg)(THIS_ ObjID objID, const char *pName, eQuestDataType type = kQuestDataUnknown) PURE;
   STDMETHOD(UnsubscribeMsg)(THIS_ ObjID objID, const char *pName) PURE;

   STDMETHOD(ObjDeleteListener)(THIS_ ObjID objID) PURE;

   // Unsubscribe to all objects 
   STDMETHOD(UnsubscribeAll)(THIS) PURE; 

#ifndef SHIP
   STDMETHOD(ObjSpewListen)(ObjID objID) PURE;
#endif

   // An application-level filter. The app can set this, allowing it to
   // deny specific value settings. This is especially useful in a networked
   // game, so that one machine can deny remote values that it doesn't like.
   // For example, if a value should only go up, this can ignore values
   // that are going down.
   //
   // The callback should return TRUE normally, FALSE iff it doesn't want
   // this Set to take place. Note that this will *not* be called for the
   // initial creation of a data item, only for subsequent Sets.
   STDMETHOD(Filter)(THIS_ 
                     QuestFilterFunc filter,
                     void *pClientData) PURE;
};

#undef INTERFACE

#define IQuestData_Create(p, a, b, c)       COMCall3(p, Set, a, b, c) 
#define IQuestData_Get(p, a)       COMCall1(p, Get, a) 
#define IQuestData_Exists(p, a)       COMCall1(p, Exists, a) 
#define IQuestData_Delete(p, a)       COMCall1(p, Delete, a) 
#define IQuestData_Iter(p, a)       COMCall1(p, Iter, a) 
#define IQuestData_DeleteAll(p)       COMCall1(p, DeleteAll) 
#define IQuestData_DeleteAllType(p, a)       COMCall1(p, DeleteAll, a) 
#define IQuestData_Save(p, a, b)       COMCall1(p, Save, a, b) 
#define IQuestData_Load(p, a, b)       COMCall1(p, Load, a, b) 

//------------------------------------
// Iterator 
//
#define INTERFACE IQuestDataIter

DECLARE_INTERFACE_(IQuestDataIter,IUnknown)
{
   DECLARE_UNKNOWN_PURE(); 

   STDMETHOD_(BOOL,Done)(THIS) PURE;
   STDMETHOD(Next)(THIS) PURE;
   STDMETHOD_(const char *,Name)(THIS) PURE;
   STDMETHOD_(int,Value)(THIS) PURE;
};
#undef INTERFACE

#define IQuestDataIter_Done(p)       COMCall1(p, Iter) 
#define IQuestDataIter_Next(p)       COMCall1(p, Iter) 
#define IQuestDataIter_Name(p)       COMCall1(p, Iter) 
#define IQuestDataIter_Value(p)      COMCall1(p, Iter) 

// Create
void QuestDataCreate();

#endif
