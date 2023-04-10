/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shksloop.cpp,v 1.57 1999/11/19 14:58:22 adurant Exp $

#include <lg.h>
#include <loopapi.h>
#include <config.h>
#include <res.h>

#include <dispbase.h>  
#include <loopmsg.h>
//#include <objmsg.h>
#include <dbasemsg.h>

#include <netman.h>

#include <ailoop.h>
#include <biploop.h>
#include <simloop.h>
#include <shkloop.h>
#include <physloop.h>
#include <plyrloop.h>
#include <objloop.h>
#include <gen_bind.h>
#include <scrnmode.h>

#include <uiloop.h>
#include <netloop.h>
#include <gamemode.h>
#include <transmod.h>

#include <headmove.h>
#include <shkgame.h>
#include <playrobj.h>
#include <simstate.h>
#include <simflags.h>
#include <prjctile.h>
//#include <netcfg.h>
#include <gamemode.h>
#include <shkmulti.h>
#include <shknet.h>
#include <frobctrl.h>
#include <shkammov.h>

#include <prjctile.h>
#include <particle.h>

//#include <appnet.h>
//#include <pklogin.h>

#include <label.h>
#include <hpprop.h>
#include <dmgprop.h>
#include <slitprop.h>
#include <doorprop.h>

#include <shkplayr.h>
#include <gunprop.h>
#include <shkpsapi.h>
#include <shkpsipw.h>
#include <shkctrl.h>
#include <shkprop.h>
#include <shkinv.h>
#include <scrptprp.h>
#include <shkpgapi.h>
#include <shkreact.h>
#include <shkcmapi.h>
#include <shkplcst.h>
#include <shkcmobj.h>
#include <shkiface.h>
#include <shkifstr.h>
#include <shkutils.h>

// E3 DEMO
#include <shkhacks.h>

// Must be last header
#include <dbmem.h>

/////////////////////////////////////////////////////////////
// SHOCK SIMULATION LOOP CLIENT
////////////////////////////////////////////////////////////

static bool gMode;

//////////////////
// CONSTANTS
//
// These are just here to separate out boiler-plate code and leave it untouched
//


#define MY_FACTORY ShockSimLoopFactory
#define MY_GUID   LOOPID_ShockSim

// My context data
typedef void Context;

// My state
typedef struct _StateRecord
{
   Context* context; // a pointer to the context data I got.
   BOOL in_game_mode;
   // State fields go here
} StateRecord;

static IPlayerGun *g_pPlayerGun = NULL;
static IShockCamera *g_pShockCamera = NULL;

////////////////////////////////////////
//
// LOOP/DISPATCH callback
// Here's where we do the dirty work.
//

static void db_message(DispatchData* msg)
{
   msgDatabaseData data;
   data.raw = msg->data;

   switch (DB_MSG(msg->subtype))
   {
      case kDatabaseReset:
         PsiPowersReset();
         {
            AutoAppIPtr(PlayerGun);
            pPlayerGun->Off();
         }
         break;
      case kDatabasePostLoad:
      case kDatabaseDefault:
         InitProjectileArchetype();
         InitParticleArchetype();
         ShockInvReset();
         //gChar->ClearEquip();
         ShockReactionsPostLoad();

         // Clear some variables that will get awkward if we begin running
         // frames without the sim on (which will happen in multiplayer).
         frobWorldSelectObj = OBJ_NULL;
         g_ifaceFocusObj = OBJ_NULL;
         gHelpString[0] = '\0';
         break;
   }
}

void EquipWeapon(void)
{
   if (PlayerObjectExists())
   {
      ObjID gunobj;
      //AutoAppIPtr(PlayerGun);
      AutoAppIPtr(ShockPlayer);
      gunobj = pShockPlayer->GetEquip(PlayerObject(),kEquipWeapon);
      //pPlayerGun->Set(gunobj);
      pShockPlayer->SetWeaponModel(gunobj,FALSE);
      ShockAmmoRefreshButtons();
   }
}

void UnequipWeapon(void)
{
   if (PlayerObjectExists())
   {
      AutoAppIPtr(ShockPlayer);
      pShockPlayer->SetWeaponModel(OBJ_NULL,FALSE);
   }
}

