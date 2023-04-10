/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <limits.h>
#include <stdio.h>
#include <windows.h>
#include <direct.h>
#include <config.h>
#include <mprintf.h>
#include <filepath.h>
#include <filespec.h>
#include <linkman.h>
#include <gen_bind.h>
#include <filevar.h>

#include <scrptapi.h>
#include <drkdiff.h>
#include <phclimb.h>
#include <physapi.h>
#include <objedit.h>

#include <appagg.h>
#include <tagfile.h>
#include <vernum.h>

#include <tagpatch.h>

#include <transmod.h>
#include <simtime.h>

#include <status.h>
#ifdef EDITOR
#include <viewmgr.h>
#endif

#include <objtype.h>
#include <objdef.h>
#include <objnotif.h>
#include <objpos.h>

#include <dbasemsg.h>
#include <dispatch.h>
#include <dispbase.h>
#include <objquery.h>
#include <wrtype.h>

#include <dbfile.h>
#include <dbtagfil.h>
#include <campaign.h>
#include <quesfile.h>
#include <simman.h>

#include <iobjsys.h>
#include <contain.h>
#include <rooms.h>
//#include <simman.h>

#include <netman.h>
#include <iobjnet.h>
#include <linkbase.h>
#include <relation.h>
#include <lnkquery.h>
#include <dpcnet.h>
#include <cfgdbg.h>
#include <netsynch.h>
#include <netmsg.h>

// sdesc
#include <sdesc.h>
#include <sdesbase.h>
#include <propbase.h>

#include <dbfile.h>
#include <dbasemsg.h>
#include <playrobj.h>

#include <aiman.h>

#include <dpcinv.h>
#include <dpcmulti.h>
#include <dpcmprop.h>
#include <dpcutils.h>
#include <dpcovrly.h>
#include <dpcovcst.h>
#include <dpcplayr.h>
#include <dpcprop.h>
#include <dpcgame.h>
#include <dpcpatch.h>

#include <ghostapi.h>

#include <dbmem.h>

////////////////////////////////////////

// code and utilities for multi-level support

////////////////////////////////////////

#define CURGAME_DIR  "current"
#define ARCHIVE_DIR  "archive"
#define MAP_DIR      "maps"
// Save directory base name:
#define SAVEROOT     "save_"

#define CAMPAIGN_FNAME  "CAMPAIGN"

IIntProperty *gDPCStartLocProp;
IIntProperty *gDPCDestLocProp;
IBoolProperty *gDPCElevAbleProp;
IBoolProperty *gDPCInittedProp;
ILabelProperty *gDPCDestLevelProp;
IVectorProperty *gDPCElevOffsetProp;

void DPCEraseDirectory(char *dir);
void doDPCLoadFull(const char *fileroot);
void PrepLoad();
void FinishLoad(BOOL prep);

static IRelation *gpLandingRel = NULL;

// The network messages used in this module; descriptors down at
// the bottom:
static cNetMsg *g_pSaveGameMsg = NULL;

////////////////////////////////////////

// so we keep the input lockdown state in a filevar so we can
// re-set it appropriately on entering game mode.  Kind of a hack, I admit.
struct sMiscData
{
   BOOL m_nomove;
};

// Here's my descriptor, which identifies my stuff to the tag file & editor
sFileVarDesc gMiscDataDesc = 
{
   kCampaignVar,         // Where do I get saved?
   "MISCDATA",          // Tag file tag
   "Misc. Data",     // friendly name
   FILEVAR_TYPE(sMiscData),  // Type (for editing)
   { 1, 0},             // version
   { 1, 0},             // last valid version 
   "DPC",              // optional: what game am I in NULL means all 
}; 

// The actual global variable
cFileVar<sMiscData,&gMiscDataDesc> gMiscData; 

EXTERN BOOL gNoMoveKeys;


////////////////////////////////////////
// Transform a slot number into a directory name. In the case of
// multiplayer games, the name is based on the host's name.
void DPCGetSaveDir(int slot, char *buf)
{
   AutoAppIPtr(NetManager);
   if (pNetManager->IsNetworkGame()) {
      // @TBD: Yes, this is going to be more than 8.3. Fixing this while
      // retaining distinctive directories for different hosts will
      // require using subdirectories, which means a bunch of changes
      // throughout here and dpcsavui. Unless we find reason to believe
      // it's actually a problem, I'm not inclined to do it...
      sprintf(buf, "%s%s_%d", SAVEROOT, DPCGetHostName(), slot);
   } else {
      sprintf(buf, "%s%d", SAVEROOT, slot);
   }
}

////////////////////////////////////////
static BOOL LGAPI inv_partition_func(ObjID obj, const sObjPartitionFilter* )
{
   AutoAppIPtr(ContainSys);
   //eContainType ctype;

   // put the players in the briefcase
   if (IsAPlayer(obj))
      return(TRUE);

   // anything in the elevator has the elevator offset property
   if (gDPCElevOffsetProp->IsRelevant(obj)) {
      // Note that we specifically want to include even proxies
      return(TRUE);
   }

   // put any object in any player's inventory in the briefcase
   if (PlayerObjectExists())
   {
      //ctype = pContainSys->IsHeld(gPlayerObj, obj);
      //if (ctype != ECONTAIN_NULL)
         //return(TRUE);


      // @TBD: does this work right? It assumes that player inventories
      // never have recursive containers. If it *is* right, then we
      // can probably get rid of the above clause, since it's essentially
      // redundant. If we *do* need to use IsHeld, then we'll have to
      // iterate over all players for every object -- icky, even for
      // just four players...

      // 5/17/99 Xemu: Inventories are in fact never recursive, and as
      // the above comment notes, above clause was redundant.  We have
      // now eliminated it, and this clause here is more general.
      ObjID container = pContainSys->GetContainer(obj);
      if (container != OBJ_NULL)
      {
         // being contained by the player qualifies
         if (IsAPlayer(container)) 
            return(TRUE);

         // as does beign contained by anything in the elevator
         if (gDPCElevOffsetProp->IsRelevant(container)) 
            return(TRUE);
      }
   }
  
   // if no other conditions met, we aren't in the briefcase
   return(FALSE);
}

////////////////////////////////////////
static void BundleProxy(ObjID obj);
static void UnbundleIfProxy(ObjID obj);

