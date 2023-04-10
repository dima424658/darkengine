/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/physics/phnet.cpp,v 1.36 1999/12/09 20:22:01 BODISAFA Exp $

#include <lg.h>
#include <mprintf.h>
#include <cfgdbg.h> // for ConfigSpew
#include <appagg.h>
#include <netman.h>
#include <phnet.h>
#include <physapi.h>
#include <phcore.h>
#include <matrix.h> // for mxs_real
#include <objpos.h>
#include <iobjnet.h>
#include <doorphys.h>
#include <phmod.h>
#include <phmods.h>
#include <netmsg.h>
#include <scrptapi.h>
#include <phscrpt.h>
#include <phmterr.h>
#include <contain.h>

// Stuff needed for heartbeats:
#include <simtime.h>
#include <playrobj.h>
#include <posprop.h>
#include <netprops.h>
#include <propman.h>
#include <rendprop.h> // for ObjHasRefs()
#include <dataops_.h> // for cClassDataOps
#include <proplist.h> // for cListPropertyStore
#include <propert_.h> // for cSpecificProperty
#include <aiapi.h>    // for ObjIsAI()
#include <ghostapi.h>

// Must be last:
#include <dbmem.h>

// The ID to use for all of our messages:
tNetMsgHandlerID gPhysNetID;

// The Network Manager:
INetManager *g_pNetMan;
// Object Networking:
IObjectNetworking *g_pObjNet;

// The position property, so we don't have to keep fetching it:
IPositionProperty *g_PosProp;

// After we do a load, we give it a couple of seconds to give things
// a chance to fall, then we check that heartbeats are turned on for
// anything still awake. This variable keeps track of when we want to
// perform that check. If it is zero, then we're not waiting:
static tSimTime g_WakeupCall;
// And this constant is how long (in ms) we're going to wait:
#define WAKEUP_WAIT_TIME 4000

// Utility: returns TRUE iff this is an object that we want to use the
// ghost system for here. Rejects both non-ghostable objects, and those
// whose ghosts are controlled elsewhere. If a physmodel is supplied, we
// make sure we don't bother with OBBs.
static BOOL IsGhostable(ObjID obj, cPhysModel *pModel)
{
   if (IsDoor(obj) ||
       IsAPlayer(obj) ||
       ObjIsAI(obj) ||
       (pModel && (pModel->GetType(0) == kPMT_OBB)))
   {
      return FALSE;
   } else {
      return TRUE;
   }
}

//////////
//
// Specific message types
//

//
// The following messages are for when we register and deregister objects;
// they tell the clients of this object that this has happened.
//
static BOOL forceContainedMsgs = FALSE;

static BOOL allowPhysicalBroadcast(ObjID obj)
{
   AutoAppIPtr_(ContainSys, pContainSys);

   if (forceContainedMsgs)
      return TRUE;
   else
      return (pContainSys->GetContainer(obj) == NULL);
}

//
// This grotesque little hack is so that higher-level systems can override
// the above override. Specifically, Shock sends its dephys message *after*
// the object goes into a container, so we need to let the message through.
//
void PhysNetForceContainedMsgs(BOOL force)
{
   forceContainedMsgs = force;
}

/////////////////////////////////////////////////////
//
// SPHERE HAT
//
static cNetMsg *g_pRegisterSphereHatMsg = NULL;

static void handleRegisterSphereHat(ObjID obj, uint flags, float radius,
                                    mxs_vector *loc, mxs_angvec *fac)
{
   ObjPosUpdate(obj, loc, fac);
   PhysRegisterSphereHat(obj, flags, radius);
   if (IsGhostable(obj, NULL))
   {
      GhostAddRemote(obj, 1, kGhostCfIsObj);
   }
}

static sNetMsgDesc sRegisterSphereHatDesc =
{
   kNMF_Broadcast,
   "RegSphHat",
   "Phys Register Sphere Hat",
   NULL,
   handleRegisterSphereHat,
   {{kNMPT_SenderObjID, kNMPF_None, "Obj"},
    {kNMPT_UInt, kNMPF_None, "Flags"},
    {kNMPT_Float, kNMPF_None, "Radius"},
    {kNMPT_Vector, kNMPF_None, "Location"},
    {kNMPT_Block, kNMPF_None, "Facing", sizeof(mxs_angvec)},
    {kNMPT_End}}
};

