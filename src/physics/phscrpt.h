/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phscrpt.h,v 1.18 1999/12/10 14:45:33 PATMAC Exp $
//

#ifndef __PHSCRPT_H
#define __PHSCRPT_H

#pragma once

#include <scrptsrv.h>
#include <scrptmsg.h>
#include <objscrt.h>

#include <matrix.h>

#include <phscrt.h>

////////////////////////////////////////
//
// Services
//

DECLARE_SCRIPT_SERVICE(Physics, 0x141)
{
   // Subsribe an object to message types
   STDMETHOD(SubscribeMsg)(object phys_obj, int message_types) PURE;

   // Unsubscribe an object to message types
   STDMETHOD(UnsubscribeMsg)(object phys_obj, int message_types) PURE;

   // @HACK: should this be here?
   // Launch Projectile 
   STDMETHOD_(object,LaunchProjectile)(object launcher, object proj, real power, integer flags, const vector ref add_vel) PURE; 

   // Velocity
   STDMETHOD(SetVelocity)(object obj, const vector ref vel) PURE; 
   STDMETHOD(GetVelocity)(object obj, vector ref vel) PURE; 
   STDMETHOD(ControlVelocity)(object obj, const vector ref vel) PURE; 
   STDMETHOD(StopControlVelocity)(object obj) PURE;

   // Gravity
   STDMETHOD(SetGravity)(object obj, float gravity) PURE;
   STDMETHOD_(float, GetGravity)(object obj) PURE;

   // Utility
   STDMETHOD_(BOOL, HasPhysics)(object obj) PURE;
   // Has sphere, point or sphere hat model instantiated 
   STDMETHOD_(BOOL, IsSphere)(object obj) PURE;
   // Has OBB model instantiated
   STDMETHOD_(BOOL, IsOBB)(object obj) PURE;

   // Control
   // Control location/rotation/position/velocity to current value
   STDMETHOD(ControlCurrentLocation)(object obj) PURE;
   STDMETHOD(ControlCurrentRotation)(object obj) PURE;
   STDMETHOD(ControlCurrentPosition)(object obj) PURE;

   // Model control
   STDMETHOD(DeregisterModel)(object obj) PURE;

   // Player 
   STDMETHOD_(void, PlayerMotionSetOffset)(int subModel, vector ref offset) PURE;

   STDMETHOD(Activate)(const object obj) PURE;

   STDMETHOD_(BOOL, ValidPos)(const object obj) PURE;
};


////////////////////////////////////////
//
// Messages
//

struct sPhysMsgBase
{
   int Submod;

   // CollisionMsg
   ePhysCollisionType collType;
   ObjID              collObj;      // either object or terrain archetype
   int                collSubmod;
   mxs_real           collMomentum; // p = mv + mv
   mxs_vector         collNormal;   // from object or terrain hit
   mxs_vector         collPt;       // only estimate for object collisions   // ContactMsg
   ePhysContactType contactType;
   ObjID            contactObj;  // either object or terrain archetype
   int              contactSubmod;

   // Enter/exit
   ObjID            transObj;
   int              transSubmod;

   sPhysMsgBase() {}

   // Collision
   sPhysMsgBase(int submod, ePhysCollisionType type, ObjID coll_object, 
            int coll_submod, const mxs_vector &normal, mxs_real momentum, const mxs_vector &pt)
   {
      Submod = submod;

      collType = type;
      collObj = coll_object;
      collSubmod = coll_submod;
      collNormal = normal;
      collMomentum = momentum;
      collPt = pt;

      contactType = kContactNone;
      contactObj = OBJ_NULL;
      contactSubmod = -1;

      transObj = OBJ_NULL;
      transSubmod = -1;
   }

   // Contact
   sPhysMsgBase(int submod, ePhysContact cd, ePhysContactType type, ObjID contact_object, int contact_submod)
   {
      Submod = submod;

      contactType = type;
      contactObj = contact_object;
      contactSubmod = contact_submod;

      collType = kCollNone;
      collObj = OBJ_NULL;
      collSubmod = -1;
      mx_zero_vec(&collNormal);
      mx_zero_vec(&collPt);
      collMomentum = 0;

      transObj = OBJ_NULL;
      transSubmod = -1;
   }

   // Enter/exit
   sPhysMsgBase(ePhysEnterExit type, ObjID trans_object, int trans_submod)
   {
      Submod = -1;

      transObj = trans_object;
      transSubmod = trans_submod;

      collType = kCollNone;
      collObj = OBJ_NULL;
      collSubmod = -1;
      mx_zero_vec(&collNormal);
      mx_zero_vec(&collPt);
      collMomentum = 0;

      contactType = kContactNone;
      contactObj = OBJ_NULL;
      contactSubmod = -1;
   }
};

#define kPhysMsgVer 3

struct sPhysMsg : public sScrMsg, public sPhysMsgBase
{
   sPhysMsg() {}

   // Creation
   sPhysMsg(ObjID objID, ePhysScriptMsgType type)
   {
      to = objID;
      free((void *)message);

      switch (type)
      {
         case kFellAsleepMsg:
            message = strdup("PhysFellAsleep");
            break;

         case kWokeUpMsg:
            message = strdup("PhysWokeUp");
            break;

         case kMadePhysMsg:
            message = strdup("PhysMadePhysical");
            break;

         case kMadeNonPhysMsg:
            message = strdup("PhysMadeNonPhysical");
            break;

         default:
            Warning(("Invalid physics message type\n"));
            message = strdup("Invalid");
            break;
      }
   }

