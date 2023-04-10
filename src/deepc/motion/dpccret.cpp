/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <dpccret.h>
#include <mmanuver.h>
#include <motprop.h>
#include <motmngr.h>
#include <creatext.h>
#include <plyrhdlr.h>
#include <plyrvmot.h>

#include <dpccrara.h>
#include <dpccrarm.h>
#include <dpccrbar.h>
#include <dpccrdrd.h>
#include <dpccrhum.h>
#include <dpccrmky.h>
#include <dpccrovr.h>
#include <dpccrdoo.h>
#include <dpccrdog.h>
#include <dpccrdhu.h>
 
#include <dbmem.h> // must be last included header

EXTERN cManeuverFactory *g_pGroundLocoManeuverFactory; // from mvrgloco.cpp
EXTERN cManeuverFactory *g_pCombatManeuverFactory; // from mvrcmbat.cpp
EXTERN cManeuverFactory *g_pSingleManeuverFactory; // from mvrsngle.cpp
EXTERN cManeuverFactory *g_pPlayerManeuverFactory; // from mvrplyr.cpp
EXTERN cManeuverFactory *g_pGroundActionManeuverFactory; // from mvrgact.cpp

static cManeuverFactory *g_aDPCManeuverFactory[] = {\
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
   int size = sizeof(g_aDPCManeuverFactory)/sizeof(g_aDPCManeuverFactory[0]);
   g_ManeuverFactoryList.SetSize(0);
   for(int i = 0; i < size; i++)
   {
      g_ManeuverFactoryList.Append(g_aDPCManeuverFactory[i]);
   }

   // build sdesc for motor controller property
   MotorControllerPropInit(motor_controller_names,sizeof(motor_controller_names)/sizeof(motor_controller_names[0]));

}

static void RegisterVirtualMotions()
{
   // register virtual motions 
   PlyrVMotRegisterMotions();
}

static const sCreatureDesc *g_aCreatureDesc[kNumDPCCreatureTypes]=\
{ 
   &sCrDPCHumanDesc,      // kDPCCRTYPE_Humanoid                     
   &sCrDPCWrenchDesc,     // kDPCCRTYPE_PlayerLimb                   
   &sCrDPCAvatarDesc,     // kDPCCRTYPE_Avatar                       
   
   &sCrOldDogDesc,        // kDPCCRTYPE_OldDog       
   &sCrDogDesc,           // kDPCCRTYPE_Dog          
   &sCrDeepHumanDesc,     // kDPCCRTYPE_DeepHuman    
//   &sCrDPCRopeDesc,       // kDPCCRTYPE_Rope - Not used.
};
 
static const cCreatureFactory *g_aCreatureFactory[kNumDPCCreatureTypes]=\
{
   NULL, // kDPCCRTYPE_Humanoid                           
   NULL, // kDPCCRTYPE_PlayerLimb                         
   NULL, // kDPCCRTYPE_Avatar                             
         
   NULL, // kDPCCRTYPE_OldDog                             
   NULL, // kDPCCRTYPE_Dog                                
   NULL, // kDPCCRTYPE_DeepHuman                          
//   NULL, // kDPCCRTYPE_Rope - Not used.
};

static const char *g_aCreatureNames[kNumDPCCreatureTypes]=\
{ 
   "Humanoid",       // kDPCCRTYPE_Humanoid                           
   "Wrench",         // kDPCCRTYPE_PlayerLimb                         
   "Avatar",         // kDPCCRTYPE_Avatar                             
   "OldDog",         // kDPCCRTYPE_OldDog                             
   "Dog",            // kDPCCRTYPE_Dog                                
   "DeepHuman",      // kDPCCRTYPE_DeepHuman                          
//   "Rope",           // kDPCCRTYPE_Rope - Since we don't use the rope.
};

void DPCCreaturesInit()
{
   SetupControllers();
   RegisterVirtualMotions();
   CreaturesInit(kNumDPCCreatureTypes,g_aCreatureDesc,g_aCreatureNames,g_aCreatureFactory);
   PlayerHandlersInit(kDPCPlayerNumModes);
}

void DPCCreaturesTerm()
{
   CreaturesClose();
   PlayerHandlersTerm();
}

