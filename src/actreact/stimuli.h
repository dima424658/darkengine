/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/stimuli.h,v 1.3 2000/01/29 12:44:52 adurant Exp $
#pragma once

#ifndef __STIMULI_H
#define __STIMULI_H

#include <comtools.h>
#include <stimtype.h>

F_DECLARE_INTERFACE(IObjectQuery);
F_DECLARE_INTERFACE(IStimuli);

////////////////////////////////////////////////////////////
// STIMULUS FACTORY COM INTERFACE 
//

#undef INTERFACE
#define INTERFACE IStimuli

DECLARE_INTERFACE_(IStimuli,IUnknown)
{
   DECLARE_UNKNOWN_PURE();

   // 
   // Get the root stimulus archetype
   //
   STDMETHOD_(StimID,GetRootStimulus)(THIS) PURE;

   //
   // Create a new stimulus, a parent of OBJ_NULL means the root stimulus
   //
   STDMETHOD_(StimID,CreateStimulus)(THIS_ const sStimDesc* desc, StimID parent) PURE;

   //
   // Get a stimulus name, or get a stimulus by name
   //
   STDMETHOD_(const char*,GetStimulusName)(THIS_ StimID id) PURE;
   STDMETHOD_(StimID,GetStimulusNamed)(THIS_ const char* name) PURE;

   //
   // Identify stimulus archetypes quickly
   // 
   STDMETHOD_(BOOL,IsStimulus)(THIS_ ObjID obj) PURE; 

   //
   // Query a stimulus' donors (quickly)
   //
   STDMETHOD_(IObjectQuery*,QueryDonors)(THIS_ StimID stim) PURE;
   
   //
   // Query a stimulus' heirs (quickly)
   //
   STDMETHOD_(IObjectQuery*,QueryHeirs)(THIS_ StimID stim) PURE;

   //
   // Determine (quickly) whether a stimulus inherits from an archetype/metaproperty
   //
   STDMETHOD_(BOOL,InheritsFrom)(THIS_ StimID stimulus, ObjID donor) PURE;
   
   //              
   // Notify the stimulus database of database changes
   //
   STDMETHOD(DatabaseNotify)(THIS_ tStimDatabaseMsg msg, IUnknown* dbfile) PURE;

   //
   // Turn an (abstract) object into a stimulus
   //
   STDMETHOD_(StimID,AddStimulus)(THIS_ ObjID obj) PURE; 


};

#undef INTERFACE 

//------------------------------------------------------------
// Factory function
//

EXTERN void StimuliCreate(void);



#endif // __STIMULI_H

