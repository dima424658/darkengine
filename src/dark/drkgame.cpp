/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkgame.cpp,v 1.106 2000/03/09 16:16:14 toml Exp $
// dark specific game features

#include <string.h>
#include <math.h>

#include <2d.h>
#include <mprintf.h>

#include <config.h>
#include <command.h>

#include <init.h>
#include <appname.h>
#include <drkgame.h>
#include <metagame.h>
#include <drkdebrf.h>
#include <bugterm.h>

#include <rect.h>
#include <kbcook.h>
#include <keydefs.h>
#include <res.h>

#include <types.h>
#include <lresname.h>

#include <editor.h>
#include <camera.h>
#include <headmove.h>
#include <player.h>
#include <playrobj.h>
#include <editobj.h>

#include <objsys.h>
#include <osysbase.h>
#include <objquery.h>
#include <objpos.h>
#include <wrtype.h>
#include <lnkquery.h>

#include <scrnman.h>
#include <scrnovls.h>
#include <partprop.h>
#include <objshape.h>
#include <gen_bind.h>

#include <drkcret.h>
#include <drkinvui.h>
#include <drkloop.h>
#include <simman.h>
#include <simdef.h>
#include <dspchdef.h>
#include <contain.h>
#include <rendprop.h>
#include <physapi.h>
#include <mnamprop.h>
#include <darkai.h>
#include <drkbook.h>
#include <pgrpprop.h>
#include <drkuires.h>
#include <scrnloop.h>
#include <drkfixpr.h>

#include <drksound.h>

#include <drkbreth.h>

#include <event.h>
#include <gadbox.h>

#include <prjctile.h>

#include <guistyle.h>
#include <cmdterm.h>
#include <simtime.h>
#include <pick.h>

#include <drk_bind.h>
#include <drkreact.h>
#include <drkinv.h>
#include <drklinks.h>
#include <drkwbow.h>
#include <drkwbdy.h>
#include <drkloot.h>
#include <drkmiss.h>
#include <drkamap.h>
#include <drkldout.h>
#include <drkdiff.h>
#include <drkmislp.h>
#include <drkvo.h>
#include <drkmenu.h>
#include <drksave.h>
#include <drksavui.h>
#include <drkreprt.h>
#include <drkscrm.h> //enter/exit script
#include <drkstats.h>
#include <drkcolor.h>
#include <gamescrn.h>
#include <scrnmode.h>
#include <meshprop.h>

#include <gametool.h>

// for player creation
#include <linkman.h>
#include <relation.h>
#include <linkbase.h>
#include <contain.h>
#include <drkplinv.h>

#include <picklock.h>
#include <drkpwups.h>

#include <drkcmbat.h>
#include <drkmsg.h>

#include <culpable.h>

#include <questapi.h>
#include <drkgoalt.h>

// Goofy alpha hack
#include <imgsrc.h>

// scriptman
#include <scrptbas.h>
#include <scrptapi.h>

// E3 HACKS
#include <propface.h>
#include <propbase.h>
#include <plycbllm.h>
#include <anim_txt.h>

#ifdef NEW_NETWORK_ENABLED
#include <netman.h>
#endif

#include <drkpanl.h>

#include <allocapi.h>

//guns
#include <gunprop.h>
#include <projlink.h>
#include <projprop.h>
#include <gunflash.h>
#include <gunproj.h>

#include <dbmem.h> // must be last included header

#define E398_HACK

// this is the default memory allocation limit
#ifdef EDITOR
#define DEFAULT_MEMORY_CAP (56*1024*1024)
#else
#define DEFAULT_MEMORY_CAP (48*1024*1024)
#endif

static void DarkScriptChangeMode(boolean resuming, boolean suspending);

////////////////////////////////////////////////////////////
// PER-FRAME GAME EXECUTION
//

//
// Sim frame update, called before rendering 
//
void dark_sim_update_frame(void)
{
   int ms=GetSimFrameTime();
   

   ectsAnimTxtTime=GetSimTime();
   ectsAnimTxtUpdateAll();

   headmoveCheck(PlayerCamera(),ms);
   playerHeadControl();
   drkCheckHeadFocus();
   PickFrameUpdate();
   BreathSimUpdateFrame(ms); 

   PlayerCbllmUpdate(ms);

}

static IImageSource* greek_letter = NULL; 

