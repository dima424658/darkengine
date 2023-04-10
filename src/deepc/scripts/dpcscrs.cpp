/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/deepc/scripts/dpcscrs.cpp,v 1.1 2000/01/26 19:05:05 porges Exp $
#include <scrptapi.h>
#include <scrptbas.h>
#include <scrptsrv.h>

#include <dpcscrs.h>
#include <dpcmain.h>
#include <plyrmode.h>
#include <config.h>
#include <playtest.h>

#include <drkstats.h>
#include <gen_bind.h>
#include <command.h>

//must be last header
#include <dbmem.h>

//
// Dark Game script service
//

DECLARE_SCRIPT_SERVICE_IMPL(cDarkGameSrv,DarkGame)
{
   STDMETHOD(KillPlayer)()
   {
#ifdef PLAYTEST
      if (CheatsActive())
         if (config_is_defined("no_endgame"))
            return S_FALSE;
#endif
      g_pPlayerMode->SetMode(kPM_Dead);
      return S_OK;
   }


   STDMETHOD(EndMission)()
   {
#ifdef PLAYTEST
      if (CheatsActive())
         if (config_is_defined("no_endgame"))
            return S_FALSE;
#endif
      UnwindToMissionLoop();
      return S_OK;
   }

   STDMETHOD(FadeToBlack)(float fade_time)
   {
#ifdef PLAYTEST
      // is this necessary?  dunno, but it would be a shame
      // to fade to black and have the mission keep going
      if (CheatsActive())
         if (config_is_defined("no_endgame"))
            return S_FALSE;
#endif
      g_pPlayerMode->StartFadeOut(tSimTime(fade_time*1000), 0, 0, 0);
      return S_OK;
   }

};

IMPLEMENT_SCRIPT_SERVICE_IMPL(cDarkGameSrv,DarkGame);

