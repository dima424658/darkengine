/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/editmode.c,v 1.73 2000/02/19 13:10:34 toml Exp $

#include <stdio.h>
#include <time.h>

#include <lg.h>
#include <loopapi.h>
#include <appagg.h>
#include <gshelapi.h>
#include <config.h>
#include <uiapp.h>
#include <mode.h>

#include <hotkey.h>
#include <menus.h>

#include <editmode.h>
#include <gamemode.h>
#include <gameapp.h>

#include <cfgtool.h>
#include <brushgfh.h>
#include <swappnp.h>
#include <command.h>
#include <contexts.h>
#include <dispatch.h>
#include <dispbase.h>
#include <vismsg.h>
#include <editbr.h>
#include <editgeom.h>
#include <loopmsg.h>
#include <biploop.h>
#include <resloop.h>
#include <scrnloop.h>
#include <plyrloop.h>
#include <scrnmode.h>
#include <scrnman.h>
#include <status.h>
#include <cmdterm.h>
#include <testloop.h>
#include <gen_bind.h>
#include <ailoop.h>
#include <uiedit.h>
#include <uiloop.h>
#include <viewmgr.h>
#include <dispapi.h>
#include <schloop.h>
#include <backup.h>
#include <motedit.h>
#include <linkdraw.h>
#include <simman.h>
#include <dspchdef.h>
#include <simdef.h>
#include <brestore.h>
#include <palette.h>
#include <rendprop.h>
#include <pgrpprop.h>
#include <render_backend.h>
#include <missiondiag.h>

// stuff for camera synch
#include <dbasemsg.h>
#include <dispbase.h>
#include <playrobj.h>
#include <objpos.h>
#include <iobjsys.h>
#include <camera.h>

// So that we can know whether we're switching into a cDarkPanel
// This wants to be handled *some* other way in the long run:
// #include <drkpanid.h>

#include <gametool.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

// convienience, since doug is lazy, and gedit.h uses vectors and stuff
// these are in gedit.h, but i dont want to need editbr and such in editmode
extern void gedit_enter(void), gedit_exit(void);
extern struct Position* PlayerPositionForRenderTest(void);

extern void UpdateMenuCheckmarks(void);

////////////////////////////////////////
// LOOPMODE DESCRIPTOR
//

//
// Here is the list of the loop clients that this mode uses.
// Most clients are implemented in fooloop.c
//

static tBindHandler inpbnd_handler;

static BOOL input_binder_ui_handler(uiEvent *event, Region *region, void *state)
{
   (void)region;
   (void)state;
   return inpbnd_handler(event);
}

#define GAMESPEC_RESERVED_CLIENT 0

static tLoopClientID* _Clients[] =
{
   &GUID_NULL,    // reserved for gamespec
   &LOOPID_Test,
   &LOOPID_ScrnMan,
   &LOOPID_UI,
   &LOOPID_Res,
   &LOOPID_Biped,
   &LOOPID_EditGeom,
   &LOOPID_Editor,
   &LOOPID_AI,
   &LOOPID_Schema,
};

//
// Here's the actual loopmode descriptor
// It gets added to the loop manager in loopapp.c
//

sLoopModeDesc EditLoopMode =
{
   { &LOOPID_EditMode, "Edit mode"}, 
   _Clients,
   sizeof(_Clients)/sizeof(_Clients[0]),
};

////////////////////////////////////////
// CONTEXT FOR LOOPMODE
//

// context for scrnman client
static sScrnMode scrnmode = 
{
   kScrnModeDimsValid|kScrnModeBitDepthValid|kScrnModeFlagsValid,
   640,480, 
   8,
   kScrnModeWindowed,
};

static ScrnManContext _scrndata = 
{ 
   { &scrnmode }, 
}; 

// context for ui client
static uiLoopContext _uidata = 
{
   NULL, // DromEd uses the native Windows arrow in its desktop window.
};