//
// post render frame updates
//
void dark_rend_update_frame(void)
{
   // display the greek letter
#ifdef PLAYTEST
   if (greek_letter)
   {
      grs_bitmap* bm = (grs_bitmap*)greek_letter->Lock(); 
      int x = grd_canvas->bm.w - bm->w; 
      int y = 0 ;
      ScrnLockDrawCanvas();
      gr_bitmap(bm,x,y); 
      ScrnUnlockDrawCanvas();
      greek_letter->Unlock(); 
   }
#endif //   

   DarkMessageUpdateFrame(); 
}



////////////////////////////////////////////////////////////
// GAME UI 
//

//
// In-game command terminal
//

#define NUM_CMD_TERM_LINES 2
#define NUM_BUG_TERM_LINES 10
#define CMD_Y_MARGIN 2

static void build_cmd_term(LGadRoot* root)
{
   Rect r = *LGadBoxRect(root);
   short w,h;

   guiStyleSetupFont(NULL,StyleFontNormal);
   gr_string_size("X",&w,&h);
   guiStyleCleanupFont(NULL,StyleFontNormal);

   r.lr.y = (short)(h * NUM_CMD_TERM_LINES + CMD_Y_MARGIN);
   CreateCommandTerminal(root,&r,kCmdTermHideUnfocused);

   r.lr.y = h * NUM_BUG_TERM_LINES + CMD_Y_MARGIN;
   CreateBugTerminal(root,&r,kCmdTermHideUnfocused);
}

//
// Custom game input handlers
//

// parse the key
static bool game_key_parse(int keycode)
{
   if (keycode&KB_FLAG_DOWN)
   {
   }
   return FALSE;
}

#pragma off(unreferenced)
static bool key_handler_func(uiEvent* ev, Region* r, void* data)
{
   uiCookedKeyEvent* kev = (uiCookedKeyEvent*)ev;
   return game_key_parse(kev->code);
}

#pragma on(unreferenced)

static int key_handler_id;

//
// Game mode initialization
//

void dark_start_gamemode(BOOL resuming)
{
   LGadRoot* root = LGadCurrentRoot();

   uiInstallRegionHandler(LGadBoxRegion(root),UI_EVENT_KBD_COOKED,key_handler_func,NULL,&key_handler_id);
   build_cmd_term(root);

   PickSetFocus(fix_make(grd_canvas->bm.w >> 1, 0),
                fix_make(grd_canvas->bm.h >> 1, 0),
                head_focus_dist2_tol);
   PickSetCanvas();

   DarkScriptChangeMode(resuming,FALSE);
   InvUIEnterMode(resuming); 

#ifdef PLAYTEST
   if (!config_is_defined("herr_rockmusiker"))
      greek_letter = CreateResourceImageSource("intrface\\","greekltr.pcx"); 
#endif // PLAYTEST

   if (gScrnLoopSetModeFailed)
   {
      DarkMessage(FetchUIString("misc","set_mode_failed")); 
   }
}

////////////////////////////////////////////////////////////
// GAME MODE SETUP/CLEANUP
//

//
// Game mode termination
//

void dark_end_gamemode(BOOL suspending)
{
   LGadRoot* root = LGadCurrentRoot();
   uiRemoveRegionHandler(LGadBoxRegion(root),key_handler_id);

   DestroyCommandTerminal();
   DestroyBugTerminal();
   SafeRelease(greek_letter); 
   DarkScriptChangeMode(FALSE,suspending);
   InvUIExitMode(suspending); 
}

//////////////////////////////////////////////////////////////
// COMMANDS
//

int master_overlay=0;

static void drkgame_OverlayToggle(int which)
{
   ScreenOverlaysChange(which,kScreenOverlaysToggle);
}


#ifndef SHIP 
// @HACK: this is for fixing up players with an "old" player object
static void fixup_player(void)
{
   AutoAppIPtr_(ObjectSystem,pObjSys); 
   ObjID old = pObjSys->GetObjectNamed("OldPlayer"); 
   if (old == OBJ_NULL)
   {
      mprintf("No old player object"); 
      return; 
   }

   ObjID marker_arch = pObjSys->GetObjectNamed("Marker"); 

   if (marker_arch == OBJ_NULL) 
      marker_arch = ROOT_ARCHETYPE; 

   ObjID start = pObjSys->BeginCreate(marker_arch,kObjectConcrete); 

   // move to player position 
   ObjPos* pos = ObjPosGet(old); 
   ObjPosCopyUpdate(start,pos); 

   // set up the "PlayerFactory" link to the "garrett" archetype
   ObjID garrett = pObjSys->GetObjectNamed("Garrett"); 
   if (garrett == OBJ_NULL) garrett = ROOT_ARCHETYPE; 

   AutoAppIPtr_(LinkManager,pLinkMan);
   cAutoIPtr<IRelation> pRel ( pLinkMan->GetRelationNamed("PlayerFactory") );
   pRel->Add(start,garrett); 

   // Name the starting point
   pObjSys->NameObject(start,"Starting Point"); 
   
   // move the old player's contents over to the marker
   AutoAppIPtr(ContainSys);
   for (sContainIter* iter = pContainSys->IterStart(old); !iter->finished; pContainSys->IterNext(iter))
      pContainSys->Add(start,iter->containee,iter->type,CTF_NONE); 
   pContainSys->IterEnd(iter); 

   pObjSys->EndCreate(start);
   mprintf("Player Starting Point Created"); 
}
#endif SHIP

