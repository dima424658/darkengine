/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <string.h>

#include <mprintf.h>
#include <config.h>
#include <cfgdbg.h>

#include <label.h>
#include <objtype.h>
#include <property.h>
//#include <frobbase.h>
#include <frobctrl.h>
#include <frobprop.h>

//#include <frobsys.h>
#include <scrptapi.h>
#include <playrobj.h>
#include <gamestr.h>

#include <appagg.h>

#include <resapi.h>

#include <netman.h>
#include <iobjnet.h>
#include <netmsg.h>
#include <phnet.h>

#include <linkman.h>
#include <relation.h>

#include <schema.h>
#include <psnd.h>
#include <appsfx.h>
#include <command.h>
#include <physapi.h>
#include <rendprop.h>

#include <combprop.h>

#include <contain.h>
#include <prcniter.h>

#include <dpcgame.h>
//#include <dpcscrpt.h>
#include <dpcpfiid.h>
#include <dpcovrly.h>
#include <dpcplayr.h>
#include <dpcinv.h>
#include <dpccurm.h>
#include <dpcutils.h>
#include <dpcnet.h>
#include <dpcfrob.h>
#include <dpcovcst.h>
#include <dpciface.h>
#include <dpccont.h>
#include <dpcprop.h>
#include <dpcobjst.h>

#include <physcast.h>

BOOL DPCInvThrowBackToWorld(ObjID o, ObjID src);
BOOL DPCInvTakeObjFromWorld(ObjID o, ObjID src);
BOOL DPCInvRemObj(ObjID o);
BOOL DPCInvToolCursor(ObjID o);

// The number of requests for frob-pickup that are outstanding to the
// host.
static int gFrobsPending;

// Returns TRUE iff we are waiting for the host to respond to some
// frobs:
BOOL DPCFrobPending()
{
   return (gFrobsPending > 0);
}

// This will get called by the frob system when it finishes with a frob
// that we requested a receipt for (which is things we are picking up):
void DPCFrobAcknowledge(ObjID o)
{
   gFrobsPending--;
}

//////////
//
// Networking Code
//

static cNetMsg *g_pFrobPickupMsg = NULL;

static void handleFrobPickup(ObjID o, ObjID oldCont)
{
   // This test shouldn't be necessary, but OBJ_NULL isn't handled right
   // in LoadCursor...
   if (o == OBJ_NULL) {
      ClearCursor();
      DPC_cursor_mode = SCM_NORMAL;
   } else {
      AutoAppIPtr(NetManager);
      if (pNetManager->Networking()) {
         AutoAppIPtr(ObjectNetworking);
         pObjectNetworking->ObjTakeOver(o);
      }

      // If the thing used to be in a container, tell everyone to take
      // it out. This must happen before we put it in our inventory:
      DPCBroadcastRemoveContainee(o, oldCont);

      // At this point, we have two choices. If the object is auto-pickup,
      // then we just put it into the player's inventory. Otherwise, pick
      // it up and put it on their cursor.
      BOOL isAutoPickup;
      if (gPropAutoPickup->Get(o, &isAutoPickup) && isAutoPickup) {
         // It's an auto-pickup object
         // Add feedback *before* picking it up, because picking the
         // object up can potentially destroy it, if it's a combinable
         // object...
         char temp[255],temp2[64], text[255];
         cStr str;
         eObjState st = ObjGetObjState(o);
         AutoAppIPtr(GameStrings);
         DPCStringFetch(temp, sizeof(temp), "PickupString", "misc");
         str = pGameStrings->FetchObjString(o,PROP_OBJSHORTNAME_NAME);

         if (st != kObjStateUnresearched)
         {
            int count;
            if (gStackCountProp->Get(o,&count))
               sprintf(temp2,str,count);
            else
               strcpy(temp2,str);
         }
         else
            DPCStringFetch(temp2,sizeof(temp2),"NameUnresearched","research");
         sprintf(text,temp,temp2);
         DPCOverlayAddText(text,DEFAULT_MSG_TIME);
         // Now pick it up...
         DPCInvAddObj(PlayerObject(), o);
      } else {
         // It's an ordinary object
         DPC_cursor_mode = SCM_NORMAL;
         DPCInvLoadCursor(o);
      }

      // snap any "tripwire" links to the object
      IRelation *pRel;
      LinkID lid;
      AutoAppIPtr(LinkManager);
      pRel = pLinkManager->GetRelationNamed("Tripwire");
      if (pRel->AnyLinks(LINKOBJ_WILDCARD,o))
      {
         lid = pRel->GetSingleLink(LINKOBJ_WILDCARD, o);
         pRel->Remove(lid);
      }

      // Current theory: the pickup sound is local to the client doing
      // the pickup, so we're not networking it...
      //sfx_parm parm={0,0,0,0,100,0,0,0,NULL,NULL};
      //g_pPropSnd->GenerateSound("pickup", &parm);     
      SchemaPlay((Label *)"pickup_item",NULL);
   }
}

