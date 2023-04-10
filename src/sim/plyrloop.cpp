/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/plyrloop.cpp,v 1.81 2000/01/25 23:19:52 adurant Exp $

#include <lg.h>
#include <loopapi.h>
#include <biploop.h>
#include <simloop.h>
#include <simtime.h>
#include <config.h>

#include <loopmsg.h>
#include <dbasemsg.h>
#include <dispbase.h>
#include <gamemode.h>

#include <objloop.h>
#include <objpos.h>
#include <objtype.h>
#include <objdef.h>
#include <osysbase.h>
#include <objnotif.h>

#include <playrobj.h>
#include <plyrloop.h>
#include <objsys.h>
#include <physapi.h>
#include <plyrmov.h>
#include <plyrmode.h>
#include <phmterr.h>

#include <viewmgr.h>
#include <camera.h>
#include <wr.h>

#include <gamecam.h>
#include <matrix.h>
#include <tagfile.h>
#include <vernum.h>

#include <simman.h>
#include <simdef.h>
#include <dspchdef.h>

#include <command.h>

#include <dlistsim.h>
#include <dlisttem.h>

#include <linkman.h>
#include <linkbase.h>
#include <relation.h>
#include <contain.h>
#include <netman.h>

// @TODO: STUFF THAT DIES WHEN PLAYER ARCHETYPE HAPPENS
#include <mnamprop.h>

#include <mprintf.h>

// Must be last header 
#include <dbmem.h>



ObjID gPlayerObj = OBJ_NULL;
Camera* gPlayerCam = NULL;
static PlayerCreateFunc create_hook = NULL;

static IRelation* gpFactoryRel = NULL;
static INetManager *gpNetMan = NULL;


////////////////////////////////////////////////////////////
// PLAYER.H API
//

ObjID get_player_object(void)
{
   if (gPlayerObj == OBJ_NULL)
   {
      Warning(("get_player_object: Player obj is OBJ_NULL\n"));
   }
   return gPlayerObj;
}

////////////////////////////////////////

Camera* get_player_camera(void)
{
   return gPlayerCam;
}


////////////////////////////////////////

void HookPlayerCreate(PlayerCreateFunc func)
{
   create_hook = func;
}

////////////////////////////////////////

const char* PlayerObjName(void)
{
   return "Player"; 
}

////////////////////////////////////////

BOOL IsAPlayer(ObjID obj)
{
#ifndef NEW_NETWORK_ENABLED
   return gPlayerObj != OBJ_NULL && obj == gPlayerObj;
#else
   if (obj == OBJ_NULL || gPlayerObj == OBJ_NULL)
      return FALSE;
   else if (obj == gPlayerObj)
      return TRUE;
   else if (!gpNetMan->Networking())
      return FALSE;
   else
   {
      ObjID netPlayer;
      FOR_ALL_NET_PLAYERS(gpNetMan, &netPlayer)
         if (netPlayer == obj)
            return TRUE;
      return FALSE;
   }
#endif  // NEW_NETWORK
}

////////////////////////////////////////

static ObjID def_player_factory()
{
   return OBJ_NULL; 
}

////////////////////////////////////////

static PlayerFactoryObjFunc factory_func = def_player_factory; 

void HookPlayerFactory(PlayerFactoryObjFunc func)
{
   factory_func = (func) ? func : def_player_factory; 
}

////////////////////////////////////////


////////////////////////////////////////////////////////////
// INITIALIZING THE PLAYER
//

#define PLAYER_PHYS_FLAGS (PHYS_CONTROLLED|PHYS_STAY_ON_FLOOR|PHYS_GRAVITY)

static void create_player_cam(void)
{
   if (gPlayerCam == NULL)
   {
      mxs_vector pos;
      mxs_angvec ang;

      mx_mk_vec(&pos, 0.0, 0.0, 0.0); 
      mx_mk_angvec(&ang, 0.0, 0.0, 0.0); 
      
      gPlayerCam = CameraInit(&pos,&ang,OBJ_NULL); 
   }
}

