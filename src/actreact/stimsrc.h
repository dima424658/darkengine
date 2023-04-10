/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/stimsrc.h,v 1.3 1997/09/29 16:24:21 mahk Exp $
#pragma once
#ifndef __STIMSRC_H
#define __STIMSRC_H

#include <comtools.h>
#include <stimtype.h>
#include <ssrctype.h>
#include <objtype.h>
#include <osystype.h>

////////////////////////////////////////////////////////////
// STIM SOURCE API 
//
// This interface is for creating,destroying and managing sources 
// of Act/React data
//


#undef INTERFACE 
#define INTERFACE IStimSources
DECLARE_INTERFACE_(IStimSources,IUnknown)
{
   DECLARE_UNKNOWN_PURE(); 

   //
   // Attach a stimulus source to an object (source info is copied)
   //
   STDMETHOD_(StimSourceID, AddSource)(THIS_ ObjID obj, StimID stimulus, const sStimSourceDesc* desc) PURE; 
   // Remove a source
   STDMETHOD(RemoveSource)(THIS_ StimSourceID stim) PURE;

   //
   // Source Accessors
   //
   STDMETHOD_(sObjStimPair,GetSourceElems)(THIS_ StimSourceID id) PURE;
   STDMETHOD_(tStimTimeStamp,GetSourceBirthDate)(THIS_ StimSourceID id) PURE;

   //
   // Look up the description of a source.
   //
   STDMETHOD(DescribeSource)(THIS_ StimSourceID id, sStimSourceDesc* desc) PURE;

   //
   // Query the source database OBJ_NULL is the wildcard. 
   //
   STDMETHOD_(IStimSourceQuery*,QuerySources)(THIS_ ObjID obj, StimID stimulus) PURE;

   //
   // Notify the stimsensors about database changes
   // 
   STDMETHOD(DatabaseNotify)(THIS_ tStimDatabaseMsg msg, IUnknown* file) PURE;

   //
   // Notify about object creation/deletion
   //
   STDMETHOD(ObjectNotify)(THIS_ eObjNotifyMsg msg, ObjNotifyData data) PURE;
}; 


#undef INTERFACE
#define INTERFACE IStimSourceQuery
DECLARE_INTERFACE_(IStimSourceQuery,IUnknown)
{
   DECLARE_UNKNOWN_PURE();

   //
   // Iteration methods
   //
   STDMETHOD(Start)(THIS) PURE;
   STDMETHOD_(BOOL,Done)(THIS) PURE;
   STDMETHOD(Next)(THIS) PURE;

   //
   // Accessors
   //
   STDMETHOD_(StimSourceID,ID)(THIS) PURE;
   STDMETHOD_(sObjStimPair,Elems)(THIS) PURE;
   // ptr lasts until Next() or final Release()
   STDMETHOD_(const sStimSourceDesc*, Source)(THIS) PURE;
};

#undef INTERFACE

//
// Factory
// 


EXTERN void StimSourcesCreate(void);

#endif // __STIMSRC_H
