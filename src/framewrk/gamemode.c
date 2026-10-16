/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/gamemode.c,v 2.119 2000/02/19 13:16:16 toml Exp $

#include <lg.h>
#include <r3d.h>
#include <appagg.h>
#include <loopapi.h>
#include <mouse.h>
#include <2d.h>
#include <res.h>
#include <mprintf.h>
#include <timer.h>
#include <lgsprntf.h>
#include <kbcook.h>
#include <keydefs.h>
#include <uiapp.h>
#include <config.h>
#include <headmove.h>
#include <simtime.h>

#include <hotkey.h>
#include <contexts.h>

#include <gamemode.h>
#include <scrnman.h>
#include <simman.h>
#include <scrnmode.h>
#include <gamescrn.h>
#include <scrnloop.h>
#include <testloop.h>
#include <ailoop.h>
#include <uiloop.h>
#include <biploop.h>
#include <simloop.h>
#include <event.h>
#include <editmode.h>
#include <objpos.h>
#include <physapi.h>
#include <phclimb.h>
#include <command.h>
#include <prjctile.h>
#include <gen_bind.h>
#ifdef NEW_NETWORK_ENABLED
#include <netloop.h>
#endif
#include <physloop.h>
#include <plyrloop.h>
#include <sndloop.h>
#include <rendloop.h>
#include <schloop.h>
#include <objloop.h>
#include <cfgtool.h>
#include <dispapi.h>
#include <psndapi.h>
#include <plyrmode.h>
#include <playtest.h>
#include <sndscrp_.h>

#include <phmoapi.h>

#include <dispbase.h>

#include <editor.h>
#include <camera.h>
#include <editobj.h>
#include <uigame.h>

#include <viewmgr.h>
#include <appsfx.h>
#include <objsys.h>
#include <osysbase.h>
#include <playrobj.h>
#include <simstate.h>
#include <simbase.h>
#include <simflags.h>
#include <objhp.h>
#include <vismsg.h>
#include <metasnd.h>
#include <appapi.h>
#include <crwpnlup.h>
#include <missrend.h>
#include <render_backend.h>

// yes, this is gross, when we structure the key bind handling better we can get rid of this
///#include <drkwswd.h>
//#include <drkwbow.h>

#include <plyrspd.h>

////////////////////////////////////////
// GAME MODE IMPLEMENTATION
//

// TODO: have a speed input, attach light to player, so on

//
// GAME MODE SIMULULATION STATE
//


static SimState gameSimState =
{
   0xFFFFFFFF,  // All flags set
};

#ifdef EDITOR
static BOOL gExitGameModeToEditor = FALSE;

void GameModeExitToEditor(void)
{
   ILoop* looper = AppGetObj(ILoop);

   // The sim menu is pushed on top of game mode.  Pop it now; game mode's
   // resume handler will consume this request and switch the underlying mode
   // to DromEd instead of leaving a suspended game below the editor.
   gExitGameModeToEditor = TRUE;
   ILoop_EndMode(looper,0);
   SafeRelease(looper);
}
#endif

#include <fixtime.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

// lots of stuff here shouldnt be here, really

static void resume_mode(void)
{
   SFX_SetMode(SFX_mode_game);
   // @TODO: perhaps there's a better way to say "no cursor"
   uiHideMouse(NULL);
   uiGameStyleSetup(); 
}

static void start_mode(void)
{  // Wake up all physics models
   PhysWakeUpAll();
   GameCreateGUI();
   metaSndEnterGame();
}

static void suspend_mode(void)
{
   uiShowMouse(NULL);
   SFX_SetMode(SFX_mode_metagame);
}

static void shutdown_mode(void)
{
   metaSndExitGame();
   GameDestroyGUI(); 
   // Update the physics properties to the current state of each
   // object that has a physics model
   PhysUpdateProperties();
   CleanupSoundObjects();
}

////////////////////////////////////////
// Only run the game for a certain number of frames
//

#ifdef PLAYTEST

static int auto_stop = -1; 
void init_auto_stop(void)
{
   auto_stop = -1;
   config_get_int("auto_stop",&auto_stop); 
}


void check_auto_stop()
{
   if (auto_stop > 0)
      auto_stop--; 
   if (auto_stop == 0)
   {
      IApplication* pApp = AppGetObj(IApplication); 
      IApplication_QueryQuit(pApp); 
      SafeRelease(pApp); 
   }
}
#else 
#define init_auto_stop()
#define check_auto_stop()
#endif 

///////////////////////////////////////////////////////////////////

#ifdef PLAYTEST
/////////////////
// commands supported by gamemode - installed in the appinit of the loopfunc way below