static sNetMsgDesc sFrobPickupDesc =
{
   kNMF_None,
   "FrobPickup",
   "Frob Pickup Obj",
   NULL,
   handleFrobPickup,
   // Can't have the assert here, because the object could be
   // OBJ_NULL:
   {{kNMPT_SenderObjID, kNMPF_AllowObjNull, "Obj"},
    {kNMPT_GlobalObjID, kNMPF_AllowObjNull, "Container"},
    {kNMPT_End}}
};

//
// Tell the client to put the given object onto its cursor
// Host code
//
void DPCSendFrobPickup(ObjID o, ObjID player, ObjID oldCont)
{
   AutoAppIPtr(NetManager);
   if (pNetManager->Networking()) {
      // Make sure we're not sending heartbeats for this thing any more
      PhysNetSetSleep(o, TRUE);
   }
   // If player is the host, then this will just loopback:
   g_pFrobPickupMsg->Send(player, o, oldCont);
}

//////////
//
// Give item to another player
//

static cNetMsg *g_pGiveObjToPlayerMsg = NULL;

// Receiver's side -- deal with this object
// Assumes that we have already received ownership of the object, due to
// the low-level Give that should have just happened.
static void handleGiveObjToPlayer(ObjID o, ObjID fromPlayer)
{
   AutoAppIPtr(ObjectNetworking);
   AssertMsg2(pObjectNetworking->ObjHostedHere(o),
              "Received possession, but not ownership, of %d from %d!",
              o, fromPlayer);

   // Add feedback *before* picking it up, because picking the
   // object up can potentially destroy it, if it's a combinable
   // object...
   char temp[255], text[255];
   AutoAppIPtr(NetManager);
   const char *fromName = pNetManager->GetPlayerName(fromPlayer);
   const char *myName = pNetManager->GetPlayerName(PlayerObject());
   char objName[128];
   ObjGetObjShortNameSubst(o, objName, sizeof(objName));
   
   if (DPCStringFetch(temp, sizeof(temp), "ReceiveString", "misc"))
   {
      sprintf(text, temp, objName, fromName);
      DPCOverlayAddText(text,DEFAULT_MSG_TIME);
   }

   // Now pick it up...
   if (DPCInvAddObj(PlayerObject(), o))
   {
      // Tell the other player that we succeeded
      if (DPCStringFetch(temp, sizeof(temp), "GiveawayString", "misc"))
      {
         sprintf(text, temp, objName, myName);
         DPCSendAddText(fromPlayer, text);
      }
   } else {
      // Tell the other player that we failed to take it
      if (DPCStringFetch(temp, sizeof(temp), "GivefailString", "misc"))
      {
         sprintf(text, temp, objName, myName);
         DPCSendAddText(fromPlayer, text);
      }
   }
}

static sNetMsgDesc sGiveObjToPlayerDesc =
{
   kNMF_AppendSenderID,
   "DPCGiveObj",
   "Give Object To Player",
   NULL,
   handleGiveObjToPlayer,
   {{kNMPT_ReceiverObjID, kNMPF_None, "Object"},
    {kNMPT_End}}
};