static void CleanLinks(ObjID obj, RelationID relid)
{
   ILinkQuery *pQuery;
   AutoAppIPtr(LinkManager);
   sLink l;

   pQuery = pLinkManager->Query(obj, LINKOBJ_WILDCARD, relid);

   while (!pQuery->Done())
   {
      pQuery->Link(&l);
      // okay, we are of a proscribed link type, do we go to outside
      // of the briefcase?
      if (!inv_partition_func(l.dest,NULL))
      {
         pLinkManager->Remove(pQuery->ID()); 
      }
      pQuery->Next();
   }
   SafeRelease(pQuery);

   pQuery = pLinkManager->Query(LINKOBJ_WILDCARD, obj, relid);

   while (!pQuery->Done())
   {
      pQuery->Link(&l);
      // okay, we are of a proscribed link type, do we go to outside
      // of the briefcase?
      if (!inv_partition_func(l.source,NULL))
      {
         pLinkManager->Remove(pQuery->ID()); 
      }
      pQuery->Next();
   }
   SafeRelease(pQuery);
}

////////////////////////////////////////
static void DPCBriefcaseSave()
{
   ObjPos *opos;
   IObjectQuery* query;
   ObjID obj;
   AutoAppIPtr_(ObjectSystem,pObjSys);
   AutoAppIPtr(NetManager);
   BOOL networking = pNetManager->Networking();
   AutoAppIPtr(ObjectNetworking);

   GhostRemoveWeapons();

   // go through all our objects, and destroy the cell hinting in them
   query = pObjSys->Iter(kObjectConcrete); 
   for (; !query->Done(); query->Next())
   {
      obj = query->Object();
      if (inv_partition_func(obj, NULL))
      {
         opos = ObjPosGet(obj);
         UpdateChangedLocation(&opos->loc);
         //gDPCInittedProp->Set(obj,TRUE);

         // snap any links to an external obj
         AutoAppIPtr(LinkManager);
         IRelation *pRel;

         pRel = pLinkManager->GetRelationNamed("Tripwire");
         CleanLinks(obj, pRel->GetID());

         pRel = pLinkManager->GetRelationNamed("AIAwareness");
         CleanLinks(obj, pRel->GetID());

      } else {
         if (networking && !IsAPlayer(obj))
         {
            // We have to bundle the proxy up in a survivable form.
            // This needs to happen for any object not in the default
            // state, where the default is owned by default host, with
            // the same objID.
            BundleProxy(obj);
         }
         // Break its proxy links, if it has any. (Even objects that
         // aren't proxies will often have proxy links.) We want to
         // make sure that anything not going into the briefcase has
         // no proxy links, before we pack up the briefcase.
         pObjectNetworking->ObjRegisterProxy(OBJ_NULL, OBJ_NULL, obj);
      }
   }

   // if the player is climbing, then break that climb
   if (PhysObjIsClimbing(PlayerObject()))
      BreakClimb(PlayerObject(),FALSE,FALSE);

   // save the "briefcase" sub-partition -- this should really be to memory
   // someday when we support that
   dbSave("briefcase.tmp",kObjPartConcrete|kObjPartBriefcase); // "briefcase.tmp"
}

////////////////////////////////////////
static void DPCBriefcaseLoad(void)
{
   char temp[255];

   // merge load the "briefcase" subpartition 
   dbMergeLoad("briefcase.tmp",kObjPartConcrete|kObjPartBriefcase); 

   // we should delete the briefcase file too
   dbFind("briefcase.tmp",temp);
   //DeleteFile(temp); // hmm, will this do the right path-savvy things?
   remove(temp);

   /*
   sPropertyObjIter iter;
   BOOL propval;

   gDPCInittedProp->IterStart(&iter);
   while (gDPCInittedProp->IterNextValue(&iter, &obj, &propval))
   {
      // make sure it is concrete, matches our target ecotype, and derives from physical (to exclude traps)
      if (OBJ_IS_CONCRETE(obj) && (propval))
      {
         gDPCInittedProp->Delete(obj);
      }
   }
   gDPCInittedProp->IterStop(&iter);
   */
}

////////////////////////////////////////
static ObjID FindObjMarker(int marker, BOOL dest)
{
   int val;
   IObjectQuery* query;
   ObjID obj;

   AutoAppIPtr(ObjectSystem);

   query = pObjectSystem->Iter(kObjectConcrete); 
   for (; !query->Done(); query->Next())
   {
      obj = query->Object();
      if (dest)
      {
         if (gDPCDestLocProp->IsRelevant(obj))
         {
            gDPCDestLocProp->Get(obj, &val);
            if (val == marker)
            {
               SafeRelease(query);
               return(obj);
            }
         }
      }
      else
      {
         if (gDPCStartLocProp->IsRelevant(obj))
         {
            gDPCStartLocProp->Get(obj, &val);
            if (val == marker)
            {
               SafeRelease(query);
               return(obj);
            }
         }
      }
   }
   SafeRelease(query);
   return(OBJ_NULL);
}
////////////////////////////////////////
static void ElevatorFixup(ObjID obj, ObjPos *markerpos)
{
   mxs_vector newloc, zloc, zvec;
   mxs_vector *offset;
   ObjPos *plrpos;
   mxs_vector safepos;

   gDPCElevOffsetProp->Get(obj, &offset);
   mx_add_vec(&newloc, offset, &(markerpos->loc.vec));

   // set up our z-fixup vector to avoid objects settling out of world
   mx_zero_vec(&zvec);
   if (obj != PlayerObject())
      zvec.z = 0.5;
   mx_add_vec(&zloc, &zvec, &newloc);

   ObjTranslate(obj, &zloc);

   // if our fixed up pos is good, put it there
   if (!PhysObjValidPos(obj, NULL))
   {
      Warning(("Hey, loc (%g, %g, %g) is bad for obj %s\n",zloc.x,zloc.y,zloc.z,ObjEditName(obj)));

      // our fixup failed somehow, so lets put it somewhere we *KNOW* is safe
      // namely, at the player's location (well, at their feet, anyways)
      if (obj != PlayerObject())
      {
         plrpos = ObjPosGet(PlayerObject());
         mx_copy_vec(&safepos, &plrpos->loc.vec);

         //safepos.x -= 0.5;

         // move it there
         ObjTranslate(obj, &safepos);
      }
   }
   // get rid of the property so they are clean
   //
   gDPCElevOffsetProp->Delete(obj);
}