static void do_version()
{
   DarkMessage(AppName()); 
}

static void win_mission()
{
  AutoAppIPtr(QuestData);
  pQuestData->Set(MISSION_COMPLETE_VAR,1);
  UnwindToMissionLoop();
}

static Command drk_ui_keys[] =
{
   { "toggle_overlay",  FUNC_INT, drkgame_OverlayToggle, "takes which to toggle" },
   { "dark_version",  FUNC_VOID, do_version, "Display version in game mode." },
   { "win_mission", FUNC_VOID, win_mission, "Win the mission", HK_GAME_MODE },
};

#ifndef SHIP
// none defined presently, and can't have empty static structure - so if 0 it out
static Command drk_debug_keys[] =
{
   { "fixup_player",  FUNC_VOID, fixup_player, "Make a player starting point based on OldPlayer" },

};
#define DebugKeys() COMMANDS(drk_debug_keys,HK_ALL)
#else  // SHIP
#define DebugKeys() 
#endif // SHIP

////////////////////////////////////////////////////////////
// SIM MESSAGE HANDLER
// 

static void sim_msg(const sDispatchMsg* msg, const sDispatchListenerDesc* )
{
   switch (msg->kind)
   {
      case kSimInit: 
         DarkStatInitMission();
         break; 

      case kSimTerm:
         DarkStatFinishMission();
         break;
   }
}

static sDispatchListenerDesc sim_listen = 
{
   &LOOPID_DarkSim,     // my guid
   kSimInit|kSimTerm,   // interests
   sim_msg,
};

static void init_sim_msg()
{
   AutoAppIPtr_(SimManager,pSimMan); 
   pSimMan->Listen(&sim_listen); 
}

/////////////////////////////////////////////////
//game mode script message handlers
/////////////////////////////////////////////////

static void DarkScriptChangeMode(boolean resuming, boolean suspending)
{

   AutoAppIPtr_(ObjectSystem,ObjSys);    
   AutoAppIPtr(ScriptMan);
         
   // Send start to all concrete objects!  Yum!         
   cAutoIPtr<IObjectQuery> query ( ObjSys->Iter(kObjectConcrete));             
   for (; !query->Done(); query->Next())            
   {               
      sDarkGameModeScrMsg msg(query->Object(),resuming,suspending);                  
      pScriptMan->SendMessage(&msg);               
   }      
}


////////////////////////////////////////////////////////////
// CONTAINS LISTENER 
//

static BOOL contain_CB(eContainsEvent ev, ObjID outer, ObjID inner, eContainType , ContainCBData )
{
   switch(ev)
   {
      case kContainRemove:
         ObjSetHasRefs(inner,TRUE);
         if (ObjIsParticleGroup(inner))
            ObjParticleSetActive(inner, TRUE);
         break;
      case kContainAdd:
         ObjSetHasRefs(inner,FALSE); 
         PhysDeregisterModel(inner);
         ObjForceReref(inner);
         if (ObjIsParticleGroup(inner))
            ObjParticleSetActive(inner, FALSE);
         break; 

      case kContainCombine:
	 //if not autoequip, don't refresh view on pickup.
	 if (atof(g_pInputBinder->ProcessCmd("echo $auto_equip"))!=0.0)
	   InvUIRefreshObj(outer); 
         break; 
   }
   return TRUE; 
}

static void setup_contain_CB(void)
{
   AutoAppIPtr(ContainSys);
   pContainSys->Listen(OBJ_NULL,contain_CB,NULL); 
}

////////////////////////////////////////////////////////////
// PLAYER CREATION CALLBACK
//

