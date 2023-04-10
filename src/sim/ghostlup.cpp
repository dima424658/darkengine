/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/ghostlup.cpp,v 1.10 1999/08/31 14:46:00 Justin Exp $
//
// Ghost system loop client, including save/load
//

#include <mprintf.h>
#include <config.h>
#include <appagg.h>
#include <loopmsg.h>
#include <loopapi.h>  // for CreateSimpleLoopClient
#include <netloop.h>
#include <dbasemsg.h>
#include <dispatch.h>
#include <dispbase.h>
#include <objremap.h>
#include <vernum.h>
#include <objedit.h>  // for ObjEditName()
#include <iobjsys.h>
#include <physloop.h> // for LOOPID_Physics
#include <simtime.h>  // for GetSimTimeFrame()
#include <netman.h>   // INetMan

#include <ghost.h>
#include <ghostapi.h>
#include <ghosttyp.h>
#include <ghostlst.h>
#include <ghostcfg.h>
#include <ghostmot.h>

#include <dbmem.h>

//////////
//
// SAVE/LOAD
//
// At the moment, we only save remote ghosts. For each one, we save a
// portion of its information, followed by its motion coordinator.
//

struct sRemoteGhostTag {
   ObjID obj;
   sGhostConfig cfg;
   int r_state;
   ObjID weap_obj;
   int n_weap;
};

static TagFileTag RemoteGhostTag = {"GHOSTREM"};
static TagVersion RemoteGhostTagVersion = {1, sizeof(sRemoteGhostTag)};

static void ReadRemoteGhostTag(ITagFile *file, eObjPartition part)
{
   TagVersion v = RemoteGhostTagVersion;
   if (SUCCEEDED(file->OpenBlock(&RemoteGhostTag,&v)))
   {
      // if old version, abandon all hope, somehow, w/o crashing horribly???
      if (v.major==RemoteGhostTagVersion.major)
      {
         sRemoteGhostTag tagData;
         // For each remote ghost...
         while (file->TellFromEnd() > 0)
         {
            // ... create the ghost and read in its basics...
            // @TBD: this could/should probably be distributed through the
            // other systems, like ghostphy...
            file->Read((char *) &tagData, sizeof(tagData));
            ObjID obj = ObjRemapOnLoad(tagData.obj);
            
            if (gGhostRemotes.Search(obj))
            {
               if (_ghost_list_load())
                  _ghost_mprintf(("GhostLoadRemote: Obj %s already in remote ghost table\n",ObjWarnName(obj)));
               return;
            }
            
            sGhostRemote *pGR = new sGhostRemote;
            GhostInitRemoteStruct(pGR);
            pGR->obj = obj;
            pGR->weap.weaponObj = ObjRemapOnLoad(tagData.weap_obj);
            pGR->weap.nWeapon = tagData.n_weap;
            pGR->cfg = tagData.cfg;
            pGR->cfg.flags |= kGhostCfNoPacket|kGhostCfLoaded;
            pGR->critter.r_state = tagData.r_state;
            gGhostRemotes.Insert(pGR);
            _GhostDebugSetupRemote(pGR);
            
            if (_ghost_track_weapons())
               if (_GhostIsType(pGR->cfg.flags,Player))
                  _ghost_mprintf(("Ghost %d loaded weap %d obj %d\n",pGR->obj,pGR->weap.nWeapon,pGR->weap.weaponObj));
            
            // ... and its motion coordinator...
            if (!(pGR->cfg.flags & kGhostCfNoCret) &&
                !(pGR->cfg.flags & kGhostCfNew) &&
                !_GhostIsType(pGR->cfg.flags,IsObj))  // shouldnt need this!
            {
               pGR->critter.pMotCoord=MotSysCreateMotionCoordinator();
               IMotor *pMotor = CreatureGetMotorInterface(pGR->obj);
               if (pMotor==NULL)
                  Warning(("Hey, couldnt create motor for %s\n",
                           ObjEditName(pGR->obj)));
               pGR->critter.pMotCoord->SetMotor(pMotor);
               if (pGR->cfg.flags & kGhostCfNoPhys)
               {
                  // This flag is really just for save; kill it, on the theory
                  // that the AI might regrow physics. Hey, it could happen.
                  pGR->cfg.flags &= ~kGhostCfNoPhys;
               }
               else
                  CreatureMakeBallistic(pGR->obj, kCMB_Posed);
               pGR->critter.pMotCoord->Load(file);
            }
            else
               pGR->critter.pMotCoord = NULL;
            if (_ghost_list_mods())   
               _ghost_mprintf(("ghost list loaded %s remote\n",
                               ObjEditName(obj)));
         }
      }
      file->CloseBlock();
   }
}