static void destroy_player_cam(void)
{
   if (gPlayerCam != NULL)
      CameraDestroy(gPlayerCam); 
}

static void init_player_cam(void)
{
   if ((gPlayerObj != OBJ_NULL) && (!CameraIsRemote(gPlayerCam)))
   {
      CameraAttach(gPlayerCam,gPlayerObj); 
      CameraUpdate(gPlayerCam); 
   }
   else
     CameraReAttach(gPlayerCam); //make sure everything is kosher.
}

static void load_player_cam_info(ITagFile *pFile)
{
  char cam[sizeof(Camera)/sizeof(char)];
  int readresult;

  AssertMsg(gPlayerCam != NULL, "Trying to load cam info into NULL camera!");

//checking version info already done by setup_tagfile

  readresult = pFile->Read(cam, sizeof(Camera));
  AssertMsg(readresult==sizeof(Camera),"Read invalid block size trying to load player camera info!");

  memcpy(gPlayerCam,cam,sizeof(Camera));

//closing done by cleanup_tagfile

}

static void save_player_cam_info(ITagFile *pFile)
{

  if (gPlayerCam == NULL)
    return; //nothing to save

//again, block checking done already

  char cam[sizeof(Camera)/sizeof(char)];

  memcpy(cam,gPlayerCam,sizeof(Camera));

  int writeresult;
  writeresult = pFile->Write(cam, sizeof(Camera));
  AssertMsg(writeresult==sizeof(Camera),"Wrote invalid block size trying to save player camera info!");

//and block closing done by cleanup_tagfile

}

static void create_player_obj(void)
{
   IObjectSystem* objsys = AppGetObj(IObjectSystem);

   ObjID factory = factory_func(); 
   ObjID arch = ROOT_ARCHETYPE; 

   // Look for a factory link, and if there is one, use the archetype it points at. 
   if (factory != OBJ_NULL)
   {
      LinkID id = gpFactoryRel->GetSingleLink(factory,LINKOBJ_WILDCARD); 
      if (id != LINKID_NULL)
      {
         sLink link;
         gpFactoryRel->Get(id,&link); 
         arch = link.dest;
      }
   }

   gPlayerObj = objsys->BeginCreate(arch,kObjectConcrete);

   init_player_cam();
   
   objsys->NameObject(gPlayerObj,PlayerObjName());

   if (create_hook != NULL)
      create_hook(kPlayerCreate,gPlayerObj);

   // copy stuff out of the factory
   if (factory != OBJ_NULL)
   {
      // copy clonable properties 
      AutoAppIPtr_(ObjectSystem,pObjSys); 
      pObjSys->CloneObject(gPlayerObj,factory); 

      // move contained objects onto the player  
      AutoAppIPtr(ContainSys);
      pContainSys->MoveAllContents(gPlayerObj,factory,CTF_COMBINE); 

      // move player to factory 
      ObjPos* pos = ObjPosGet(factory); 
      if (pos)
         ObjPosCopyUpdate(gPlayerObj,pos); 
   }

   objsys->EndCreate(gPlayerObj);

   SafeRelease(objsys);

#if 0
   //! @HACK  Temporary code.  Later there will be some kind of major mode that
   // is invoked while waiting for players to join a newly loaded level.
   AutoAppIPtr_(NetManager, pNetMan);
   if (pNetMan->IsNetworkGame())
      JoinUp();
#endif
}

void PlayerDestroy(void)
{
   if (gPlayerObj != OBJ_NULL)
   {
      if (create_hook)
         create_hook(kPlayerDestroy,gPlayerObj); 

      AutoAppIPtr_(ObjectSystem,pObjSys);
      if (pObjSys->Exists(gPlayerObj))
         pObjSys->Destroy(gPlayerObj);
      gPlayerObj = OBJ_NULL;
   }
   if (gPlayerCam != NULL)
   {
      CameraDetach(gPlayerCam);
   }
}

