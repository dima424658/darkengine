/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/ainet.cpp,v 1.5 2000/02/19 12:48:44 toml Exp $
//
// This module serves as the primary interface between AIs and networking.
//

#include <lg.h>
#include <appagg.h>

#include <netprops.h>
#include <netman.h>

#include <phnet.h>

#include <ainet.h>
#include <aibasctm.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAINetwork
//

STDMETHODIMP_(const char *) cAINetwork::GetName()
{
   return "Networking component";
}

///////////////////////////////////////

STDMETHODIMP_(void) cAINetwork::Init()
{
   SetNotifications(kAICN_ModeChange);
   SetNotifications(kAICN_SimStart);
}

///////////////////////////////////////
//
// When the AI changes modes, update its heartbeat rate appropriately.
// At the moment, the rates are hardcoded; this should probably be
// stored in some more flexible fashion in the long run...
//

// @TBD: These rates are chosen out of a hat, and should be tuned:
#define EFFICIENT_RATE 10000
STDMETHODIMP_(void) cAINetwork::OnModeChange(eAIMode previous, eAIMode mode)
{
   AutoAppIPtr(NetManager);
   switch(mode) {
      case kAIM_Asleep:
      case kAIM_Dead:
      case kAIM_SuperEfficient:
      case kAIM_Efficient:
         if (previous > kAIM_Efficient)
            PhysNetSetSleep(GetID(), TRUE);
         break;
      case kAIM_Normal:
      case kAIM_Combat:
         ObjSetHeartbeat(GetID(), mode==kAIM_Normal ? NORMAL_RATE : COMBAT_RATE);
         if (previous < kAIM_Normal)
            PhysNetSetSleep(GetID(), FALSE, TRUE);
         break;
      default:
         Warning(("cAINetwork Mode Change: Unknown mode %d\n", mode));
         break;
   }
}

///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(void) cAINetwork::OnSimStart()
{
   // This is now done in aiman::AllNetPlayersJoined() instead, due
   // to timing constraints:
   //eAIMode mode = m_pAIState->GetMode();
   //AutoAppIPtr(NetManager);
   //PhysNetSetSleep(GetID(), mode <= kAIM_Efficient);
   //ObjSetHeartbeat(GetID(), mode==kAIM_Combat ? COMBAT_RATE : NORMAL_RATE);
}

///////////////////////////////////////////////////////////////////////////////