   // Collision
   sPhysMsg(ObjID objID, int submod, ePhysCollisionType type, ObjID coll_object, 
      int coll_submod, const mxs_vector &normal, mxs_real momentum, const mxs_vector &pt):
      sPhysMsgBase(submod, type, coll_object, coll_submod, normal, momentum, pt)
   {
      to = objID;
      free((void *)message);
      message = strdup("PhysCollision");
   }

   // Contact
   sPhysMsg(ObjID objID, int submod, ePhysContact cd, ePhysContactType type, ObjID contact_object, int contact_submod):
      sPhysMsgBase(submod, cd, type, contact_object, contact_submod)
   {
      static char *contact_names[2] = { "PhysContactCreate", "PhysContactDestroy" };

      to = objID;
      free((void *)message);
      message = strdup(contact_names[cd]);
   }

   // Enter/exit
   sPhysMsg(ObjID objID, ePhysEnterExit type, ObjID trans_object, int trans_submod):
      sPhysMsgBase(type, trans_object, trans_submod)
   {
      static char *enter_exit_names[2] = { "PhysEnter", "PhysExit" };

      to = objID;
      free((void *)message);
      message = strdup(enter_exit_names[type]);
   }

   DECLARE_SCRMSG_PERSISTENT();
};

#ifdef SCRIPT
#define OnPhysFellAsleep()             SCRIPT_MESSAGE_HANDLER_SIGNATURE_(PhysFellAsleep, sPhysMsg)
#define DefaultOnPhysFellAsleep()      SCRIPT_CALL_BASE(PhysFellAsleep)
#define OnPhysWokeUp()                 SCRIPT_MESSAGE_HANDLER_SIGNATURE_(PhysWokeUp, sPhysMsg)
#define DefaultOnPhysWokeUp()          SCRIPT_CALL_BASE(PhysWokeUp)

#define OnPhysMadePhysical()           SCRIPT_MESSAGE_HANDLER_SIGNATURE_(PhysMadePhysical, sPhysMsg)
#define DefaultOnPhysMadePhysical()    SCRIPT_CALL_BASE(PhysMadePhysical)
#define OnPhysMadeNonPhysical()        SCRIPT_MESSAGE_HANDLER_SIGNATURE_(PhysMadeNonPhysical, sPhysMsg)
#define DefaultOnPhysMadeNonPhysical() SCRIPT_CALL_BASE(PhysMadeNonPhysical)

#define OnPhysCollision()              SCRIPT_MESSAGE_HANDLER_SIGNATURE_(PhysCollision, sPhysMsg)
#define DefaultOnPhysCollision()       SCRIPT_CALL_BASE(PhysCollision)
#define OnPhysContactCreate()          SCRIPT_MESSAGE_HANDLER_SIGNATURE_(PhysContactCreate, sPhysMsg)
#define DefaultOnPhysContactCreate()   SCRIPT_CALL_BASE(PhysContactCreate)
#define OnPhysContactDestroy()         SCRIPT_MESSAGE_HANDLER_SIGNATURE_(PhysContactDestroy, sPhysMsg)
#define DefaultOnPhysContactDestroy()  SCRIPT_CALL_BASE(PhysContactDestroy)
#define OnPhysEnter()                  SCRIPT_MESSAGE_HANDLER_SIGNATURE_(PhysEnter, sPhysMsg)
#define DefaultOnPhysEnter()           SCRIPT_CALL_BASE(PhysEnter)
#define OnPhysExit()                   SCRIPT_MESSAGE_HANDLER_SIGNATURE_(PhysExit, sPhysMsg)
#define DefaultOnPhysExit()            SCRIPT_CALL_BASE(PhysExit)
#endif


////////////////////////////////////////
//
// Default physics script
//

#ifdef SCRIPT

BEGIN_SCRIPT(Physics, RootScript)

   MESSAGE_DEFS:

      SCRIPT_DEFINE_MESSAGE_DEFAULT_(PhysFellAsleep,     sPhysMsg)
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(PhysWokeUp,         sPhysMsg)
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(PhysMadePhysical,   sPhysMsg)
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(PhysMadeNonPhysical,sPhysMsg)
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(PhysCollision,      sPhysMsg)
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(PhysContactCreate,  sPhysMsg)
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(PhysContactDestroy, sPhysMsg)
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(PhysEnter,          sPhysMsg)
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(PhysExit,           sPhysMsg)
   
      SCRIPT_BEGIN_MESSAGE_MAP()
         SCRIPT_MSG_MAP_ENTRY(PhysFellAsleep)
         SCRIPT_MSG_MAP_ENTRY(PhysWokeUp)
         SCRIPT_MSG_MAP_ENTRY(PhysMadePhysical)
         SCRIPT_MSG_MAP_ENTRY(PhysMadeNonPhysical)
         SCRIPT_MSG_MAP_ENTRY(PhysCollision)
         SCRIPT_MSG_MAP_ENTRY(PhysContactCreate)
         SCRIPT_MSG_MAP_ENTRY(PhysContactDestroy)
         SCRIPT_MSG_MAP_ENTRY(PhysEnter)
         SCRIPT_MSG_MAP_ENTRY(PhysExit)
      SCRIPT_END_MESSAGE_MAP()

END_SCRIPT(Physics)

#endif

#endif // __PHSCRPT_H   

