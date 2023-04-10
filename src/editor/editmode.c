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
#include <config.h>
#include <uiapp.h>

#include <hotkey.h>
#include <menus.h>

#include <editmode.h>
#include <gamemode.h>

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

// stuff for camera synch
#include <dbasemsg.h>
#include <dispbase.h>
#include <playrobj.h>
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

extern void UpdateMenuCheckmarks(void);

////////////////////////////////////////
// LOOPMODE DESCRIPTOR
//

//
// Here is the list of the loop clients that this mode uses.
// Most clients are implemented in fooloop.c
//

static bool (*inpbnd_handler)(uiEvent *, Region *, void *);

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
   "editor\\cursor",
};

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
   BOOL in_mode; 
} StateRecord;

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
static char *ProcessEditKey (char *cmd, char *val, BOOL already_down)
{
   return CommandExecute (cmd);
}//end ProcessEditKey ()


////////////////////////////////////////
//
// LOOP/DISPATCH callback
// Here's where we do the dirty work.
//

#pragma off(unreferenced)
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

   info.raw = hdata;

   switch(msg)
   {
      case kMsgResumeMode:
      case kMsgEnterMode:
         pal_update();  // set the palette 
         EditorCreateGUI();
         // run the scripts
         state->first_frame = TRUE;
         state->in_mode = TRUE;

         //input binding stuff
         g_pInputBinder->lpVtbl->GetHandler (g_pInputBinder, &inpbnd_handler);
         uiInstallRegionHandler (root, UI_EVENT_KBD_COOKED, inpbnd_handler, NULL, &cookie);
         g_pInputBinder->lpVtbl->SetMasterProcessCallback (g_pInputBinder, ProcessEditKey);
         g_pInputBinder->lpVtbl->SetContext (g_pInputBinder, HK_BRUSH_EDIT, TRUE);
         HotkeyContext = HK_BRUSH_EDIT;

         StatusEnable();
         gedit_enter();
         if (config_is_defined("editorcam_from_game"))
            synch_edit_camera();
         SetMainMenu("edit");
         UpdateMenuCheckmarks();
         state->from_game = IsEqualGUID(info.mode->from.pID,&LOOPID_GameMode);

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
         SetMainMenu(NULL);
         EditorDestroyGUI();
         StatusDisable();
         
         // Determine if we're going into game mode...
         if (IsEqualGUID(info.mode->to.pID,&LOOPID_GameMode) ||
			    GameToolIsToGameModeGUID(info.mode->to.pID))
         {
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
         time(&ltime);
         memcpy(&time_of_day, localtime(&ltime), sizeof(struct tm));
         StatusField(SF_TIME,asctime(&time_of_day));
         StatusUpdate();
         GFHUpdate(GFH_FRAME);
         if(g_InMotionEditor)
            MotEditUpdate(info.frame->dTicks); 

         if (state->first_frame)
         {
            if (!once)
               process_config_scripts("edit_script");
            once = TRUE;
            process_config_scripts("edit_always_script");

            if (state->from_game)
            {
               RestoreMissionBackup(); 
               RemoveMissionBackup(); 
               RestoreBrushSelection(); 
            }

            state->first_frame = FALSE;

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

#pragma off(unreferenced)
static ILoopClient* LGAPI _CreateClient(sLoopClientDesc * pDesc, tLoopClientData data)
{
   StateRecord* state;
   // allocate space for our state, and fill out the fields
   state = Malloc(sizeof(StateRecord));
   memset(state,0,sizeof(*state));
   state->context = (Context*)data;
   
   return CreateSimpleLoopClient(_LoopFunc,state,&EditorLoopClientDesc);
}
#pragma on(unreferenced)

//
// The loop client descriptor
// 

sLoopClientDesc EditorLoopClientDesc =
{
   &LOOPID_Editor,                        // client's guid
   "Editor client",                       // string name
   kPriorityNormal,                       // priority
   kMsgEnd | kMsgsMode | kMsgsFrame | kMsgVisual | kMsgDatabase | kMsgsAppOuter,   // messages we want

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

   {
      ILoop* looper = AppGetObj(ILoop);
      sLoopInstantiator* loop;

      loop = DescribeEditMode(mmEditNoChange,desc);
      ILoop_ChangeMode(looper,kLoopModeSwitch,loop);

      SafeRelease(looper);
   }
}





