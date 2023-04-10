/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <appagg.h>

#include <scrptapi.h>
#include <scrptsrv.h>
#include <scrptbas.h>

#include <gen_bind.h>

#include <traitman.h>
#include <traitbas.h>
#include <filevar.h>
#include <esnd.h>
#include <objquery.h>
#include <tweqctrl.h>
#include <dbfile.h>
#include <questapi.h>
#include <simtime.h>
#include <phmod.h>
#include <phmods.h>
#include <phcore.h>
#include <physapi.h>
#include <physcast.h>

#include <dpcincst.h>
#include <dpctrcst.h>

#include <dpcmenu.h>
#include <dpcgame.h>
#include <dpcscrpt.h>
#include <dpcinv.h>
#include <dpcovrly.h>
#include <dpcovcst.h>
#include <dpcbooko.h>
#include <dpccmapi.h>
#include <dpcplpup.h>
#include <dpccont.h>
#include <dpcplayr.h>
#include <dpcyorn.h>
#include <dpckeypd.h>
#include <dpcmprop.h>
#include <dpcmulti.h>
#include <dpcprop.h>
#include <gunapi.h>
#include <dpcpdcst.h>
#include <dpcplprp.h>
#include <dpcplcst.h>
#include <dpcpda.h>
#include <dpcutils.h>
#include <dpcspawn.h>
#include <dpcobjst.h>
#include <dpchrm.h>
#include <dpchazpr.h>
#include <dpcemail.h>
#include <dpcscapi.h>
#include <dpcrend.h>
// #include <dpcexp.h>
#include <dpcmap.h>
#include <dpcnet.h>
#include <dpcalarm.h>
#include <dpcparam.h>
#include <dpcpgapi.h>
#include <gunprop.h>
#include <dpcpldmg.h>
#include <dpctluct.h>
#include <dpcloot.h>
#include <transmod.h>
#include <dpclding.h>
#include <dpcmelee.h>
#include <dpchud.h>

#include <schema.h>
#include <appsfx.h>

#include <iobjsys.h>
#include <questapi.h>
#include <playrobj.h>
#include <plyrmov.h>
#include <contain.h>
#include <keysys.h>
#include <lockprop.h>
#include <keyprop.h> 
#include <gamestr.h>
#include <objhp.h>
#include <objpos.h>
#include <rand.h>
#include <traitman.h>
#include <wrtype.h>
#include <panltool.h>
#include <plyrmode.h>
#include <netman.h>
#include <iobjnet.h>
#include <netmsg.h>

#include <linkman.h>
#include <linkscpt.h>
#include <linkbase.h>
#include <relation.h>
#include <matrix.h>
#include <property.h>
#include <propbase.h>

#include <config.h>

#include <frobctrl.h>
#include <dpcfsys.h>

#include <ghostapi.h>

#include <aiman.h>

#include <phmterr.h>

#include <dbmem.h>

extern IInputBinder *g_pInputBinder;
EXTERN BOOL gNoMoveKeys, gNoLookAround;

//
// Deep Cover-specific script implementation goes here
//

// this is actually the distance squared
#define BASE_SPAWN_DIST 30.0 // who knows if this is right.  Maybe it should become a mission parameter?
#define MAX_PLAYER_SPAWN_DIST (BASE_SPAWN_DIST * BASE_SPAWN_DIST) 

static BOOL gScriptAllowSwap;

////////////////////////////////////////
//
// Networking code
//

////////////////////////////////////////
// 
// Deep Cover gamesys scripting service implementation
//


void InitLinks(); 

