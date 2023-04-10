/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/sndloop.cpp,v 1.27 2000/02/28 17:28:28 toml Exp $

#include <lg.h>
#include <loopapi.h>

// messages I get
#include <dispatch.h>
#include <dispbase.h>
#include <loopmsg.h>
#include <dbasemsg.h>

#include <iobjsys.h>
#include <objtype.h>
#include <objdef.h>
#include <objnotif.h>
#include <appagg.h>
#include <simdef.h>
#include <simman.h>
#include <dspchdef.h>

// loops i use
#include <ailoop.h>
#include <objloop.h>
#include <schloop.h>
#include <simloop.h>
#include <sndloop.h>

// Actual stuff I need to do my thing
#include <gamemode.h>
#include <camera.h>
#include <physapi.h> //for camera related stuff ugh.
#include <matrixs.h>
#include <simstate.h>
#include <simflags.h>
#include <playrobj.h>
#include <appsfx.h>
#include <sndapp.h>
#include <sndprop.h>
#include <sndgen.h>
#include <psndapi.h>
#include <ambprop.h>
#include <ambient.h>
#include <schema.h>
#include <config.h>
#include <sndscrp_.h>
#include <songutil.h>
#include <songmiss.h>
#include <sndframe.h>

#include <dbmem.h>

/////////////////////////////////////////////////////////////
// SOUND LOOP CLIENT
////////////////////////////////////////////////////////////

//////////////////
// CONSTANTS
//
// These are just here to separate out boiler-plate code and leave it untouched
//

#define MY_GUID   LOOPID_Sound
extern int sfx_use_channels;

// My context data
typedef void Context;
// My state
typedef struct _StateRecord
{
   Context* context; // a pointer to the context data I got.
   // State fields go here
} StateRecord;

BOOL sgInGameMode = FALSE;

//////////////////////////////////////////////////////////////
// Forward declarations
//

//static void init_sim_msg();

static void _SongStart();
static void _SongStop();
static void _SongPause();
static void _SongResume();

////////////////////////////////////////
//
// Database message handler
//
static void db_message(DispatchData * msg)
{
   msgDatabaseData data;

   data.raw = msg->data;

   switch (DB_MSG(msg->subtype))
   {
      case kDatabaseReset:
         SchemaHaltAll();
         SFX_KillAll(SFX_mode_game);
         AmbientReset();
         ScriptSoundReset();
         SongUtilResetMission();
         break;

      case kDatabaseLoad:  // postload not load?
         if (msg->subtype & kDBMission)
         {
            AmbientLoad(data.load);
            ScriptSoundLoad(data.load);
            SongUtilLoadMission(data.load);
         }
         break;

      case kDatabasePostLoad:
#ifdef OLD_WAY
         if ((msg->subtype & kFiletypeAll)==0)
            ScriptSoundPostLoad();  // secret post post post load
#endif
         break;

      case kDatabaseSave:
         if (msg->subtype & kDBMission)
         {
            AmbientSave(data.save);
            ScriptSoundSave(data.save);
            SongUtilSaveMission(data.load);
         }
         break;
   }
}

////////////////////////////////////////
// Frame update
//

static void do_frame(void)
{
   mxs_vector pos;
   mxs_angvec ang;
   Camera* cam = PlayerCamera();

   // some camera modes (REMOTE_CAM and VIEW_CAM) are normally achieved
   // in play through a remote viewing system.  Player sound should propagate
   // to the player object, not the remote camera object.  If we were doing
   // this in the best way, sounds would propagate to BOTH (ugh), but for
   // Thief 2 purposes we only want to player object, so that's all for now.
   // AMSD
   if (((CameraGetMode(cam) == REMOTE_CAM) || (CameraGetMode(cam) == VIEW_CAM))
       && (PhysObjHasPhysics(PlayerObject())))
     {
       PhysGetModRotation(PlayerObject(),&ang);
       PhysGetModLocation(PlayerObject(),&pos);
     }
   else
     CameraGetLocation(PlayerCamera(), &pos, &ang);
   AmbientRunFrame(&pos);
   if (SimStateCheckFlags(kSimSound))
      SFX_Frame(&pos,&ang);
}

////////////////////////////////////////
//
// Object message handler
//

#pragma off(unreferenced)
static void obj_message(ObjID obj, eObjNotifyMsg msg, void *data)
{
   switch (msg)
   {
      case kObjNotifyDelete:
         SoundHaltObj(obj);
         break;
   }
}
#pragma on(unreferenced)

static void init_obj_message(void)
{
   IObjectSystem* pObjSys = AppGetObj(IObjectSystem);
   sObjListenerDesc desc = { obj_message, NULL };
   IObjectSystem_Listen(pObjSys,&desc);
   SafeRelease(pObjSys);
}

////////////////////////////////////////
//
// LOOP/DISPATCH callback
// Here's where we do the dirty work.
//

