/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phmsg.cpp,v 1.28 2000/02/24 13:05:37 ccarollo Exp $
//

#include <lg.h>
#include <appagg.h>
#include <comtools.h>

#include <dmgmodel.h>
#include <textarch.h>

#include <iobjnet.h>
#include <playrobj.h>
#include <netman.h>

#include <phcore.h>
#include <phmod.h>
#include <phmods.h>
#include <phclsn.h>
#include <phmodutl.h>
#include <physapi.h>
#include <phnet.h>

#include <subscrip.h>

#include <scrptapi.h>
#include <phscrpt.h>
#undef ref
#undef vector

#include <phmsg.h>
#include <phlistn_.h>

// doorphys, doorprop & plyrmode are needed for listening-at-doors hack - patmc
#include <doorphys.h>
#include <doorprop.h>
#include <plyrmode.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////////////////////////
//
// Common code for checking whether messages should be propagated
//
// General philosophy: secondary effects based on physics should only
// happen on the object's host, because minute physical differences
// between the worldreps can lead to enormous consequential differences.
// So we don't usually kick off scripts from physics on clients. There
// are a few specific exceptions.
//

// If we're networking, then check whether this is a proxy, and don't
// send script messages if so.
#ifdef NEW_NETWORK_ENABLED
#define RETURN_IF_PROXY(obj) if (physMessageIsProxy(obj)) return;
#else
#define RETURN_IF_PROXY(obj)
#endif

static BOOL physMessageIsProxy(ObjID object)
{
   AutoAppIPtr(NetManager);
   if (pNetManager->Networking()) {
      AutoAppIPtr(ObjectNetworking);
      return pObjectNetworking->ObjIsProxy(object);
   } else {
      return FALSE;
   }
}

////////////////////////////////////////////////////////////////////////////////

void PhysMessageFellAsleep(ObjID object)
{
   RETURN_IF_PROXY(object);

   if (!pPhysSubscribeService->IsSubscribed(object, kFellAsleepMsg))
      return;

   AutoAppIPtr(ScriptMan);

   sPhysMsg phys_message(object, kFellAsleepMsg);
   pScriptMan->SendMessage(&phys_message);
}

////////////////////////////////////////

void PhysMessageWokeUp(ObjID object)
{
   RETURN_IF_PROXY(object);

   if (!pPhysSubscribeService->IsSubscribed(object, kWokeUpMsg))
      return;

   AutoAppIPtr(ScriptMan);

   sPhysMsg phys_message(object, kWokeUpMsg);
   pScriptMan->SendMessage(&phys_message);
}

////////////////////////////////////////////////////////////////////////////////

void PhysMessageMadePhysical(ObjID object)
{
   RETURN_IF_PROXY(object);

   AutoAppIPtr(ScriptMan);

   sPhysMsg phys_message(object, kMadePhysMsg);
   pScriptMan->SendMessage(&phys_message);
}

////////////////////////////////////////

void PhysMessageMadeNonPhysical(ObjID object)
{
   RETURN_IF_PROXY(object);

   AutoAppIPtr(ScriptMan);

   sPhysMsg phys_message(object, kMadeNonPhysMsg);
   pScriptMan->SendMessage(&phys_message);
}

////////////////////////////////////////////////////////////////////////////////

