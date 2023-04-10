/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkcret.cpp,v 1.18 2000/02/23 17:38:46 adurant Exp $

#include <drkcret.h>
#include <mmanuver.h>
#include <motprop.h>
#include <motmngr.h>
#include <creatext.h>
#include <plyrhdlr.h>
#include <plyrvmot.h>

// creature subclasses
#include <ccrhuman.h>

// specific creature descriptor modules
#include <crhumand.h>
#include <crplyarm.h>
#include <crbowarm.h>
#include <crburrik.h>
#include <crspider.h>
#include <crbugbst.h>
#include <crcraymn.h>
#include <crconsta.h>
#include <crappa.h>
#include <crsweel.h>
#include <crrope.h>
#include <crzombie.h>
#include <crspidsm.h>
#include <crfrog.h>
#include <crcutty.h>
#include <crrobot.h>
#include <crsmrbt.h>
#include <crspirob.h>

#include <dbmem.h> // must be last included header

EXTERN cManeuverFactory *g_pGroundLocoManeuverFactory; // from mvrgloco.cpp
EXTERN cManeuverFactory *g_pCombatManeuverFactory; // from mvrcmbat.cpp
EXTERN cManeuverFactory *g_pSingleManeuverFactory; // from mvrsngle.cpp
EXTERN cManeuverFactory *g_pPlayerManeuverFactory; // from mvrplyr.cpp
EXTERN cManeuverFactory *g_pFlexBowManeuverFactory; // from mvrflbow.cpp
EXTERN cManeuverFactory *g_pGroundActionManeuverFactory; // from mvrgact.cpp

// this should be in same order is names array and enum in drkcret.h
static cManeuverFactory *g_aDarkManeuverFactory[] = {\
   g_pGroundLocoManeuverFactory,
   g_pCombatManeuverFactory,
   g_pGroundActionManeuverFactory,
   g_pPlayerManeuverFactory,
   g_pFlexBowManeuverFactory,
   g_pSingleManeuverFactory,
};   

static char *motor_controller_names[] = { 
   "GroundLoco",
   "Combat",
   "GroundAction",
   "PlayerArm",
   "PlayerBow",
   "Single",
};

// register motor controllers and virtual motions
static void SetupControllers()
{
   // register motor controllers
   int i,size=sizeof(g_aDarkManeuverFactory)/sizeof(g_aDarkManeuverFactory[0]);
   g_ManeuverFactoryList.SetSize(0);
   for(i=0;i<size;i++)
   {
      g_ManeuverFactoryList.Append(g_aDarkManeuverFactory[i]);
   }

   // build sdesc for motor controller property
   MotorControllerPropInit(motor_controller_names,sizeof(motor_controller_names)/sizeof(motor_controller_names[0]));

}

static void RegisterVirtualMotions()
{
   // register virtual motions 
   PlyrVMotRegisterMotions();
}


static const sCreatureDesc *g_aCreatureDesc[kNumDarkCreatureTypes]=\
{ 
   &sCrHumanoidDesc,
   &sCrPlayerArmDesc,
   &sCrBowArmDesc,
   &sCrBurrickDesc,
   &sCrSpiderDesc,
   &sCrBugBeastDesc,
   &sCrCrayManDesc,
   &sCrConstantineDesc,
   &sCrApparitionDesc,
   &sCrSweelDesc,
   &sCrRopeDesc,
   &sCrZombieDesc,
   &sCrSpiderSmallDesc,
   &sCrFrogDesc,
   &sCrCuttyDesc,
   &sCrAvatarDesc,
   &sCrRobotDesc,
   &sCrSmallRobotDesc,
   &sCrSpiderBotDesc,
};

EXTERN cCreatureFactory *g_pBugCreatureFactory; // from ccrbug.cpp
EXTERN cCreatureFactory *g_pCrayCreatureFactory; // from ccrcray.cpp
EXTERN cCreatureFactory *g_pApparitionCreatureFactory; // from ccrappa.cpp

static const cCreatureFactory *g_aCreatureFactory[kNumDarkCreatureTypes]=\
{
   g_pHumanoidCreatureFactory, // for human
   NULL, // use default 
   NULL, // use default 
   NULL, // use default 
   NULL, // use default 
   g_pBugCreatureFactory, // for bugbeast
   g_pCrayCreatureFactory, // for crayman
   NULL, // use default 
   g_pApparitionCreatureFactory, // use default 
   NULL, // use default 
   NULL, // use default
   NULL,
   NULL,
   NULL,
   NULL,
   NULL, // use default
   NULL, // use default
   NULL, // use default
   NULL, // use default
};

static const char *g_aCreatureNames[kNumDarkCreatureTypes]=\
{ 
   "Humanoid",
   "PlayerArm",
   "PlayerBowArm",
   "Burrick",
   "Spider",
   "BugBeast",
   "Crayman",
   "Constantine",
   "Apparition",
   "Sweel",
   "Rope",
   "Zombie",
   "Small Spider",
   "Frog",
   "Cutty",
   "Avatar",
   "Robot",
   "Small Robot",
   "Spider Bot"
};

void DarkCreaturesInit()
{
   SetupControllers();
   RegisterVirtualMotions();
   CreaturesInit(kNumDarkCreatureTypes,g_aCreatureDesc,g_aCreatureNames,g_aCreatureFactory);
   PlayerHandlersInit(kPlayerNumModes);
}

void DarkCreaturesTerm()
{
   CreaturesClose();
   PlayerHandlersTerm();
}





