/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaigrb.h,v 1.3 2000/01/29 13:40:16 adurant Exp $
//
#pragma once

#ifndef __SHKAIGRB_H
#define __SHKAIGRB_H

#include <ai.h>
#include <aibasbhv.h>
#include <aibascmp.h>

///////////////////////////////////////////////////////////////////////////////
//
// Grub behavior set
//

class cAIGrubBehaviorSet : public cAIBehaviorSet
{ 
public: 
   STDMETHOD_(const char *, GetName)()
   {
      return "Grub";
   }
   
protected:
   virtual void CreateNonCombatAbilities(cAIComponentPtrs * pComponents);
   virtual void CreateCombatAbilities(cAIComponentPtrs * pComponents);
   virtual void CreateGenericAbilities(cAIComponentPtrs * pComponents);
   virtual void CreateNonAbilityComponents(cAIComponentPtrs * pComponents);
};

///////////////////////////////////////////////////////////////////////////////

#endif /* !__SHKAIGRB_H */