static void show_native_editor_cursor(BOOL show)
{
   IGameShell *shell = AppGetObj(IGameShell);
   int flags = 0;
   IGameShell_GetFlags(shell, &flags);
   if (show)
      flags |= kShowNativeCursor;
   else
      flags &= ~kShowNativeCursor;
   IGameShell_SetFlags(shell, flags);
   SafeRelease(shell);
}

// context for resource sys client
static ResLoopContext _resdata =
{ 
   { 
      "editor.res",  // a list of files to open
   }
};

static sLoopModeInitParm _EditContext[] =
{
   { &LOOPID_ScrnMan, (tLoopClientData)&_scrndata}, 
   { &LOOPID_UI, (tLoopClientData)&_uidata}, 
   { &LOOPID_Res, (tLoopClientData)&_resdata},
   { &LOOPID_EditGeom, (tLoopClientData)1},  // listen to frame events. 

   { NULL, } // terminator
};

////////////////////////////////////////////////////////////
// GAME SPEC CLIENT INSTALLATION

void EditModeSetGameSpecClient(const GUID* ID)
{
   _Clients[GAMESPEC_RESERVED_CLIENT] = ID;
}


////////////////////////////////////////
// INSTANTIATOR
//

static sLoopInstantiator _instantiator = 
{
   &LOOPID_EditMode,
   mmEditBrush,
   _EditContext,
};

//
// This function fills out and returns the instantiator for this mode, based on
// an "edit mode descriptor structure."  The point of this is to hide the list 
// of loop clients behind the "edit mode" abstraction barrier.  So a system 
// that wants to change to edit mode doesn't need to know which loop client the 
// mode uses to keep track of screen res, etc. 
// 

sLoopInstantiator* DescribeEditMode(EditMinorMode minorMode, EditModeDesc* desc)
{
   if (minorMode != mmEditNoChange)
   {
      _instantiator.minorMode = minorMode;
   }
   if (desc != NULL)
   {
      if (desc->scrnmode)
         ScrnModeCopy(&scrnmode,desc->scrnmode,kScrnModeAllValid);
   }

   return &_instantiator;
}





/////////////////////////////////////////////////////////////
// EDITOR LOOP CLIENT
////////////////////////////////////////////////////////////

// this is a generic loop client for all sorts of stuff that only happens in editor
// mode and doesn't quite merit its own loop client.

// This client has no context data.  If it did, this type would not be void.
// Your client could have any type it wants.  
typedef void Context;

// The client stores all its state here, plus a pointer to its context. 
// This client happens to have no state.

typedef struct _StateRecord
{
   Context* context;
   BOOL first_frame;
   BOOL from_game;
   BOOL from_editor;
   BOOL in_mode; 
} StateRecord;

static mxs_ang degrees_to_ang(float degrees)
{
   return (mxs_ang)(degrees * (65536.0 / 360.0));
}

static void configure_render_test_view(void)
{
   char value[160];
   char diagnostic[256];
   mxs_vector position = { 0, 0, 0 };
   mxs_angvec facing = { 0, 0, 0 };
   Position *player_start = NULL;
   IObjectSystem *object_system = AppGetObj(IObjectSystem);
   ObjID start_object = IObjectSystem_GetObjectNamed(object_system,
                                                     "Starting Point");
   float pitch = -35.0;
   float heading = 0.0;
   float roll = 0.0;

   if (start_object != OBJ_NULL)
      player_start = ObjPosGet(start_object);
   if (!player_start)
      player_start = PlayerPositionForRenderTest();
   if (player_start)
   {
      position = player_start->loc.vec;
      facing = player_start->fac;
      heading = (float)facing.tz * (360.0f / 65536.0f);
   }
   SafeRelease(object_system);

   if (config_get_raw("render_test_pos",value,sizeof(value)))
      sscanf(value,"%f,%f,%f",&position.x,&position.y,&position.z);

   // User-facing order is pitch, heading, roll in degrees. Dark stores those
   // axes as ty, tz, tx respectively.
   if (config_get_raw("render_test_angles",value,sizeof(value)))
      sscanf(value,"%f,%f,%f",&pitch,&heading,&roll);
   facing.tx = degrees_to_ang(roll);
   facing.ty = degrees_to_ang(pitch);
   facing.tz = degrees_to_ang(heading);

   vm_set_cur_region(0);
   vm_set_cur_camera(0);
   vm_set_3d(0,TRUE);
   vm_set_render_mode(0,RM_SOLID_PORTAL);
   vm_set_location(0,&position);
   vm_set_facing(0,&facing);
   vm_redraw();
   snprintf(diagnostic,sizeof(diagnostic),
      "Render-test camera: source object %d, position %.3f,%.3f,%.3f, "
      "angles %.3f,%.3f,%.3f",start_object,position.x,position.y,
      position.z,pitch,heading,roll);
   diagnostic[sizeof(diagnostic)-1]='\0';
   MissionDiagnosticsIssue("INFO","editor-view",diagnostic);
   RenderBackendTrace(
      "render-test-view pos=%.3f,%.3f,%.3f angles=%.3f,%.3f,%.3f",
      position.x,position.y,position.z,pitch,heading,roll);
}