void PhysBroadcastRegisterSphereHat(ObjID objID, unsigned flags, mxs_real radius)
{
   if (!allowPhysicalBroadcast(objID))
      return;

   mxs_vector *loc;
   mxs_angvec *fac;
   mxs_vector null_loc = {0,0,0};
   mxs_angvec null_fac = {0,0,0};
   ObjPos *pos = ObjPosGet(objID);
   if (pos)
   {
      loc = &(pos->loc.vec);
      fac = &(pos->fac);
   } else {
      loc = &null_loc;
      fac = &null_fac;
   }
   g_pRegisterSphereHatMsg->Send(OBJ_NULL, objID, flags, radius, loc, fac);
   if (IsGhostable(objID, NULL))
   {
//      Warning(("PhysBroadcastRegisterSphereHat: adding %d\n", objID));
      GhostAddLocal(objID, 1, kGhostCfIsObj);
   }
}

/////////////////////////////////////////////////////
//
// SPHERE
//
static cNetMsg *g_pRegisterSphereMsg = NULL;

static void handleRegisterSphere(ObjID obj, 
                                 int numSubModels,
                                 uint flags, 
                                 float radius,
                                 mxs_vector *loc,
                                 mxs_angvec *fac)
{
   ObjPosUpdate(obj, loc, fac);
   PhysRegisterSphere(obj, numSubModels, flags, radius);
   if (IsGhostable(obj, NULL))
   {
      GhostAddRemote(obj, 1, kGhostCfIsObj);
   }
}

static sNetMsgDesc sRegisterSphereDesc =
{
   kNMF_Broadcast,
   "RegSphere",
   "Phys Register Sphere",
   NULL,
   handleRegisterSphere,
   {{kNMPT_SenderObjID, kNMPF_None, "Obj"},
    {kNMPT_Int, kNMPF_None, "Num Submodels"},
    {kNMPT_UInt, kNMPF_None, "Flags"},
    {kNMPT_Float, kNMPF_None, "Radius"},
    {kNMPT_Vector, kNMPF_None, "Location"},
    {kNMPT_Block, kNMPF_None, "Facing", sizeof(mxs_angvec)},
    {kNMPT_End}}
};

void PhysBroadcastRegisterSphere(ObjID objID,
                                 tPhysSubModId /* int */ numSubModels,
                                 unsigned flags,
                                 mxs_real radius)
{
   if (!allowPhysicalBroadcast(objID))
      return;

   mxs_vector *loc;
   mxs_angvec *fac;
   mxs_vector null_loc = {0,0,0};
   mxs_angvec null_fac = {0,0,0};
   ObjPos *pos = ObjPosGet(objID);
   if (pos)
   {
      loc = &(pos->loc.vec);
      fac = &(pos->fac);
   } else {
      loc = &null_loc;
      fac = &null_fac;
   }
   g_pRegisterSphereMsg->Send(OBJ_NULL, objID, numSubModels, flags, radius,
                              loc, fac);
   if (IsGhostable(objID, NULL))
   {
//      Warning(("PhysBroadcastRegisterSphere: adding %d\n", objID));
      GhostAddLocal(objID, 1, kGhostCfIsObj);
   }
}

/////////////////////////////////////////////////////
//
// OBB
//
static cNetMsg *g_pRegisterOBBMsg = NULL;

static void handleRegisterOBB(ObjID obj, 
                              uint flags)
{
   PhysRegisterOBB(obj, flags);
}

static sNetMsgDesc sRegisterOBBDesc =
{
   kNMF_Broadcast,
   "RegOBB",
   "Phys Register OBB",
   NULL,
   handleRegisterOBB,
   {{kNMPT_SenderObjID, kNMPF_None, "Obj"},
    {kNMPT_UInt, kNMPF_None, "Flags"},
    {kNMPT_End}}
};

void PhysBroadcastRegisterOBB(ObjID objID,
                              unsigned flags)
{
   g_pRegisterOBBMsg->Send(OBJ_NULL, objID, flags);
}

/////////////////////////////////////////////////////
//
// DEREGISTER
//
static cNetMsg *g_pDeregisterMsg = NULL;

static void handleDeregister(ObjID obj)
{
   GhostNotifyPhysDeregister(obj);
   PhysDeregisterModel(obj);
   if (IsGhostable(obj, NULL))
   {
      GhostRemRemote(obj);
   }
}

static sNetMsgDesc sDeregisterDesc =
{
   kNMF_Broadcast,
   "Deregister",
   "Phys Deregister",
   NULL,
   handleDeregister,
   {{kNMPT_SenderObjID, kNMPF_None, "Obj"},
    {kNMPT_End}}
};