void PhysMessageCollision(cPhysClsn *pClsn, mxs_real magnitude, int *reply1, int *reply2)
{
   ePhysCollisionType type;
   ObjID      coll_obj;
   int        coll_submod;
   mxs_vector normal;
   cMultiParm mp_reply1 = kPM_StatusQuo;      
   cMultiParm mp_reply2 = kPM_StatusQuo;      
   BOOL sub1;
   BOOL sub2;
   BOOL someoneListening;

   // hey, lets INITIALIZE the return variables, and not return random memory, woo woo
   *reply1=*reply2=kPM_StatusQuo;

   // We can't do a simpleminded cutoff here, since we have two objects to
   // worry about. We track whether they are proxies, and only call the
   // listeners for objects that are not.
   BOOL proxy1 = FALSE;
   BOOL proxy2 = FALSE;
#ifdef NEW_NETWORK_ENABLED
   AutoAppIPtr(ObjectNetworking);
   proxy1 = pObjectNetworking->ObjIsProxy(pClsn->GetObjID());
#endif

   sub1 = pPhysSubscribeService->IsSubscribed(pClsn->GetObjID(), kCollisionMsg);

   if (pClsn->GetType() & kPC_Object)
   {
      sub2 = pPhysSubscribeService->IsSubscribed(pClsn->GetObjID2(), kCollisionMsg);
#ifdef NEW_NETWORK_ENABLED
      proxy2 = pObjectNetworking->ObjIsProxy(pClsn->GetObjID2());
#endif
      if (proxy1 && proxy2)
         return;  // They're both proxies, so this is none of our business
   }
   else 
   {  // A terrain collision, so if the object's a proxy, we're done here
      if (proxy1)
         return;
      sub2 = FALSE;
   }

   someoneListening = g_pPhysListeners->HasListener(pClsn->GetObjID(), kCollisionMsg);
   if (!someoneListening && (pClsn->GetType() & kPC_Object))
      someoneListening = g_pPhysListeners->HasListener(pClsn->GetObjID2(), kCollisionMsg);

   if (sub1 || sub2 || someoneListening)
   {
      if (pClsn->GetType() & kPC_Object)
      {
         type = kCollObject;
         coll_obj = pClsn->GetObjID2();
         coll_submod = pClsn->GetSubModId2();
         PhysGetObjsNorm(pClsn->GetModel(), pClsn->GetSubModId(), pClsn->GetModel2(), pClsn->GetSubModId2(), normal);
      }
      else
      {
         type = kCollTerrain;
         coll_obj = GetTextureObj(pClsn->GetTexture());
         coll_submod = 0;
         normal = pClsn->GetNormal();
      }
   }

   if (someoneListening)
   {
      // Send messages to code listeners. Currently, these don't get any chance to reply.
      if (!proxy1)
         g_pPhysListeners->CallCollision(pClsn->GetObjID(), pClsn->GetSubModId(), type, coll_obj, coll_submod, normal, magnitude, pClsn->GetClsnPt());
      if (type == kCollObject)
         if (!proxy2)
            g_pPhysListeners->CallCollision(pClsn->GetObjID2(), pClsn->GetSubModId(), type, coll_obj, coll_submod, normal, magnitude, pClsn->GetClsnPt());
   }
     
   if (!sub1 && !sub2)
   {
      *reply1 = (int)mp_reply1;
      *reply2 = (int)mp_reply2;
      return;
   }

   AutoAppIPtr(ScriptMan);

   if (sub1 && !proxy1)
   {
      mxs_vector clsn_pt;
      mx_copy_vec(&clsn_pt, &pClsn->GetClsnPt());

      sPhysMsg phys_message(pClsn->GetObjID(), pClsn->GetSubModId(), type, coll_obj, coll_submod, normal, magnitude, clsn_pt);
   
      pScriptMan->SendMessage(&phys_message, &mp_reply1);
   }

   if ((type == kCollObject) && sub2 && !proxy2)
   {
      coll_obj = pClsn->GetObjID();
      mx_scaleeq_vec(&normal, -1.0);

      mxs_vector clsn_pt;
      mx_copy_vec(&clsn_pt, &pClsn->GetClsnPt());
         
      sPhysMsg phys_message2(pClsn->GetObjID2(), pClsn->GetSubModId(), type, coll_obj, coll_submod, normal, magnitude, clsn_pt);

      pScriptMan->SendMessage(&phys_message2, &mp_reply2);
   }

   *reply1 = (int)mp_reply1;
   *reply2 = (int)mp_reply2;

   AutoAppIPtr_(DamageModel, pDM);

   switch(*reply1)
   {
      case kPM_Slay: 
      {
         if (type == kCollObject)
            pDM->SlayObject(pClsn->GetObjID(), pClsn->GetObjID2());
         else
            pDM->SlayObject(pClsn->GetObjID(), GetTextureObj(pClsn->GetTexture()));

         break;
      }
     
      case kPM_NonPhys:
      {
         PhysDeregisterModel(pClsn->GetObjID());
         break;
      }
   }

   switch (*reply2)
   {
      case kPM_Slay:
      {
         pDM->SlayObject(pClsn->GetObjID2(), pClsn->GetObjID());
         break;
      }
      case kPM_NonPhys:
      {
         PhysDeregisterModel(pClsn->GetObjID2());
         break;
      }
   }
}

////////////////////////////////////////////////////////////////////////////////

void PhysMessageContactCreate(cPhysModel *pModel, int submod, int type, ObjID contact_object, int contact_submod)
{
#ifdef NEW_NETWORK_ENABLED
   AutoAppIPtr(NetManager);
   if (pNetManager->Networking()) {
      if (IsPlayerObj(contact_object)) {
         // The player is contacting this thing; we might want to take
         // it over now:
         PhysNetPlayerContactCreate(pModel);
      } 
      if (physMessageIsProxy(pModel->GetObjID())) {
         return;
      }
   }
#endif

   // RETURN_IF_PROXY(pModel->GetObjID());

   // hack for listening at doors
   if ( pModel->IsPlayer() && IsDoor(contact_object) )
      CheckDoorLeanSoundBlocking();

   if (!pPhysSubscribeService->IsSubscribed(pModel->GetObjID(), kContactMsg))
      return;

   cMultiParm reply = kPM_StatusQuo;
   sPhysMsg phys_message(pModel->GetObjID(), submod, kContactCreate, type, contact_object, contact_submod);

   AutoAppIPtr(ScriptMan);
   pScriptMan->SendMessage(&phys_message, &reply);

   switch (reply)
   {
      case kPM_Slay:
      {
         AutoAppIPtr_(DamageModel, pDM);
         if (type & kContactTerrain) // @TODO: use texture object 
            pDM->SlayObject(pModel->GetObjID(), OBJ_NULL);
         else
            pDM->SlayObject(pModel->GetObjID(), contact_object);
         break;
      }
      case kPM_NonPhys:
      {
         PhysDeregisterModel(pModel->GetObjID());
         break;
      }
      case kPM_Nothing:
      case kPM_Bounce:
      {
         Warning(("Invalid return value from contact script message\n"));
         break;
      }
   }
}

