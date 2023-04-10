/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkcret.cpp,v 1.13 1999/04/28 13:43:39 JON Exp $

#include <shkcret.h>
#include <mmanuver.h>
#include <motprop.h>
#include <motmngr.h>
#include <creatext.h>
#include <plyrhdlr.h>
#include <plyrvmot.h>

#include <shkcrara.h>
#include <shkcrarm.h>
#include <shkcrbar.h>
#include <shkcrdrd.h>
#include <shkcrhum.h>
#include <shkcrmky.h>
#include <shkcrovr.h>
#include <shkcrrum.h>
#include <shkcrsho.h>
 
#include <dbmem.h> // must be last included header

EXTERN cManeuverFactory *g_pGroundLocoManeuverFactory; // from mvrgloco.cpp
EXTERN cManeuverFactory *g_pCombatManeuverFactory; // from mvrcmbat.cpp
EXTERN cManeuverFactory *g_pSingleManeuverFactory; // from mvrsngle.cpp
EXTERN cManeuverFactory *g_pPlayerManeuverFactory; // from mvrplyr.cpp
EXTERN cManeuverFactory *g_pGroundActionManeuverFactory; // from mvrgact.cpp

static cManeuverFactory *g_aShockManeuverFactory[] = {\
   g_pGroundLocoManeuverFactory,
   g_pCombatManeuverFactory,
   g_pGroundActionManeuverFactory,
   g_pPlayerManeuverFactory,
   g_pSingleManeuverFactory,
};   

static char *motor_controller_names[] = { 
   "GroundLoco",
   "Combat",
   "GroundAction",
   "PlayerArm",
   "Single",
};

// register motor controllers and virtual motions
static void SetupControllers()
{
   // register motor controllers
   int i,size=sizeof(g_aShockManeuverFactory)/sizeof(g_aShockManeuverFactory[0]);
   g_ManeuverFactoryList.SetSize(0);
   for(i=0;i<size;i++)
   {
      g_ManeuverFactoryList.Append(g_aShockManeuverFactory[i]);
   }

   // build sdesc for motor controller property
   MotorControllerPropInit(motor_controller_names,sizeof(motor_controller_names)/sizeof(motor_controller_names[0]));

}

static void RegisterVirtualMotions()
{
   // register virtual motions 
   PlyrVMotRegisterMotions();
}

static const sCreatureDesc *g_aCreatureDesc[kNumShockCreatureTypes]=\
{ 
   &sCrShockHumanDesc,
   &sCrShockWrenchDesc,
   &sCrShockAvatarDesc,
   &sCrRumblerDesc,
   &sCrDroidDesc,
   &sCrOverlordDesc,
   &sCrArachnidDesc,
   &sCrMonkeyDesc,
   &sCrBabyArachnidDesc, 
   &sCrShodanDesc,
};
 
static const cCreatureFactory *g_aCreatureFactory[kNumShockCreatureTypes]=\
{
   NULL, // for human
   NULL, // for player arm
   NULL, // for avatar
};

static const char *g_aCreatureNames[kNumShockCreatureTypes]=\
{ 
   "Humanoid",
   "Wrench",
   "Avatar",
   "Rumbler",
   "Droid",
   "Overlord",
   "Arachnid",
   "Monkey",
   "BabyArach",
   "Shodan",
};
 
void ShockCreaturesInit()
{
   SetupControllers();
   RegisterVirtualMotions();
   CreaturesInit(kNumShockCreatureTypes,g_aCreatureDesc,g_aCreatureNames,g_aCreatureFactory);
   PlayerHandlersInit(kShPlayerNumModes);
}

void ShockCreaturesTerm()
{
   CreaturesClose();
   PlayerHandlersTerm();
}