// Giver's side -- hand the object off
static void GiveAwayObject(ObjID targetPlayer, ObjID obj)
{
   AssertMsg1(targetPlayer != PlayerObject(),
              "Trying to give object %d to myself!",
              obj);

   if (obj == OBJ_NULL)
   {
      Warning(("Trying to give away OBJ_NULL!\n"));
      return;
   }

   // Hand off ownership, and then the object itself, to the target;
   // it's his problem now:
   AutoAppIPtr(ObjectNetworking);
   if (!pObjectNetworking->ObjHostedHere(obj))
   {
      Warning(("Trying to give away unowned object %d!\n", obj));
      return;
   }
   pObjectNetworking->ObjGiveWithoutObjID(obj, targetPlayer);
   g_pGiveObjToPlayerMsg->Send(targetPlayer, obj);

   // And now we're no longer holding it:
   ClearCursor();
}

//////////

#define MAX_RAYCAST_OBJ 512    
void DPCDoFrob(bool in_inv)
{
   if (!in_inv)
   {
      // make sure we can actually reach the object
      // maybe we need to special case containers?
      if ((frobWorldSelectObj != OBJ_NULL) && ObjHasRefs(frobWorldSelectObj))
      {
         Location hit;
         ObjID hit_obj;
         ObjPos *p1, *p2;
         int result;
         ObjID objlist[MAX_RAYCAST_OBJ];
         int n;
         ObjID obj;
         BOOL propval;

         p1 = ObjPosGet(PlayerObject());
         p2 = ObjPosGet(frobWorldSelectObj);

         if ((p1 != NULL) && (p2 != NULL))
         {
            cConcreteIter iter(gPropBlockFrob);
            n = 0;
            iter.Start();   
            while (iter.Next(&obj))
            {
               gPropBlockFrob->Get(obj, &propval);
               // make sure it is concrete, matches our target BlockFrob, and derives from physical (to exclude traps)
               if (propval && OBJ_IS_CONCRETE(obj))
               {
                  objlist[n] = obj;
                  n++;
                  if (n == MAX_RAYCAST_OBJ)
                  {
                     Warning(("DPCDoFrob: Raycast obj limit exceeded!\n"));
                     break;
                  }
               }
            }
            iter.Stop();

            PhysRaycastSetObjlist(objlist, n);
            result = PhysRaycast(p1->loc, p2->loc, &hit, &hit_obj, 0.0, kCollideAllPhys);
            PhysRaycastClearObjlist();
            if (result != kCollideNone)
            {
               if (hit_obj != frobWorldSelectObj)
                  return;  // maybe needs feedback?
            }
         }
      }

      if (FrobSoundIsRelevant(frobWorldSelectObj))
      {
         SchemaPlay(FrobSoundGet(frobWorldSelectObj),NULL);
      }
      if (gPropUseMessage->IsRelevant(frobWorldSelectObj))
      {
         AutoAppIPtr(GameStrings);
         cStr str = pGameStrings->FetchObjString(frobWorldSelectObj, PROP_USEMESSAGE_NAME);
         DPCOverlayAddText(str,DEFAULT_MSG_TIME);
      }
   }

   sFrobActivate sfa;
   sfa.frobber = PlayerObject();
   sfa.ms_down = 100; // fakes
   sfa.flags = 0;
   sfa.src_obj = OBJ_NULL;

   // support for inv-inv frobbing should go here
   if (DPC_cursor_mode == SCM_DRAGOBJ) // USEOBJ)
   {
      // object on cursor
      if (IsAPlayer(frobInvSelectObj))
      {
         // It's been dragged onto another player, so try giving it to them.
         // This sorta goes around frob.
         GiveAwayObject(frobInvSelectObj, drag_obj);
      } 
      else
      {
         sfa.dst_obj = frobInvSelectObj;
         sfa.dst_loc = kFrobLocWorld;
         sfa.src_obj = drag_obj; 
         sfa.src_loc = kFrobLocInv;
      }
   }
   else
   {
      // normal world frob
      sfa.dst_obj = OBJ_NULL;
      if (in_inv)
      {
         sfa.src_obj = frobInvSelectObj;
         sfa.src_loc = kFrobLocInv;
      }
      else
      {
         if (frobWorldSelectObj != OBJ_NULL)
            sfa.src_obj = frobWorldSelectObj;
         else
            sfa.src_obj = g_ifaceFocusObj;
         sfa.src_loc = kFrobLocWorld;
      }
   }

   // more sure our src obj has a frobinfo prop
   if (!pFrobInfoProp->IsRelevant(sfa.src_obj))
   {
      // we don't? bail out
      return;
   }

   if (FrobWouldPickup(&sfa)) {
      // FrobInstantExecute will send two acks:
      gFrobsPending += 2;
      // Ask frob to tell us when it's done:
      sfa.flags |= FROBFLG_RETURN_RECEIPT;
   }
   if (sfa.src_obj != OBJ_NULL) {
      FrobInstantExecute(&sfa);
   }
}