////////////////////////////////////////
// Database message handler
//

static void synch_edit_camera(void)
{
   // actually write the current camera pos back into the editor spotlight
   mxs_vector *plypos;
   mxs_angvec *plyang;
   Camera* player_cam = PlayerCamera();
   if (vm_spotlight_loc(&plypos,&plyang))
      CameraGetLocation(player_cam, plypos, plyang);
}

static void db_message(DispatchData* msg)
{
   msgDatabaseData data;
   data.raw = msg->data;
   switch (DB_MSG(msg->subtype))
   {
      case kDatabaseReset:
         GFHSetCurrentBrush(NULL);
         break;
      case kDatabasePostLoad:
         if ((msg->subtype & (kObjPartConcrete|kObjPartBriefcase)) == kObjPartConcrete)
            if (!BackupLoading())
               synch_edit_camera();

         if (!(msg->subtype & kFiletypeAll))
            pal_update(); 
         break;

      case kDatabaseDefault:
         pal_update(); 
         break; 

   }
}



/*
----------------------------------------
Processes keys sent from input binder
----------------------------------------
*/
static char *ProcessEditKey (const char *cmd, const char *val, BOOL already_down)
{
   (void)val;
   (void)already_down;
   return CommandExecute (cmd);
}//end ProcessEditKey ()


////////////////////////////////////////
//
// LOOP/DISPATCH callback
// Here's where we do the dirty work.
//