////////////////////////////////////////

void PhysMessageContactDestroy(cPhysModel *pModel, int submod, int type, ObjID contact_object, int contact_submod)
{
#ifdef NEW_NETWORK_ENABLED
   AutoAppIPtr(NetManager);
   if (pNetManager->Networking()) {
      if (IsPlayerObj(contact_object)) {
         // The player has stopped contacting this thing:
         PhysNetPlayerContactDestroy(pModel);
      }
      if (physMessageIsProxy(pModel->GetObjID())) {
         return;
      }
   }
#endif

   // RETURN_IF_PROXY(pModel->GetObjID());

   // hack for listening at doors
   if ( pModel->IsPlayer() && IsDoor(contact_object))
      CheckDoorLeanSoundBlocking();

   if (!pPhysSubscribeService->IsSubscribed(pModel->GetObjID(), kContactMsg))
      return;
 
   cMultiParm reply = kPM_StatusQuo;
   sPhysMsg phys_message(pModel->GetObjID(), submod, kContactDestroy, type, contact_object, contact_submod);

   AutoAppIPtr(ScriptMan);
   pScriptMan->SendMessage(&phys_message, &reply);

   switch (reply)
   {
      case kPM_Slay:
      {
         AutoAppIPtr_(DamageModel, pDM);
         if (type & kContactTerrain) // @TODO: use texture object 
            pDM->SlayObject(pModel->GetObjID(), OBJ_NULL);
         else
            pDM->SlayObject(pModel->GetObjID(), contact_object);
         break;
      }
      case kPM_NonPhys:
      {
         PhysDeregisterModel(pModel->GetObjID());
         break;
      }
      case kPM_Nothing:
      case kPM_Bounce:
      {
         Warning(("Invalid return value from contact script message\n"));
         break;
      }
   }
}
////////////////////////////////////////////////////////////////////////////////
//
// Entrance/Exiting
//
// Networking treats these differently, because we really want to filter
// based on who is *entering*, not who is *being entered*. That is, we
// want to filter on the trans_object. Then, sadly, we have to send a
// network message saying that we've entered the thing to its owner.
//
void PhysMessageEnter(cPhysModel *pModel, ObjID trans_object, int trans_submod)
{
   // If the thing entering is a proxy, just ignore the event:
   RETURN_IF_PROXY(trans_object);

   // We'll assume that subscriptions are the same on all machines,
   // so this is a useful filter to avoid excess network msgs:
   if (!pPhysSubscribeService->IsSubscribed(pModel->GetObjID(), kEnterExitMsg))
      return;

#ifdef NEW_NETWORK_ENABLED
   // Now, do we dispatch this message elsewhere to handle?
   if (PhysNetBroadcastEnterExit(kEnter, 
                                 pModel->GetObjID(), 
                                 trans_object,
                                 trans_submod))
   {
      // Okay, it's been dispatched
      return;
   }
#endif
 
   sPhysMsg phys_message(pModel->GetObjID(), kEnter, trans_object, trans_submod);

   AutoAppIPtr(ScriptMan);
   pScriptMan->SendMessage(&phys_message);
}

////////////////////////////////////////

void PhysMessageExit(cPhysModel *pModel, ObjID trans_object, int trans_submod)
{
   RETURN_IF_PROXY(trans_object);

   if (!pPhysSubscribeService->IsSubscribed(pModel->GetObjID(), kEnterExitMsg))
      return;
 
#ifdef NEW_NETWORK_ENABLED
   // Now, do we dispatch this message elsewhere to handle?
   if (PhysNetBroadcastEnterExit(kExit,
                                 pModel->GetObjID(), 
                                 trans_object,
                                 trans_submod))
   {
      // Okay, it's been dispatched
      return;
   }
#endif
 
   sPhysMsg phys_message(pModel->GetObjID(), kExit, trans_object, trans_submod);

   AutoAppIPtr(ScriptMan);
   pScriptMan->SendMessage(&phys_message);
}

////////////////////////////////////////////////////////////////////////////////