////////////////////////////////////////

static void TransportCore(const char *oldfile, const char *newfile, int marker, uint flags)
{
   char temp[255];
   ObjID markerobj,obj;
   ObjPos *opos,*markerpos;
   IObjectQuery* query;
   const ObjID *obj_list;
   int num_objs,i;
   mxs_vector deltaloc; 
   cRoom *roomp;
   BOOL elevable;
   tSimTime curtime;

   AutoAppIPtr(NetManager);
   BOOL networking = pNetManager->Networking();

   markerobj = FindObjMarker(marker, TRUE);
   if (markerobj == OBJ_NULL) {
      Warning(("Asked to transport using non-existent marker %d\n", marker));
      return;
   }

   markerpos = ObjPosGet(markerobj);
   curtime = GetSimTime();

   AutoAppIPtr(ObjectSystem);
   AutoAppIPtr(ObjectNetworking);
   //AutoAppIPtr(SimManager);
   
   // okay, time to move to the new level

   // stop the sim
   //pSimManager->StopSim(); // or do I want to suspend?
   
   // go find any elevator-relevant objects
   // this is defined as all the objects within the same room as the marker
   if (flags & SMF_ELEVATOR)
   {
      roomp = g_pRooms->GetObjRoom(markerobj);
      if (roomp == NULL)
      {
         Warning(("TransportCore:: marker not in any room!\n"));
      }
      else
      {
         roomp->GetObjList(&obj_list, &num_objs);
         // given them the elevator offset property
         for (i=0; i < num_objs; i++)
         {
            if (gDPCElevAbleProp->IsRelevant(obj_list[i]))
            {
               gDPCElevAbleProp->Get(obj_list[i],&elevable);
               // We only do this for objects that we own.
               if (elevable && !pObjectNetworking->ObjIsProxy(obj_list[i]))
               {
                  opos = ObjPosGet(obj_list[i]);
                  mx_sub_vec(&deltaloc, &(opos->loc.vec), &(markerpos->loc.vec)); 
                  // Note that this property is being implicitly networked:
                  gDPCElevOffsetProp->Set(obj_list[i],&deltaloc);
               }
            }
         }
      }

      // Now let all of the ElevOffset properties settle out before we
      // proceed:
      if (networking) 
      {
         pNetManager->PreFlush();
         pNetManager->SynchFlush(NULL, NULL);
      }
   }

   // Turn off all networking except crucial metagame messages:
   pNetManager->SuspendMessaging();

   // pack up the briefcase
   DPCBriefcaseSave();

   // delete everything in the briefcase out of the world
   query = pObjectSystem->Iter(kObjectConcrete); 
   for (; !query->Done(); query->Next())
   {
      obj = query->Object();

      if (inv_partition_func(obj, NULL) && !IsAPlayer(obj))
      {
         if (networking && pObjectNetworking->ObjIsProxy(obj)) {
            pObjectNetworking->ObjDeleteProxy(obj);
         } else {
            DestroyObject(obj);
         }
      }
   }

   // Now we can destroy the avatars; we have to wait until their
   // inventories are wiped out first:
   if (networking) {
      FOR_ALL_NET_PLAYERS(pNetManager, &obj) {
         pObjectNetworking->ObjDeleteProxy(obj);
      }
   }

   PlayerDestroy(); // put a cap in the player separately, since he requires strange
                    // destruction protocol

   // now save the current mission sans briefcase junk
   sprintf(temp,"%s\\%s.mis",CURGAME_DIR,oldfile);
   dbSave(temp,kFiletypeMIS|kObjPartTerrain);

   PrepLoad();

   // turn networking on, if requested
   if (flags & SMF_NETWORK_ON) {
      NetSynchEnable();
   }

   // load up the new level
   doDPCLoadFull(newfile);

   // now unpack the briefcase
   // In the specific case of entering the "real" game from training,
   // indicated by SMF_NETWORK_ON, we specifically need to suppress any
   // remap-object messages for the briefcase, because the other players
   // don't know about our contents yet. netman will deal with telling
   // the other players to create proxies for our stuff later.
   if (flags & SMF_NETWORK_ON) {
      NetSynchDisableRemapMsgs();
   }
   DPCBriefcaseLoad();
   if (flags & SMF_NETWORK_ON) {
      NetSynchEnableRemapMsgs();
   }
   ObjPos *pos = ObjPosGet(gPlayerObj);
   //mprintf("transcore: playerloc: %g %g %g\n", pos->loc.vec.x, pos->loc.vec.y, pos->loc.vec.z);

   // poke in the time held over from before
   SetSimTime(curtime);
   FinishLoad(TRUE);

   if (networking) {
      // Unbundle any proxies that may be waiting on the new level. This
      // must happen *after* we unpack the briefcase; we count on the
      // fact that these entries never go through RebuildProxyTable:
      query = pObjectSystem->Iter(kObjectConcrete); 
      for (; !query->Done(); query->Next())
      {
         obj = query->Object();
         UnbundleIfProxy(obj);
      }
      // Now that we've unbundled, and the ownerships are all now presumably
      // correct, tell the AI system to make sure it's got it all right:
      AutoAppIPtr(AINetManager);
      pAINetManager->RecheckAIOwnership();
   }

   markerobj = FindObjMarker(marker, FALSE);
   if (markerobj == OBJ_NULL) {
      Warning(("Transported to level without corresponding marker %d\n", marker));
   } else {
      markerpos = ObjPosGet(markerobj);

      // Find the landing point for this player. If there isn't an apppropriate
      // landing point, just place the player at the transport marker.
      int myPlayerNum = pNetManager->MyPlayerNum();
      ObjID landingMarker = OBJ_NULL;
      ILinkQuery *pQuery = gpLandingRel->Query(markerobj, LINKOBJ_WILDCARD);
      if (pQuery != NULL) {
         for ( ; 
               !pQuery->Done() && (landingMarker == OBJ_NULL); 
               pQuery->Next()) 
         {
            int *pLandingNum = (int *) pQuery->Data();
            if ((pLandingNum != NULL) && (*pLandingNum == myPlayerNum)) {
               sLink link;
               pQuery->Link(&link);
               landingMarker = link.dest;
            }
         }
      }
      SafeRelease(pQuery);
      if (landingMarker == OBJ_NULL) {
         landingMarker = markerobj;
      }

      opos = ObjPosGet(landingMarker);
      // move player to the marker
      ObjPosCopyUpdate(PlayerObject(), opos);
#ifdef EDITOR
      // if persistent_player_pos is suppressed, we need to go
      // poke in the right new data.
      mxs_vector *plypos;
      mxs_angvec *plyang;
      if (vm_spotlight_loc(&plypos,&plyang))
      {
         *plypos = opos->loc.vec;
         *plyang = opos->fac;
      }
#endif

      // find all elevator-offset objects and fix up their position 
      // relative to the player
      if (flags & SMF_ELEVATOR)
      {
         // go through the offset objects and move them
         query = pObjectSystem->Iter(kObjectConcrete); 
         for (; !query->Done(); query->Next())
         {
            obj = query->Object();
            if (gDPCElevOffsetProp->IsRelevant(obj))
            {
               ElevatorFixup(obj, markerpos);
            }
         }
      }
   }

   // AIs ignore sounds for the first second
   AutoAppIPtr(AIManager);
   pAIManager->SetIgnoreSoundTime(1000);

   // restart the sim
   //pSimManager->StartSim();

   // Restart networking
   pNetManager->ResumeMessaging();

   // are there other things we want to do upon arriving in a
   // new level that aren't already handled in DB message handlers?
}