DECLARE_SCRIPT_SERVICE_IMPL(cDPCGameSrv, DPCGame)
{
public:

   // Behold the power of script service initializers
   // MAHK 9/10
   STDMETHOD_(void,Init)()
   {
      // This is the dumb part. We don't init the links  
      // if we're not running DPC 
#ifdef PLAYTEST
      char buf[16];
      if (config_get_raw("game",buf,sizeof(buf)) 
          && stricmp(buf,"deepc") != 0)
         return ; 
#endif 

      InitLinks(); 

   }

   STDMETHOD_(void, End)()
   {
   }

   STDMETHOD(DestroyCursorObj)()
   {      
      DPCInvDestroyCursorObj();
      return S_OK;
   }

   STDMETHOD(DestroyInvObj)(const object ref DestroyObj)
   {
      ObjID victim = ScriptObjID(DestroyObj);
      if (victim == drag_obj)
         ClearCursor();

      // inv system will be automatically notified when object goes away
      IObjectSystem* pOS = AppGetObj(IObjectSystem);
      pOS->Destroy(victim);
      SafeRelease(pOS);

      return TRUE;
   }

   STDMETHOD(DestroyAllByName)(const char *name)
   {
      AutoAppIPtr(ObjectSystem);
      AutoAppIPtr(TraitManager);
      ObjID arch, obj;
      IObjectQuery* query;

      // find the archetype
      arch = pObjectSystem->GetObjectNamed(name);

      if (OBJ_IS_CONCRETE(arch))
      {
         Warning(("DestroyAllByName: %s is not abstract!\n",name));
         return(E_FAIL);
      }

      // now blow away all concrete objects that derive from it
      query = pTraitManager->Query(arch,kTraitQueryAllDescendents);
      while (!query->Done())
      {
         obj = query->Object();
         if (OBJ_IS_CONCRETE(obj))
         {
            pObjectSystem->Destroy(obj);
         }
         query->Next();
      }
      SafeRelease(query);

      return(S_OK);
   }

   STDMETHOD(SlayAllByName)(const char *name)
   {
      AutoAppIPtr(ObjectSystem);
      AutoAppIPtr(TraitManager);
      AutoAppIPtr(DamageModel);
      ObjID arch, obj;
      IObjectQuery* query;

      // find the archetype
      arch = pObjectSystem->GetObjectNamed(name);

      if (OBJ_IS_CONCRETE(arch))
      {
         Warning(("DestroyAllByName: %s is not abstract!\n",name));
         return(E_FAIL);
      }

      // now blow away all concrete objects that derive from it
      query = pTraitManager->Query(arch,kTraitQueryAllDescendents);
      while (!query->Done())
      {
         obj = query->Object();
         if (OBJ_IS_CONCRETE(obj))
         {
            pDamageModel->SlayObject(obj,obj);
         }
         query->Next();
      }
      SafeRelease(query);

      return(S_OK);
   }

   STDMETHOD(TweqAllByName)(const char *name, BOOL state) 
   {
      AutoAppIPtr(ObjectSystem);
      AutoAppIPtr(TraitManager);
      ObjID arch, obj;
      IObjectQuery* query;

      // find the archetype
      arch = pObjectSystem->GetObjectNamed(name);

      if (OBJ_IS_CONCRETE(arch))
      {
         Warning(("TweqAllByName: %s is not abstract!\n",name));
         return(E_FAIL);
      }

      // now blow away all concrete objects that derive from it
      query = pTraitManager->Query(arch,kTraitQueryAllDescendents);
      while (!query->Done())
      {
         obj = query->Object();
         if (OBJ_IS_CONCRETE(obj))
         {
            if (state)
            {
               TweqProcessAll(obj,kTweqDoActivate);
            }
            else
            {
               TweqProcessAll(obj,kTweqDoHalt);
            }
         }
         query->Next();
      }
      SafeRelease(query);

      return(S_OK);
   }

   STDMETHOD(HideInvObj)(const object ref DestroyObj)
   {
      ObjID victim = ScriptObjID(DestroyObj);

      // take object out of inventory, but don't replace in world
      AutoAppIPtr(ContainSys);
      pContainSys->Remove(PlayerObject(), victim);

      return TRUE;
   }

   STDMETHOD_(BOOL, AddInvObj)(const object ref obj)
   {
      ObjID objID = ScriptObjID(obj);
      return DPCInvAddObj(PlayerObject(), objID);
   }

   STDMETHOD(SetLogTime)(int level, int logtype, int which)
   {
      DPCPDASetTime(level,logtype,which);
      return(S_OK);
   }

   STDMETHOD(UseLog)(const object ref LogObj, BOOL PickedUpByMe)
   {
      ObjID obj = ScriptObjID(LogObj);
      int uselevel, usewhich,i;
      for (i=0; i < 4; i++)
      {
         if (FindLogData(LogObj, i, &uselevel, &usewhich) == S_OK)
            break;
      }
      if (i >= 4)
         return(S_FALSE);

      // play the content, and bring up the MFD
      //TriggerLog(i,uselevel,usewhich,FALSE);
      DPCPDANeedLog(i, uselevel, usewhich);

      char temp[255], fmt[255];
      char name[255], levelname[32];
      sprintf(levelname,"level%02d",uselevel + 1);
      DPCStringFetch(name,sizeof(name),"logname",levelname,usewhich+1);
      for (i=0; i < strlen(name); i++)
      {
         // find the first newline and terminate the string there.
         if (name[i] == '\n')
         {
            name[i] = '\0';
            break;
         }
      }

      if (PickedUpByMe)
      {
         // I got it, and am transmitting it to everyone else
         if (DPCStringFetch(fmt,sizeof(fmt),"LogPickup","misc"))
         {
            // Show my own display about this pickup happening:
            sprintf(temp,fmt,name);
            DPCOverlayAddText(temp,DEFAULT_MSG_TIME);
         }
         // Tell the other players about it. Note that the actual
         // broadcast of the log happens at the script level, but
         // we send the text separately, while we know the sending
         // player's name:
         AutoAppIPtr(NetManager);
         if (pNetManager->Networking() &&
             DPCStringFetch(fmt,sizeof(fmt),"LogReceived","Network"))
         {
            // Note that this makes assumptions about the string:
            sprintf(temp, fmt, 
                    name, pNetManager->GetPlayerName(OBJ_NULL));
            DPCSendAddText(OBJ_NULL, temp);
         }

         // finally, shoot the messenger!
         // Doesn't really matter which player does this, so long as
         // only one does:
         IObjectSystem* pOS = AppGetObj(IObjectSystem);
         pOS->Destroy(obj);
         SafeRelease(pOS);
      }

      return(S_OK);
   }

   // types: 0: email 1: log 2: notes 3: infokiosk
   STDMETHOD_(BOOL,TriggerLog)(int usetype, int uselevel, int which, BOOL show_mfd) 
   {
      if (usetype <= 1)
      {
         if (DPCPDANeedLog(usetype, uselevel, which)) {
            DPCPDAUseLog(usetype,uselevel,which,show_mfd);
            return(TRUE);
         } else {
            return(FALSE);
         }
      }
      else
      {
         DPCPDAUseLog(usetype, uselevel, which, show_mfd);
         return(TRUE);
      }
   }

   STDMETHOD(FindLogData)(const object ref LogObj, int usetype, int *plevel, int *pwhich)
   {
      ObjID obj = ScriptObjID(LogObj);
      return(DPCFindLogData(obj, usetype, plevel, pwhich));
   }

   STDMETHOD(AttachCamera)(const string ref s)
   {
      AutoAppIPtr(DPCCamera);
      pDPCCamera->Attach((const char *)s);
      return S_OK;
   }
   STDMETHOD(CutSceneModeOff)(void)
   {
      AutoAppIPtr(DPCCamera);
      pDPCCamera->SetEditMode(kCameraNormal);
      return S_OK;
   }
   STDMETHOD(CutSceneModeOn)(const string ref sceneName)
   {
      AutoAppIPtr(DPCCamera);
      pDPCCamera->SetScene((const char*)sceneName);
      pDPCCamera->SetEditMode(pDPCCamera->GetEditState());
      return S_OK;
   }
   STDMETHOD_(int, CreatePlayerPuppet)(const string ref modelName)
   {
      return PlayerPuppetCreate((const char *)modelName);
   }
   STDMETHOD_(int, CreatePlayerPuppet)(void)
   {
      AutoAppIPtr(DPCPlayer);
      return PlayerPuppetCreate((const char *)(pDPCPlayer->GetExternalModel()));
   }
   STDMETHOD(DestroyPlayerPuppet)() 
   {
      PlayerPuppetDestroy();
      return TRUE;
   }
   STDMETHOD(Container)(const object ref ContainObj)
   {
      ObjID cont = ScriptObjID(ContainObj);
      DPCContainerOpen(cont);
      return(S_OK);
   }

   STDMETHOD(TechHacking)(const object ref TechObj)
   {
      /*
      ObjID o = ScriptObjID(TechObj);
      DPCOverlayChange(kOverlayHacking,kOverlayModeOn);
      DPCHackingBegin(o);
      */
      return(S_OK);
   }

   STDMETHOD(YorN)(const object ref BaseObj, const string ref s)
   {
      DPCYorNPopup(ScriptObjID(BaseObj), (const char *)s);
      return(S_OK);
   }

   STDMETHOD(Keypad)(const object ref BaseObj)
   {
      DPCKeypadOpen(ScriptObjID(BaseObj));
      return(S_OK);
   }

   STDMETHOD(PayNanites)(int quan)
   {
      if (DPCInvPayNanites(quan))
         return(S_OK);
      else
         return(E_FAIL);
   }

   STDMETHOD(OverlayChange)(int which, int mode)
   {
      DPCOverlayChange(which,mode);
      return(S_OK);
   }

   STDMETHOD(OverlaySetObj)(int which, const object ref Obj)
   {
      ObjID obj = ScriptObjID(Obj);
      DPCOverlaySetObj(which, obj);
      return(S_OK);
   }

   STDMETHOD_(object, OverlayGetObj)() 
   {
      return(DPCOverlayGetObj());
   }

   STDMETHOD(OverlayChangeObj)(int which, int mode, const object ref Obj)
   {
      ObjID obj = ScriptObjID(Obj);
      DPCOverlayChangeObj(which, mode, obj);
      return(S_OK);
   }

   STDMETHOD_(BOOL, OverlayOn)(int which) 
   {
      return(DPCOverlayCheck(which));
   }

   STDMETHOD_(object, Equipped)(int slot)
   {
      AutoAppIPtr(DPCPlayer);
      return(pDPCPlayer->GetEquip(PlayerObject(), (ePlayerEquip)slot));
   }

   STDMETHOD(Equip)(int slot, const object ref Obj)
   {
      ObjID obj = ScriptObjID(Obj);
      AutoAppIPtr(DPCPlayer);

      pDPCPlayer->Equip(PlayerObject(), (ePlayerEquip)slot, obj,TRUE);
      return(S_OK);
   }

   STDMETHOD(WearArmor)(const object ref Obj)
   {
      ObjID obj = ScriptObjID(Obj);
      if (obj == OBJ_NULL)
      {
         AutoAppIPtr(DPCPlayer);
         ObjID armor = pDPCPlayer->GetEquip(PlayerObject(),kEquipArmor);
         DPCUnequipArmor(PlayerObject(),armor);
      }
      else
         DPCEquipArmor(PlayerObject(),obj);
      return(S_OK);
   }

   STDMETHOD(LevelTransport)(const char *newlevel, int marker, uint flags)
   {
      // We need to make sure we don't re-enter this. TransModeSwitchLevel
      // processes net messages under the hood, so re-entrance is an issue.
      if (gbDPCTransporting)
      {
         ConfigSpew("net_spew", 
                    ("Redundant call to level transport rejected.\n"));
         return(S_FALSE);
      }

      // clean up anything relevant
      AutoAppIPtr(NetManager);
      AutoAppIPtr(ScriptMan);

      ObjID netobj;

      FOR_ALL_PLAYERS(pNetManager,&netobj)
      {
         sScrMsg msg(netobj, "EndLevel"); 
         msg.flags |= kSMF_MsgPostToOwner;
         pScriptMan->SendMessage(&msg); 
      }

      //pScriptMan->PumpMessages(); // safe?

      TransModeSwitchLevel(newlevel,marker,flags); 
      return(S_OK);
   }

   STDMETHOD(LevelTransport)(const char *newlevel, object ref marker, uint flags)
   {
      /*
      ObjID markerobj = ScriptObjID(marker);
      int markerval;
      gDPCStartLocProp->Get(markerobj, &markerval);

      strcpy(gTransLevel,newlevel);
      gTransMarker = markerval;
      gTransFlags = flags;
      */
      return(S_OK);
   }

   STDMETHOD(SpewLockData)(const object ref LockObj, BOOL success)
   {
      ObjID obj = ScriptObjID(LockObj);
      sKeyInfo *keyinfo;
      int region;
      int lockid;
      extern char *access_bit_names[];
      char temp[255];
      int bit;

      // delve into the guts of how we use keys and 
      // spew to the status line some relevant data
      KeyDstProp->Get(obj,&keyinfo);
      region = keyinfo->region_mask;
      lockid = keyinfo->lock_id;

      // normal "access card" bits are stored in the region ID, and all access cards 
      // will have the master bit set.
      // one-shot keys will actually use their lock_id and region 0.

      for (bit = 0; bit < 32; bit++)
      {
         if ((0x1 << bit) & region)
            break;
      }
      // hm, no bit data
      if (bit >= 32)
      {
         Warning(("SpewLockData:: no bit information for bit %d (%x)\n",bit,region));
         return(S_OK);
      }

      // create and output the string
      char bitname[255];
      char fmt[255];
      DPCStringFetch(bitname,sizeof(bitname),"Access","misc",bit);
      if (success)
         DPCStringFetch(fmt,sizeof(fmt),"AccessUsed","misc");
      else
         DPCStringFetch(fmt,sizeof(fmt),"AccessRequired","misc");
      sprintf(temp,fmt,bitname);

      // Send the string to whoever started this whole process. Since
      // we're buried way down, ask the network manager who that was:
      AutoAppIPtr(NetManager);
      DPCSendAddText(pNetManager->OriginatingPlayer(),
                       temp,
                       DEFAULT_MSG_TIME);
      // DPCOverlayAddText(temp,DEFAULT_MSG_TIME);
      //mprintf("SpewLockData: %d, region bits 0x%x, lock id %d\n",success, region, lockid);
      return(S_OK);
   }

   STDMETHOD_(BOOL, CheckLocked)(const object ref CheckObj, 
				 BOOL verbose,
				 const object ref PlayerRef)
   {
      AutoAppIPtr(ContainSys);
      AutoAppIPtr(KeySys);
      sKeyInfo *key, *lock;
      ObjID obj;

      obj = ScriptObjID(CheckObj);

      // if not locked at all, that's the easy case
      if (!ObjSelfLocked(obj))
         return(FALSE);

      if (!KeyDstProp->Get(obj,&lock))
         return TRUE;

      // since we never want to unlock "quietly", don't even attempt to if not verbose
      if (!verbose)
         return(TRUE);

      ObjID player = ScriptObjID(PlayerRef);

      // We don't know which player to go sifting through.
      if (player == OBJ_NULL)
      {
         Warning(("CheckLocked should have a Player if verbose.\n"));
         return(TRUE);
      }

      // okay, we are locked, so sift through the player's inventory
      // to look for a key object with the right bits
      sContainIter *iterp;
      iterp = pContainSys->IterStart(player);
      while (!iterp->finished)
      {
         ObjID invobj = iterp->containee;
         if (KeySrcProp->IsRelevant(invobj))
         {
            KeySrcProp->Get(invobj,&key);
            if (KeyWorksOnLock(key,lock))
            {
               if (verbose)
                  SpewLockData(CheckObj, TRUE);
               ObjSetSelfLocked(CheckObj, FALSE);
               pContainSys->IterEnd(iterp);
               return(FALSE);
            }
         }
         pContainSys->IterNext(iterp);
      }
      pContainSys->IterEnd(iterp);

      SchemaPlay((Label *)"cardfail",NULL);         

      // if none of the keys in our inventory matched, then spew
      // some text to the status line to give the player feedback
      if (gPropLockedMessage->IsRelevant(obj))
      {
         AutoAppIPtr(GameStrings);
         cStr str = pGameStrings->FetchObjString(obj,PROP_LOCKEDMESSAGE_NAME);
         DPCSendAddText(player, str, DEFAULT_MSG_TIME);
         // DPCOverlayAddText(str,DEFAULT_MSG_TIME);
      }
      else
      {
         if (verbose)
            SpewLockData(CheckObj, FALSE);
      }
      return(TRUE);
   }

   STDMETHOD(AddText)(const char *msg, const object ref playerRef, int time)
   {
      ObjID player = ScriptObjID(playerRef);
      DPCSendAddText(player, msg, time);
      if (player == OBJ_NULL) {
         // All players, including this one
         DPCOverlayAddText(msg,time);
      }
      return(S_OK);
   }

   STDMETHOD(AddTranslatableText)(const char *msg, const char *table, const object ref playerRef, int time = DEFAULT_MSG_TIME)
   {
      char temp[255];
      DPCStringFetch(temp,sizeof(temp),msg,table);
      ObjID player = ScriptObjID(playerRef);
      DPCSendAddText(player, temp, time);
      if (player == OBJ_NULL) {
         // All players, including this one
         DPCOverlayAddText(temp,time);
      }
      return(S_OK);
   }

   STDMETHOD(AddTranslatableTextInt)(const char *msg, const char *table, const object ref playerRef, int val, int time = DEFAULT_MSG_TIME)
   {
      return(AddTranslatableTextIndexInt(msg,table,playerRef,-1,val,time));
   }

   STDMETHOD(AddTranslatableTextIndexInt)(const char *msg, const char *table, const object ref playerRef, 
      int index, int val, int time = DEFAULT_MSG_TIME)
   {
      char temp[255], final[255];
      DPCStringFetch(temp,sizeof(temp),msg,table,index);
      ObjID player = ScriptObjID(playerRef);
      sprintf(final,temp,val);
      DPCSendAddText(player, final, time);
      if (player == OBJ_NULL) {
         // All players, including this one
         DPCOverlayAddText(final,time);
      }
      return(S_OK);
   }

   STDMETHOD(AddTextObjProp)(const object ref Obj, const char *propname, const object ref playerRef, int time = DEFAULT_MSG_TIME)
   {
      cStr str;
      char temp[255];
      ObjID obj = ScriptObjID(Obj);
      AutoAppIPtr(GameStrings);

      str = pGameStrings->FetchObjString(obj,propname);
      strncpy(temp,str,sizeof(temp));
      ObjID player = ScriptObjID(playerRef);
      DPCSendAddText(player, temp, time);
      if (player == OBJ_NULL) {
         // All players, including this one
         DPCOverlayAddText(temp,time);
      }
      return(S_OK);
   }

   STDMETHOD(AmmoLoad)(const object ref GunObj, const object ref AmmoObj)
   {
      ObjID gun = ScriptObjID(GunObj);
      ObjID ammo = ScriptObjID(AmmoObj);
    
      if (!GunCanLoad(gun,ammo))
         return(E_FAIL);

      if (ObjGetObjState(gun) != kObjStateNormal)
      {
         ESndPlay(&cTagSet("Event NoLoad"), gun, OBJ_NULL); 
         // play sound effect
         return(E_FAIL);
      }

      // should also fail if gun is full?
      GunLoad(gun,ammo,LF_NONE);
      return(S_OK);
   }

   STDMETHOD_(int,GetClip)(const object ref GunObj) 
   {
      ObjID gun = ScriptObjID(GunObj);
      return(BaseGunDescGetClip(gun));
   }

   STDMETHOD(SetModify)(const object ref Obj, int modlevel) 
   {
      ObjID gun = ScriptObjID(Obj);
      GunSetModification(gun,modlevel);
      return(S_OK);
   }

#if 0
   STDMETHOD(AddExp)(const object ref Who, int amt, BOOL verbose)
   {
      ObjID player = ScriptObjID(Who);
      if (player == OBJ_NULL)
         player = PlayerObject();
      DPCAddExp(player, amt, verbose);
      return(S_OK);
   }
#endif // 0

   STDMETHOD_(BOOL,HasTrait)(const object ref Who, int trait)
   {
      BOOL retval;
      ObjID player = ScriptObjID(Who);
      AutoAppIPtr(DPCPlayer);
      retval = pDPCPlayer->HasTrait(player,(eTrait)trait);
      return(retval);
   }
   STDMETHOD(HealObj)(const object ref Who, int amt)
   {
      ObjID o = ScriptObjID(Who);
      int hitPoints;
      int maxhp;
      int newhp;

      if (!ObjGetHitPoints(o,&hitPoints))
         return(S_FALSE);
      if (!ObjGetMaxHitPoints(o,&maxhp))
         return(S_FALSE);

      newhp = hitPoints + amt;
      if (newhp > maxhp)
         newhp = maxhp;
      ObjSetHitPoints(o,newhp);
      return(S_OK);
   }

   STDMETHOD_(string,GetArchetypeName)(const object ref Obj)
   {
      AutoAppIPtr(ObjectSystem);
      AutoAppIPtr(TraitManager);
      ObjID arch; 
      ObjID o = ScriptObjID(Obj);
      arch = pTraitManager->GetArchetype(o);
      return(pObjectSystem->GetName(arch));
   }

   STDMETHOD_(int,GetStat)(const object ref Who, int which)
   {
      ObjID player = ScriptObjID(Who);
      AutoAppIPtr(DPCPlayer);
      // GetStat doesn't take obj ID yet
      return pDPCPlayer->GetStat((eStats)which);
   }

   STDMETHOD_(object,GetSelectedObj)(void)
   {
      return frobWorldSelectObj;
   }

   STDMETHOD_(object,GetDistantSelectedObj)(void)
   {
      return g_distPickObj;
   }

   BOOL ValidSpawn(ObjID candidate, uint flags)
   {
      BOOL addlist = TRUE;
      IRelation *pRel2;
      AutoAppIPtr(LinkManager);
      pRel2 = pLinkManager->GetRelationNamed("Spawned");

      if ((flags & kSpawnFlagPopLimit) && (pRel2->AnyLinks(candidate, LINKOBJ_WILDCARD)))
      {
         addlist = FALSE;
      }
      if (flags & kSpawnFlagPlayerDist) 
      {
         ObjPos *plrpos, *pos;
         plrpos = ObjPosGet(PlayerObject());
         pos = ObjPosGet(candidate);

         float dist; // not a true distance, but a squared distance comparison
         float dx, dy;
         dx = (pos->loc.vec.x - plrpos->loc.vec.x);
         dy = (pos->loc.vec.y - plrpos->loc.vec.y);
         dx = dx * dx;
         dy = dy * dy;
         dist = dx + dy;
            
         //mx_dist_vec(&pos->loc.vec,&plrpos->loc.vec);
         
         //mprintf("dist = %g (%g)\n",dist,MAX_PLAYER_SPAWN_DIST);
         if (dist < MAX_PLAYER_SPAWN_DIST)
         {
            addlist = FALSE;
         }
      }
      return(addlist);
   }

   STDMETHOD_(object,FindSpawnPoint)(const object ref Obj, uint flags)
   {
      ObjID o;
      ObjID candidate;
      ObjID objlist[MAX_SPAWN_POINTS];
      IRelation *pRel;
      ILinkQuery *query;
      int count;
      int randval;
      sLink link;

      AutoAppIPtr(LinkManager);

      o = ScriptObjID(Obj);

      // look through all of our spawnlinks
      pRel = pLinkManager->GetRelationNamed("SpawnPoint");
      query = pRel->Query(o, LINKOBJ_WILDCARD);
      count = 0;
      if (flags & kSpawnFlagSelfMarker)
      {
         if (ValidSpawn(o,flags))
         {
            objlist[0] = o;
            count = 1;
         }
      }
      while (!query->Done() && (count < MAX_SPAWN_POINTS))
      {
         query->Link(&link);
         candidate = link.dest;
         // only consider ones that are "free"
         if (ValidSpawn(candidate,flags))
         {
            objlist[count] = candidate;
            count++;
         }
         query->Next();
      }
      SafeRelease(query);

      // bail out directly if none available
      if (count == 0)
         return(OBJ_NULL);

      // should probably modularize this out
      if (flags & kSpawnFlagFarthest)
      {
         // eschew our normal random selection, and just pick the furthest one
         int i;
         float dist; // not a true distance, but a squared distance comparison
         float dx, dy;
         float max_dist;
         ObjPos *plrpos, *pos;

         plrpos = ObjPosGet(PlayerObject());
         int farthest;
         max_dist = 0;
         for (i=0; i < count; i++)
         {
            pos = ObjPosGet(objlist[i]);

            dx = (pos->loc.vec.x - plrpos->loc.vec.x);
            dy = (pos->loc.vec.y - plrpos->loc.vec.y);
            dx = dx * dx;
            dy = dy * dy;
            dist = dx + dy;

            if (dist > max_dist)
            {
               farthest = i;
               max_dist = dist;
            }
         }
         return(objlist[farthest]);
      }

      // choose randomly among those that remain.
      randval = Rand() % count;

      if (flags & kSpawnFlagRaycast)
      {
         Location hit;
         ObjID hit_obj;
         ObjPos *p1, *p2;
         ObjID candidate;
         int result;

         candidate = objlist[randval];
         p1 = ObjPosGet(candidate);
         p2 = ObjPosGet(PlayerObject());
         // attempt to do a raycast from the player to the spawn location
         // if there is a hit, then for the love of god, montressor, pick another one
         result = PhysRaycast(p1->loc, p2->loc, &hit, &hit_obj, 0.0, kCollideTerrain);
         if (result != kCollideNone)
            return(objlist[randval]);
         else
            return(OBJ_NULL);
      }
      return(objlist[randval]);
   }

   STDMETHOD_(int, CountEcoMatching)(int val)
   {
      sPropertyObjIter iter;
      ObjID obj;
      ObjID physarch;
      int propval;
      int retval;

      retval = 0;
      AutoAppIPtr(ObjectSystem);
      AutoAppIPtr(TraitManager);

      // iterate over all things with the ecotype property
      physarch = pObjectSystem->GetObjectNamed("Physical");
      gPropEcoType->IterStart(&iter);
      while (gPropEcoType->IterNextValue(&iter, &obj, &propval))
      {
         // make sure it is concrete, matches our target ecotype, and derives from physical (to exclude traps)
         if (OBJ_IS_CONCRETE(obj) && (propval == val) && (pTraitManager->ObjHasDonor(obj,physarch)))
         {
            retval++;
         }
      }
      gPropEcoType->IterStop(&iter);

      return(retval);
   }

   STDMETHOD(RecalcStats)(const object ref who) 
   {
      AutoAppIPtr(DPCPlayer);
      pDPCPlayer->RecalcData(ScriptObjID(who));
      return(S_OK);
   }

   STDMETHOD(PlayVideo)(const char *vidname)
   {
      MoviePanel(vidname);
      return(S_OK);
   }

   STDMETHOD(HRM)(integer hacktype, const object ref Obj, BOOL frompsi)
   {
      ObjID obj = ScriptObjID(Obj);
      int techtype = DPCFindTechType(obj);

      if (techtype != hacktype)
         return(S_FALSE);
      return(S_OK);
   }

   STDMETHOD(TechTool)(const object ref Obj)
   {
      HRESULT retval;
      ObjID o;
      int techtype;

      o = ScriptObjID(Obj);
      techtype = DPCFindTechType(o);

      if (techtype == -1)
         return(S_FALSE);

      retval = HRM(techtype, Obj, FALSE);
      return(retval);
   }

   STDMETHOD_(BOOL, LoadCursor)(const object ref obj)
   {
      DPCInvLoadCursor(OBJ_NULL);
      return DPCInvLoadCursor((ObjID)obj);
      return(S_OK);
   }

   STDMETHOD_(void, SetPlayerVolume)(float volume)
   {
      g_pPlayerMovement->SetVolume(volume);
   }

   STDMETHOD_(int, RandRange)(int low, int high)
   {
      return ::RandRange(low, high);
   }

   // set the speed factor for the player 
   STDMETHOD_(void,AddSpeedControl)(const char *name, float speed_fac, float rot_fac)
   {
      AddSpeedScale(name, speed_fac, rot_fac);
   }

   STDMETHOD_(void,RemoveSpeedControl)(const char *name)
   {
      RemoveSpeedScale(name);
   }

   STDMETHOD(PreventSwap)(void)
   {
      // okay, this is kind of dumb, I admit
      gScriptAllowSwap = FALSE;
      return(S_OK);
   }

   STDMETHOD(SetObjState)(const object ref Obj, int state) 
   {
      ObjID obj = ScriptObjID(Obj);
      ObjSetObjState(obj, (eObjState)state);
      return(S_OK);
   }

   STDMETHOD(DisableAlarmGlobal)(void) 
   {
      DPCAlarmDisableAll();

      return(S_OK);
   }

   STDMETHOD(AddAlarm)(int time) 
   {
      int t = GetSimTime() + time;
      DPCAlarmAdd(t);
      return(S_OK);
   }

   STDMETHOD(RemoveAlarm)(void) 
   {
      DPCAlarmRemove();
      return(S_OK);
   }

   STDMETHOD_(void, Frob)(BOOL in_inv)
   {
      DPCDoFrob(in_inv);
   }

   STDMETHOD(SetExplored)(int maploc, char val = 1) 
   {
      DPCMapSetExplored(maploc,val);
      return(S_OK);
   }

   // This tells the other players, and our own system, that this object
   // has been taken from its container.
   STDMETHOD(RemoveFromContainer)(const object ref Obj, 
                                  const object ref Container)
   {
      ObjID obj = ScriptObjID(Obj);
      ObjID container = ScriptObjID(Container);
      DPCBroadcastRemoveContainee(obj, container);
      return(S_OK);
   }

   STDMETHOD(ActivateMap)(void) 
   {
      char curfile[255], temp[255];
      dbCurrentFile(curfile, sizeof(curfile));
      strcpy(temp, curfile); 
      temp[strlen(curfile) - 4] = '\0'; // to strip out extension

      char qbname[255];
      sprintf(qbname,"Map%s",temp);
      AutoAppIPtr(QuestData);
      if (!pQuestData->Exists(qbname))
         pQuestData->Create(qbname,1,kQuestDataCampaign);
      else
         pQuestData->Set(qbname,1);
      return(S_OK);
   }

   STDMETHOD_(int, SimTime)(void)
   {
      return(GetSimTime());
   }

   STDMETHOD_(void, StartFadeIn)(int time, uchar red, uchar green, uchar blue)
   {
      PlayerModeStartFadeIn(time, red, green, blue);
   }
   STDMETHOD_(void, StartFadeOut)(int time, uchar red, uchar green, uchar blue)
   {
      PlayerModeStartFadeOut(time, red, green, blue);
   }

   STDMETHOD(PlayerModeSimple)(int mode) 
   {
      SetPlayerMode((ePlayerMode)mode);
      return(S_OK);
   }

   STDMETHOD(PlayerMode)(int mode) 
   {
      SetPlayerMode((ePlayerMode)mode);
      if (mode == kPM_Stand)
      {
         cPhysModel *pModel;
         mxs_vector offset;
         ObjID obj = PlayerObject();

         pModel = g_PhysModels.GetActive(obj);

         mx_mk_vec(&offset, 0, 0, PLAYER_HEAD_POS);
         pModel->SetSubModOffset(PLAYER_HEAD, offset);
         mx_mk_vec(&offset, 0, 0, PLAYER_BODY_POS);
         pModel->SetSubModOffset(PLAYER_BODY, offset);
         mx_mk_vec(&offset, 0, 0, PLAYER_KNEE_POS);
         pModel->SetSubModOffset(PLAYER_KNEE, offset);
         mx_mk_vec(&offset, 0, 0, PLAYER_SHIN_POS);
         pModel->SetSubModOffset(PLAYER_SHIN, offset);
         mx_mk_vec(&offset, 0, 0, PLAYER_FOOT_POS);
         pModel->SetSubModOffset(PLAYER_FOOT, offset);

         // untwist the neck
         mxs_angvec headvec = pModel->GetRotation(PLAYER_HEAD);
         headvec.tx = headvec.ty = 0;
         pModel->SetRotation(PLAYER_HEAD, headvec);

         // kill any low-level remaining rotation
         mxs_vector zerovec;
         mx_zero_vec(&zerovec);
         PhysSetSubModRotationalVelocity(obj, PLAYER_HEAD, &zerovec);

         // Then call PhysSetModLocation() with the "stood up" location.
         ObjPos *pos = ObjPosGet(obj);
         //PhysSetModRotation(obj, &pos->fac);
         PhysSetModLocation(obj, &pos->loc.vec);

         // make sure we aren't still walking or anything
         PhysStopControlLocation(obj);
         PhysStopControlRotation(obj);
         PhysStopControlVelocity(obj);
         PhysStopControlRotationalVelocity(obj);

         ObjID weaponObj;
         AutoAppIPtr(DPCPlayer);
         weaponObj = pDPCPlayer->GetEquip(PlayerObject(),kEquipWeapon);
         pDPCPlayer->SetWeaponModel(weaponObj, FALSE);

         // AIs ignore sounds for the first second
         AutoAppIPtr(AIManager);
         pAIManager->SetIgnoreSoundTime(1000);

         // we have set the player to dead, tell ghost?
         GhostNotify(PlayerObject(),kGhostStRevive);
      }
      else if (mode == kPM_Dead)
      {
         AutoAppIPtr(DPCPlayer);
         AutoAppIPtr(PlayerGun);

         // make sure we squeeze off any final shot as we die
         if (pPlayerGun->IsTriggerPulled())
            pPlayerGun->ReleaseTrigger();

         pDPCPlayer->SetWeaponModel(OBJ_NULL,FALSE);

         // we have set the player to dead, tell ghost?
         GhostNotify(PlayerObject(),kGhostStDying);
      }
      
      return(S_OK);
   }

   STDMETHOD(EndGame)(void) 
   {
      //UnwindToMissionLoop(); 
      // take anything off of the cursor that might be there
      if (DPC_mouse)
      {
         ClearCursor();
         MouseMode(FALSE,TRUE);
      }
      SwitchToDPCMainMenuMode(FALSE);
      return(S_OK);
   }

   STDMETHOD_(BOOL, AllowDeath)(void)
   {
      if (config_is_defined("undead"))
         return(FALSE);
      
      return(TRUE);
   }

   STDMETHOD_(float,GetHazardResistance)(int endur) 
   {
      sStatParams *params;
      params = GetStatParams();
      if (endur > MAX_STAT_VAL)
         endur = MAX_STAT_VAL;
      if (endur < 1)
         endur = 1;
      return(params->m_hazard[endur-1]);
   }

   STDMETHOD_(object,PlayerGun)(void)
   {
      AutoAppIPtr(PlayerGun);
      ObjID retval;
      retval = pPlayerGun->Get();
      return(retval);
   }

   STDMETHOD_(object, PseudoProjectile)(const object ref Source, const object ref Emittype) 
   {
      ObjID source = ScriptObjID(Source);
      ObjID emittype = ScriptObjID(Emittype);

      // find our target location
      ObjPos *pPos = ObjPosGet(source);
      ObjPos newpos;
      mxs_angvec facing = pPos->fac;
      mxs_matrix mat;
      mxs_vector unit;
      mxs_vector newloc;
      mx_ang2mat(&mat, &facing);
      mx_copy_vec(&unit,&mat.vec[0]);
      // pushout by scaling
      mx_scaleeq_vec(&unit,3.0);
      mx_add_vec(&newloc,&pPos->loc.vec,&unit);

      // update the location by copying and
      // then modifying to use previous hinting
      memcpy(&newpos,pPos,sizeof(ObjPos));
      mx_copy_vec(&newpos.loc.vec,&newloc);
      UpdateChangedPosition(&newpos);

      // determine whether it is in the world
      if (newpos.loc.hint == CELL_INVALID)
         return(OBJ_NULL);

      // if so, create it
      AutoAppIPtr(ObjectSystem);
      ObjID newobj;
      newobj = pObjectSystem->BeginCreate(emittype,TRUE);
      ObjPosCopyUpdate(newobj, &newpos);
      pObjectSystem->EndCreate(newobj);
      return(newobj);
   }

   STDMETHOD_(BOOL, Censored)(void) 
   {
      if (config_is_defined("menschveracht"))
         return(FALSE);
      else
         return(TRUE);
   }

   STDMETHOD(TlucTextAdd)(char *name, char *table, int offset) 
   {
      DPCTlucTextAdd(name,table,offset);
      return(S_OK);
   }

   STDMETHOD(Mouse)(BOOL mode, BOOL clear)
   {
      // clear out any HUD selection boxes
      sPropertyObjIter iter;
      ObjID obj;
      gPropHUDTime->IterStart(&iter);
      while (gPropHUDTime->IterNext(&iter,&obj))
      {
         gPropHUDTime->Delete(obj);
      }
      gPropHUDTime->IterStop(&iter);
      DPCHUDClear();
      if (clear)
      {
         if (drag_obj != OBJ_NULL)
            ThrowObj(drag_obj, PlayerObject());
         ClearCursor();
      }
      MouseMode(mode,clear);
      return(S_OK);
   }

   STDMETHOD(RefreshInv)(void)
   {
      DPCInvRefresh();
      return(S_OK);
   }

   STDMETHOD(TreasureTable)(const object ref Obj) 
   {
      ObjID corpse = ScriptObjID(Obj);
      GenerateLoot(corpse);
      return(S_OK);
   }

   // use only with great caution!
   STDMETHOD(VaporizeInv)(void)
   {
      sContainIter *iterp;
      ObjID player = PlayerObject();
      ObjID hackobj; // this is a special easteregg obj we spare from destruction

      AutoAppIPtr(ContainSys);
      AutoAppIPtr(ObjectSystem);

      hackobj = pObjectSystem->GetObjectNamed("Magic Ball");

      iterp = pContainSys->IterStart(player);
      while (!iterp->finished)
      {
         ObjID invobj = iterp->containee;
         if ((invobj != hackobj) && (iterp->type < (DPCCONTAIN_PDOLLBASE + kEquipFakeNanites)))
            pObjectSystem->Destroy(invobj);
         pContainSys->IterNext(iterp);
      }
      pContainSys->IterEnd(iterp);

      return(S_OK);
   }

   STDMETHOD(SetQBHacked)(const string ref name, int qbval) 
   {
      AutoAppIPtr(QuestData);
      char qbname[255];
      strcpy(qbname,(const char *)name);

      if (strnicmp(qbname,"note",4) == 0)
      {
         // we are a "notes" QB, so do some zany shit
         int currval;
         currval = pQuestData->Get(qbname);
         switch (qbval)
         {
         case 2:
            // if we are trying to mark a not-yet gained quest as done
            // then mark it as "secretly done" instead
            if (currval == 0)
               qbval = 3;
            break;
         case 1:
            // if our current state is "secretly done" and we are being
            // set to "not yet done", then actually set to "done"
            if (currval == 3)
               qbval = 2;

            // if we are already done with this quest, don't mark it as "not yet done", do nothing instead
            if (currval == 2)
               return(S_FALSE);

            break;
         }
      }
      // finally, do the set
      if (!pQuestData->Exists(qbname))
      {
         pQuestData->Create(qbname,qbval,kQuestDataCampaign);
      }
      else
         pQuestData->Set(qbname,qbval);
      return(S_OK);
   }

   STDMETHOD(ZeroControls)(const object ref Obj, BOOL poll)
   {
      ObjID obj = ScriptObjID(Obj);
      // make sure said object even has physics first
      if (PhysObjHasPhysics(obj))
      {
         PhysStopControlLocation(obj);
         PhysStopControlRotation(obj);
         PhysStopControlVelocity(obj);
         PhysStopControlRotationalVelocity(obj);
      }

      if (poll)
         g_pInputBinder->PollAllKeys();

      return(S_OK);
   }

   STDMETHOD_(BOOL, ValidGun)(const object ref Obj) 
   {
      ObjID o = ScriptObjID(Obj);

      // if you don't have a gun state you clearly aren't one
      if (!ObjHasGunState(o))
         return(FALSE);

      // melee weapons are weapons that aren't appropriate
      int wpntype;
      g_pWeaponTypeProperty->Get(o,&wpntype);

      if (!IsMelee(o))
      {
          return(TRUE);
      }
      else
      {
          return(FALSE);
      }
   }

   STDMETHOD_(BOOL, IsAlarmActive)(void)
   {
      // iterate through all objects, and anything with a population system on it, 
      // and is alarmed, means there is an active alarm.
      ObjID obj;
      sPropertyObjIter iter;
      int ecostate;
      AutoAppIPtr(ScriptMan); 

      gPropEcology->IterStart(&iter);
      while (gPropEcology->IterNext(&iter, &obj))
      {
         // are we an ecology
         if (gPropEcoState->IsRelevant(obj))
         {
            gPropEcoState->Get(obj, &ecostate);
            if (ecostate == kEcologyAlert)
            {
               gPropEcology->IterStop(&iter);
               return(TRUE);
            }
         }
      }
      gPropEcology->IterStop(&iter);

      return(FALSE);
   }

   STDMETHOD(NoMove)(BOOL move_disallowed) 
   {
      gNoMoveKeys = gNoLookAround =  move_disallowed;
      if (move_disallowed) {
         SetTurningState(0.0);
         UpdatePlayerSpeed();
      }
      return(S_OK);
   }

   STDMETHOD(UpdateMovingTerrainVelocity)(const object objID, const object next_node, float speed)
   {
      ::UpdateMovingTerrainVelocity(ObjID(objID), ObjID(next_node), speed);
      return S_OK;
   }

   STDMETHOD_(BOOL, MouseCursor)()
   {
      return DPC_mouse;
   }

   STDMETHOD_(BOOL, ConfigIsDefined)(const char *name)
   {
      return config_is_defined(name);
   }

   STDMETHOD_(BOOL, ConfigGetInt)(const char *name, int ref value)
   {
      return config_get_int(name, &value);
   }
};

