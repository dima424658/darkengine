/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/reaction.h,v 1.3 2000/01/29 12:44:35 adurant Exp $
#pragma once

#ifndef __REACTION_H
#define __REACTION_H

#include <comtools.h>
#include <reactype.h>
#include <stimtype.h>
#include <senstype.h>

////////////////////////////////////////////////////////////
//
// DATABASE OF ACT/REACT EFFECTS ("REACTIONS")
//
//

F_DECLARE_INTERFACE(IReactions);
F_DECLARE_INTERFACE(IReactionQuery);

#undef INTERFACE
#define INTERFACE IReactions

DECLARE_INTERFACE_(IReactions,IUnknown)
{
   DECLARE_UNKNOWN_PURE();
   
   //
   // Add a new reaction
   //
   STDMETHOD_(ReactionID,Add)(THIS_ sReactionDesc* desc, ReactionFunc func, tReactionFuncData data) PURE;
   
   //
   // Look up a reaction by name.  
   //
   STDMETHOD_(ReactionID,GetReactionNamed)(THIS_ const char* name) PURE;
   
   //
   // Describe a reaction
   //
   STDMETHOD_(const sReactionDesc*, DescribeReaction)(THIS_ ReactionID) PURE;

   //
   // Get the list of all reactions
   //
   STDMETHOD_(IReactionQuery*, QueryAll)(THIS) PURE;

   //
   // Fire a reaction based on an event
   //
   STDMETHOD_(eReactionResult,React)(THIS_ ReactionID reaction, sReactionEvent* event, const sReactionParam* param) PURE;
   

};

#undef INTERFACE
#define INTERFACE IReactionQuery
DECLARE_INTERFACE_(IReactionQuery,IUnknown)
{
   DECLARE_UNKNOWN_PURE();

   //
   // Iteration
   //
   STDMETHOD(Start)(THIS) PURE;
   STDMETHOD_(BOOL,Done)(THIS) PURE;
   STDMETHOD(Next)(THIS) PURE;

   //
   // Access
   //
   STDMETHOD_(ReactionID,ID)(THIS) PURE;
   // valid until Next or Release
   STDMETHOD_(const sReactionDesc*,Reaction)(THIS) PURE;
   
};

#undef INTERFACE


EXTERN void ReactionsCreate(void); 

#endif // __REACTION_H
