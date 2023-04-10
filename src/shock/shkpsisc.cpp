/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkpsisc.cpp,v 1.3 1999/02/21 20:19:20 XEMU Exp $

#include <shkpsisc.h>

#include <appagg.h>

#include <scrptapi.h>
#include <scrptsrv.h>
#include <scrptbas.h>

#include <playrobj.h>

#include <shkpsapi.h>
#include <shkpsipw.h>
#include <shkpsipr.h>

// must be last header
#include <dbmem.h>

DECLARE_SCRIPT_SERVICE_IMPL(cShockPsiSrv, ShockPsi)
{
public:
   STDMETHOD(OnDeactivate)(ePsiPowers power)
   {
      AutoAppIPtr(PlayerPsi);
      pPlayerPsi->OnDeactivate(power);
      return S_OK;
   }
   STDMETHOD_(tSimTime, GetActiveTime)(ePsiPowers power)
   {
      return PsiPowerGetTime(power, PlayerObject());
   }

   STDMETHOD_(BOOL, IsOverloaded)(ePsiPowers power) 
   {
      return(PsiOverloaded(PlayerObject(),power));
   }
};

IMPLEMENT_SCRIPT_SERVICE_IMPL(cShockPsiSrv, ShockPsi);
