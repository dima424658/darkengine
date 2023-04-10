/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//
//

#ifndef __DPCAICMD_H
#define __DPCAICMD_H

#include <aibasabl.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAICameraDeath
//

class cAICameraDeath : public cAIAbility
{
public:   
   // Standard component methods
   STDMETHOD_(const char *, GetName)();
   STDMETHOD_(void, Init)();

   // Notifications
   STDMETHOD_(void, OnDeath)(const sDamageMsg * pMsg);

   STDMETHOD (SuggestGoal)(cAIGoal * pPrevious, cAIGoal ** ppNew);

   // Ability flow
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);

protected:
   BOOL m_dying;
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif // !__DPCAICMD_H