void PhysBroadcastDeregister(ObjID objID)
{
   if (!allowPhysicalBroadcast(objID))
      return;
   if (GhostAllowDeregisterPacket(objID))
      g_pDeregisterMsg->Send(OBJ_NULL, objID);
   if (IsGhostable(objID, NULL))
   {
      GhostRemLocal(objID);
   }
}

/////////////////////////////////////////////////////
//
// DOORS
//
// This message tells the clients that the state of a door has changed.
// This code is closely tied to doorphys.
//
// @TBD: this only transmits the status changes, *not* the final result.
// At least when the door stops, we should transmit the final location,
// so the systems wind up synchronized.
//
static cNetMsg *g_pDoorMsg = NULL;

static void handleDoor(ObjID obj, 
                       uchar byte_what_happened)
{
   eDoorStatus what_happened = byte_what_happened;

   switch (what_happened)
   {
      case kDoorClosed:
         // @NOTE: for now, we aren't responding to these two messages,
         // because DoorSlam appears to cause rendering glitchiness. Besides,
         // the semantics are a bit suspect -- we don't necessarily want to
         // try to synch to this degree. So we let each door close at its
         // own rate.
         // Assuming these two cases stay out, we should modify the sender
         // above, to not send them as messages...
         // DoorSlamClosed(obj);
         break;
      case kDoorOpen:
         // DoorSlamOpen(obj);
         break;
      case kDoorClosing:
         DoorPhysActivate(obj, DOORPHYS_CLOSE);
         break;
      case kDoorOpening:
         DoorPhysActivate(obj, DOORPHYS_OPEN);
         break;
      case kDoorHalt:
         DoorPhysAbort(obj);
         break;
      default:
         Warning(("Phnet: unknown door status %d\n", what_happened));
         break;
   }
}

static sNetMsgDesc sDoorDesc =
{
   kNMF_Broadcast,
   "Door",
   "Phys Door Change",
   NULL,
   handleDoor,
   {{kNMPT_SenderObjID, kNMPF_None, "Obj"},
    {kNMPT_UByte, kNMPF_None, "What Happened?"},
    {kNMPT_End}}
};

// For now, we don't do anything with what_was_happening; it's in as a
// param in case we want it later:
void PhysBroadcastDoor(ObjID objID,
                       int what_happened,
                       int /* what_was_happening */)
{
   // Check against truncation:
   Assert_(((uchar) what_happened) == what_happened);

   g_pDoorMsg->Send(OBJ_NULL, objID, (uchar) what_happened);
}

/////////////////////////////////////////////////////
//
// Control velocity
//
// @TBD: For now we're experimentally not networking these at all. If
// this proves to work as expected, then this code should probably
// be excised entirely.
//
static cNetMsg *g_pControlVelocityMsg = NULL;

static void handleControlVelocity(ObjID obj, mxs_vector *velocity)
{
   mxs_vector zero_vec;
   mx_zero_vec(&zero_vec);
   if (!mx_is_identical(velocity,&zero_vec,0.0))
      PhysControlVelocity(obj, velocity);
   else
      PhysStopControlVelocity(obj);
}

static sNetMsgDesc sControlVelocityDesc =
{
   kNMF_Broadcast,
   "CtrlVel",
   "Phys Control Velocity",
   NULL,
   handleControlVelocity,
   {{kNMPT_SenderObjID, kNMPF_None, "Obj"},
    {kNMPT_Vector, kNMPF_None, "Velocity"},
    {kNMPT_End}}
};

void PhysBroadcastControlVelocity(ObjID objID, mxs_vector *velocity)
{
   // @TBD: with the advent of near-universal ghosts, do we *ever* want
   // to network control velocities? What about axis and rotational ctrl
   // velocities?

   // AIs get dealt with through the ghost system:
#ifdef NETWORK_CONTROL_VELOCITIES
   if (!ObjIsAI(objID))
      g_pControlVelocityMsg->Send(OBJ_NULL, objID, velocity);
#endif
}

/////////////////////////////////////////////////////
//
// Axis Control velocity
//
static cNetMsg *g_pAxisControlVelocityMsg = NULL;

static void handleAxisControlVelocity(ObjID obj, uchar axis, float speed)
{
   if (speed != 0)
      PhysAxisControlVelocity(obj, axis, speed);
   else
      PhysStopAxisControlVelocity(obj, axis);
}

