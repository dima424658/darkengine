/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaishb.h,v 1.3 2000/01/29 13:40:21 adurant Exp $
//
#pragma once

#ifndef __SHKAISHB_H
#define __SHKAISHB_H

#include <shkaisbs.h>

///////////////////////////////////////////////////////////////////////////////
//
// Shodan behavior set
//

class cAIShodanBehaviorSet : public cAIShockBehaviorSet
{ 
public: 
   STDMETHOD_(const char *, GetName)()
   {
      return "Shodan";
   }
   
protected:
   virtual void CreateCombatAbilities(cAIComponentPtrs * pComponents);
};

///////////////////////////////////////////////////////////////////////////////

#endif /* !__SHKAISHB_H */
