/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicbrmod.h,v 1.3 1999/04/16 17:02:19 JON Exp $
//
//
//

#ifndef __AICBRMOD_H
#define __AICBRMOD_H

#include <aicbrnew.h>
#include <aiapiact.h>
#include <aitrginf.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

class cAIRangedMode
{
public:
   cAIRangedMode(cAINewRangedSubcombat* pOwner);
   ~cAIRangedMode(void);

   virtual void Reset(void); 
   virtual void Save(void) const;
   virtual void Load(void);

   virtual eAIRangedModeID GetModeID(void) const = NULL;
   virtual const char* GetName(void) const = NULL;

   virtual int SuggestApplicability(void);
   virtual int SuggestInterrupt(void);
   virtual int SuggestContinuation(void);
   virtual int GetPriority(void);

   virtual BOOL CheckContinuation(void);
   virtual BOOL CheckPreconditions(void);

   virtual void OnLosingControl(void);
   virtual void OnGainingControl(void);

   virtual cAIAction* SuggestAction(void) = NULL; 

   BOOL TestFlags(eAIRangedModeFlags flags) const;
   void SetFlags(eAIRangedModeFlags flags);

protected:
   // Ranged combat access functions
   ObjID GetTarget(void) const;
   const mxs_vector& GetTargetLoc(void) const;
   floatang GetTargetHeading(void) const;
   floatang GetTargetHeadingDelta(void) const;
   const sAITargetInfo * GetTargetInfo() const;

   sAIRangedCombatProp* GetRangedCombatProp(void) const;
   eAIRangedCombatRange GetRangeClass(void) const;

   eAIRangedModeID GetCurrentModeID(void) const;

   IInternalAI* GetInternalAI(void) const;
   const cAIState* GetState(void) const;
   ObjID GetID(void) const;

   // Properties
   int GetApplicabilityWeight(int adj) const;

   // Movement related
   float GetLocationScore(void) const;
   float GetCurrentLocationScore(void) const;

   // Motions
   cAIAction* CreateMotionAction(const char* pTagString);

   // Current Location
   cAIRangedLocation* GetCurrentLocation(void) const;

   // Events
   BOOL FindEvent(eAINewRangedCombatEvent event) const;
   sAIRangedCombatEvent* GetEvent(eAINewRangedCombatEvent event) const;

   cAINewRangedSubcombat* m_pOwner;

   eAIRangedModeFlags m_flags;
};
 
///////////////////////////////////////////////////////////////////////////////

inline cAIRangedMode::cAIRangedMode(cAINewRangedSubcombat* pOwner): m_pOwner(pOwner), m_flags(NULL) {m_pOwner->AddRef();}
inline cAIRangedMode::~cAIRangedMode(void) {SafeRelease(m_pOwner);}

inline void cAIRangedMode::Reset(void) {};
inline void cAIRangedMode::Save(void) const {};
inline void cAIRangedMode::Load(void) {};

inline int cAIRangedMode::SuggestApplicability(void) {return kAIRC_AppNormal;}
inline int cAIRangedMode::SuggestInterrupt(void) {return SuggestApplicability();}
inline int cAIRangedMode::SuggestContinuation(void) {return SuggestApplicability();}
inline int cAIRangedMode::GetPriority(void) {return kAIRC_PriNormal;}

inline BOOL cAIRangedMode::CheckContinuation(void) {return TRUE;}
inline BOOL cAIRangedMode::CheckPreconditions(void) {return TRUE;}

inline void cAIRangedMode::OnLosingControl(void) {};
inline void cAIRangedMode::OnGainingControl(void) {};

inline BOOL cAIRangedMode::TestFlags(eAIRangedModeFlags flags) const {return m_flags&flags;}
inline void cAIRangedMode::SetFlags(eAIRangedModeFlags flags) {m_flags|=flags;}

inline ObjID cAIRangedMode::GetTarget(void) const {return m_pOwner->GetTarget();}
inline const mxs_vector& cAIRangedMode::GetTargetLoc(void) const {return m_pOwner->GetTargetLoc();}
inline floatang cAIRangedMode::GetTargetHeading(void) const {return m_pOwner->GetTargetInfo()->targetHeading;}
inline floatang cAIRangedMode::GetTargetHeadingDelta(void) const {return m_pOwner->GetTargetInfo()->targetHeadingDelta;}
inline const sAITargetInfo* cAIRangedMode::GetTargetInfo() const {return m_pOwner->GetTargetInfo();}

inline cAIRangedLocation* cAIRangedMode::GetCurrentLocation(void) const {return m_pOwner->GetCurrentLocation();}

inline BOOL cAIRangedMode::FindEvent(eAINewRangedCombatEvent event) const {return m_pOwner->FindEvent(event);}
inline sAIRangedCombatEvent* cAIRangedMode::GetEvent(eAINewRangedCombatEvent event) const {return m_pOwner->GetEvent(event);}

inline sAIRangedCombatProp* cAIRangedMode::GetRangedCombatProp(void) const {return m_pOwner->m_pRangedCombatProp;}
inline eAIRangedCombatRange cAIRangedMode::GetRangeClass(void) const {return m_pOwner->GetCurrentLocation()->GetRangeClass();};
inline eAIRangedModeID cAIRangedMode::GetCurrentModeID(void) const {return (m_pOwner->m_pModes[m_pOwner->m_currentMode])->GetID();}
inline IInternalAI* cAIRangedMode::GetInternalAI(void) const {return m_pOwner->m_pAI;}
inline const cAIState* cAIRangedMode::GetState(void) const {return m_pOwner->m_pAI->GetState();}
inline ObjID cAIRangedMode::GetID(void) const {return m_pOwner->m_pAI->GetState()->GetID();};

EXTERN int g_AIRangedCombatModeWeights[kAIRC_NumApplicabilities];

#pragma pack()

#endif /* !__AICBRMOD_H */