////////////////////////////////////////

// Actually switch levels
static void doDPCLevelTransport(const char *newfile, int marker, uint flags)
{
   char curfile[255], temp[255];
   
   // throw any objects on the cursor into the world
   // This must happen before the synch, so it has time to take effect:
   if (drag_obj != OBJ_NULL)
   {
      ThrowObj(drag_obj,PlayerObject());
      ClearCursor();
   }

   // Make sure that all of the player are on the same page:
   AutoAppIPtr(NetManager);
   if (pNetManager->Networking()) {
      // @TBD: put in a callback here, to supply a timeout:
      pNetManager->SynchFlush(NULL, NULL);
   }

   // first things first, deal with any extraneous effects that have to 
   // get shut down at level change time
   /*
   float hacktime;
   AutoAppIPtr(DPCPlayer);
   if (gPropHackVisibility->Get(PlayerObject(), &hacktime))
   {
      DPCOverlayChange(kOverlayHackIcon, kOverlayModeOff);
      gPropHackVisibility->Set(PlayerObject(),1.0);
      pDPCPlayer->RecalcData(PlayerObject());
   }
   */

   // clear out all selection boxes by iterating over the hudtime prop
   sPropertyObjIter iter;
   ObjID obj;
   gPropHUDTime->IterStart(&iter);
   while (gPropHUDTime->IterNext(&iter,&obj))
   {
      gPropHUDTime->Delete(obj);
   }
   gPropHUDTime->IterStop(&iter);

   // we should eventually send out messages to the scripts
   // that care here... ?

   // maybe also run some sim time so that things settle to the floor, etc.?
   // ick hard problem yuck run flee hide

   // now do the actual level transition 
   dbCurrentFile(curfile, sizeof(curfile));
   strcpy(temp, curfile); 
   temp[strlen(curfile) - 4] = '\0'; // to strip out extension
   TransportCore(temp,newfile,marker,flags);
}

// Switch to the named new level.
// @NOTE: it is essential for multiplayer that the caller has called
// INetManager::PreFlush() as soon as it was notified to begin the
// transport.
void DPCLevelTransport(const char *newfile, int marker, uint flags)
{
   doDPCLevelTransport(newfile, marker, flags);
}

////////////////////////////////////////
/*
void DPCElevatorTest(char *args)
{
   char newfile[255]; // ,oldfile[255];
   int marker;

   sscanf(args,"%s %d",newfile,&marker);

//   TransportCore(oldfile, newfile, marker, SMF_ELEVATOR);
   SwitchToLevelTrans(newfile, marker, SMF_NONE);
}
*/


static void PrepLoad()
{
   //
   // Stop the sim and reset the database 
   //
   AutoAppIPtr_(SimManager,pSimMan); 
   pSimMan->StopSim(); 

   dbReset(); 
}

#define CURRENT_LEVEL_PATCH_VERSION (1)
#define QUESTVAR_PATCHLEVEL "PatchLevel"
static void _CheckAndApplyLevelPatches(void)
{
   char curfile[255];
   AutoAppIPtr(QuestData);
   BOOL existed=pQuestData->Exists(QUESTVAR_PATCHLEVEL);
   int old_ver=0;
   if (existed)
      old_ver=pQuestData->Get(QUESTVAR_PATCHLEVEL);
   if (old_ver<CURRENT_LEVEL_PATCH_VERSION)
   {  // if it is below our level, apply the diff
      dbCurrentFile(curfile, sizeof(curfile));
      PatchLevelData(curfile, old_ver, CURRENT_LEVEL_PATCH_VERSION);
      if (existed)
         pQuestData->Set(QUESTVAR_PATCHLEVEL, CURRENT_LEVEL_PATCH_VERSION);
      else
         pQuestData->Create(QUESTVAR_PATCHLEVEL, CURRENT_LEVEL_PATCH_VERSION, kQuestDataMission);
   }
}

static void FinishLoad(BOOL prep)
{
   AutoAppIPtr_(SimManager,pSimMan);

   // send the final post load
   DispatchData msg = { kDatabasePostLoad, 0 }; 
   DispatchMsg(kMsgDatabase,&msg); 

   // well, prep is always true, mysteriously
   // but in theory, it is where "level fixer up" sorta things happen...
   if (prep)
   {
      DarkPrepLevelForDifficulty();
      _CheckAndApplyLevelPatches();
   }

   // merge load doesn't suspend the sim, so we do it ourselves.
   pSimMan->SuspendSim(); 

   // now start things up again.
   //pSimMan->StartSim();

}