static sNetMsgDesc sAxisControlVelocityDesc =
{
   kNMF_Broadcast,
   "AxisCtrlVel",
   "Phys Axis Control Velocity",
   NULL,
   handleAxisControlVelocity,
   {{kNMPT_SenderObjID, kNMPF_None, "Obj"},
    {kNMPT_UByte, kNMPF_None, "Axis"},
    {kNMPT_Float, kNMPF_None, "Speed"},
    {kNMPT_End}}
};

void PhysBroadcastAxisControlVelocity(ObjID objID, int axis, mxs_real speed)
{
#ifdef NETWORK_CONTROL_VELOCITIES
   // We don't send these messages for AIs; they get handled elsewhere:
   if (ObjIsAI(objID))
      return;

   // Check for truncation:
   Assert_(((uchar) axis) == axis);

   g_pAxisControlVelocityMsg->Send(OBJ_NULL, objID, (uchar) axis, speed);
#endif
}

/////////////////////////////////////////////////////
//
// Rotational Control velocity
//
static cNetMsg *g_pControlRotationalVelocityMsg = NULL;

static void handleControlRotationalVelocity(ObjID obj, mxs_vector *velocity)
{
   PhysControlRotationalVelocity(obj, velocity);
}

static sNetMsgDesc sControlRotationalVelocityDesc =
{
   kNMF_Broadcast,
   "CtrlRotVel",
   "Phys Control Rotational Velocity",
   NULL,
   handleControlRotationalVelocity,
   {{kNMPT_SenderObjID, kNMPF_None, "Obj"},
    {kNMPT_Vector, kNMPF_None, "Velocity"},
    {kNMPT_End}}
};

void PhysBroadcastControlRotationalVelocity(ObjID objID, mxs_vector *velocity)
{
#ifdef NETWORK_CONTROL_VELOCITIES
   g_pControlRotationalVelocityMsg->Send(OBJ_NULL, objID, velocity);
#endif
}

//////////
//
// Physics Sleep/Wake
//
// Start or stop sending heartbeats for the specified object. If an
// object is asleep, we don't send beats for it.
//
// Note that "heartbeats" are now implemented through the ghost system,
// rather than being a separate mechanism. So this is basically just
// the glue between physics and ghost.
//
void PhysNetSetSleep(ObjID obj, BOOL sleep, BOOL startNow)
{
   if (g_WakeupCall) {
      // We're waiting to establish the ghosts
      return;
   }

   cPhysModel *pModel = g_PhysModels.Get(obj);
   if (!pModel) {
      // It doesn't have physics!
      return;
   }

   if (!IsGhostable(obj, pModel)) {
      return;
   }

   if (!g_pObjNet->ObjHostedHere(obj)) {
      // Not our problem, or it's local-only
      return;
   }

   if (sleep) {
      // Tell the ghost system to put this thing to sleep:
      GhostNotify(obj, kGhostStSleep);
   } else {
      // Tell the ghost system to wake this object up:
      GhostNotify(obj, kGhostStRevive);
   }
}

/////////////////////////////////////////////////////
//
// Object Contact
//
// In order to get timing right, we really need to own any objects that we
// are trying to push. (Otherwise, the out-of-synch heartbeats sent by
// the world host will screw us up.) So this code is designed to take
// over objects that we are trying to push. It's fairly complex, in order
// to deal with the possible boundary conditions.
//

//
// No-Borrow property
//
// This property indicates that we should not allow this object to be
// borrowed. It is largely intended for situations when we want to
// temporarily prevent the object from being borrowed, although it could
// also be used in a longer-term way.
//
static IBoolProperty *g_PropNoBorrow;

static sPropertyDesc NoBorrowDesc =
{
   "NoBorrow",
   0,
   NULL, 0, 0,
   {"Networking", "Disallow Borrow"},
   0,
};

//
// Pending-Borrow property
//
// This property indicates that we are in the process of trying to borrow
// this object. At some point, we might combine it with the borrowed-object
// property below, but historical inertia has left them separate...
//
static IBoolProperty *g_PropPendingBorrow;

static sPropertyDesc PendingBorrowDesc =
{
   "Borrowing",
   kPropertyNoEdit,
   NULL, 0, 0,
   {"Networking", "Pending Borrow"},
   kPropertyChangeLocally,
};