static void player_create_CB(ePlayerEvent event, ObjID player)
{
   switch (event)
   {
      case kPlayerCreate:
      {
         char buf[80];
         // @TODO: get rid of this
         if (config_get_raw("player_model",buf,sizeof(buf)))
         {
            buf[sizeof(buf)-1] = '\0';
            ObjSetModelName(player,buf);
         }

         PlayerCbllmCreate(); // creates "lower brain" and body
         PhysCreateDefaultPlayer(player);
      }
      break;

      case kPlayerLoad:
         PlayerCbllmCreate(); // creates "lower brain" and body
         break; 

      case kPlayerDestroy:
         PlayerCbllmDestroy(); 
         break; 
   }
}


// Look through the playerfactory links on the level. Ideally, we find
// a link whose data is this player's playernum. If not, we use some other
// link as a default.
static ObjID player_factory_CB(void)
{
   AutoAppIPtr_(LinkManager,pLinkMan);
   cAutoIPtr<IRelation> pRel ( pLinkMan->GetRelationNamed("PlayerFactory") );
   
   ulong myPlayerNum = 0;
#ifdef NEW_NETWORK_ENABLED
   // If this is a multiplayer-capable game, then find our player number.
   // If not, then there's probably only a single PlayerFactory link anyway.
   AutoAppIPtr(NetManager);
   if (pNetManager->IsNetworkGame()) {
      myPlayerNum = pNetManager->MyPlayerNum();
   } else {
      myPlayerNum = 0;
   }
#endif
   
   // LinkID id = pRel->GetSingleLink(LINKOBJ_WILDCARD,LINKOBJ_WILDCARD); 
   LinkID defaultFactory = LINKID_NULL;
   LinkID id = LINKID_NULL;
   ILinkQuery *pQuery = pRel->Query(LINKOBJ_WILDCARD,LINKOBJ_WILDCARD);
   if (pQuery == NULL)
      return OBJ_NULL;

   // Run through the PlayerFactory links, and see if any of them work
   for ( ; (!pQuery->Done()) && (id == LINKID_NULL); pQuery->Next()) 
   {
      int *factoryPtr = (int *) pQuery->Data();
      if (factoryPtr == NULL) {
         // It's an old factory with no player num; use it as the
         // default
         defaultFactory = pQuery->ID();
      } else {
         int factoryNum = *factoryPtr;
         if (factoryNum == myPlayerNum) {
            // Got the right one
            id = pQuery->ID();
         } else if (defaultFactory == LINKID_NULL) {
            // We don't have any default yet, so try this one
            defaultFactory = pQuery->ID();
         }
      }
   }

   SafeRelease(pQuery);

   if (id == LINKID_NULL) {
      // We didn't find an appropriate one, so fall back on a default
      if (defaultFactory == LINKID_NULL) {
         // There aren't *any* factories on this level!
         return OBJ_NULL;
      } else {
         id = defaultFactory;
      }
   }

   sLink link; 
   pRel->Get(id,&link); 
   return link.source; 
}

static void setup_player_CB()
{
   PlayerCbllmInit();
   HookPlayerCreate(player_create_CB);
   HookPlayerFactory(player_factory_CB); 
}

////////////////////////////////////////////////////////////
// GAME SCREEN MODE CONSTRAINT CB
//

static void gamescreen_cb(sScrnMode* mode)
{
   if (!(mode->valid_fields & kScrnModeFlagsValid))
   {
      mode->flags = kScrnModeFullScreen|kScrnMode2dDriver|kScrnMode3dDriver; 
      mode->valid_fields |= kScrnModeFlagsValid;
   }

   mode->bitdepth = 16; // always 16 bit

   mode->valid_fields |= kScrnModeBitDepthValid; 

}

////////////////////////////////////////
// Init culpability
struct sCulpRelations
{
   const char* name; 
   ulong flags; 
}; 

static sCulpRelations culp_rels[] = 
{
   {"Contains", 0 }, 
   { "~Firer", kCulpTransitive},
   { "CurWeapon", 0 }, 
}; 

#define NUM_CULP_RELS (sizeof(culp_rels)/sizeof(culp_rels[0]))

// Set up culpability relations for dark
static void dark_init_culpability()
{
   // set up culpability listeners
   AutoAppIPtr_(LinkManager,pLinkMan); 
   for (int i = 0; i < NUM_CULP_RELS; i++)
   {
      sCulpRelations& rel = culp_rels[i]; 
      cAutoIPtr<IRelation> pRel = pLinkMan->GetRelationNamed(rel.name); 
      AddCulpabilityRelation(pRel,rel.flags); 
   }
}

