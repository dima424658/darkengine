/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkscrs.cpp,v 1.9 2000/02/29 19:10:54 patmac Exp $
#include <scrptapi.h>
#include <scrptbas.h>
#include <scrptsrv.h>

#include <drkscrs.h>
#include <drkmislp.h>
#include <plyrmode.h>
#include <config.h>
#include <playtest.h>
#include <drkamap.h>

#include <drkstats.h>
#include <gen_bind.h>
#include <command.h>

//must be last header
#include <dbmem.h>

//
// Dark UI script service
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

   STDMETHOD(FoundObject)(ObjID obj)
   {
      if (DarkStatCheckBit(obj, kDarkStatBitHidden))
      {
         DarkStatInc(kDarkStatSecrets);
         DarkStatSetBit(obj, kDarkStatBitHidden, FALSE);
      }
      return S_OK;
   }


   STDMETHOD_(BOOL, ConfigIsDefined)(const char *name)
   {
      return config_is_defined(name);
   }

   STDMETHOD_(BOOL, ConfigGetInt)(const char *name, int ref value)
   {
      return config_get_int(name, &value);
   }

   STDMETHOD_(BOOL, ConfigGetFloat)(const char *name, float ref value)
   {
      return config_get_float(name, &value);
   }

   STDMETHOD_(real, BindingGetFloat)(const char *name)
   {
       char comval[255];
       sprintf(comval,"echo $%s",name);
       return (atof (g_pInputBinder->ProcessCmd (comval)));
   }

   STDMETHOD_(BOOL, GetAutomapLocationVisited)(int page, int location)
   {
      return DarkAutomapGetLocationVisited( page, location );
   }

   STDMETHOD(SetAutomapLocationVisited)(int page, int location)
   {
      DarkAutomapSetLocationVisited( page, location );
      return S_OK;
   }
};

IMPLEMENT_SCRIPT_SERVICE_IMPL(cDarkGameSrv,DarkGame);

