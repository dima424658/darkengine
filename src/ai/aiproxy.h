/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiproxy.h,v 1.2 2000/01/29 12:45:37 adurant Exp $
//
//
//
#pragma once

#ifndef __AIPROXY_H
#define __AIPROXY_H

#include <ai.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIProxy.  For multi-player games, where the "brains" of this AI are elsewhere.
//

class cAIProxy: public cAI
{
public:
   STDMETHOD_(BOOL, IsNetworkProxy)();  // True for all instances of this class.
   STDMETHOD_(void, StartProxyAction)(IAIAction *pAction, ulong deltaTime);
   virtual HRESULT OnNormalFrame();
   virtual void Enact();
};

///////////////////////////////////////////////////////////////////////////////

#endif /* !__AIPROXY_H */