void PlayerCreate(void)
{
   PlayerDestroy();
   create_player_obj();
}

////////////////////////////////////////

struct Position* PlayerPosition()
{
   if (gPlayerObj != OBJ_NULL)
      return ObjPosGet(gPlayerObj); 

   ObjID factory = factory_func(); 
   if (factory != OBJ_NULL)
      return ObjPosGet(factory);

   return NULL; 
}

////////////////////////////////////////////////////////////
// PLAYER TAG FILE
//

TagFileTag MyTag = { "PLAYER" };
TagVersion MyVersion = { 0, 4};

TagFileTag MyCamTag = { "PLAYERCAM" };
TagVersion MyCamVersion = { 0, 1};

static BOOL setup_tagfile(ITagFile* file, TagFileTag usetag, TagVersion useversion)
{
   if (file->OpenMode() == kTagOpenRead && file->BlockSize(&usetag) == 0)
      return FALSE; 

   TagVersion v = useversion;
   HRESULT result = file->OpenBlock(&usetag,&useversion);
   if (FAILED(result))
      return FALSE; 

   if (!VersionNumsEqual(&v,&useversion))
   {
      file->CloseBlock(); 
      return FALSE;
   }

   return TRUE; 
}

static void cleanup_tagfile(ITagFile* file)
{
   file->CloseBlock();
}

BOOL ContainsPlayerTag(ITagFile* file)
{
   BOOL result = setup_tagfile(file,MyTag,MyVersion); 
   if (result)
      cleanup_tagfile(file);
   return result; 
}

//
// This is kind of pathetic, but its all we need right now
//

static ObjID read_player_tag(ITagFile* file)
{
   ObjID result = OBJ_NULL;
   if (file->Read((char*)&result,sizeof(result)) == sizeof(result))
   {
      AutoAppIPtr_(ObjectSystem,pObjSys); 
      result = pObjSys->RemapOnLoad(result); 
   }
    return result;
}

static void write_player_tag(ITagFile* file, ObjID obj)
{
   ITagFile_Write(file,(char*)&obj,sizeof(obj));
}

////////////////////////////////////////////////////////////
// Mode transition position set
//

#ifdef EDITOR
bool persistent_player_pos = TRUE;
#endif

static void reset_player_position(void)
{
   ObjPos* pos = ObjPosGet(gPlayerObj);
#ifdef EDITOR
   mxs_vector *plypos;
   mxs_angvec *plyang;
#endif 
   mxs_vector zero;

   if (pos == NULL)
   {
      Warning(("player has NULL position\n"));
      return ;
   }

#ifdef EDITOR
   if (vm_spotlight_loc(&plypos,&plyang))
   {
      pos->loc.vec = *plypos;
      pos->fac = *plyang;
   }
   else
#endif // EDITOR
   {
      mxs_angvec ang = { 0, 0, 0 };
      mx_zero_vec(&pos->loc.vec);
      pos->fac = ang;
   }
   ObjPosCopyUpdate(gPlayerObj,pos); 

   mx_zero_vec(&zero);
   PhysSetVelocity(gPlayerObj, &zero);
}


////////////////////////////////////////////////////////////
// SIM LISTENER
//

static void sim_msg(const sDispatchMsg* msg, const sDispatchListenerDesc* )
{
   switch (msg->kind)
   {
      case kSimInit: 
         ResetMovingTerrain();

      case kSimResume:
         if (gPlayerObj == OBJ_NULL)
            PlayerCreate();
         init_player_cam(); 
#ifdef EDITOR
         if (!persistent_player_pos)
            reset_player_position(); 
#endif
         break; 
   }
}

static sDispatchListenerDesc sim_listen = 
{
   &LOOPID_Player, // my guid
   kSimInit|kSimResume,   // interests
   sim_msg,
};