#pragma off(unreferenced)
static eLoopMessageResult LGAPI _LoopFunc(void* data, eLoopMessage msg, tLoopMessageData hdata)
{
   // useful stuff for most clients
   eLoopMessageResult result = kLoopDispatchContinue; 
   StateRecord* state = (StateRecord*)data;
   LoopMsg info;

   info.raw = hdata; 

   AutoAppIPtr(ShockPlayer);

   switch(msg)
   {
      case kMsgAppInit:
         {
            shock_init_game();
            //ResOpenFile("shkres.res");
            g_pPlayerGun = AppGetObj(IPlayerGun);
            g_pShockCamera = AppGetObj(IShockCamera);
            ShockNetInit();
         }
         break;

      case kMsgAppTerm:
         // gee, maybe we should terminate some stuff here
         // well, now we do!
         shock_term_game();
         SafeRelease(g_pPlayerGun);
         SafeRelease(g_pShockCamera);
         ShockNetTerm();
         break;

      case kMsgEnterMode:
         state->in_game_mode = IsEqualGUID(*info.mode->to.pID,LOOPID_GameMode);
         if (state->in_game_mode)
         {
            // @HACK: start player with pistol and shotgun
            /*
            gChar->AddGun(0, (Label*)"Pistol");
            gChar->AddGun(1, (Label*)"Shotgun");
            gChar->SelectGun(0);
            */
            // E3 DEMO
            shock_init_hacks();
            // center so mouse look around will work
            headmoveInit();
            EquipWeapon();
            shock_start_gamemode();
            // since we may have updated the difficulty level
            pShockPlayer->RecalcData(PlayerObject());
         }
         break;
      case kMsgExitMode:
         if (state->in_game_mode)
         {
            shock_end_gamemode();
            shock_kill_hacks();
            UnequipWeapon();
         }
         break;

      case kMsgNormalFrame:
         if (SimStateCheckFlags(kSimGameSpec))
         {
            shock_check_keys();
            AutoAppIPtr(PlayerPsi);
            pPlayerPsi->Frame(info.frame->dTicks);
            g_pShockCamera->Frame();
            g_pPlayerGun->Frame(info.frame->dTicks);
            AutoAppIPtr(CameraObjects);
            pCameraObjects->Frame(info.frame->dTicks);

            shock_sim_update_frame(info.frame->dTicks);

            // The player started a quicksave, and we've delayed a frame
            // in order to display a message:
            if (gQuickSaveHack > 0)
            {
               gQuickSaveHack--;
               if (gQuickSaveHack == 0)
                  ShockQuickSave();
            }

            // We've changed levels, so let's do a save now:
            if (gTransSaveHack)
            {
               gTransSaveHack = FALSE;

               // If we're networked, then only the host decides to save.
               // As a side-effect, this will tell everyone else to save
               // as well.
               AutoAppIPtr(NetManager);
               if (!pNetManager->IsNetworkGame() ||
                   pNetManager->AmDefaultHost())
               {
                  char temp[255];
                  ShockStringFetch(temp,sizeof(temp),"QuickSaveName","misc");
                  ShockSaveGame(-1,temp);
               }
            }
         }

         // Some other player initiated a save, and we've delayed a frame
         // in order to display a message. This will *often* happen when
         // the sim is paused...
         if (gRemoteSaveHack > 0)
         {
            gRemoteSaveHack--;
            if (gRemoteSaveHack == 0)
            {
               DoShockSaveGame(gRemoteSaveSlot, gpRemoteSaveDesc);
               if (gpRemoteSaveDesc)
               {
                  free(gpRemoteSaveDesc);
                  gpRemoteSaveDesc = NULL;
               }
            }
         }
         break;

      case kMsgDatabase:
         db_message(info.dispatch);
         break;

         // MAHK 7/2/99  Why is suspend/resume totally divergent from exit/enter?
         // I am fixing this so that it does nothing in non-game modes,
         // since our db_message implies that we are in the base mode.
         // I bet there are other suspend/resume bugs, but fuck, we
         // were supposed to ship this yesterday.     
      case kMsgSuspendMode:
         if (state->in_game_mode)
         {
            if (shock_mouse)
            {
               gMode = TRUE;
               MouseMode(FALSE,FALSE);
            }
            else
               gMode = FALSE;
            UnequipWeapon();
         }
         break;
      case kMsgResumeMode:
         if (state->in_game_mode)
         {
            if (gMode)
            {
               MouseMode(TRUE,FALSE);
               gMode = FALSE;
            }
            else
            {
               sScrnMode smode;
               int centerx,centery;
               ScrnModeGet(&smode);
               centerx = smode.w / 2; //grd_visible_canvas->bm.w / 2;
               centery = smode.h / 2; //grd_visible_canvas->bm.h / 2;
               mouse_put_xy(centerx, centery);
            }
            EquipWeapon();
            // since we may have updated the difficulty level
            pShockPlayer->RecalcData(PlayerObject());
         }
         break;
      case kMsgEnd:
         Free(state);
         break;   
   }
   return result;
}

////////////////////////////////////////////////////////////
//
// Loop client factory function.
//

#pragma off(unreferenced)
static ILoopClient* LGAPI _CreateClient(const sLoopClientDesc* desc, tLoopClientData data)
{
   StateRecord* state;
   // allocate space for our state, and fill out the fields
   state = (StateRecord*)Malloc(sizeof(StateRecord));
   state->context = (Context*)data;

   return CreateSimpleLoopClient(_LoopFunc,state,desc);
}
#pragma on(unreferenced)

///////////////
// DESCRIPTOR
//

sLoopClientDesc ShockSimLoopClientDesc =
{
   &MY_GUID,                           // GUID
   "Shock Simulation",                    // NAME        
   kPriorityNormal,                    // PRIORITY
   kMsgEnd | kMsgsMode | kMsgsFrameMid | kMsgsAppOuter | kMsgDatabase,   // INTERESTS

   kLCF_Callback,
   _CreateClient,
   
   NO_LC_DATA,
   
   {
      {kConstrainAfter, &LOOPID_Physics, kMsgsFrame},
      {kConstrainBefore, &LOOPID_SimFinish, kMsgsFrame},
      {kConstrainAfter, &LOOPID_UI, kMsgsMode},
      {kConstrainAfter, &LOOPID_Game, kMsgsMode},
      {kConstrainAfter, &LOOPID_Biped, kMsgsAppOuter},
      {kConstrainBefore, &LOOPID_Biped, kMsgsFrame},
      {kConstrainAfter, &LOOPID_Player, kMsgsFrame},
      {kConstrainBefore, &LOOPID_AI, kMsgsAppOuter},
      //{kConstrainAfter, &LOOPID_Render, kMsgsFrame},
// @Note (toml 04-13-98) dark has the following constraint for the equivalent client: 
      {kConstrainAfter, &LOOPID_ObjSys, kMsgDatabase},
      {kNullConstraint} // terminator
   }
};


