/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aimultcb.h,v 1.5 1999/05/23 17:04:08 JON Exp $
//

#ifndef __AIMULTCB_H
#define __AIMULTCB_H

#include <aicombat.h>
#include <aisubabl.h>

#pragma once
#pragma pack(4)

class cAIMultiCombat;
class cAISubcombat;
struct sAITargetInfo;

///////////////////////////////////////////////////////////////////////////////

BOOL AIInitMultiCombat(IAIManager *);
BOOL AITermMultiCombat();

///////////////////////////////////////

struct sAICombatTacticalRanges
{
   float Near;
   float Ideal;
   float Huge;
   float HugeZ;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIMultiCombat
//

class cAIMultiCombat : public cAICombat
{
public:
   cAIMultiCombat();
   ~cAIMultiCombat();
   
   void SubSignalAction(cAISubcombat *);
   
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);

   //
   // On suggest actions, this function is called to select a suggestor
   //
   virtual cAISubcombat * SelectSubcombat(cAIGoal * pGoal, const cAIActions & previous) = 0;
   
   //
   // Access target info
   //
   const sAITargetInfo * GetTargetInfo() const;

   //
   // Access ranges
   //
   const sAICombatTacticalRanges* GetRanges(void) const;
   
   //
   // Access controlling subcombat
   //
   cAISubcombat * GetActiveSubcombat();

   virtual void SetSubcombat(cAISubcombat * pSubcombat);

   virtual void SignalFrustration(void) {};

protected:

   //
   // Called at start of SuggestActions()
   // Default just gathers target info
   //
   virtual BOOL BeginSuggestActions();

   //
   // Update target info
   //
   virtual BOOL GatherTargetInfo();

   cAISubcombat * m_pActiveSubcombat;
   
private:
   sAITargetInfo * m_pTargetInfo;
   
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAISubcombat
//
// @TBD (toml 03-04-99): clean up/document this declaration

class cAISubcombat : public cAISubability<cAIMultiCombat>
{
protected:
   void SignalAction();
   ObjID GetTarget();
   const mxs_vector & GetTargetLoc();
   const sAITargetInfo * GetTargetInfo() const;
   const sAICombatTacticalRanges* GetRanges(void) const;
   BOOL CanPlayReactCharge();
   void SetPlayedReactCharge();
   BOOL InControl();
   void SignalFrustration();
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAISimpleMultiCombat
//

class cAISimpleMultiCombat : public cAIMultiCombat
{
public:
   cAISimpleMultiCombat();
   cAISimpleMultiCombat(cAISubcombat * pSubcombat);
   ~cAISimpleMultiCombat();
   
   void SetSubcombat(cAISubcombat * pSubcombat);
   STDMETHOD_(void, GetSubComponents)(cAIComponentPtrs *);

   cAISubcombat * SelectSubcombat(cAIGoal * pGoal, const cAIActions & previous);
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIMultiCombat, inline functions
//

inline void cAIMultiCombat::SubSignalAction(cAISubcombat * pSubcombat) 
{ 
   if (pSubcombat == m_pActiveSubcombat)
      SignalAction();
}

///////////////////////////////////////

inline const sAICombatTacticalRanges* cAISubcombat::GetRanges(void) const
{
   return m_pOwner->GetRanges(); 
}

///////////////////////////////////////

inline const sAITargetInfo * cAIMultiCombat::GetTargetInfo() const
{
   return m_pTargetInfo;
}

///////////////////////////////////////

inline cAISubcombat * cAIMultiCombat::GetActiveSubcombat()
{
   return m_pActiveSubcombat;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAISubcombat, inline functions
//

inline void cAISubcombat::SignalAction() 
{
   m_pOwner->SubSignalAction(this);
}

///////////////////////////////////////

inline ObjID cAISubcombat::GetTarget() 
{
   return m_pOwner->GetTarget(); 
}

///////////////////////////////////////

inline const mxs_vector & cAISubcombat::GetTargetLoc()
{
   return m_pOwner->GetTargetLoc();
}

///////////////////////////////////////

inline const sAITargetInfo * cAISubcombat::GetTargetInfo() const
{
   return m_pOwner->GetTargetInfo(); 
}

///////////////////////////////////////

inline void cAISubcombat::SignalFrustration()
{
   m_pOwner->SignalFrustration();
}

///////////////////////////////////////

inline BOOL cAISubcombat::CanPlayReactCharge()
{  
   return m_pOwner->CanPlayReactCharge(); 
}

///////////////////////////////////////

inline void cAISubcombat::SetPlayedReactCharge() 
{ 
   m_pOwner->SetPlayedReactCharge();
}

///////////////////////////////////////

inline BOOL cAISubcombat::InControl()
{
   return (m_pOwner->InControl() && m_pOwner->GetActiveSubcombat() == this);
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIMULTCB_H */
