/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/dark/daielem.h,v 1.3 2000/01/31 09:39:49 adurant Exp $
//
//
//
#pragma once

#ifndef __DAIELEM_H
#define __DAIELEM_H

#include <ai.h>
#include <aibascmp.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIElemental
//

class cAIElemental : public cAI
{
public:
   STDMETHOD_(float, GetGroundOffset)();
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIElementalLightAbility
//

class cAIElementalLightAbility : public cAIComponentBase<IAIComponent, &IID_IAIComponent>
{
public:
   cAIElementalLightAbility();

   STDMETHOD_(const char *, GetName)();
   STDMETHOD_(void, Init)();

   // Notifications
   STDMETHOD_(void, OnModeChange)(eAIMode previous, eAIMode mode);

private:
   int m_lastLightProp;
};

///////////////////////////////////////////////////////////////////////////////

#endif /* !__DAIELEM_H */
