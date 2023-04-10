/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicbrclo.h,v 1.3 1999/05/29 21:55:35 JON Exp $
//
//
//

#ifndef __AICBRCLO_H
#define __AICBRCLO_H

#include <aibasact.h>
#include <aicbrnew.h>
#include <aicbrmod.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

class cAIRangedClose: public cAIRangedMode
{
public:
   cAIRangedClose(cAINewRangedSubcombat* pOwner);
   virtual void Reset(void);

   virtual eAIRangedModeID GetModeID(void) const;
   virtual const char* GetName(void) const;

   virtual void OnLosingControl(void);
   virtual int SuggestApplicability(void);
   virtual int GetPriority(void);


   BOOL CheckStepForwardPreconditions(void);
   BOOL CheckLocoPreconditions(void);
   virtual BOOL CheckPreconditions(void);

   cAIAction* SuggestStepForwardAction(void);
   cAIAction* SuggestLocoAction(void);
   virtual cAIAction* SuggestAction(void); 

private:
   cAILocoAction* m_pLocoAction; // cached action (kind of gross)
   BOOL m_rePathBlock;           // we are blocking ourselves from pathfinding for a while
   tSimTime m_failedPathTime;    // last time we failed a pathfind
   Location m_failedPathLoc;     // where it was to
};
 
///////////////////////////////////////////////////////////////////////////////
 
inline void cAIRangedClose::Reset(void) {SafeRelease(m_pLocoAction);} // shouldn't ever be necessary

inline eAIRangedModeID cAIRangedClose::GetModeID(void) const {return kAIRC_CloseMode;}
inline const char* cAIRangedClose::GetName(void) const {return "Close";}

inline int cAIRangedClose::GetPriority(void) {return kAIRC_PriLow;}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AICBRCLO_H */