static eLoopMessageResult LGAPI _LoopFunc(void* data, eLoopMessage msg, tLoopMessageData hdata)
{
   int cookie;
   Region* root = GetRootRegion();
   // useful stuff for most clients
   eLoopMessageResult result = kLoopDispatchContinue; 
   StateRecord* state = (StateRecord*)data;
   LoopMsg info;

   // our specific stuff
   struct tm time_of_day;
   time_t ltime;
   static bool once = FALSE;
   static bool start_game_mode_consumed = FALSE;
   static bool render_test_configured = FALSE;

   info.raw = hdata;

   switch(msg)
   {
      case kMsgResumeMode:
      case kMsgEnterMode:
         // Editor view panes use incremental CPU-canvas redraws.  Game mode
         // restores aspect-fit scaling when DromEd switches into play preview.
         RenderBackendSetScaleToWindow(FALSE);
         RenderBackendSetPreserveCanvas(TRUE);
         show_native_editor_cursor(TRUE);
         pal_update();  // set the palette 
         EditorCreateGUI();
         // run the scripts
         state->first_frame = TRUE;
         state->in_mode = TRUE;

         //input binding stuff
         IInputBinder_GetHandler (g_pInputBinder, &inpbnd_handler);
         uiInstallRegionHandler (root, UI_EVENT_KBD_COOKED, input_binder_ui_handler, NULL, &cookie);
         IInputBinder_SetMasterProcessCallback (g_pInputBinder, ProcessEditKey);
         IInputBinder_SetContext (g_pInputBinder, HK_BRUSH_EDIT, TRUE);
         HotkeyContext = HK_BRUSH_EDIT;

         StatusEnable();
         gedit_enter();
         if (config_is_defined("editorcam_from_game"))
            synch_edit_camera();
         SetMainMenu("edit");
         UpdateMenuCheckmarks();
         // Legacy mode setup sizes the frame while the editor menu is
         // detached. Restore the user's exact outer rectangle after putting
         // the menu back so the new canvas and final client area still match.
         EditorRestoreWindowRect();
         // Installing the hook after the menu is in place prevents the
         // menu's client-area adjustment from masquerading as a user resize.
         EditorStartWindowResizeTracking();
         state->from_game = IsEqualGUID(info.mode->from.pID,&LOOPID_GameMode);
          state->from_editor = IsEqualGUID(info.mode->from.pID,&LOOPID_EditMode);

         if (state->from_game)
         {
            ISimManager* pSimMan = AppGetObj(ISimManager); 
            ISimManager_SuspendSim(pSimMan); 
            SafeRelease(pSimMan); 

         }
         ParticleGroupEnterMode(); 

         break;

      case kMsgExitMode:
         if (!state->in_mode)
            break; 
      case kMsgSuspendMode:
         state->in_mode = FALSE; 
         ParticleGroupExitMode(); 
         if(g_InMotionEditor)
            MotEditClose();
         // A resize rebuild removes the menu and screen mode before entering
         // the editor again.  Stop tracking first so those internal WM_SIZE
         // messages cannot schedule another rebuild and create an endless
         // resize/flicker loop.
         EditorStopWindowResizeTracking();
         SetMainMenu(NULL);
         EditorDestroyGUI();
         StatusDisable();
         
         // Determine if we're going into game mode...
         if (IsEqualGUID(info.mode->to.pID,&LOOPID_GameMode) ||
			    GameToolIsToGameModeGUID(info.mode->to.pID))
         {
            show_native_editor_cursor(FALSE);
            ISimManager* pSimMan = AppGetObj(ISimManager); 
            sDispatchMsg msg = { kSimInit}; 

            gedit_exit();
            SaveBrushSelection(); 
            BackupMission(); 
            ISimManager_StartSim(pSimMan); 
            SafeRelease(pSimMan); 
         }


         break;

      case kMsgAppInit:
         InitDrawnRelations();
         break;

      case kMsgAppTerm:
         TermDrawnRelations();
         break;

      case kMsgEnd:
         Free(state);
         break;

      case kMsgNormalFrame:
         if (config_is_defined("render_test") &&
             RenderBackendCaptureComplete())
         {
            quit_game();
            break;
         }
         {
            int width,height;

            if (EditorGetPendingWindowSize(&width,&height))
            {
               sScrnMode current;
               ScrnModeGet(&current);
               if (!(current.valid_fields&kScrnModeDimsValid) ||
                   current.w!=width || current.h!=height)
               {
                  int mode=gr_register_mode(width,height,16);
                  if (mode>=0)
                  {
                     char dimensions[32];
                     grd_mode_info[mode].flags|=GRM_CAN_WINDOW;
                     sprintf(dimensions,"%d,%d",width,height);
                     enter_edit_mode(dimensions);
                     break;
                  }
               }
            }
         }
         time(&ltime);
         memcpy(&time_of_day, localtime(&ltime), sizeof(struct tm));
         StatusField(SF_TIME,asctime(&time_of_day));
         StatusUpdate();
         GFHUpdate(GFH_FRAME);
         cmdterm_redraw();
         if(g_InMotionEditor)
            MotEditUpdate((float)info.frame->dTicks);

         if (state->first_frame)
         {
            if (state->from_game)
            {
               RestoreMissionBackup(); 
               RemoveMissionBackup(); 
            }

            // Object brushes are transient editor views of concrete database
            // objects. The mission is loaded before the first edit-mode enter,
            // so its object-create notifications precede the brush listener.
            // Rebuild them on every completed edit-mode entry, including the
            // initial one and a canvas-size edit-to-edit transition.
            RestoreBrushSelection();

            // Command-line automation may save immediately. Run it only
            // after the transient object brushes have been reconstructed so
            // BRLIST output cannot silently omit every concrete object.
            if (!once)
               process_config_scripts("edit_script");
            once = TRUE;
            process_config_scripts("edit_always_script");

            state->first_frame = FALSE;

            if (!render_test_configured && config_is_defined("render_test"))
            {
               render_test_configured = TRUE;
               configure_render_test_view();
               vm_render_cameras();
            }

            // A command-line playtest must use the same transition as the
            // editor's Game Mode command.  Waiting until this point ensures
            // the mission, editor loop, screen manager, and simulation handoff
            // are initialized before the mode switch is requested.
            if (!state->from_game && !start_game_mode_consumed &&
                config_is_defined("start_game_mode"))
            {
               start_game_mode_consumed = TRUE;
               do_game_switch_loud("");
            }

         }

         break;

      case kMsgVisual:
         uieditStyleSetup();   // recompute the guiStyle
         vm_redraw();
         uieditRedrawAll();
         StatusDrawStringAll();
         break;

      case kMsgDatabase:
         db_message(info.dispatch);
         break;
   }
   return result;
}