#pragma off(unreferenced)
static eLoopMessageResult LGAPI _LoopFunc(void* data, eLoopMessage msg, tLoopMessageData hdata)
{
   // useful stuff for most clients
   eLoopMessageResult result = kLoopDispatchContinue;
   StateRecord* state = (StateRecord*)data;
   LoopMsg info;
   BOOL wasInGameMode;

   info.raw = hdata;

   switch(msg)
   {
      case kMsgDatabase:
         db_message(info.dispatch);
         break;

      case kMsgEnterMode:
      case kMsgResumeMode:
         // If we switch to game mode, start the music.
         wasInGameMode = sgInGameMode;
         sgInGameMode = IsEqualGUID(*info.mode->to.pID,LOOPID_GameMode);
         if ( !(wasInGameMode) && sgInGameMode )
         {
            if ( msg == kMsgEnterMode ) {
               _SongStart();
            } else {
               _SongResume();
            }
         }
         break;

      case kMsgExitMode:
      case kMsgSuspendMode:
         if ( sgInGameMode )
         {
            sgInGameMode = FALSE;
            if ( msg == kMsgExitMode ) {
               _SongStop();
            } else {
               _SongPause();
            }
         }
         break;

      case kMsgNormalFrame:
         if (SimStateCheckFlags(kSimSound)) {
            do_frame();
            // the schema frame needs to be here in order to make the
            // recording of callbacks work properly
            SchemaFrame();
         }
         break;
         
      case kMsgEndFrame:
      {
         if (SimStateCheckFlags(kSimSound)) 
            SNDFrameNextFrame();
         break;
      }

      case kMsgAppInit:
      {
         int vol;
         SFXInit();
         //         init_sim_msg();
         SoundPropsInit();
         SoundGenInit();
         ScriptSoundInit();
         AmbSoundPropInit();
         init_obj_message();
         SNDFrameInit();

         if (config_get_int("master_volume",&vol))
            SFX_SetMasterVolume(vol);
      }
      break;

      case kMsgAppTerm:
      {
         int vol = SFX_GetMasterVolume();
         config_set_int("master_volume",vol);
         config_set_int("sfx_channels",SFX_GetNumChannels());

         SNDFrameTerm();
         SoundPropsClose();
         SFXClose();
         SoundGenShutdown();
         ScriptSoundTerm();
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
static ILoopClient* LGAPI _CreateClient(sLoopClientDesc * pDesc, tLoopClientData data)
{
   StateRecord* state;
   // allocate space for our state, and fill out the fields
   state = (StateRecord*)Malloc(sizeof(StateRecord));
   state->context = (Context*)data;

   return CreateSimpleLoopClient(_LoopFunc,state,pDesc);
}
#pragma on(unreferenced)

///////////////
// DESCRIPTOR
//



sLoopClientDesc SoundLoopClientDesc =
{
   &MY_GUID,                           // GUID
   "Sound Client",                     // NAME
   kPriorityNormal,                    // PRIORITY
   kMsgEnd | kMsgsMode | kMsgsFrameMid | kMsgEndFrame | kMsgDatabase | kMsgsAppOuter,// INTERESTS

   kLCF_Callback,
   _CreateClient,

   NO_LC_DATA,

   {
      {kConstrainAfter, &LOOPID_SimFinish, kMsgsFrame},
      {kConstrainAfter, &LOOPID_ObjSys, kMsgsFrame},

      // We want to handle frame messages before systems which would
      // be affected by sound end callbacks.

      // well, to be fair, we want after actually, so we can fixup names, though
      // woo woo, do we need a new loop client, that would suck
      {kConstrainBefore, &LOOPID_Schema, kMsgDatabase},
      {kConstrainBefore, &LOOPID_AI, kMsgDatabase},
      {kConstrainBefore, &LOOPID_ObjSys, kMsgDatabase},
      {kNullConstraint} // terminator
   }
};


static void _SongStart ()
{
   const sMissionSongParams* pSongParams = GetMissionSongParams();
   if ( '\0' != pSongParams->songName[0] )
   {
      SongUtilLoad (pSongParams->songName);
      SongUtilPlay();
   }
}

static void _SongStop ()
{
   SongUtilStop();
}

static void
_SongPause( void )
{
   SongUtilPause();
}

static void
_SongResume( void )
{
   SongUtilResume();
}


////////////////////////////////////////////////////////////
// SIM LISTENER
//

/* Old way.. Now based on mode change.
static void sim_msg (const struct sDispatchMsg* msg, const struct sDispatchListenerDesc* dummy )
{
   const sMissionSongParams* pSongParams;

   switch (msg->kind)
   {
      case kSimInit:
      case kSimResume:
         pSongParams = GetMissionSongParams();
         if ( '\0' != pSongParams->songName[0] )
         {
            SongUtilLoad (pSongParams->songName);
            SongUtilPlay();
         }
         break;

      case kSimTerm:
      case kSimSuspend:
         SongUtilStop();
         break;
   }
}

static struct sDispatchListenerDesc sim_listen =
{
   &MY_GUID,                                            // my guid
   kSimInit|kSimResume|kSimTerm|kSimSuspend,            // interests
   sim_msg
};

static void init_sim_msg()
{
   AutoAppIPtr(SimManager);
   pSimManager->Listen(&sim_listen);
}
*/