static void doDPCLoadFull(const char *fileroot)
{
   char temp[255],buf[255];
   char map_dir[_MAX_PATH];

   // Since all players are reloading at the same time, theoretically
   // back to the same state, any messages sent from here are at best
   // redundant. And we specifically need to *not* send any deregister
   // messages from the DB Reset, which can cause major problems, due
   // to message timing...
   AutoAppIPtr(NetManager);
   pNetManager->SuspendMessaging();

   // always get the map out of the official archive dir
   sprintf(temp,"%s.mis",fileroot);
   strcpy(map_dir,".");
   ITagFile* mapfile = DPCOpenPatchedTagFile(temp); 
   if (!mapfile)
   {
      // okay, couldn't find it local, so let's look in the archive dir
      if (!config_get_raw("archive_dir",map_dir,_MAX_PATH))
         strcpy(map_dir,MAP_DIR);
      sprintf(temp,"%s\\%s.mis",map_dir,fileroot);
      mapfile = DPCOpenPatchedTagFile(temp); 
   }

   dbMergeLoadTagFile(mapfile,kDBMap|kFiletypeGAM);
   // poke in the name, which is dumb, but needed now that we mergeload
   dbSetCurrentFile(temp); 
   SafeRelease(mapfile); 


   // load the mis file out of current
   sprintf(temp,"%s\\%s.mis",CURGAME_DIR, fileroot);
   ITagFile* missfile = NULL; 

   if (dbFind(temp,buf))
      missfile = dbOpenFile(buf,kTagOpenRead); 
   
   if (!missfile)
   {
      // that failed, so look in the archive
      sprintf(temp,"%s\\%s.mis",map_dir,fileroot);
      missfile = DPCOpenPatchedTagFile(temp); 
   }
   dbMergeLoadTagFile(missfile,kFiletypeMIS|kObjPartTerrain);
   SafeRelease(missfile); 

   pNetManager->ResumeMessaging();
}

void DPCLoadFull(const char *fileroot)
{
   PrepLoad();
   doDPCLoadFull(fileroot);
   ObjPos *pos = ObjPosGet(gPlayerObj);
   //mprintf("SLF: playerloc: %g %g %g\n", pos->loc.vec.x, pos->loc.vec.y, pos->loc.vec.z);
   FinishLoad(TRUE);
}

// Copies all files matching the filespec from one dir to the other
bool DPCCopyFiles(char *filespec, char *fromdir, char *todir)
{
   char curgame[_MAX_PATH];
   HANDLE hand;
   char oldfile[_MAX_PATH];
   char newfile[_MAX_PATH];
   WIN32_FIND_DATA filedata;
   bool fileiter = TRUE;
   bool retval = TRUE;

   sprintf(curgame,"%s\\%s",fromdir,filespec);
   hand = FindFirstFile(curgame,&filedata);
   if (hand == INVALID_HANDLE_VALUE)
      fileiter = FALSE;

   while (fileiter)
   {
      //GetShortPathName(filedata.cFileName, fileroot, _MAX_PATH);
      sprintf(oldfile,"%s\\%s",fromdir,filedata.cFileName);
      sprintf(newfile,"%s\\%s",todir,filedata.cFileName);
      CopyFile(oldfile,newfile,FALSE);

      if (FindNextFile(hand, &filedata) == 0)
      {
         if (GetLastError() != ERROR_NO_MORE_FILES)
         {
            Warning(("DPCCopyFiles: file iteration abnormally terminated!"));
            retval = FALSE;
         }
         fileiter = FALSE; 
      }
   }
   FindClose(&filedata);
   return(retval);
}

// Does a full actual "save" of the game
// Copies the current game dir out to a more permanent subdir
// Do we want to deal with thumbnails and save strings in here, or up
// at the interface level?
// Client code
static TagFileTag desc_tag = { "SAVEDESC" }; 
static TagVersion desc_ver = { 1, 1 };

static inline BOOL partially_invalid(const TagVersion& v)
{
   if (config_is_defined("load_old_saves"))
      return FALSE; 
   return VersionNumsCompare(&v,&desc_ver) < 0; 
}

static inline BOOL fully_invalid(const TagVersion& v)
{
   if (config_is_defined("load_old_saves"))
      return FALSE; 
   return v.major < desc_ver.major; 
}

struct sSaveGameDesc 
{
   char text[1024]; // string description 
   char level[32]; // which level are we actively in?
   ulong flags;
};

BOOL DPCGetSaveGameDesc(ITagFile *tagfile, char *buf, int buflen)
{
   sSaveGameDesc desc; 
   TagVersion v = desc_ver; 
   if (FAILED(tagfile->OpenBlock(&desc_tag,&v)))
   {
      Warning(("DPCGetSaveGameDesc: couldn't open desc block!\n"));
      return FALSE;
   }

   int len;
   len = tagfile->Read((char*)&desc,sizeof(desc)); 

   tagfile->CloseBlock();

   if (len != sizeof(desc))
   {
      Warning(("DPCGetSaveGameDesc: only read %d bytes (vs %d)\n",len,sizeof(desc)));
      return(FALSE);
   }
   else
   {
      strncpy(buf,desc.text,buflen);
      return(TRUE);
   }
}