IMPLEMENT_SCRIPT_SERVICE_IMPL(cDPCGameSrv, DPCGame);

struct sLinkTableEntry 
{
   sRelationDesc desc;
   sRelationDataDesc ddesc;
   RelationQueryCaseSet cases; 
}; 

static sLinkTableEntry DPC_links[] = 
{
   {   
      { "SwitchLink" },  
      LINK_NO_DATA,
      kQCaseSetSourceKnown|kQCaseSetDestKnown, 
   },

   { 
      { "Questbit" },
      LINK_DATA_DESC(Label), 
      kQCaseSetSourceKnown|kQCaseSetDestKnown, 
   },

   {   
      { "HackingLink" },  
      LINK_NO_DATA,
      kQCaseSetSourceKnown|kQCaseSetDestKnown, 
   },

   {   
      { "Research" },  
      LINK_NO_DATA,
      kQCaseSetSourceKnown|kQCaseSetDestKnown, 
   },

   {   
      { "SpawnPoint" },  
      LINK_NO_DATA,
      kQCaseSetSourceKnown|kQCaseSetDestKnown, 
   },

   {   
      { "Spawned" },  
      LINK_NO_DATA,
      kQCaseSetSourceKnown|kQCaseSetDestKnown, 
   },

   {   
      { "ApparStart" },  
      LINK_NO_DATA,
      kQCaseSetSourceKnown|kQCaseSetDestKnown, 
   },

   {   
      { "Tripwire" },  
      LINK_NO_DATA,
      kQCaseSetSourceKnown|kQCaseSetDestKnown, 
   },

   {   
      { "Organ" },  
      LINK_NO_DATA,
      kQCaseSetSourceKnown|kQCaseSetDestKnown, 
   },

   {   
      { "TelePath" },  
      LINK_NO_DATA,
      kQCaseSetSourceKnown|kQCaseSetDestKnown, 
   },

};

#define NUM_LINKS (sizeof(DPC_links)/sizeof(DPC_links[0]))

static void InitLinks(void)
{
   for (int i = 0; i < NUM_LINKS; i++)
   {
      sLinkTableEntry& entry = DPC_links[i]; 

      IRelation* relation = CreateStandardRelation(&entry.desc,&entry.ddesc,entry.cases);
      SafeRelease(relation); 
   }
}


////////////////////////////////////////
//
// YorN scripting message implementation
//

IMPLEMENT_SCRMSG_PERSISTENT(sYorNMsg)
{
   PersistenceHeader(sScrMsg, kYorNMsgVer);
   return TRUE;
}

////////////////////////////////////////
//
// Keypad scripting message implementation
//

IMPLEMENT_SCRMSG_PERSISTENT(sKeypadMsg)
{
   PersistenceHeader(sScrMsg, kKeypadMsgVer);
   return TRUE;
}

////////////////////////////////////////

// returns whether or not any script has disallowed a swap,
// and resets the flag
BOOL DPCScriptAllowSwap()
{
   BOOL retval = gScriptAllowSwap;
   gScriptAllowSwap = TRUE;

   return(retval);
}
