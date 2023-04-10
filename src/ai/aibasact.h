/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aibasact.h,v 1.13 1999/12/01 16:05:48 BFarquha Exp $
//
// Base AI action class. This is an abstract base class.
//

#ifndef __AIBASACT_H
#define __AIBASACT_H

#include <lg.h>
#include <str.h>

#include <aiacttyp.h>
#include <aiapiabl.h>
#include <aiapiact.h>
#include <aiapiiai.h>
#include <aiutils.h>

#pragma once
#pragma pack(4)

class cAIState;

class cAIAction;
class cAIMoveAction;
class cAILocoAction;
class cAIMotionAction;
class cAISoundAction;
class cAIOrientAction;
class cAIFrobAction;
class cAIFollowAction;
class cAIInvestAction;
class cAIWanderAction;
class cAIPsdScrAction;
class cAIWaitAction;

///////////////////////////////////////////////////////////////////////////////

enum eAIBaseActionFlags
{
   // This is the first time enact has been called
   kAIAF_Started = kAIAF_ApiFirstAvail,

   kAIAF_BaseFirstAvail = (kAIAF_Started << 1)
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIAction
//

class cAIAction : public cCTUnaggregated<IAIAction, &IID_IAIAction, kCTU_Default>,
                  protected sAIAction
{
public:
   cAIAction(tAIActionType type,
             IAIActor * pOwner,
             DWORD data = 0);

   void SetData(DWORD data, unsigned index = 0);

   // Tag manipulation. Note that not all actions need, or even want, tags
   void ClearTags();
   void AddTag(const char *pszTypeName, const char *pszValueName);
   void AddTag(const char *pszTypeName, int iValue);
   void AddTag(const cTag &Tag);
   void AddTags(const cTagSet &Source);

   const cTagSet & GetTags() const;

   // Describe the action, in detailed human terms
   STDMETHOD_(void, Describe)(cStr * pStr);

   // Access the internals of the action
   STDMETHOD_(BOOL,              InProgress)();
   STDMETHOD_(tAIActionType,     GetType)();
   STDMETHOD_(eAIResult,         GetResult)();
   STDMETHOD_(DWORD,             GetData)(unsigned index = 0);
   STDMETHOD_(const sAIAction *, Access)();

   // Proxy support
   STDMETHOD_(BOOL,              IsProxy)();
   STDMETHOD_(IAIAction *,       GetInnerAction)();
   STDMETHOD_(IAIAction *,       GetTrueAction)();
   STDMETHOD_(IAIActor *,        GetTrueOwner)();

   STDMETHOD_(BOOL,              Started)();

   // Update the action
   STDMETHOD_(eAIResult, Update)();

   // Start the action. Instantaneous actions will return result
   STDMETHOD_(eAIResult, Enact)(ulong deltaTime);

   // Terminate the action
   STDMETHOD_(eAIResult, End)();

   // Save/Load already created action.  Load returns false if the action should be deleted.
   STDMETHOD_(BOOL, Save)(ITagFile * pTagFile);
   STDMETHOD_(BOOL, Load)(ITagFile * pTagFile);

   STDMETHOD_(IAIPath *, GetPath)();

protected:

   virtual BOOL CanEnact();

   void SetStarted(BOOL);

   IInternalAI *    m_pAI;
   const cAIState * m_pAIState;

   //
   // Factory method helpers (an unfortunate but useful reference to derivees)
   //
   cAIMoveAction * CreateMoveAction(DWORD data = 0);
   cAILocoAction * CreateLocoAction(DWORD data = 0);
   cAIMotionAction * CreateMotionAction(DWORD data = 0);
   cAISoundAction * CreateSoundAction(DWORD data = 0);
   cAIOrientAction * CreateOrientAction(DWORD data = 0);
   cAIFrobAction * CreateFrobAction(DWORD data = 0);
   cAIFollowAction * CreateFollowAction(DWORD data = 0);
   cAIInvestAction * CreateInvestAction(DWORD data = 0);
   cAIWanderAction * CreateWanderAction(DWORD data = 0);
   cAIPsdScrAction * CreatePsdScrAction(DWORD data = 0);
   cAIWaitAction *   CreateWaitAction(DWORD data = 0);

   // Save/Load the data common to all actions.
   // Called by derived class's implementations of Save() & Load().
   void SaveActionBase(ITagFile * pTagFile);
   void LoadActionBase(ITagFile * pTagFile);
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAINullAction
//

class cAINullAction : public cAIAction
{
public:
   cAINullAction(IAIActor * pOwner, DWORD data = 0);

};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIAction, inline functions
//

inline cAIAction::cAIAction(tAIActionType initType,
                            IAIActor * pInitOwner,
                            DWORD data)
{
   type            = initType;
   flags           = 0;
   time            = AIGetTime();
   pctComplete     = kAIA_PctUnknown;
   result          = kAIR_NoResult;
   pOwner          = pInitOwner;
   ownerDataSet[0] = data;
   ownerDataSet[1] = 0;
   m_pAI           = (pOwner) ? pOwner->AccessOuterAI() : NULL;
   m_pAIState      = (pOwner) ? m_pAI->GetState() : NULL;
}

///////////////////////////////////////

inline void cAIAction::SetData(DWORD data, unsigned index)
{
   ownerDataSet[index] = data;
}

///////////////////////////////////////

inline void cAIAction::SetStarted(BOOL b)
{
   if (b)
      flags |= kAIAF_Started;
   else
      flags &= ~kAIAF_Started;
}

///////////////////////////////////////

inline void cAIAction::ClearTags()
{
   tags.Clear();
}

///////////////////////////////////////

inline void cAIAction::AddTag(const char *pszTypeName, const char *pszValueName)
{
   tags.Add(cTag(pszTypeName, pszValueName));
}

///////////////////////////////////////

inline void cAIAction::AddTag(const char *pszTypeName, int iValue)
{
   tags.Add(cTag(pszTypeName, iValue));
}

///////////////////////////////////////

inline void cAIAction::AddTag(const cTag & tag)
{
   tags.Add(tag);
}

///////////////////////////////////////

inline void cAIAction::AddTags(const cTagSet & source)
{
   tags.Append(source);
}

///////////////////////////////////////

inline const cTagSet & cAIAction::GetTags() const
{
   return tags;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAINullAction, inline functons
//

inline cAINullAction::cAINullAction(IAIActor * pOwner, DWORD data)
 : cAIAction(kAIAT_Null, pOwner, data)
{
   result = kAIR_Success;
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIBASACT_H */
