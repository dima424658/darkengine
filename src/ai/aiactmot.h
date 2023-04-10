/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiactmot.h,v 1.13 1999/04/17 13:13:43 JON Exp $
//
//
//

#ifndef __AIACTMOT_H
#define __AIACTMOT_H

#include <str.h>
#include <ctagset.h>

#include <fix.h>

#include <aiapiiai.h>
#include <aibasact.h>
#include <aiutils.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIMotionAction
//

const fix kAIMA_defaultFocusSpeed = 0x3800;  // default in motion system

class cAIMotionAction : public cAIAction
{
public:
   cAIMotionAction(IAIActor * pOwner, DWORD data = 0);
   virtual ~cAIMotionAction();

   // Set motion to play by name
   const char * GetName() const;
   void SetByName(const char *);
   void ClearName();
   
   // Set the desired facing, if any
   BOOL IsFacingSet() const;
   void SetFacing(const floatang &);
   const floatang & GetFacing() const;
   void ClearFacing();
   
   // set the object to continuously track during execution
   void SetFocus(ObjID focus);
   void SetFocus(ObjID focus, fix speed);
   BOOL GetFocus(ObjID *pFocus, fix* pSpeed);

   // Restart (reuse) the action
   void Restart();

   // Update the action.
   STDMETHOD_(eAIResult, Update)();

   // Start the action. Instantaneous actions will return result
   STDMETHOD_(eAIResult, Enact)(ulong deltaTime);

protected:
   enum eFlags
   {
      kFacing   = kAIAF_BaseFirstAvail,
      kContinue = (kFacing << 1),
      kFocusObj = (kContinue << 1),
   };

   // Motion to play, by name
   cStr              m_Name;
   
   // Facing desires
   floatang          m_Facing;

   // Focus desires
   ObjID             m_FocusObj;
   fix               m_FocusSpeed;

   // Timer to enforce minimum interruptability rate
   cAITimer          m_MinInterrupt;

   // Timer to enforce detect a timeout
   cAITimer          m_Timeout;

   // Cached pointer to move enactor
   IAIMoveEnactor * m_pEnactor;
};

///////////////////////////////////////

inline cAIMotionAction::cAIMotionAction(IAIActor * pOwner, DWORD data)
 : cAIAction(kAIAT_Motion, pOwner, data),
   m_MinInterrupt(kAIT_2Hz),
   m_Timeout(AICustomTime(20000))
{
   m_pEnactor = m_pAI->AccessMoveEnactor();
}

///////////////////////////////////////

inline const char * cAIMotionAction::GetName() const
{
   if (!m_Name.IsEmpty())
      return m_Name;
   return NULL;
}

///////////////////////////////////////

inline void cAIMotionAction::SetByName(const char * pszName)
{
   m_Name = pszName;
}

///////////////////////////////////////

inline void cAIMotionAction::ClearName()
{
   m_Name.Empty();
}

///////////////////////////////////////

inline BOOL cAIMotionAction::IsFacingSet() const
{
   return !!(flags & kFacing);
}

///////////////////////////////////////

inline void cAIMotionAction::SetFacing(const floatang & facing)
{
   m_Facing = facing;
   flags |= kFacing;
}

///////////////////////////////////////

inline const floatang & cAIMotionAction::GetFacing() const
{
   return m_Facing;
}

///////////////////////////////////////

inline void cAIMotionAction::ClearFacing()
{
   flags &= ~kFacing;
}

///////////////////////////////////////

inline void cAIMotionAction::SetFocus(ObjID focus)
{
   SetFocus(focus, kAIMA_defaultFocusSpeed);
}

///////////////////////////////////////

inline void cAIMotionAction::SetFocus(ObjID focus, fix speed)
{
   if(focus==OBJ_NULL)
   {
      flags&= ~kFocusObj;
   } else
   {
      flags |= kFocusObj;
   }
   m_FocusObj=focus;
   m_FocusSpeed=speed;
}

///////////////////////////////////////

inline BOOL cAIMotionAction::GetFocus(ObjID *pObj, fix* pSpeed)
{
   if(flags&kFocusObj)
   {
      *pObj=m_FocusObj;
      *pSpeed=m_FocusSpeed;
      return TRUE;
   }
   return FALSE;
}

///////////////////////////////////////

inline void cAIMotionAction::Restart()
{
   SetStarted(FALSE);
   result = kAIR_NoResult;
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIACTMOT_H */