static void player_instant_halt(void)
{
   mxs_vector zero;

   PhysStopControlVelocity(PlayerObject());
   PhysStopControlRotationalVelocity(PlayerObject());

   mx_zero_vec(&zero);
   PhysSetVelocity(PlayerObject(), &zero);
}

// add in file load/save of current position hacks
static void player_cam_controls(int mode)
{
   switch (mode)
   {
      case 0: headmoveReset(PlayerCamera(),FALSE); break;
      case 1: 
      {
         switch (CameraGetMode(PlayerCamera()))
         {
            case FIRST_PERSON: 
               CameraDetach(PlayerCamera());
               break;
            case DETACHED:
               CameraAttach(PlayerCamera(), PlayerObject());
               CameraSetMode(PlayerCamera(), TRACKING);
               break;
            case TRACKING:
               CameraAttach(PlayerCamera(), PlayerObject());
               CameraSetMode(PlayerCamera(), THIRD_PERSON);
               break;
            case OBJ_ATTACH:
            case THIRD_PERSON:
               CameraAttach(PlayerCamera(), PlayerObject());
               break;
         }
         break;
      }
      case 2: CameraDetach(PlayerCamera()); break;
      case 3: CameraAttach(PlayerCamera(), PlayerObject()); break;
   }
}

static void player_cam_disk_save(char *str)
{
   CameraSave(PlayerCamera(),str);
}

static void player_cam_disk_load(char *str)
{
   CameraLoad(PlayerCamera(),str);
}

static void player_add_hp(int val)
{
   int hp=0, max_hp=100;
   if (CheatsActive())
   {
      ObjGetHitPoints(PlayerObject(),&hp);
      hp+=val;
      if (ObjGetMaxHitPoints(PlayerObject(),&max_hp))
      {
         if (val==0)    hp=max_hp;
         if (hp>max_hp) hp=max_hp;
      }
      ObjSetHitPoints(PlayerObject(),hp);
   }
}

static void player_cam_obj_attach(ObjID obj)
{
   ObjPos * pPos;
   
   if (!obj)
   {
      player_cam_controls(FIRST_PERSON);
      return;
   }
   
   pPos = ObjPosGet(obj);
   
   if (pPos)
      CameraAttach(PlayerCamera(), obj);
}

// 0 means on, 1 means off
static void set_phys_off(int val)
{
   if (val!=0)
   {
      PhysSetGravity(PlayerObject(), FALSE);
      PhysSetBaseFriction(PlayerObject(), 320.0);
   }
   else
   {   
      PhysSetGravity(PlayerObject(), TRUE);
      PhysSetBaseFriction(PlayerObject(), 0.0);
   }
}

static Command gamemode_keys[] =
{
   {"halt_player",FUNC_VOID,player_instant_halt,"stop all player motion/controls"},
   {"player_cam_control",FUNC_INT,player_cam_controls,"0 reset pos, 1 next mode, 2 detach, 3 attach, 4 save, 5 load"},
   {"cam_attach",FUNC_INT,player_cam_obj_attach,"attach camera to object"},
   {"add_hp", FUNC_INT, player_add_hp, "give player # hp" },

   // should these work in edit mode too?  what is player cam in gamemode?  or should it just know?
   {"player_cam_save", FUNC_STRING, player_cam_disk_save, "save player camera to ARG, or drom000.cam" },
   {"player_cam_load", FUNC_STRING, player_cam_disk_load, "load ARG or cam000.loc as player camera" },

   {"unfly", FUNC_INT, set_phys_off },
};
#endif

////////////////////////////////////////
// LOOPMODE DESCRIPTOR
//

#define GAMESPEC_RESERVED_CLIENT 0

static tLoopClientID* _ModeClients[] =
{
   &GUID_NULL,          // reserved for gamespec clients
   &LOOPID_ScrnMan,
   &LOOPID_UI,
   &LOOPID_Game,
   &LOOPID_Biped,
   &LOOPID_AI,
   &LOOPID_Player,
   &LOOPID_Physics,
   &LOOPID_SimFinish,
   &LOOPID_Sound,
   &LOOPID_Render,
   &LOOPID_Schema,
   &LOOPID_CreatureWeapon,

#ifdef NEW_NETWORK_ENABLED
   &LOOPID_Network,
   &LOOPID_NetworkReset,
   &LOOPID_ObjNet,
   &LOOPID_NetworkSim,
   &LOOPID_Ghost,
#endif
};

sLoopModeDesc GameLoopMode =
{
   { &LOOPID_GameMode, "Game Mode"},
   _ModeClients,
   sizeof(_ModeClients)/sizeof(_ModeClients[0]),
};