//
// The Borrowed-Object Property
//
// This little property keeps track of objects that we have "borrowed"
// from the world host, in order to push them. We need to keep track of
// that so that we don't try to give back objects that we actually
// created ourselves. There are temporary objects like projectile
// casings, that can get created on us, and then get destroyed; we
// must *not* try to hand those off, since they then get immediately
// destroyed, and we get into an unstable state.
//
static IBoolProperty *g_PropBorrowedForPush;

static sPropertyDesc BorrowedForPushDesc =
{
   "Borrowed",
   kPropertyNoEdit,
   NULL, 0, 0,
   {"Networking", "Borrowed For Push"},
   kPropertyChangeLocally,
};

// Note that this object is no longer to be considered "borrowed". This
// should be called by anything that's going to keep ownership of the
// object, while breaking the physical contact.
void PhysNetDiscardBorrow(ObjID obj)
{
   g_PropBorrowedForPush->Delete(obj);
}

// Give this object back to the world host. That might not be who we
// borrowed it from, but that's okay; it's always reasonable for him
// to own world objects.
static void returnToHost(ObjID obj)
{
   // Don't try to return an object that's been destroyed:
   AutoAppIPtr(ObjectSystem);
   if (pObjectSystem->Exists(obj))
   {
      g_pObjNet->ObjGiveWithoutObjID(obj, g_pNetMan->DefaultHost());
   }
}

//
// The message to allow (or deny) a request for an object:
//
static void handleTakeOver(ObjID obj, uchar permit)
{
   BOOL pending;
   if (g_PropPendingBorrow->Get(obj, &pending) && pending) {
      // Okay, we still have a pending request
      // The old owner should have given it to us, so all should be
      // cool
      if (permit) {
         // Record that we've borrowed this object
         g_PropBorrowedForPush->Set(obj, TRUE);
      } else {
         // We've still got a remote ghost; let it work again:
         GhostChangeRemoteConfig(obj, kGhostCfDisable, TRUE);
      }
   } else {
      if (permit) {
         // Looks like we deleted the request before they got back to us
         // Ask the world host to take it back
         returnToHost(obj);
      }
   }
   // One way or t'other, the request is no longer pending:
   g_PropPendingBorrow->Delete(obj);
}

static sNetMsgDesc sPermitTakeOverDesc =
{
   kNMF_None,
   "PermTakeOver",
   "Permit PhysNet TakeOver",
   NULL,
   handleTakeOver,
   {{kNMPT_SenderObjID, kNMPF_NoAssertHostedHere, "Object"},
    {kNMPT_UByte, kNMPF_None, "Permit"},
    {kNMPT_End}}
};

static cNetMsg *g_pPermitTakeOverMsg = NULL;

//
// The message to ask for an object:
//
// @TBD: We really ought to rewrite this to use AppendSenderID and a
// normal handler, on general principles.
static sNetMsgDesc sRequestTakeOverDesc =
{
   kNMF_SendToObjOwner | kNMF_NoForwarding,
   "ReqTakeOver",
   "PhysNet Requests Object",
   NULL,
   NULL,
   {{kNMPT_ReceiverObjID, kNMPF_AllowNonExistentObj, "Obj"},
    {kNMPT_ObjID, kNMPF_None, "New Proxy ID"},
    {kNMPT_End}}
};

// We actually need to handle this message specially, rather than
// with a simple handler function:
class cRequestTakeOverMsg : public cNetMsg {
public:
   cRequestTakeOverMsg(sNetMsgDesc *pDesc)
      : cNetMsg(pDesc, NULL)
   {}

   // When we get this message, we (may) hand the object over to the
   // requestor.
   void InterpretMsg(ObjID fromPlayer)
   {
      // We only hand over the object if we still own it; this is to
      // deal with the raceway condition of multiple players trying to
      // take over the same thing. Also check the obscure case of the
      // thing not having refs, which can happen if they tried to
      // pick something up, and then immediately kicked it. And the
      // case of the thing having been destroyed, which can happen with,
      // say, explosions. And don't allow something in a container -- like
      // our weapon -- to be borrowed.
      ObjID obj = (ObjID) GetParam(0);
      AutoAppIPtr(ObjectSystem);
      AutoAppIPtr(ContainSys);
      BOOL noBorrow = FALSE;
      g_PropNoBorrow->Get(obj, &noBorrow);
      if (!noBorrow &&
          pObjectSystem->Exists(obj) &&
          g_pObjNet->ObjHostedHere(obj) &&
          !pContainSys->Contains(OBJ_NULL, obj) &&
          ObjHasRefs(obj))
      {
         // Okay, give it to him
         g_pObjNet->ObjGiveTo(obj, fromPlayer, GetParam(1));
         g_pPermitTakeOverMsg->Send(fromPlayer, obj, (uchar) TRUE);
      } else {
         // Nope, don't let him have it
         g_pPermitTakeOverMsg->Send(fromPlayer, obj, (uchar) FALSE);
      }
   }
};