// for DELETE frob bit
BOOL DPCInvRemObj(ObjID o)
{
   return(TRUE);
   /*
   ObjID victim = o;
   int x,y;

   if (DPCInvFindObjSlot(victim, &x, &y))
      DPCInvForceObj(x,y,OBJ_NULL);

   IObjectSystem* pOS = AppGetObj(IObjectSystem);
   pOS->Destroy(victim);
   SafeRelease(pOS);

   return TRUE;
   */
}

// for MOVE from world, puts obj on cursor
// Host code
BOOL DPCInvTakeObjFromWorld(ObjID o, ObjID owner)
{
   AutoAppIPtr(ContainSys);
   ObjID container = pContainSys->GetContainer(o);
   if (IsAPlayer(container)) {
      // We don't count players as containers for frob purposes
      container = OBJ_NULL;
   }

   // Check for raceway conditions: has someone else already picked up this
   // object, or otherwise taken it over?
   if (!ObjHasRefs(o))
   {
      // Make sure it's not contained by something
      if (container == OBJ_NULL)
      {
         // The object isn't ours to give away...
         DPCSendFrobPickup(OBJ_NULL, owner, OBJ_NULL);
         return(FALSE);
      }
   }

   AutoAppIPtr(ObjectNetworking);
   AutoAppIPtr(NetManager);
   if (pNetManager->Networking() && !pObjectNetworking->ObjHostedHere(o))
   {
      DPCSendFrobPickup(OBJ_NULL, owner, OBJ_NULL);
      return(FALSE);
   }

   // If we've borrowed this object from the world, stop worrying about
   // that. Otherwise, deregistering it will return the object
   // automatically:
   PhysNetDiscardBorrow(o);
   ObjSetHasRefs(o,FALSE);
   PhysDeregisterModel(o);
   DPCSendFrobPickup(o, owner, container);

   // Finally, remove the thing from the container locally, so that we
   // will decline any further attempts to frob it:
   if (container != OBJ_NULL)
      pContainSys->Remove(container, o);

   return(TRUE);
}

// for MOVE from inv
// not a supported notion in Deep Cover
BOOL DPCInvThrowBackToWorld(ObjID o, ObjID src)
{
   return(TRUE);
}

// when the object should be used as a tool cursor
BOOL DPCInvToolCursor(ObjID o)
{
   return(TRUE);
}

void DPCFrobsysInit(void)
{
   frobInvRemObj           = DPCInvRemObj;
   frobInvTakeObjFromWorld = DPCInvTakeObjFromWorld;
   frobInvThrowBackToWorld = DPCInvThrowBackToWorld;
   frobInvToolCursor       = DPCInvToolCursor;
   frobAcknowledge         = DPCFrobAcknowledge;
   gFrobsPending           = 0;
   g_pFrobPickupMsg = new cNetMsg(&sFrobPickupDesc);
   g_pGiveObjToPlayerMsg = new cNetMsg(&sGiveObjToPlayerDesc);
}

void DPCFrobsysTerm(void)
{
   delete g_pFrobPickupMsg;
   delete g_pGiveObjToPlayerMsg;
}