////////////////////////////////////////
// CONTEXT FOR LOOPMODE
//

#ifdef NEW_NETWORK_ENABLED
NetLoopContext netdata = { TRUE, }; // connect on mode enter
NetResetLoopContext netresetdata = { TRUE, };
ObjNetLoopContext objnetdata = { TRUE, };
NetSimLoopContext netsimdata = { TRUE, };
GhostLoopContext ghostdata = { TRUE, };
#endif


static ScrnManContext scrndata = 
{
   NULL, 
};

// Fit mode owns a runtime screen description whose dimensions follow the
// drawable window client. Keep it separate from gGameScrnMode so resizing a
// window does not overwrite the fixed resolution the user selected for when
// Fit is disabled.
static sScrnMode gFitScreenMode;
static BOOL gFitResizeRestart = FALSE;

static uiLoopContext _uidata =
{
   NULL, 
};



static sLoopModeInitParm _InitContext[] =
{
   { &LOOPID_ScrnMan, (tLoopClientData)&scrndata},
   { &LOOPID_UI, (tLoopClientData)&_uidata},

   { NULL, } // terminator
};

////////////////////////////////////////
// INSTANTIATOR
//

static sLoopInstantiator _instantiator =
{
   &LOOPID_GameMode,
   mmGameMission,
   _InitContext,
};

static BOOL build_fit_screen_mode(sScrnMode* mode)
{
   const sScrnMode* selected = GetGameScreenMode();
   int fit = 0;
   int width;
   int height;
   int registered;

   if (!config_get_int("game_screen_fit", &fit) || !fit ||
#ifndef __ANDROID__
       !(selected->flags & kScrnModeWindowed) ||
#endif
       !RenderBackendGetSettledClientSize(&width, &height) ||
       width < 400 || height < 300 || width > 8191 || height > 8191)
      return FALSE;

   registered = gr_register_mode(width, height, 16);
   if (registered < 0)
      return FALSE;
   grd_mode_info[registered].flags |= GRM_CAN_WINDOW;

   *mode = *selected;
   mode->valid_fields |= kScrnModeDimsValid | kScrnModeBitDepthValid;
   mode->w = width;
   mode->h = height;
   mode->bitdepth = 16;
   return TRUE;
}

sLoopInstantiator* DescribeGameMode(GameMinorMode minorMode, GameModeDesc* desc)
{
   if (minorMode != mmGameNoChange)
   {
      _instantiator.minorMode = minorMode;
   }
   if (desc != NULL)
   {
      if (desc->scrnmode != NULL)
         SetGameScreenMode(desc->scrnmode); 
   }

   if (build_fit_screen_mode(&gFitScreenMode))
      scrndata.mode_params.preferred = &gFitScreenMode;
   else
      scrndata.mode_params.preferred = (sScrnMode*)GetGameScreenMode();

   return &_instantiator;
}

static BOOL resize_fit_canvas_to_client(void)
{
   sScrnMode current;
   sLoopInstantiator* loop;
   ILoop* looper;

   if (!build_fit_screen_mode(&gFitScreenMode))
      return FALSE;

   ScrnModeGet(&current);
   if ((current.valid_fields & kScrnModeDimsValid) &&
       current.w == gFitScreenMode.w && current.h == gFitScreenMode.h)
      return FALSE;
   scrndata.mode_params.preferred = &gFitScreenMode;

   RenderBackendTrace("fit-canvas source=%dx%d client=%dx%d",
                      current.w, current.h,
                      gFitScreenMode.w, gFitScreenMode.h);
   gFitResizeRestart = TRUE;
   looper = AppGetObj(ILoop);
   loop = &_instantiator;
   ILoop_ChangeMode(looper, kLoopModeSwitch, loop);
   SafeRelease(looper);
   return TRUE;
}

////////////////////////////////////////
// GAME SPEC CLIENTS
//

void GameModeSetGameSpecClient(const GUID* ID)
{
   _ModeClients[GAMESPEC_RESERVED_CLIENT] = ID;
};




/////////////////////////////////////////////////////////////
// GAME LOOP CLIENT
////////////////////////////////////////////////////////////


typedef void Context;

typedef struct _StateRecord
{
   Context* context;
   SimState* oldsim;
   BOOL first_frame;
} StateRecord;

////////////////////////////////////////
//
// LOOP/DISPATCH callback
//

#ifndef SHIP
#define HISTO
#endif

#ifdef HISTO

//
// @OPTIMIZE:
// This entire histogram tracking thing is a HACK.
// It doesn't belong here and should definitely be removed
// at some point in the future.  But it might be useful
// for a while.
//
// @HAHA: the future... yea

