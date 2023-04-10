/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/reactio_.h,v 1.4 2000/01/29 12:44:36 adurant Exp $
#pragma once

#ifndef __REACTIO__H
#define __REACTIO__H

#include <aggmemb.h>
#include <reaction.h>
#include <reacbase.h>
#include <hashpp.h>
#include <dynarray.h>
#include <str.h>

////////////////////////////////////////////////////////////
// IMPLEMENTATION CLASS FOR IREACTIONS
//


class cReactions : public IReactions 
{
   DECLARE_AGGREGATION(cReactions);

public: 
   cReactions(IUnknown* pOuter);
   virtual ~cReactions();

   STDMETHOD_(ReactionID,Add)(sReactionDesc* desc, ReactionFunc func, tReactionFuncData data);
   STDMETHOD_(ReactionID,GetReactionNamed)(const char* name);
   STDMETHOD_(const sReactionDesc*, DescribeReaction)(ReactionID);
   STDMETHOD_(eReactionResult,React)(ReactionID reaction, sReactionEvent* event, const sReactionParam* param);
   STDMETHOD_(IReactionQuery*,QueryAll)();

protected:

   //----------------------------------------
   // Aggregate Protocol 
   //

   HRESULT Init();
   HRESULT End();

   //
   // Helper types
   //

public:
   struct sReactionEntry 
   {
      sReactionDesc desc;
      ReactionFunc func;
      tReactionFuncData data;
   };

   typedef cHashTable<const char*,ReactionID,cCaselessStringHashFuncs> ReactionTable;

   typedef cDynArray<sReactionEntry> ReactionVec; 
   typedef cDynClassArray<cStr> cStrVec; 

private:
   //
   // Data Members
   //
   ReactionTable ByName;
   ReactionVec ByID;
   cStrVec Names; 

}; 

#endif // __REACTIO__H



