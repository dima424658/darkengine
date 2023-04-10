/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiactprx.h,v 1.2 1999/03/04 13:30:34 TOML Exp $
//
//
//

#ifndef __AIACTPRX_H
#define __AIACTPRX_H

#include <aibasact.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIProxAction
//

class cAIProxAction : public cAIAction
{
public:
   cAIProxAction(IAIAction * pInner, IAIActor * pOwner, DWORD data = 0);
   ~cAIProxAction();

   // Describe the action, in detailed human terms
   STDMETHOD_(void, Describe)(cStr * pStr);

   // Access the internals of the action
   STDMETHOD_(BOOL,              InProgress)();
   STDMETHOD_(tAIActionType,     GetType)();
   STDMETHOD_(eAIResult,         GetResult)();
   STDMETHOD_(DWORD,             GetData)(unsigned index = 0);
   STDMETHOD_(const sAIAction *, Access)();

   STDMETHOD_(BOOL,              IsProxy)();
   STDMETHOD_(IAIAction *,       GetInnerAction)();
   STDMETHOD_(IAIAction *,       GetTrueAction)();
   STDMETHOD_(IAIActor *,        GetTrueOwner)();

   // Update the action
   STDMETHOD_(eAIResult, Update)();

   // Start the action. Instantaneous actions will return result
   STDMETHOD_(eAIResult, Enact)(ulong deltaTime);

   // Terminate the action
   STDMETHOD_(eAIResult, End)();

private:
   IAIAction * m_pInner;
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIACTPRX_H */
