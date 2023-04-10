/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/arscrs.h,v 1.5 1999/03/03 17:40:17 mahk Exp $
#pragma once  
#ifndef __ARSCRS_H
#define __ARSCRS_H

#include <objscrpt.h>
#include <arscrt.h>

DECLARE_SCRIPT_SERVICE(ActReact, 0xF4)
{

   //
   // Invoke an Act/React reaction 
   // 
   STDMETHOD(React)(reaction_kind what, 
                    real stim_intensity, 
                    object target = 0, 
                    object agent = 0, 
                    const cMultiParm& parm1 = 0, 
                    const cMultiParm& parm2 = 0, 
                    const cMultiParm& parm3 = 0, 
                    const cMultiParm& parm4 = 0, 
                    const cMultiParm& parm5 = 0, 
                    const cMultiParm& parm6 = 0, 
                    const cMultiParm& parm7 = 0, 
                    const cMultiParm& parm8 = 0) PURE; 

   //
   // Stimulate an object directly. 
   //
   STDMETHOD(Stimulate)(object who, stimulus_kind what, real how_much, object source = object(0)) PURE; 

   //
   // Reaction name accessors
   //
   STDMETHOD_(integer,GetReactionNamed)(const char* name) PURE; 
   STDMETHOD_(string,GetReactionName)(long id) PURE;                  

   //
   // Inform act/react that you are interested in stimulus messages
   //
   STDMETHOD(SubscribeToStimulus)(object obj, stimulus_kind what) PURE;
   STDMETHOD(UnsubscribeToStimulus)(object obj, stimulus_kind what) PURE; 

   // 
   // Manage "Contacts" between objects.  The "Script" propagator will 
   // propagate stimuli between objects that are in contact.  Note
   // that contact is uni-directional. 

   // Begin a contact
   STDMETHOD(BeginContact)(object source, object sensor) PURE; 
   // End a contact
   STDMETHOD(EndContact)(object source, object sensor) PURE; 

   // Begin a contact, ending any others with this sensor
   STDMETHOD(SetSingleSensorContact)(object source, object sensor) PURE; 

   



};

#ifdef SCRIPT

inline reaction_kind::reaction_kind(const char* name)
   : id(ActReact.GetReactionNamed(name))
{
}

inline reaction_kind::operator string() const
{
   return ActReact.GetReactionName(id); 
}

#endif 





#endif // __ARSCRS_H
