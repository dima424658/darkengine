/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicbrvan.h,v 1.1 1999/03/25 17:31:19 JON Exp $
//
//
//

#ifndef __AICBRVAN_H
#define __AICBRVAN_H

#include <aicbrngd.h>
#include <aicbrmod.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

class cAIRangedVantage: public cAIRangedMode
{
public:
   cAIRangedVantage(cAIRangedSubcombat* pOwner);

   virtual eAIRangedModeID GetModeID(void) const;
   virtual const char* GetName(void) const;

   virtual int SuggestApplicability(void);
   virtual int SuggestInterrupt(void);
   virtual BOOL CheckPreconditions(void);
   virtual cAIAction* SuggestAction(void); 

private:
   mxs_vector* m_pLoc;
};
 
///////////////////////////////////////////////////////////////////////////////

inline cAIRangedVantage::cAIRangedVantage(cAIRangedSubcombat* pOwner): cAIRangedMode(pOwner) {}

inline eAIRangedModeID cAIRangedVantage::GetModeID(void) const {return kAIRC_VantageMode;}
inline const char* GetName(void) const {return "Vantage";}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AICBRVAN_H */