static cRequestTakeOverMsg *g_pRequestTakeOverMsg = NULL;

// This gets called when the player comes into contact with an object.
// If that object is pushable, and we don't already own it, try to take
// it over.
void PhysNetPlayerContactCreate(cPhysModel *pModel)
{
   ObjID obj = pModel->GetObjID();

   if (ObjIsAI(obj))
      return;

   // Don't sweat it unless the object's a proxy owned by someone else:
   if (!g_pObjNet->ObjIsProxy(obj))
      return;

   // Is it pushable?
   if (pModel->IsLocationControlled())
      // Nope.
      return;

   // Is it a door? Doors become un-location-controlled when they're
   // moving, but we still don't want to borrow them:
   if (IsDoor(obj))
      return;

   // Is it moving terrain? We could wreck ruddy havoc by borrowing that,
   // I suspect:
   if (IsMovingTerrain(obj))
      return;

   // Is it attached to something? If so, we probably oughtn't borrow it:
   if (pModel->IsAttached())
      return;

   // Is it another player?
   if (IsAPlayer(obj))
      // Yep -- we'd better not try to take it over...
      return;

   // Have we already asked to take it?
   BOOL borrowPending = FALSE;
   if (g_PropPendingBorrow->Get(obj, &borrowPending) && borrowPending)
      // Yep; don't bother with another
      return;

   // Okay, if we're at this point, then it's an object that we want to
   // try pushing. Take it over for the time being.
   g_pRequestTakeOverMsg->Send(OBJ_NULL, obj, obj);
   // Record that we've asked for it:
   g_PropPendingBorrow->Set(obj, TRUE);
   // Disable ghost messages for the time being:
   GhostChangeRemoteConfig(obj, kGhostCfDisable, FALSE);
}

// This gets called when the player stops contacting the object. If we're
// not the world host, hand the object ownership back to the host.
void PhysNetPlayerContactDestroy(cPhysModel *pModel)
{
   ObjID obj = pModel->GetObjID();

   BOOL borrowPending = FALSE;
   g_PropPendingBorrow->Get(obj, &borrowPending);

   // One way or t'other, we're not waiting for it any more:
   g_PropPendingBorrow->Delete(obj);

   BOOL borrowed;
   if (!g_PropBorrowedForPush->Get(obj, &borrowed) || !borrowed) {
      if (borrowPending) {
         // This object wasn't actually borrowed by us, so don't
         // return it, but *do* let ghost msgs come through again:
         GhostChangeRemoteConfig(obj, kGhostCfDisable, TRUE);
      }
      return;
   }

   if (g_pNetMan->AmDefaultHost())
      // We should own it anyway
      return;

   if (!g_pObjNet->ObjHostedHere(obj))
      // We never took it over anyway
      return;

   // Stop sending heartbeats for it:
   PhysNetSetSleep(obj, TRUE, TRUE);

   // Record that we're returning it:
   g_PropBorrowedForPush->Delete(obj);

   // Ask the world host to take it back
   returnToHost(obj);
}

//////////
//
// Enter/Exit messages
//
// These get triggered iff something local enters another object. They
// should transmit a message iff the other object isn't local.
//
// @TBD: when we get a mechanism to allow subtyped script messages to be
// automatically dispatched over the net, remove this mechanism and
// replace it with just dispatching the enter/exit messages appropriately.
//
static cNetMsg *g_pEnterExitMsg = NULL;

// For now, we just deal with this locally. It's scungy, but it's easiest.
static void handleEnterExit(uchar inout, 
                            ObjID enteree, 
                            ObjID enterer, 
                            ushort submod)
{
   sPhysMsg phys_message(enteree, inout, enterer, submod);
   AutoAppIPtr(ScriptMan);
   pScriptMan->SendMessage(&phys_message);
}

static sNetMsgDesc sEnterExitDesc =
{
   kNMF_SendToObjOwner,
   "EnterExit",
   "Enter or Exit Object",
   NULL,
   handleEnterExit,
   {{kNMPT_UByte, kNMPF_None, "Enter or Exit?"},
    {kNMPT_ReceiverObjID, kNMPF_None, "Enteree"},
    {kNMPT_SenderObjID, kNMPF_None, "Enterer"},
    {kNMPT_UShort, kNMPF_None, "Submod"},
    {kNMPT_End}}
};