// Init the Dark-specific game tools.
void DarkToolsInit(void)
{
   IGameTools* pGameTools = AppGetObj(IGameTools);
   pGameTools->SetIsToGameModeGUIDCallback(DarkIsToGameModeGUID);
   SafeRelease(pGameTools);
}

//////////////////////////////////////////////////////////////
// ONE-TIME APP INIT/TERM
//

void dark_init_game(void)
{
   COMMANDS(drk_ui_keys,HK_GAME_MODE);
   DebugKeys();

   config_get_int("drkgame_overlay",&master_overlay);

   // @TODO: perhaps there should be a good runtime way of figuring 
   // out how to start in editor 

#ifdef EDITOR
   BOOL start_metagame = config_is_defined("start_metagame")
      || config_is_defined("play"); 
#else
   BOOL start_metagame = TRUE;
#endif 

   // set the memory allocation cap
   sAllocLimits allocLimits;
   AllocGetLimits(&allocLimits);
   long memoryCap = (allocLimits.allocCap > DEFAULT_MEMORY_CAP) ? allocLimits.allocCap : DEFAULT_MEMORY_CAP;         // quick kids!  lets put on our memory caps!
   if (config_is_defined("memory_cap"))
      config_get_int( "memory_cap", &memoryCap );
   AllocSetAllocCap( memoryCap );

   ConstrainGameScreenMode(gamescreen_cb); 

   // This needs to come very early
   // to get called before 16 bit images are loaded
   DarkColorInit();

   MetaGameInit();
   MissionLoopInit(); 

   if (start_metagame)
      gPrimordialMode = (sLoopInstantiator*)DescribeMissionLoopMode(); 
   
   init_sim_msg(); 
   setup_contain_CB(); 
   InitDarkReactions();
   DarkInitLinks();
   DarkInitProps();
   drkInvInit();
   DarkCreaturesInit();
   PickLockInit();
   BeltLinkInit();
   AltLinkInit();
   BowInit(); 
   BodyCarryInit();
   DrkPowerupInit();
   InvUIInit(); 
   setup_player_CB();
   DarkSoundInit(); 
   BreathSimInit(); 
   DarkCombatInit(); 
//new gun init code
   BaseGunDescPropertyInit();
   AIGunDescPropertyInit();
   GunStatePropertyInit();
   ProjectileLinksInit();
   ProjectilePropertyInit();
   GunFlashInit();
   GunProjectileInit();
//wow,thats a lot for just guns AMSD
   DarkLootInit();
   init_fixture_prop();
   DebriefInit(); 
   MissionDataInit(); 
   MapSourceInfoInit();
   DarkAutomapInit(); 
   LoadoutInit(); 
   DarkDifficultyInit(); 
   DarkMessageInit(); 
   DarkAIInit();
   DarkBookInit(); 
   DarkVoiceOverInit(); 
   DarkMenusInit(); 
   DarkSaveGameInit(); 
   DarkSaveInitUI();
   DarkReportInit();
   DarkStatInit();
   dark_init_culpability();
   SetGameIBVarsFunc (InitDarkIBVars);
   DarkToolsInit();
}

void dark_term_game(void)
{
   PickLockTerm();
   BeltLinkTerm();
   AltLinkTerm();
   BowTerm();
   BodyCarryTerm();
   DrkPowerupTerm();
   InvUITerm(); 
   DarkSoundTerm(); 
   BreathSimTerm(); 
   DarkCombatTerm();
//releasing the gun stuff, although they really should have their own
//functions... 
   SafeRelease(g_baseGunDescProperty);
   SafeRelease(g_aiGunDescProperty);
   SafeRelease(g_pGunStateProperty);
   ProjectileLinksTerm();
   SafeRelease(g_pProjectileProperty);
   GunFlashTerm();
   GunProjectileTerm();
// Guns released.
   DarkLootTerm(); 
   term_fixture_prop();
   DebriefTerm(); 
   MissionDataTerm(); 
   MapSourceInfoTerm();
   DarkAutomapTerm(); 
   LoadoutTerm(); 
   MetaGameTerm(); 
   DarkDifficultyTerm(); 
   DarkMessageTerm(); 
   DarkCreaturesTerm();
   MissionLoopTerm(); 
   DarkAITerm();
   DarkBookTerm(); 
   DarkVoiceOverTerm(); 
   DarkMenusTerm(); 
   DarkSaveGameTerm(); 
   DarkSaveTermUI();
   DarkReportTerm();
   DarkStatTerm();
}