static void init_sim_msg()
{
   AutoAppIPtr_(SimManager,pSimMan); 
   pSimMan->Listen(&sim_listen); 
}


////////////////////////////////////////////////////////////
// "PLAYER FACTORY" RELATION 
//

void create_factory_rel(void)
{
   static sRelationDesc reldesc = { "PlayerFactory" }; 
   static sRelationDataDesc datadesc = LINK_DATA_DESC(int);
   gpFactoryRel = CreateStandardRelation(&reldesc,&datadesc,0); 
}


/////////////////////////////////////////////////////////////
// PLAYER LOOP CLIENT
////////////////////////////////////////////////////////////

//////////////////
// CONSTANTS
//
// These are just here to separate out boiler-plate code and leave it untouched
//


//
//
#define MY_GUID   LOOPID_Player

//
//
// My context data
typedef void Context;

//
//
// My state
typedef struct _StateRecord
{
   Context* context; // a pointer to the context data I got.
   // State fields go here
} StateRecord;

////////////////////////////////////////////////////////////
// DATABASE MESSAGE HANDLER
//

static void db_message(DispatchData* msg)
{
   msgDatabaseData data;
   data.raw = msg->data;
   switch (DB_MSG(msg->subtype))
   {
      case kDatabaseReset:
         // This is here just to make sure the create_hook gets called. 
         PlayerMovementReset();
         PlayerModeReset();
         PlayerDestroy();
         break;

      case kDatabaseLoad:
         if (msg->subtype & kObjPartConcrete)
         {
            if (setup_tagfile(data.load,MyTag,MyVersion))
            {
               g_pPlayerMode->Read(data.load);

               ObjID obj = read_player_tag(data.load); 
               if (obj != OBJ_NULL)
               {
                  gPlayerObj = obj;
                  if (create_hook)
                     create_hook(kPlayerLoad,obj); 
                  PhysStopControlVelocity(obj);
               }

               cleanup_tagfile(data.load);
            }
	    if (setup_tagfile(data.load,MyCamTag,MyCamVersion))
	      {
		load_player_cam_info(data.load);
		cleanup_tagfile(data.load);
	      }
	    else
	      init_player_cam();
         }
         break;

      case kDatabasePostLoad:
         if (gPlayerObj == OBJ_NULL) 
         {
            // Back compatibility issue: 
            // find any bogus object named "Player" and rename it
            if (msg->subtype & kObjPartConcrete)
            {
               AutoAppIPtr_(ObjectSystem,pObjSys);
               ObjID imposter = pObjSys->GetObjectNamed(PlayerObjName()); 
               if (imposter != OBJ_NULL)
                  pObjSys->NameObject(imposter,"OldPlayer"); 
            }
            
            // make the player camera, and put it at the starting point
            ObjPos* pos = PlayerPosition(); 
            if ((pos) && (gPlayerCam->objid==OBJ_NULL))
	      CameraSetLocation(gPlayerCam,&pos->loc.vec,&pos->fac); 
         }

         PostLoadMovingTerrain();

         break; 

      case kDatabaseSave:
      {
         if (msg->subtype & kObjPartConcrete)
         {
            if (setup_tagfile(data.save,MyTag,MyVersion))
            {
               g_pPlayerMode->Write(data.load);

               AutoAppIPtr_(ObjectSystem,pObjSys); 
               if (pObjSys->IsObjSavePartition(gPlayerObj,msg->subtype))
                  write_player_tag(data.save,gPlayerObj);
 
               cleanup_tagfile(data.save);
            }
	    if (setup_tagfile(data.load,MyCamTag,MyCamVersion))
	      {
		save_player_cam_info(data.load);
		cleanup_tagfile(data.load);
	      }
         }
      }
      break;
   }
}

////////////////////////////////////////////////////////////
// OBJECT MESSAGE HANDLER
//