#define MAX_HISTOGRAM_SLICES 500

ulong frame_time;

static void draw_histogram(int histogram[], int histo_slices, int histo_max, float histo_slice_size)
{
    static int max_val;
    static float hist_accum;
    static int i, j;

    // Find largest value
    max_val = histogram[0];
    for (i=1; i<histo_slices; ++i) {
       if (histogram[i] > max_val)
          max_val = histogram[i];
    }

    // Output actual histogram, in graph format
    for (i=max_val; i>0; --i) {
       for (j=0; j<histo_slices; ++j) {
          if (histogram[j] >= i)
             mprintf("*");
          else
             mprintf(" ");
       }
       mprintf("\n");
    }

    // Output X-axis
    for (i=0, hist_accum=0; i<histo_slices; i++, hist_accum+=histo_slice_size) {
       if ( (((int)hist_accum)%20) < (((int)(hist_accum-histo_slice_size))%20)) {
          if ( (((int)hist_accum)%100) < (((int)(hist_accum-histo_slice_size))%100))
             mprintf("|");
          else
             mprintf("+");
       } else
          mprintf("-");
    }
    mprintf("\n");
}
#endif

static eLoopMessageResult LGAPI _LoopFunc(void* data, eLoopMessage msg, tLoopMessageData hdata)
{
   eLoopMessageResult result = kLoopDispatchContinue;
   StateRecord* state = (StateRecord*)data;
   //   sLoopFrameInfo* info = (msg & kMsgsFrame) ? (sLoopFrameInfo*)hdata : NULL;

   static bool once = FALSE;

#ifdef HISTO
   // Histogram vars
   static bool  histo_defined;
   static int   histo_max, histo_slices;
   static float histo_slice_size;
   static int   histo_index;
   static long  frame_ms, accum_ms;
   static long  frame_count, half_frame_count;
   static int   histogram[MAX_HISTOGRAM_SLICES];
#endif

   static int   i;
   static int   cookie;
   static Region *root;

   switch(msg)
   {
      case kMsgAppInit:
      {
         sScrnMode mode = *GetGameScreenMode(); 
         ScrnModeGetConfig(&mode,"game_"); 
         SetGameScreenMode(&mode);
#ifdef PLAYTEST
         COMMANDS(gamemode_keys,HK_GAME_MODE);
#endif
      }
      break;

      case kMsgAppTerm:
      {
            sScrnMode mode = *GetGameScreenMode(); 
            ScrnModeSetConfig(&mode,"game_"); 

      }
      break;

      case kMsgEnterMode:
      {
         int fit = 0;
         config_get_int("game_screen_fit", &fit);
         // DromEd and the retail game share this loop client. Fixed modes are
         // aspect-fitted to the host; Fit mode replaces the logical canvas
         // with the settled client size so its CPU HUD overlay stays 1:1.
         RenderBackendSetScaleToWindow(TRUE);
         RenderBackendSetFitToViewport(fit != 0);
         RenderBackendSetPreserveCanvas(FALSE);
#ifdef HISTO
         ILoop * pLoop = AppGetObj(ILoop);
         int profile = 0; 
         config_get_int("game_mode_profile",&profile); 
         ILoop_SetProfile(pLoop, profile, NULL);
         SafeRelease(pLoop);
#endif

         state->oldsim = SimStateGet();
         SimStateSet(&gameSimState);

         init_auto_stop(); 


#ifdef HISTO
         // Check for histogram tracking
         histo_defined = config_is_defined("histo_stats");

         // Set up histogram tracking, if applicable
         if (histo_defined) {
            config_get_int("histo_slices", &histo_slices);
            config_get_int("histo_max", &histo_max);
            histo_slice_size = (float)histo_max / (float)histo_slices;
            frame_count = 0;
            memset(histogram, 0, MAX_HISTOGRAM_SLICES * sizeof(int));
            accum_ms = 0;
         }
#endif
         start_mode();
         state->first_frame = TRUE;
      }
      // Fall through
      case kMsgResumeMode:
      {
         int fit = 0;
         config_get_int("game_screen_fit", &fit);
         RenderBackendSetScaleToWindow(TRUE);
         RenderBackendSetFitToViewport(fit != 0);
         RenderBackendSetPreserveCanvas(FALSE);

#ifdef EDITOR
         if (gExitGameModeToEditor)
         {
            gExitGameModeToEditor = FALSE;
            enter_edit_mode("");
            break;
         }

#endif

         // Panels pause by replacing the active SimState with a zero-flag
         // state. Reassert game ownership on resume so rendering cannot stay
         // disabled after Continue.
         SimStateSet(&gameSimState);

         {
            // Make sure the sim is running...
            ISimManager *pSimMan = AppGetObj(ISimManager);
            ISimManager_StartSim(pSimMan);
            SafeRelease(pSimMan);
         }

         root = GetRootRegion();
         HotkeyContext = HK_GAME_MODE; 
         InstallIBHandler (HK_GAME_MODE, UI_EVENT_KBD_RAW | UI_EVENT_MOUSE | UI_EVENT_MOUSE_MOVE | UI_EVENT_JOY, TRUE);
         mouse_set_relative_mode(TRUE);
         MissionPalSet();
         resume_mode();
         // i love the fall through, i love the control, i love it all
         if (msg==kMsgEnterMode)
            ScriptSoundEnterModeHack();
      }
      break;

      case kMsgExitMode:
      {
#ifdef HISTO
         ILoop * pLoop = AppGetObj(ILoop);
         ILoop_SetProfile(pLoop, 0, NULL);
         SafeRelease(pLoop);
#endif

         {
            ISimManager* pSimMan = AppGetObj(ISimManager); 
            ISimManager_SuspendSim(pSimMan); 
            SafeRelease(pSimMan); 
         }

         SimStateSet(state->oldsim);
         shutdown_mode();
#ifdef HISTO
         if (histo_defined) {
            mprintf("-------------\n");
            mprintf("%ld frames drawn, %ld ms elapsed\n", frame_count, accum_ms);
            mprintf("%g avg fps (%g avg ms)\n",
                     (frame_count * 1000.0) / (float)(accum_ms),
                     (float)(accum_ms) / frame_count);
            i=0;
            half_frame_count = frame_count >> 1;
            while(frame_count > half_frame_count) {
               frame_count -= histogram[i];
               i++;
            }
            mprintf("%d to %d median ms\n", (int)(i*histo_slice_size), (int)((i+1)*histo_slice_size));
            mprintf("-------------\n");
            draw_histogram(histogram, histo_slices, histo_max, histo_slice_size);
         }
#endif
      }
         // fall through 
      case kMsgSuspendMode:
         mouse_set_relative_mode(FALSE);
         RemoveIBHandler ();
         suspend_mode();
         break;
      case kMsgEnd:
         Free(state);
         break;
      case kMsgNormalFrame:
         // @OPTIMIZE: maybe test a boolean
         if (resize_fit_canvas_to_client())
            break;
         if (state->first_frame)
         {
            if (!once)
            {
               process_config_scripts("game_script");
               once = TRUE;
               process_config_scripts("game_always_script");
            }
            else if (!gFitResizeRestart)
               process_config_scripts("game_always_script");
            gFitResizeRestart = FALSE;
            state->first_frame = FALSE;
         }
#ifdef HISTO
         if (histo_defined) {
            frame_ms = frame_time;
            histo_index = (int)((float)frame_ms / histo_slice_size);
            if (histo_index < histo_slices)
               histogram[histo_index]++;
            else
               histogram[histo_slices - 1]++;

            accum_ms += frame_ms;
            frame_count++;
         }
#endif

         check_auto_stop(); 

         break;

      case kMsgVisual:
         uiGameStyleSetup();   // recompute the guiStyle
         break; 

   }
   return result;
}