BOOL PhysNetBroadcastEnterExit(ePhysEnterExit inout,
                               ObjID enteree,
                               ObjID enterer,
                               int submod)
{
   if (g_pNetMan->Networking() &&
       g_pObjNet->ObjIsProxy(enteree) &&
       g_pObjNet->ObjHostedHere(enterer))
   {
      g_pEnterExitMsg->Send(OBJ_NULL, inout, enteree, enterer, submod);
      return TRUE;
   } else {
      return FALSE;
   }
}

//////////
//
// Change an object's position.
//
static cNetMsg *g_pObjPosMsg = NULL;

void handleObjPos(ObjID obj, mxs_vector *pos, mxs_angvec *fac)
{
   if (PhysObjHasPhysics(obj))
   {
      PhysSetModLocation(obj,pos);
      PhysSetModRotation(obj,fac);
   }
   ObjPosUpdate(obj,pos,fac);
}

static sNetMsgDesc sObjPosDesc =
{
   kNMF_Broadcast,
   "ObjPos",
   "Object Position",
   NULL,
   handleObjPos,
   {{kNMPT_SenderObjID, kNMPF_None, "Object"},
    {kNMPT_Vector, kNMPF_None, "Location"},
    {kNMPT_Block, kNMPF_None, "Facing", sizeof(mxs_angvec)},
    {kNMPT_End}}
};

void PhysNetBroadcastObjPos(ObjID obj,
                            mxs_vector *pos,
                            mxs_angvec *fac)
{
   if (g_pNetMan->Networking() &&
       g_pObjNet->ObjHostedHere(obj))
   {
      g_pObjPosMsg->Send(OBJ_NULL, obj, pos, fac);
   }
}

void PhysNetBroadcastObjPosByObj(ObjID obj)
{
   mxs_vector pos;
   mxs_angvec fac;
   PhysGetModLocation(obj, &pos);
   PhysGetModRotation(obj, &fac);
   PhysNetBroadcastObjPos(obj, &pos, &fac);
}

//////////
//
// Set an object's gravity
//
static cNetMsg *g_pGravityMsg = NULL;

static void handleGravity(ObjID obj, mxs_real gravity)
{
   PhysSetGravity(obj, gravity);
}

static sNetMsgDesc sGravityDesc =
{
   kNMF_Broadcast,
   "Gravity",
   "Object Gravity",
   NULL,
   handleGravity,
   {{kNMPT_SenderObjID, kNMPF_IfHostedHere, "Object"},
    {kNMPT_Float, kNMPF_None, "Gravity"},
    {kNMPT_End}}
};

void PhysNetBroadcastGravity(ObjID obj, mxs_real gravity)
{
   // Don't network the gravity for players or AIs; they are represented
   // by zero-gravity ghosts on the other end.
   if (IsAPlayer(obj) || ObjIsAI(obj))
      return;

   g_pGravityMsg->Send(OBJ_NULL, obj, gravity);
}

//////////
//
// High-level Stuff
//

// This method actually does the wakeup check. It just iterates over the
// active objects, and makes sure each is awake:
void WakeupHeartbeats()
{
   // First, run through all the moving objects, and create their ghosts:
   cPhysModel *pModel = g_PhysModels.GetFirstMoving();
   while (pModel)
   {
      ObjID obj = pModel->GetObjID();
      if (IsGhostable(obj, pModel))
      {
         if (g_pObjNet->ObjHostedHere(obj)) {
//            Warning(("WakeupHeartbeats: 0 adding %d\n", obj));
            GhostAddLocal(obj, 1, kGhostCfIsObj);
         } else if (g_pObjNet->ObjIsProxy(obj)) {
            GhostAddRemote(obj, 1, kGhostCfIsObj);
         } // else it must be local-only
      }
      pModel = g_PhysModels.GetNextMoving(pModel);
   }

   // Now, do the same for stationary objects, but put them to sleep:
   pModel = g_PhysModels.GetFirstStationary();
   while (pModel)
   {
      ObjID obj = pModel->GetObjID();
      if (IsGhostable(obj, pModel))
      {
         if (g_pObjNet->ObjHostedHere(obj)) {
//            Warning(("WakeupHeartbeats: 1 adding %d\n", obj));
            GhostAddLocal(obj, 1, kGhostCfIsObj);
            GhostNotify(obj, kGhostStSleep);
         } else if (g_pObjNet->ObjIsProxy(obj)) {
            GhostAddRemote(obj, 1, kGhostCfIsObj);
         } // else it must be local-only
      }
      pModel = g_PhysModels.GetNextStationary(pModel);
   }
}