#pragma off(unreferenced)
static void obj_message(ObjID obj, eObjNotifyMsg msg, void* data)
{
   switch(msg)
   {
      case kObjNotifyDelete:
         if (obj == gPlayerObj)
         {  
            gPlayerObj = OBJ_NULL;  
            if (create_hook)
               create_hook(kPlayerDestroy,obj); 
            PlayerDestroy(); // get the camera too
         }
         break;
   }
}
#pragma on(unreferenced)

static void init_obj_message(void)
{
   IObjectSystem* pObjSys = AppGetObj(IObjectSystem); 
   sObjListenerDesc desc = { obj_message, NULL }; 
   pObjSys->Listen(&desc);
   SafeRelease(pObjSys); 
}

#ifdef PLAYTEST
void register_player_commands(void);
#else
#define register_player_commands()
#endif

////////////////////////////////////////
//
// LOOP/DISPATCH callback
// Here's where we do the dirty work.
//

#pragma off(unreferenced)
static eLoopMessageResult LGAPI _LoopFunc(void* data, eLoopMessage msg, tLoopMessageData hdata)
{  // useful stuff for most clients
   eLoopMessageResult result = kLoopDispatchContinue;
   StateRecord* state = (StateRecord*)data;
   LoopMsg info;

   info.raw = hdata;

   switch(msg)
   {

      case kMsgNormalFrame:
      case kMsgPauseFrame:
         CameraUpdate(gPlayerCam);
         PlayerModeUpdate();
         if (IsSimTimePassing())
            PlayerMovementUpdate();
         break;

      case kMsgAppInit:
#ifdef NEW_NETWORK_ENABLED
         gpNetMan = AppGetObj(INetManager);
#endif
         init_sim_msg(); 
         create_player_cam(); 
         register_player_commands();
         init_obj_message(); 
         create_factory_rel(); 
         PlayerMovementInit();
         PlayerModeInit();
         break;

      case kMsgAppTerm:
         destroy_player_cam(); 
         PlayerMovementTerm();
         PlayerModeTerm();
         break; 

      case kMsgDatabase:
         db_message(info.dispatch);
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

sLoopClientDesc PlayerLoopClientDesc =
{
   &MY_GUID,                           // GUID
   "Player Object Client",             // NAME
   kPriorityNormal,                    // PRIORITY
   kMsgEnd | kMsgsFrame | kMsgDatabase 
   | kMsgsAppOuter ,       // INTERESTS

   kLCF_Callback,
   _CreateClient,
   
   NO_LC_DATA,
   
   {
      { kConstrainBefore, &LOOPID_Biped, kMsgsFrameMid},  // so player motion works from updated cam position
      { kConstrainAfter, &LOOPID_ObjSys, kMsgDatabase}, 
      { kConstrainBefore, &LOOPID_SimFinish, kMsgsFrameMid},
      {kNullConstraint} // terminator
   }
};

#ifdef PLAYTEST
////////////////////////////////////////////////////////////
// COMMANDS
//

//////////////////////////////
//
// Move game mode camera
//
void move_game_cam (mxs_vector *pos)
{
   Camera* player_cam = PlayerCamera();
   CameraSetLocation(player_cam, pos, NULL);
}

//////////////////////////////
//
// Like the above but take a string
//
void move_game_cam_str (char *args)
{
   mxs_vector pos;

   sscanf (args, "%f,%f,%f", &pos.x, &pos.y, &pos.z);
   move_game_cam (&pos);
}

////////////////////////////////////////

Command PlayerCommands[] =
{
   { "move_game_camera", FUNC_STRING, move_game_cam_str, "teleport game camera" },
#ifdef EDITOR
   { "persistent_player_pos", TOGGLE_BOOL, &persistent_player_pos, "Toggle whether the player comes back to game mode" },
#endif
};

void register_player_commands(void)
{
   COMMANDS(PlayerCommands,HK_ALL);
}
#endif