HRESULT DoDPCSaveGame(int slot, char *savedesc)
{
   char savedir[_MAX_PATH];
   char curdir[_MAX_PATH];
   char newdir[_MAX_PATH];
   char curgame[_MAX_PATH];
   char temp[255];
   char curfile[255];

   /*
   DPCStringFetch(temp,sizeof(temp),"SaveStart","misc");
   DPCOverlayAddText(temp, DEFAULT_MSG_TIME);
   */

   // update our "misc data" before we go to save
   gMiscData.m_nomove = gNoMoveKeys;

   // Make sure that all the players have cleared out their buffers
   AutoAppIPtr(NetManager);
   if (pNetManager->Networking()) {
      // @TBD: put in a callback to manage a timeout here:
      // @NOTE: it is essential that the callers have somewhere called
      // PreFlush(). If this was kicked off by a network message,
      // PreFlush() should have been called right after we got that
      // message.
      pNetManager->SynchFlush(NULL, NULL);
   }

   getcwd(curdir,_MAX_PATH);

   if (slot != -1)
   {
      // Make sure the target directory exists
      DPCGetSaveDir(slot, savedir);
      //sprintf(savedir,"%s%d",SAVEROOT,slot);
   //   if (_chdir(savedir))
      if (chdir(savedir))
      {
         // dir does not exist, create it
         if (mkdir(savedir) != 0)
         {
            Warning(("DPCSaveGame: Could not create directory %d!\n",savedir));
         }
      }
      chdir(curdir);
   }

   // Save out the current state of the game to the current directory
   dbCurrentFile(curfile, sizeof(curfile));
   sprintf(temp, "%s\\%s\\%s", curdir, CURGAME_DIR, curfile);
   temp[strlen(temp) - 4] = '\0'; // to strip out extension

   dbSave(temp,kFiletypeMIS|kObjPartTerrain|kObjPartBriefcase);

   // write the descriptor
   //
   sprintf(temp,"%s\\%s\\%s",curdir, CURGAME_DIR, CAMPAIGN_FNAME);
   ITagFile* file = TagFileOpen(temp,kTagOpenWrite); 

   TagVersion v = desc_ver; 
   if (FAILED(file->OpenBlock(&desc_tag,&v)))
      return kDPCSaveBlockOpen; 
   
   sSaveGameDesc desc = { "", "", 0 }; 
   strcpy(temp, curfile);
   temp[strlen(temp) - 4] = '\0'; // to strip out extension
   strncpy(desc.level,temp,sizeof(desc.level)); 
   if (savedesc != NULL) // && (strlen(savedesc) > 0))
      strncpy(desc.text,savedesc,sizeof(desc.text));
   else
      strcpy(desc.text,"n/a");

   int len = file->Write((char*)&desc,sizeof(desc)); 

   if (FAILED(file->CloseBlock()))
      return kDPCSaveBlockClose; 
   if (len != sizeof(desc))
      return kDPCSaveReadWrite; 

   // save out the campaign file 
   AutoAppIPtr(Campaign); 
   if (FAILED(pCampaign->Save(file)))
      return kDPCSaveCampaign;
   SafeRelease(file); 

   // now iterate over all the files in the "current" directory
   // and copy them into our save dir.
   if (slot != -1)
   {
      sprintf(curgame,"%s\\%s",curdir,CURGAME_DIR);
      sprintf(newdir,"%s\\%s",curdir,savedir);

      // first lets clean out everything
      DPCEraseDirectory(newdir);

      // now do the actual copy
      if (DPCCopyFiles("*.*",curgame,newdir))
         Status("Save Game Successful.");
      else
         Status("Save Game Failed.");
   }

   DPCStringFetch(temp,sizeof(temp),"SaveEnd","misc");
   DPCOverlayAddText(temp, DEFAULT_MSG_TIME);
   return(S_OK);
}

HRESULT DPCSaveGame(int slot, char *savedesc)
{
   g_pSaveGameMsg->Send(OBJ_NULL, slot, savedesc);
   AutoAppIPtr(NetManager);
   if (pNetManager->Networking())
   {
      pNetManager->PreFlush();
   }
   HRESULT retval = DoDPCSaveGame(slot, savedesc);
   return(retval);
}

int gQuickSaveHack = 0;

void DPCQuickSaveHack()
{
   char temp[255];

   // We don't allow save on earth/station
   AutoAppIPtr(NetManager);
   if (pNetManager->IsNetworkGame())
   {
      char levelname[64];
      DPCGetLevelName(levelname,sizeof(levelname));
      // okay this is hack-a-riffic
      if ((stricmp(levelname,"station") == 0) || (stricmp(levelname,"earth") == 0))
      {
         if (DPCStringFetch(temp, sizeof(temp), "EarlySave", "misc"))
         {
            DPCOverlayAddText(temp, DEFAULT_MSG_TIME);
         }
         return;
      }
   }

   DPCStringFetch(temp,sizeof(temp),"SaveStart","misc");
   DPCOverlayAddText(temp, DEFAULT_MSG_TIME);
   gQuickSaveHack = 2;
}

void DPCQuickSave()
{
   // throw any objects on the cursor into the world
   if (drag_obj != OBJ_NULL)
   {
      ThrowObj(drag_obj,PlayerObject());
      ClearCursor();
   }
   char temp[255];
   DPCStringFetch(temp,sizeof(temp),"QuickSaveName","misc");

   // make sure we save overlay state from mouse mode.
   if (DPC_mouse)
      MouseMode(FALSE,TRUE);

   // refresh the overlay state so that it is saved out correctly
   DPCOverlayComputeRestore();

   DPCSaveGame(-1,temp);
}

// This code gets called when some other player initiates a save.
// We print out a message, wait a frame or so for it to display, then
// do the save ourselves.
int gRemoteSaveHack = 0;
int gRemoteSaveSlot = 0;
char *gpRemoteSaveDesc = NULL;
static void handleSaveGame(int slot, const char *savedesc)
{
   AutoAppIPtr(NetManager);
   if (pNetManager->Networking())
   {
      pNetManager->PreFlush();
   }

   // throw any objects on the cursor into the world
   if (drag_obj != OBJ_NULL)
   {
      ThrowObj(drag_obj,PlayerObject());
      ClearCursor();
   }

   gRemoteSaveSlot = slot;
   if (savedesc == NULL) {
      gpRemoteSaveDesc = NULL;
   } else {
      gpRemoteSaveDesc = (char *) malloc(strlen(savedesc) + 1);
      strcpy(gpRemoteSaveDesc, savedesc);
   }
   char temp[255];
   DPCStringFetch(temp,sizeof(temp),"SaveStart","misc");
   DPCOverlayAddText(temp, DEFAULT_MSG_TIME);

   // refresh the overlay state so that it is saved out correctly
   // Note that we could be in metagame or something, where the overlays
   // aren't real. We count on this method to be smart enough to *not*
   // compute the restores under such circumstances:
   DPCOverlayComputeRestore();

   gRemoteSaveHack = 2;
}