// This should get called each frame. Currently, it just checks for
// pending heartbeats:
void PhysNetFrame()
{
   if (g_WakeupCall &&
       GetSimTime() > g_WakeupCall)
   {
      WakeupHeartbeats();
      g_WakeupCall = 0;
   }
}

// This should get called after we finish loading a level.
void PhysNetPostLoad()
{
   g_WakeupCall = GetSimFileTime() + WAKEUP_WAIT_TIME;
}

// Set up the network client. We assume that NetManager has been initialized
void PhysNetInit()
{
#ifdef NEW_NETWORK_ENABLED
   g_pNetMan = AppGetObj(INetManager);
   Assert_(g_pNetMan);

   AutoAppIPtr_(PropertyManager, pPropMan);
   IProperty* pprop = pPropMan->GetPropertyNamed(PROP_POSITION_NAME); 
   Verify(SUCCEEDED(pprop->QueryInterface(IID_IPositionProperty,
                                          (void**)&g_PosProp))); 
   g_pObjNet = AppGetObj(IObjectNetworking);

   // Any local variables:
   g_WakeupCall = 0;

   // Our internal properties:
   g_PropPendingBorrow = CreateBoolProperty(&PendingBorrowDesc,
                                            kPropertyImplSparse);
   g_PropBorrowedForPush = CreateBoolProperty(&BorrowedForPushDesc,
                                              kPropertyImplDense);
   g_PropNoBorrow = CreateBoolProperty(&NoBorrowDesc,
                                       kPropertyImplSparse);

   // And the messages themselves:
   g_pRegisterSphereHatMsg = new cNetMsg(&sRegisterSphereHatDesc);
   g_pRegisterSphereMsg = new cNetMsg(&sRegisterSphereDesc);
   g_pRegisterOBBMsg = new cNetMsg(&sRegisterOBBDesc);
   g_pDeregisterMsg = new cNetMsg(&sDeregisterDesc);
   g_pDoorMsg = new cNetMsg(&sDoorDesc);
   g_pControlVelocityMsg = new cNetMsg(&sControlVelocityDesc);
   g_pAxisControlVelocityMsg = new cNetMsg(&sAxisControlVelocityDesc);
   g_pControlRotationalVelocityMsg =
      new cNetMsg(&sControlRotationalVelocityDesc);
   g_pPermitTakeOverMsg = new cNetMsg(&sPermitTakeOverDesc);
   g_pRequestTakeOverMsg = new cRequestTakeOverMsg(&sRequestTakeOverDesc);
   g_pEnterExitMsg = new cNetMsg(&sEnterExitDesc);
   g_pObjPosMsg = new cNetMsg(&sObjPosDesc);
   g_pGravityMsg = new cNetMsg(&sGravityDesc);
#endif
}

// Shut down Physics-specific networking.
void PhysNetTerm()
{
#ifdef NEW_NETWORK_ENABLED

   // Remove the PhysNetTerm block.
   cPhysModel* pModel = g_PhysModels.GetFirstStationary();
   while (pModel)
   {
      ObjID obj = pModel->GetObjID();
      if (IsGhostable(obj, pModel))
      {
         if (g_pObjNet->ObjHostedHere(obj))
         {
            GhostRemLocal(obj);
         }
         else if (g_pObjNet->ObjIsProxy(obj))
         {
            GhostRemRemote(obj);
         }
         // else it must be local-only
      }
      pModel = g_PhysModels.GetNextStationary(pModel);
   }

   SafeRelease(g_pNetMan);
   SafeRelease(g_pObjNet);
   SafeRelease(g_PosProp);

   delete g_pRegisterSphereHatMsg;
   delete g_pRegisterSphereMsg;
   delete g_pRegisterOBBMsg;
   delete g_pDeregisterMsg;
   delete g_pDoorMsg;
   delete g_pControlVelocityMsg;
   delete g_pAxisControlVelocityMsg;
   delete g_pControlRotationalVelocityMsg;
   delete g_pPermitTakeOverMsg;
   delete g_pRequestTakeOverMsg;
   delete g_pEnterExitMsg;
   delete g_pObjPosMsg;
   delete g_pGravityMsg;
#endif
}