static void WriteRemoteGhostTag(ITagFile *file, eObjPartition part)
{
   TagVersion v = RemoteGhostTagVersion;
   if (SUCCEEDED(file->OpenBlock(&RemoteGhostTag,&v)))
   {
      sRemoteGhostTag tagData;
      tHashSetHandle remote_iter;
      AutoAppIPtr(ObjectSystem);
      sGhostRemote *pGR = gGhostRemotes.GetFirst(remote_iter);
      while (pGR)
      {
         ObjID obj = pGR->obj;
         _GhostDebugSetupRemote(pGR);
         if (pObjectSystem->Exists(obj) &&
             pObjectSystem->IsObjSavePartition(obj, part))
         {
            // Fill the tag...
            tagData.obj = obj;
            tagData.cfg = pGR->cfg;
            if (!PhysObjHasPhysics(obj))
               // Record that it doesn't have physics, so we don't add
               // them at load time stupidly:
               tagData.cfg.flags |= kGhostCfNoPhys;
            tagData.r_state = pGR->critter.r_state;
            tagData.weap_obj = pGR->weap.weaponObj;
            tagData.n_weap = pGR->weap.nWeapon;
            // ... write it out...
            file->Write((char *) &tagData, sizeof(tagData));
            // ... write out the motion coordinator...
            if (!(pGR->cfg.flags & kGhostCfNoCret) &&
                !(pGR->cfg.flags & kGhostCfNew))
            {
               pGR->critter.pMotCoord->Save(file);
            }
            // ... and on to the next.
         }
         pGR = gGhostRemotes.GetNext(remote_iter);
      }

      file->CloseBlock();
   }
}

//////////
//
// LOOP CLIENT
//

// State record for use by the ghost loop client
// Currently unused, but since it was in the boilerplate, I decided to
// keep it in case we want it later:
typedef struct _StateRecord
{
   int dummy;  // placeholder
} StateRecord;

static void ghost_db_message(DispatchData *msg)
{
   msgDatabaseData data;
   data.raw = msg->data;

   switch (DB_MSG(msg->subtype))
   {
      case kDatabaseReset:
         GhostDBReset();
         break;
      case kDatabaseLoad:
         if (msg->subtype & kObjPartConcrete)
            ReadRemoteGhostTag(data.load, msg->subtype);
         break;
      case kDatabasePostLoad:
         break;
      case kDatabaseSave:
         if (msg->subtype & kObjPartConcrete)
            WriteRemoteGhostTag(data.load, msg->subtype);
         break;
   }
}

static eLoopMessageResult LGAPI _LoopFunc(void* data, 
                                          eLoopMessage msg, 
                                          tLoopMessageData hdata)
{
   // useful stuff for most clients
   eLoopMessageResult result = kLoopDispatchContinue; 
   StateRecord* state = (StateRecord*)data;
   LoopMsg info;
   info.raw = hdata;

   switch(msg)
   {
      case kMsgAppInit:
         GhostInit();
         break;
      case kMsgAppTerm:
         GhostTerm();
         break;
      case kMsgNormalFrame:
      {
         AutoAppIPtr_(NetManager,pNetMan);
         if (pNetMan->Networking())
            GhostFrame(GetSimFrameTime());
         break;
      }
      case kMsgDatabase:
         ghost_db_message(info.dispatch);
         break;
      case kMsgEnd:
         delete state;
         break;   
   }
   return result;
}

// Factory function for our loop client.
static ILoopClient* LGAPI _CreateClient(sLoopClientDesc * pDesc,
                                        tLoopClientData data)
{
   StateRecord* state;
   // allocate space for our state, and fill out the fields
   state = new StateRecord;

   return CreateSimpleLoopClient(_LoopFunc,state,&GhostLoopClientDesc);
}

// The public loop client descriptor
// used by loopapp.c
sLoopClientDesc GhostLoopClientDesc =
{
   &LOOPID_Ghost,
   "Ghost Client",
   kPriorityNormal,              
   kMsgDatabase | kMsgsMode | kMsgsAppOuter | kMsgsFrameMid | kMsgEnd, // interests 

   kLCF_Callback,
   _CreateClient,
   
   NO_LC_DATA,

   {
      {kConstrainAfter,  &LOOPID_Physics, kMsgsAppOuter },
      {kConstrainBefore, &LOOPID_Physics, kMsgsFrame },
      {kConstrainAfter,  &LOOPID_Network, kMsgsFrame },
      {kNullConstraint}
   }
};