// Load in the game from the numbered slot.
// Client code
static HRESULT doDPCLoadGame(int slot)
{
   char curdir[_MAX_PATH];
   char curgame[_MAX_PATH];
   char savedir[_MAX_PATH];
   char temp[255];
   AutoAppIPtr(NetManager);

   /*
   DPCStringFetch(temp,sizeof(temp),"LoadStart","misc");
   DPCOverlayAddText(temp, DEFAULT_MSG_TIME);
   */

   getcwd(curdir,_MAX_PATH);

   if (slot != -1)
   {
      sprintf(curgame,"%s\\%s",curdir,CURGAME_DIR);
      char savesubdir[_MAX_PATH];
      DPCGetSaveDir(slot, savesubdir);
      sprintf(savedir,"%s\\%s",curdir,savesubdir);
      // clear out everything in current first
      DPCEraseDirectory(curgame);

      // then copy over all the relevant junk from our save game slot
      if (DPCCopyFiles("*.mis",savedir,curgame) &&  DPCCopyFiles(CAMPAIGN_FNAME,savedir,curgame))
      {
         Status("Load Game Successful.");
      }
      else
         Status("Game Load Failed.");
   }

   //
   // Read the descriptor
   //
   sprintf(temp,"%s\\%s\\%s",curdir, CURGAME_DIR, CAMPAIGN_FNAME);
   ITagFile* file = TagFileOpen(temp,kTagOpenRead); 

   if (file == NULL)
      return kDPCLoadNoFile;

   sSaveGameDesc desc; 
   TagVersion v = desc_ver; 
   if (FAILED(file->OpenBlock(&desc_tag,&v)))
      return kDPCSaveBlockOpen; 

   int len = file->Read((char*)&desc,sizeof(desc)); 

   if (FAILED(file->CloseBlock()))
      return kDPCSaveBlockClose; 

   if (len != sizeof(desc))
   {
      return kDPCSaveReadWrite; 
   }

   // Invalidate save games
   if (fully_invalid(v))
      return kDPCSaveVersionInvalid; 

   // load in the campaign file
   // Since all players are reloading at the same time, theoretically
   // back to the same state, any messages sent from here are at best
   // redundant. And we specifically need to *not* send any deregister
   // messages from the DB Reset, which can cause major problems, due
   // to message timing...
   pNetManager->SuspendMessaging();

   AutoAppIPtr(Campaign); 
   if (FAILED(pCampaign->Load(file)))
      return kDPCSaveCampaign;

   SafeRelease(file);

   // now load up the actual individual mission that we were 
   // on when we saved
   PrepLoad();
   doDPCLoadFull(desc.level);
   FinishLoad(TRUE);
   pNetManager->ResumeMessaging();

   // go restore any "misc" data
   gNoMoveKeys = gMiscData.m_nomove;

   // give player feedback
   DPCStringFetch(temp,sizeof(temp),"LoadEnd","misc");
   DPCOverlayAddText(temp, DEFAULT_MSG_TIME);

   // make sure we didn't lose any ups in the big delay
   g_pInputBinder->PollAllKeys();

   return S_OK; 

}

HRESULT DPCLoadGame(int slot)
{
   int retval;
   retval = doDPCLoadGame(slot);
   return(retval);
}

void DPCQuickLoad()
{
   // throw any objects on the cursor into the world
   if (drag_obj != OBJ_NULL)
   {
      ThrowObj(drag_obj,PlayerObject());
      ClearCursor();
   }
   //DPCLoadGame(-1);
   TransModeLoadGame(-1);
}

//
// Open a mission file, using cool dif technology. 
//

static ITagFile* find_and_open_file(const char* filename)
{
   char full[256]; 
   if (dbFind(filename,full))
      return dbOpenFile(full,kTagOpenRead);
   return NULL; 
   
}

ITagFile* DPCOpenPatchedTagFile(const char* name)
{
   char base[64]; 
   strncpy(base,name,sizeof(base)); 
   base[sizeof(base)-1] = '\0'; 

   // find the extension 
   char* dot = strrchr(base,'.'); 
   if (dot)
      strcpy(dot+1,"mis");
   else
   {
      strcat(base,".mis"); 
      dot = strrchr(base,'.'); 
   }
   
   // Open the mis file
   ITagFile* basefile = find_and_open_file(base); 

   if (!basefile)
      return NULL; 

   // Open the dif file
   strcpy(dot+1,"dif"); 
   ITagFile* diffile = find_and_open_file(base); 

   if (diffile)
   {
      ITagFile* result = CreatePatchedTagFile(basefile,diffile); 
      SafeRelease(basefile); 
      SafeRelease(diffile);
      return result;
   }
   else 
      return basefile; 
}


// Intended for command-line only.
// Load the numbered slot, and get the named level from it.
/*
void DPCLoadGameAndLevel(char *slotAndLevel)
{
   int slot;
   char levelName[255];

   sscanf(slotAndLevel,"%d %s", &slot, levelName);

   DPCLoadGame(slot);
   DPCLoadFull(levelName);
}
*/

static sPropertyDesc DPCElevAbleDesc =
{
   "ElevAble", 0, 
   NULL, 0, 0, // constraints, versions
   { "MultiLevel", "Elevator-able?" },
};

static sPropertyDesc DPCInittedDesc =
{
   "Initted", 0, 
   NULL, 0, 0, // constraints, versions
   { "MultiLevel", "Script Initted?" },
};

static sPropertyDesc DPCStartLocDesc =
{
   "StartLoc", 0, 
   NULL, 0, 0, // constraints, versions
   { "MultiLevel", "Start Loc" },
};

static sPropertyDesc DPCDestLocDesc =
{
   "DestLoc", 0, 
   NULL, 0, 0, // constraints, versions
   { "MultiLevel", "Dest Loc" },
};

static sPropertyDesc DPCDestLevelDesc =
{
   "DestLevel", 0, 
   NULL, 0, 0, // constraints, versions
   { "MultiLevel", "Dest Level" },
};

static sPropertyDesc DPCElevOffsetDesc =
{
   "ElevOffset", 0, 
   NULL, 0, 0, // constraints, versions
   { "MultiLevel", "Elevator Offset" },
};

////////////////////////////////////////
//
// NETWORK CODE
//

//
// The stored-proxy property
//
// This little bit of nastiness is here to deal with a consequence of
// level transitions. If a player picks something up in one level, then
// transits, and drops it in a different level, the object needs to have
// a proxy, since it's ObjID is likely to be different on all clients.
// But if the owner of the object (usually the world host) then leaves
// the level, and comes back to it, the clients forget about the proxy
// information, since that's stored as a link to the object.
//
// So, just before we save the level, we store the proxy information in
// an immutable form, which can be rebuilt later.
//
// @HACK: the form is pretty ugly, but I'm trying to get this working ASAP,
// and integer properties are easiest to whip in. So we're storing it as
// an integer, combining the player number with the remote ObjID...
//
static sPropertyDesc DPCProxyStoreDesc =
{
   "ProxyStore",
   kPropertyNoEdit,
   NULL, 0, 0, // constraints, versions
   { "Networking", "Proxy Storage" },
   kPropertyChangeLocally,
};

