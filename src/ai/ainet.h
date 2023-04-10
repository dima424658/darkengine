/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/ainet.h,v 1.3 1999/02/20 15:07:23 Justin Exp $
//

#ifndef __AINET_H
#define __AINET_H

#include <aibascmp.h>

#pragma once
#pragma pack(4)

//////////
//
// Constants
//

// The rates at which we send heartbeats for AIs:
#define NORMAL_RATE 5000
#define COMBAT_RATE 500

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAINetwork
//

typedef cAIComponentBase<IAIComponent, &IID_IAIComponent> cAINetworkBase;

class cAINetwork : public cAINetworkBase
{
public:   
   cAINetwork();

   // Standard component methods
   STDMETHOD_(const char *, GetName)();
   STDMETHOD_(void, Init)();

   // Notifications
   STDMETHOD_(void, OnModeChange)(eAIMode previous, eAIMode mode);
   STDMETHOD_(void, OnSimStart)();

private:
};

///////////////////////////////////////

inline cAINetwork::cAINetwork()
{
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AINET_H */