static ILoopClient* LGAPI _CreateClient(sLoopClientDesc * pDesc, tLoopClientData data)
{
   StateRecord* state;
   //   Assrt((void*)data != NULL);
   state = (StateRecord*) Malloc(sizeof(StateRecord));
   state->context = (Context*)data;

   return CreateSimpleLoopClient(_LoopFunc,state,pDesc);
}

sLoopClientDesc GameLoopClientDesc =
{
   &LOOPID_Game,
   "Generic Game Mode Client",
   kPriorityNormal,
   (ulong)kMsgsMode|kMsgsFrame|kMsgsAppOuter|kMsgEnd|kMsgVisual,

   kLCF_Callback,
   _CreateClient,
   
   NO_LC_DATA,

   {
      { kConstrainAfter, &LOOPID_ScrnMan, kMsgsMode|kMsgsFrame},
      { kConstrainAfter, &LOOPID_UI, kMsgsMode},
      { kConstrainAfter, &LOOPID_Biped, kMsgsMode},
      { kConstrainAfter, &LOOPID_Player, kMsgsMode},
#ifdef NEW_NETWORK_ENABLED
      { kConstrainAfter, &LOOPID_Network, kMsgsMode},
#endif
      { kConstrainAfter, &LOOPID_ObjSys, kMsgsMode},

      {kNullConstraint}
   }
};