static IIntProperty *gDPCProxyStoreProp;

static void BundleProxy(ObjID obj) 
{
   AutoAppIPtr(NetManager);
   AutoAppIPtr(ObjectNetworking);

   // Okay, what's the proxy info for this thing?
   ObjID playerObj;
   NetObjID objOnHost;
   pObjectNetworking->ObjHostInfo(obj, &playerObj, &objOnHost);
   if ((playerObj == pNetManager->DefaultHost()) &&
       (objOnHost == obj))
   {
      // It's implicitly hosted, so don't sweat it
      return;
   }
   int playerNum = pNetManager->ObjToPlayerNum(playerObj);
   AssertMsg((playerNum < 0xFF), "Player number preposterously high!");
   if (playerNum == 0) {
      Warning(("Got zero player num for proxy %d (obj %d:%d)!\n", 
               obj, playerObj, objOnHost));
      return;
   }

   // Bundle it up:
   int bundle = (playerNum << 16) | ((short) objOnHost);
   gDPCProxyStoreProp->Set(obj, bundle);

   ConfigSpew("net_proxy_spew", ("Bundled up proxy %d for %d:%d as %x\n",
                                 obj, playerObj, objOnHost, bundle));
}

static void UnbundleIfProxy(ObjID obj)
{
   // Is this a bundled proxy?
   if (!gDPCProxyStoreProp->IsRelevant(obj)) {
      // Nope
      return;
   }

   int bundle;
   if (!gDPCProxyStoreProp->Get(obj, &bundle)) {
      return;
   }

   // Okay, unbundle the proxy info:
   AutoAppIPtr(NetManager);
   AutoAppIPtr(ObjectNetworking);

   ObjID playerNum = (bundle & 0xFF0000) >> 16;
   ObjID playerObj = pNetManager->PlayerNumToObj(playerNum);
   if (playerObj == OBJ_NULL) {
      Warning(("Can't find the object for player #%d from bundle %x (obj %d)!\n", 
               playerNum, bundle, obj));
      return;
   }
   NetObjID objOnHost = bundle & 0xFFFF;
   pObjectNetworking->ObjRegisterProxy(playerObj, objOnHost, obj);

   ConfigSpew("net_proxy_spew", ("Unbundled proxy %d for %d:%d from %x\n",
                                 obj, playerObj, objOnHost, bundle));

   // And kill the bundle:
   gDPCProxyStoreProp->Delete(obj);
}

//////////
//
// The save-game message
//

sNetMsgDesc sSaveGameDesc = {
   kNMF_MetagameBroadcast,
   "SaveGame",
   "SaveGame",
   NULL,
   handleSaveGame,
   {{kNMPT_Int, kNMPF_None, "Slot"},
    {kNMPT_String, kNMPF_None, "Save Desc"},
    {kNMPT_End}}
};

////////////////////////////////////////////////////////////
//
// "Landing Point" relation
//
// A level-destination marker should have one landing point marker for
// each player, connected by this relation.

static void createLandingPointRel(void)
{
   static sRelationDesc reldesc = { "LandingPoint" }; 
   static sRelationDataDesc datadesc = LINK_DATA_DESC(int);
   gpLandingRel = CreateStandardRelation(&reldesc,&datadesc,0); 
}

void DPCMultiLevelInit(void)
{
   gDPCStartLocProp = CreateIntProperty(&DPCStartLocDesc, kPropertyImplDense);
   gDPCDestLocProp = CreateIntProperty(&DPCDestLocDesc, kPropertyImplDense);
   gDPCElevAbleProp = CreateBoolProperty(&DPCElevAbleDesc, kPropertyImplDense);
   gDPCInittedProp = CreateBoolProperty(&DPCInittedDesc, kPropertyImplDense);
   gDPCDestLevelProp = CreateLabelProperty(&DPCDestLevelDesc, kPropertyImplDense);
   gDPCElevOffsetProp = CreateVectorProperty(&DPCElevOffsetDesc, kPropertyImplDense);
   createLandingPointRel();
   gDPCProxyStoreProp = CreateIntProperty(&DPCProxyStoreDesc, kPropertyImplDense);
   g_pSaveGameMsg = new cNetMsg(&sSaveGameDesc);

   // register the filter that describes the "briefcase" sub-partition 
   AutoAppIPtr_(ObjectSystem,pObjSys);
   sObjPartitionFilter filter = { inv_partition_func}; 
   pObjSys->SetSubPartitionFilter(kObjPartBriefcase,&filter); 
}
void DPCMultiLevelTerm(void)
{
   delete g_pSaveGameMsg;
}

////////////////////////////////////////////////////////////

void DPCBeginGame()
{
   // clear out the "current" directory
   char curdir[_MAX_PATH];
   char temp[_MAX_PATH];
   getcwd(curdir,_MAX_PATH);
   sprintf(temp,"%s\\%s",curdir,CURGAME_DIR);
   DPCEraseDirectory(temp);
}

////////////////////////////////////////////////////////////
void DPCEraseDirectory(char *dir)
{
   char temp[_MAX_PATH];
   cFilePath cpath(dir);
   sFindContext context;
   cFileSpec fspec(cpath,"*.*");
   BOOL go = FALSE;
   if (cpath.FindFirst(fspec,context))
      go = TRUE;
   while (go)
   {
      //mprintf("filename is %s\n",fspec.GetFileName());
      sprintf(temp,"%s\\%s",dir,fspec.GetFileName());
      remove(temp);
      go = cpath.FindNext(fspec, context);
   }
   cpath.FindDone(context);
}
////////////////////////////////////////////////////////////
void DPCGetLevelName(char *str, int len)
{
   char curfile[255], temp[255];
   dbCurrentFile(curfile, sizeof(curfile));
   strcpy(temp, curfile); 
   temp[strlen(curfile) - 4] = '\0'; // to strip out extension
   strncpy(str,temp,len);
}
