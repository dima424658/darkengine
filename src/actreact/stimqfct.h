/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/stimqfct.h,v 1.1 1997/09/26 16:06:00 mahk Exp $
#pragma once  
#ifndef __STIMQFCT_H
#define __STIMQFCT_H

#include <osetlnkq.h>
#include <stimuli.h>
#include <relation.h>

////////////////////////////////////////////////////////////
// STIMULUS LINK QUERY FACTORY
//

class cStimulusQueryFactory : public cLinkQueryFactory
{
   IStimuli* Stimuli;
   StimID Stim;
   IRelation* Rel;

public:
   cStimulusQueryFactory(IStimuli* stimuli, StimID stim, IRelation* rel)
      : Rel(rel),
        Stim(stim),
        Stimuli(stimuli)
   {
      Rel->AddRef();
      Stimuli->AddRef();
   }

   ~cStimulusQueryFactory() 
   { 
      SafeRelease(Rel);
      SafeRelease(Stimuli);
   };

   ILinkQuery* Query(ObjID obj) 
   { 
      cLinkQueryFactory* fact = CreateDestSetQueryFactory(Rel,obj);
      cAutoIPtr<IObjectQuery> objset (Stimuli->QueryDonors(Stim));
      
      return CreateObjSetLinkQuery(objset,fact);
   }; 
   
};



#endif // __STIMQFCT_H
