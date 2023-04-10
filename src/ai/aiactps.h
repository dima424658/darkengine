/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiactps.h,v 1.5 1999/03/02 17:42:11 TOML Exp $
//
//
//

#ifndef __AIACTPS_H
#define __AIACTPS_H

#include <lg.h>
#include <str.h>
#include <dynarray.h>

#include <aibasact.h>

#include <lg.h>
#include <dynarray.h>

#include <aibasact.h>
#include <aipsdscr.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIPsdScrAction
//

class cAIPsdScrAction : public cAIAction
{
public:
   cAIPsdScrAction(IAIActor * pOwner, DWORD data = 0);
   ~cAIPsdScrAction();

   // Add to the sequence
   void Set(const sAIPsdScrAct *, unsigned nActs, int iFirst = 0);
   void SetLink(ObjID source, ObjID dest);
   
   // Get the current step
   int GetCurrentStep() const;

   // Update the action.
   STDMETHOD_(eAIResult, Update)();

   // Start the action. Instantaneous actions will return result
   STDMETHOD_(eAIResult, Enact)(ulong deltaTime);

   // Save/Load the action.  Assumes pTagFile cursor is positioned for this action.
   STDMETHOD_(BOOL, Save)(ITagFile * pTagFile);
   STDMETHOD_(BOOL, Load)(ITagFile * pTagFile);

   BOOL Advance();
   BOOL IsDone() const;
   BOOL IsLast() const;

private:
   BOOL StartActions();
   cAIAction * HandleCurrent();

   int                     m_iCurrent;
   cDynArray<sAIPsdScrAct> m_actions;
   
   // Source and dest, if pseudoscript is associated with a link of some kind
   ObjID                   m_source;
   ObjID                   m_dest;
   
   cAIAction *             m_pCurrent;
};

///////////////////////////////////////

inline cAIPsdScrAction::cAIPsdScrAction(IAIActor * pOwner, DWORD data)
 : cAIAction(kAIAT_PsuedoScript, pOwner, data),
   m_iCurrent(0),
   m_pCurrent(NULL),
   m_source(m_pAIState->GetID()),
   m_dest(m_pAIState->GetID())
{
}

///////////////////////////////////////

inline void cAIPsdScrAction::Set(const sAIPsdScrAct * pActs, unsigned nActs, int iFirst)
{
   m_actions.SetSize(nActs);
   m_actions.MemCpy(pActs, nActs);
   m_iCurrent = iFirst;
}

///////////////////////////////////////

inline void cAIPsdScrAction::SetLink(ObjID source, ObjID dest)
{
   m_source = source;
   m_dest = dest;
}

///////////////////////////////////////

inline BOOL cAIPsdScrAction::IsDone() const
{
   return (m_iCurrent >= m_actions.Size());
}

///////////////////////////////////////

inline BOOL cAIPsdScrAction::IsLast() const
{
   return (m_iCurrent >= m_actions.Size() - 1);
}

///////////////////////////////////////

inline int cAIPsdScrAction::GetCurrentStep() const
{
   return m_iCurrent;
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIACTPS_H */
