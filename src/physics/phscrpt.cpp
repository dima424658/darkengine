/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phscrpt.cpp,v 1.14 1999/12/10 14:45:17 PATMAC Exp $
//

#include <lg.h>

#include <scrptapi.h>
#include <scrptbas.h>
#include <scrptsrv.h>

#include <objpos.h>

#include <phcore.h>
#include <phmoapi.h>
#include <phmods.h>
#include <phmod.h>
#include <physapi.h>
#include <phcontct.h>

#include <prjctile.h>

#include <subscrip.h>
#include <phscrpt.h>

#include <netmsg.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////
//
// NETWORKING CODE
//

//////////
// 
// ControlPosition
//
// @HACK: In principle, these probably ought to be networked at a lower
// level, probably in physapi/phnet. But in practice, we're pretty close
// to shipping Shock, and there is great peril in adding so dramatic an
// enhancement at this point. In particular, networking the controls
// naively could lead to a massive bandwidth blowup. So for now, we're
// going to network just the high level that we need, and do the low
// level later, when we can study its effects.
//
// Note that we assume that prior heartbeats have set the object at a
// resting position.
//
static cNetMsg *g_pControlPositionMsg = NULL;

static void handleControlPosition(ObjID obj)
{
   PhysControlLocation(obj, &(ObjPosGet(obj)->loc.vec));
   PhysControlRotation(obj, &(ObjPosGet(obj)->fac));
}

static sNetMsgDesc sControlPositionDesc =
{
   kNMF_Broadcast,
   "ContPos",
   "Phys Script Control Position",
   NULL,
   handleControlPosition,
   {{kNMPT_SenderObjID, kNMPF_IfHostedHere, "Obj"},
    {kNMPT_End}}
};

////////////////////////////////////////
//
// Physics service implementation
//

DECLARE_SCRIPT_SERVICE_IMPL(cPhysSrv, Physics)
{
public:
   STDMETHOD_(void, Init)()
   {
      g_pControlPositionMsg = new cNetMsg(&sControlPositionDesc);
   }

   STDMETHOD_(void, End)()
   {
      delete g_pControlPositionMsg;
   }

   // Subsribe an object to message types
   STDMETHOD(SubscribeMsg)(object phys_obj, int message_types)
   {
      Assert_(pPhysSubscribeService);
      pPhysSubscribeService->Subscribe(phys_obj, message_types);

      return TRUE;
   }

   // Unsubscribe an object to message types
   STDMETHOD(UnsubscribeMsg)(object phys_obj, int message_types) 
   {
      Assert_(pPhysSubscribeService);
      pPhysSubscribeService->Unsubscribe(phys_obj, message_types);

      return TRUE;
   }

   STDMETHOD_(object,LaunchProjectile)(object launcher, object proj, real power, integer flags, const vector ref add_vel) 
   {
      return launchProjectile(launcher,proj,power,flags,&add_vel,NULL,NULL); 
   }

   STDMETHOD(SetVelocity)(object obj, const vector ref vel)
   {
      // Setting velocities from scripts wants to be initially clear from the
      // effects of friction, so we break all contacts.  
      cPhysModel *pModel = g_PhysModels.GetActive(obj);
      if (pModel)
      {
         for (int i=0; i<pModel->NumSubModels(); i++)
         {
            pModel->DestroyAllTerrainContacts(i);
            DestroyAllObjectContacts(obj, i, pModel);
         }
      }
      PhysSetVelocity(obj, (mxs_vector*)&vel);
      return S_OK;
   }
   STDMETHOD(GetVelocity)(object obj, vector ref vel)
   {
      PhysGetVelocity(obj, &vel);
      return S_OK;
   }

   STDMETHOD(SetGravity)(object obj, float gravity)
   {
      PhysSetGravity(obj, gravity);
      return S_OK;
   }
   STDMETHOD_(float, GetGravity)(object obj)
   {
      return PhysGetGravity(obj);
   }

   STDMETHOD_(BOOL, HasPhysics)(object obj)
   {
      return PhysObjHasPhysics(obj);
   }

   STDMETHOD_(BOOL, IsSphere)(object obj)
   {
      cPhysModel *pModel;

      if ((pModel = g_PhysModels.Get(obj)) != NULL)
      {
         ePhysModelType type = pModel->GetType(0);
         return ((type == kPMT_Sphere) || (type == kPMT_Point) || (type == kPMT_SphereHat));
      }
      return FALSE;
   }

   STDMETHOD_(BOOL, IsOBB)(object obj)
   {
      cPhysModel *pModel;

      if ((pModel = g_PhysModels.Get(obj)) != NULL)
      {
         ePhysModelType type = pModel->GetType(0);
         return (type == kPMT_OBB);
      }
      return FALSE;
   }

   STDMETHOD(ControlCurrentLocation)(object obj)
   {
      if (ObjID(obj) != OBJ_NULL)
      {
         PhysControlLocation(obj, &(ObjPosGet(obj)->loc.vec));
         return S_OK;
      }
      return E_FAIL;
   }

   STDMETHOD(ControlCurrentRotation)(object obj)
   {
      if (ObjID(obj) != OBJ_NULL)
      {
         PhysControlRotation(obj, &(ObjPosGet(obj)->fac));
         return S_OK;
      }
      return E_FAIL;
   }

   STDMETHOD(ControlCurrentPosition)(object obj)
   {
      if (ObjID(obj) != OBJ_NULL)
      {
         ControlCurrentLocation(obj);
         ControlCurrentRotation(obj);
         g_pControlPositionMsg->Send(OBJ_NULL, obj);
         return S_OK;
      }
      return E_FAIL;
   }

   STDMETHOD(ControlVelocity)(object obj, const vector ref vel)
   {
      PhysControlVelocity(obj, (mxs_vector*)&vel);
      return S_OK;
   }

   STDMETHOD(StopControlVelocity)(object obj)
   {
      PhysStopControlVelocity(obj);
      return S_OK;
   }

   STDMETHOD(DeregisterModel)(object obj)
   {
      if (ObjID(obj) != OBJ_NULL)
      {
         PhysDeregisterModel(obj);
         return S_OK;
      }
      return E_FAIL;
   }

   STDMETHOD_(void, PlayerMotionSetOffset)(int subModel, vector ref offset)
   {
      ::PlayerMotionSetOffset(subModel, &offset);         
   }

   STDMETHOD(Activate)(const object obj)
   {
      cPhysModel *pModel;

      if ((pModel = g_PhysModels.Get(ObjID(obj))) != NULL)
      {
         pModel->Activate();
         return S_OK;
      }
      return E_FAIL;
   }

   STDMETHOD_(BOOL, ValidPos)(const object obj)
   {
      return PhysObjValidPos(ObjID(obj), NULL);
   }
};

IMPLEMENT_SCRIPT_SERVICE_IMPL(cPhysSrv, Physics);

////////////////////////////////////////
//
// Physics message implementation
//

IMPLEMENT_SCRMSG_PERSISTENT(sPhysMsg)
{
   PersistenceHeader(sPhysMsg, kPhysMsgVer);
   return TRUE;
}