// 
// Loop client factory function. 
//

static ILoopClient* LGAPI _CreateClient(sLoopClientDesc * pDesc, tLoopClientData data)
{
   StateRecord* state;
   // allocate space for our state, and fill out the fields
   state = Malloc(sizeof(StateRecord));
   memset(state,0,sizeof(*state));
   state->context = (Context*)data;
   
   return CreateSimpleLoopClient(_LoopFunc,state,&EditorLoopClientDesc);
}

//
// The loop client descriptor
// 

sLoopClientDesc EditorLoopClientDesc =
{
   &LOOPID_Editor,                        // client's guid
   "Editor client",                       // string name
   kPriorityNormal,                       // priority
   (ulong)kMsgEnd | kMsgsMode | kMsgsFrame | kMsgVisual | kMsgDatabase | kMsgsAppOuter,   // messages we want

   kLCF_Callback,
   _CreateClient,
   
   NO_LC_DATA,
   
   {
      { kConstrainAfter, &LOOPID_ScrnMan, kMsgsMode},
      { kConstrainAfter, &LOOPID_UI, kMsgsMode},
      { kConstrainAfter, &LOOPID_Res, kMsgsMode}, 
      { kConstrainAfter, &LOOPID_EditGeom, kMsgsMode}, 

      { kConstrainAfter, &LOOPID_Player, kMsgDatabase}, 
      { kConstrainBefore, &LOOPID_ScrnMan, kMsgVisual},

   //   { kConstrainAfter, &LOOPID_People, kMsgsMode}, 
      { kNullConstraint }
   }
};

////////////////////////////////////////////////////////////
// Command: edit_mode w,h
// Change to edit mode.  w,h are screen dims
// 

void enter_edit_mode(char* args)
{
   EditModeDesc adesc;
   EditModeDesc* desc = &adesc;
   sScrnMode scrnmode = { 0 };  
   int w = 0, h = 0; 

   desc->scrnmode = &scrnmode; 

   sscanf(args,"%d,%d",&w,&h);
   if (w != 0 && h != 0)
   {
      scrnmode.valid_fields |= kScrnModeDimsValid;
      scrnmode.w = w; 
      scrnmode.h = h; 
   }

   // An edit-to-edit mode switch (notably a window resize) destroys all
   // transient object/light brushes.  Remember the current brush before the
   // switch so the first frame in the rebuilt editor can recreate those
   // brushes and restore the user's selection.
   SaveBrushSelection();

   {
      ILoop* looper = AppGetObj(ILoop);
      sLoopInstantiator* loop;

      loop = DescribeEditMode(mmEditNoChange,desc);
      ILoop_ChangeMode(looper,kLoopModeSwitch,loop);

      SafeRelease(looper);
   }
}





