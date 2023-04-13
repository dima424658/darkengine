/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phmod.cpp,v 1.154 2000/03/20 09:45:23 adurant Exp $
//
// Model-specific code
//

//#define PROFILE_ON

#include <lg.h>
#include <config.h>
#include <allocapi.h>
#include <matrixs.h>
#include <timings.h>
#include <float.h>

#include <wrtype.h>
#include <objpos.h>
#include <sphrcst.h>
#include <creatext.h>
#include <doorprop.h>
#include <prjctile.h>
#include <port.h>
#include <plyrspd.h>

#include <playrobj.h>
#include <plyrmode.h>
#include <netman.h>
#include <iobjnet.h>

// for squishing
#include <stimtype.h>
#include <stimbase.h>
#include <stimsens.h>
#include <stimul8r.h>
#include <simtime.h>

#include <autolink.h>
#include <lnkquery.h>
#include <linkbase.h>

#include <aiapi.h>  // for ObjIsAI()
#include <media.h>
#include <objmedia.h>
#include <physapi.h>
#include <phclimb.h>
#include <phconst.h>
#include <phcontct.h>
#include <phcore.h>
#include <phmod.h>
#include <phmods.h>
#include <phdyn.h>
#include <phctrl.h>
#include <phclsn.h>
#include <phref.h>
#include <phutils.h>
#include <phmoapi.h>
#include <phprop.h>
#include <phoprop.h>
#include <phmtprop.h>
#include <phnet.h>

#include <phmsg.h>
#include <phscrt.h>

#include <iobjsys.h>
#include <appagg.h>

// For disk check:
#include <aipthobb.h>
#include <aiprcore.h>


#include <hshsttem.h>


// Must be last header
#include <dbmem.h>

extern BOOL gInsideMT;

mxs_real kSpringCapMag = 25.0; //extern deffed in phconst

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysModelTable
//

#if defined(_MSC_VER)
template cPhysModelTableBase;
#endif

tHashSetKey cPhysModelTable::GetKey(tHashSetNode node) const
{
   return (tHashSetKey)((((cPhysModel *)(node))->m_objID));
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysModel
//

cPhysModel::cPhysModel(ObjID objID, ePhysModelType type,
                       tPhysSubModId numSubModels, unsigned flags)
 : m_objID(objID),
   m_nSubModels(numSubModels),
   m_flags(0),
   m_base_friction(0),
   m_nObjectContacts(0),
   m_nFaceContacts(0),
   m_nEdgeContacts(0),
   m_nVertexContacts(0),
   m_pointVsTerrain(FALSE),
   m_passThruDT(0),
   bDiskCheck(TRUE)
{
   ILinkQuery *query;
   int i;

   // Sphere-hats always have 2 submodels
   if (type == kPMT_SphereHat)
      m_nSubModels = 2;

   // Init the flags
   if (flags & kPMCF_LowDetail)
      m_flags |= kPMF_LowDetail;
   if (flags & kPMCF_Remote)
      m_flags |= kPMF_Remote;
   if (flags & kPMCF_NonMoveable)
      m_flags |= kPMF_NonMoveable;
   if (flags & kPMCF_Uncontrollable)
      m_flags |= kPMF_Uncontrollable;
   if (flags & kPMCF_Player)
      m_flags |= kPMF_Player;
   if (flags & kPMCF_Creature)
      m_flags |= kPMF_Creature;
   if (flags & kPMCF_Rope)
      m_flags |= kPMF_Rope;
   if (flags & kPMCF_Weapon)
      m_flags |= kPMF_Weapon;
   if (flags & kPMF_DiskCheck)
      m_flags |= kPMF_DiskCheck;

   BOOL facevel;
   if (g_pPhysFaceVelProp->Get(objID, &facevel) && facevel)
      m_flags |= kPMF_FacesVel;

   if (g_pMovingTerrainProperty->IsRelevant(objID))
      m_flags |= kPMF_MovingTerrain;

   if (g_pRotDoorProperty->IsRelevant(objID) || g_pTransDoorProperty->IsRelevant(objID))
      m_flags |= kPMF_Door;

   BOOL ai_collides;
   if (g_pPhysAICollideProp->Get(objID, &ai_collides) && ai_collides)
      m_flags |= kPMF_AICollides;

   if (flags & kPMCF_NoGravity)
      m_gravity = 0.0;
   else
      m_gravity = 1.0;

   // Init all submodels to the given type
   m_pType = new ePhysModelType[m_nSubModels];
   for (i=0; i<m_nSubModels; i++)
      m_pType[i] = type;

   m_base_friction = 0.0;

   // Init the spring constant
   LGALLOC_PUSH_CREDIT();
   m_springInfo.SetSize(m_nSubModels);
   LGALLOC_POP_CREDIT();
   for (i=0; i<m_nSubModels; i++)
   {
      m_springInfo[i].tension = 0.0;
      m_springInfo[i].damping = 0.0;
   }

   // Init overall model position & cog offset
   MakePositionFromVectors(&m_pos.m_position, &(ObjPosGet(objID)->loc.vec), &(ObjPosGet(objID)->fac));
   MakePositionFromVectors(&m_pos.m_endposition, &m_pos.m_position.loc.vec, &m_pos.m_position.fac);
   MakePositionFromVectors(&m_pos.m_targetposition, &m_pos.m_position.loc.vec, &m_pos.m_position.fac);

   mx_zero_vec(&m_cog);

   // Check for PhysAttach links
   query = g_pPhysAttachRelation->Query(objID, LINKOBJ_WILDCARD);
   if (!query->Done())
      m_isAttached = TRUE;
   else
      m_isAttached = FALSE;
   SafeRelease(query);

   query = g_pPhysAttachRelation->Query(LINKOBJ_WILDCARD, objID);
   m_nAttachments = 0;
   for (; !query->Done(); query->Next())
      m_nAttachments++;
   SafeRelease(query);

   m_rotAxes = ZAxis;
   m_restAxes = NoAxes;
   m_atRest = FALSE;

   m_climbingObj = OBJ_NULL;
   m_ropeSegment = -1;
   m_ropePct = 0.0;
   m_mantlingState = 0;
   mx_zero_vec(&m_mantlingTargVec);

   m_referenceFrameObj = OBJ_NULL;

   m_lastSquishTime = -1.0;

   // Allocate force list array for location controlling
   m_pForceList = new cPhysForceList[m_nSubModels];

   // Allocate position lists
   m_pPosition = new cPhysPos[m_nSubModels];

   // Allocate and initialize offsets
   LGALLOC_PUSH_CREDIT();
   m_Offset.SetSize(m_nSubModels);
   LGALLOC_POP_CREDIT();
   for (i=0; i<m_nSubModels; i++)
      mx_zero_vec(&m_Offset[i]);

   // Place all submodels at center of object with a zero facing
   // and init backpointers while we're at it
   for (i=0; i<m_nSubModels; i++)
   {
      MakePositionFromVectors(&m_pPosition[i].m_position, &m_pos.m_position.loc.vec, &m_pos.m_position.fac);
      mx_zero_vec(&m_pPosition[i].m_endposition.loc.vec);
      mx_zero_vec(&m_pPosition[i].m_targetposition.loc.vec);
      mx_mk_angvec(&m_pPosition[i].m_endposition.fac, 0, 0, 0);
      mx_mk_angvec(&m_pPosition[i].m_targetposition.fac, 0, 0, 0);
      m_pPosition[i].m_pModel = this;
   }

   // Initialize submodel constraint lists
   LGALLOC_PUSH_CREDIT();
   m_SubModConstraintList.SetSize(m_nSubModels);
   LGALLOC_POP_CREDIT();

   // Allocate dynamics if moveable
   if (!(flags & kPMCF_NonMoveable) && (type != kPMT_OBB))
      m_pDynamicsData.SetSize(m_nSubModels);
   else
      m_pDynamicsData.SetSize(0);

   // Allocate controls if controllable
   if (!(flags & kPMCF_Uncontrollable) && (type != kPMT_OBB))
      m_pControlData.SetSize(m_nSubModels);
   else
      m_pControlData.SetSize(0);

   // Set up backpointers
   m_DynamicsData.SetModel(this);
   m_DynamicsData.SetSubModId(-1);
   m_ControlData.SetModel(this);
   m_ControlData.SetSubModId(-1);

   for (i=0; i<m_nSubModels && m_pDynamicsData.Size() && m_pControlData.Size(); i++)
   {
      m_pDynamicsData[i].SetModel(this);
      m_pDynamicsData[i].SetSubModId(i);
      m_pControlData[i].SetModel(this);
      m_pControlData[i].SetSubModId(i);
   }

   // Add to model lists
   if (IsActive())
   {
      if (IsMoveable())
         g_PhysModels.AddToMoving(this);
      else
         g_PhysModels.AddToStationary(this);
   }
   else
      // low detail models start inactive by default
      g_PhysModels.AddToInactive(this);

   m_AngLimitList.SetSize(0);

   m_medium = kMS_Invalid;

   if (m_flags & kPMF_Weapon)
      SetSleep(TRUE);
}

///////////////////////////////////////

cPhysModel::cPhysModel(PhysReadWrite func) :
   m_nFaceContacts(0),
   m_nEdgeContacts(0),
   m_nVertexContacts(0),
   m_nObjectContacts(0),
   bDiskCheck(FALSE)
{
   int flag;
   int i;
   tPhysRef dummy;

   AutoAppIPtr_(ObjectSystem,pObjSys);

   func((void *)&m_objID, sizeof(ObjID), 1);
   m_objID = pObjSys->RemapOnLoad(m_objID);
   func((void *)&m_nSubModels, sizeof(int), 1);
   func((void *)&m_flags, sizeof(unsigned), 1);

   BOOL facevel;
   if (g_pPhysFaceVelProp->Get(m_objID, &facevel) && facevel)
      m_flags |= kPMF_FacesVel;

   if (g_pMovingTerrainProperty->IsRelevant(m_objID))
      m_flags |= kPMF_MovingTerrain;

   if (g_pRotDoorProperty->IsRelevant(m_objID) || g_pTransDoorProperty->IsRelevant(m_objID))
      m_flags |= kPMF_Door;

   BOOL ai_collides = FALSE;
   if (g_pPhysAICollideProp->Get(m_objID, &ai_collides) && ai_collides)
      m_flags |= kPMF_AICollides;

   if (g_PhysVersion >= 6)
      func((void *)&m_gravity, sizeof(mxs_real), 1);
   else
      m_gravity = 1.0;

   m_pType = new ePhysModelType[m_nSubModels];
   func((void *)m_pType, sizeof(ePhysModelType), m_nSubModels);

   func((void *)&m_base_friction, sizeof(mxs_real), 1);

   if (g_PhysVersion >= 8)
      func((void *)&m_medium, sizeof(int), 1);
   else
      m_medium = 1;

   if (g_PhysVersion < 26)
      m_medium = kMS_Invalid;

   LGALLOC_PUSH_CREDIT();
   m_springInfo.SetSize(m_nSubModels);
   LGALLOC_POP_CREDIT();
   if (g_PhysVersion >= 11)
   {
      for (i=0; i<m_nSubModels; i++)
      {
         func((void *)&m_springInfo[i].tension, sizeof(mxs_real), 1);
         func((void *)&m_springInfo[i].damping, sizeof(mxs_real), 1);
      }
   }
   else
   {
      for (i=0; i<m_nSubModels; i++)
      {
         m_springInfo[i].tension = 0.0;
         m_springInfo[i].damping = 0.0;
      }
   }

   if (g_PhysVersion >= 13)
      func((void *)&m_pointVsTerrain, sizeof(BOOL), 1);
   else
      m_pointVsTerrain = FALSE;

   if (g_PhysVersion >= 22)
      func((void *)&m_passThruDT, sizeof(BOOL), 1);
   else
      m_passThruDT = 0;

   if (g_PhysVersion >= 14)
   {
      func((void *)&m_nAttachments, sizeof(int), 1);
      func((void *)&m_isAttached, sizeof(BOOL), 1);
   }
   else
   {
      m_nAttachments = 0;
      m_isAttached = FALSE;
   }

   if (g_PhysVersion >= 15)
   {
      func((void *)&m_rotAxes, sizeof(ePhysAxes), 1);
      func((void *)&m_restAxes, sizeof(ePhysAxes), 1);
   }
   else
   {
      m_rotAxes = ZAxis;
      m_restAxes = NoAxes;
   }
   m_atRest = FALSE;

   if (g_PhysVersion >= 18)
      func((void *)&m_climbingObj, sizeof(ObjID), 1);
   else
      m_climbingObj = OBJ_NULL;

   if (g_PhysVersion >= 21)
   {
      func((void *)&m_ropeSegment, sizeof(int), 1);
      func((void *)&m_ropePct, sizeof(mxs_real), 1);
   }
   else
   {
      m_ropeSegment = -1;
      m_ropePct = 0.0;
   }

   if (g_PhysVersion >= 19)
   {
      func((void *)&m_mantlingState, sizeof(int), 1);
      func((void *)&m_mantlingTargVec, sizeof(mxs_vector), 1);
   }
   else
   {
      m_mantlingState = 0;
      mx_zero_vec(&m_mantlingTargVec);
   }

   if (g_PhysVersion >= 31)
      func((void *)&m_referenceFrameObj, sizeof(ObjID), 1);
   else
      m_referenceFrameObj = NULL;

   if (g_PhysVersion >= 25)
      func((void *)&m_lastSquishTime, sizeof(long), 1);
   else
      m_lastSquishTime = -1.0;

   m_pForceList = new cPhysForceList[m_nSubModels];

   // Allocate and Read positions for each submodel
   m_pPosition = new cPhysPos[m_nSubModels];
   if (g_PhysVersion >= 9)
      func(m_pPosition, sizeof(cPhysPos), m_nSubModels);
   else
   {
      for (i=0; i<m_nSubModels; i++)
      {
         func(&m_pPosition[i].m_position, sizeof(Position), 1);
         func(&m_pPosition[i].m_endposition, sizeof(Position), 1);
         func(&dummy, sizeof(tPhysRef), 1);
         func(&m_pPosition[i].m_pModel, sizeof(cPhysModel *), 1);
      }
   }

   if (g_PhysVersion >= 9)
      func((void *)&m_pos, sizeof(cPhysPos), 1);
   else
   {
      func(&m_pos.m_position, sizeof(Position), 1);
      func(&m_pos.m_endposition, sizeof(Position), 1);
      func(&dummy, sizeof(tPhysRef), 1);
      func(&m_pos.m_pModel, sizeof(cPhysModel *), 1);
   }

   // Update pointers
   for (i=0; i<m_nSubModels; i++)
      m_pPosition[i].m_pModel = this;
   m_pos.m_pModel = this;

   // Read center-of-gravity
   func((void *)&m_cog, sizeof(mxs_vector), 1);

   // Read submodel offsets
   LGALLOC_PUSH_CREDIT();
   m_Offset.SetSize(m_nSubModels);
   LGALLOC_POP_CREDIT();
   if (g_PhysVersion >= 10)
   {
      for (i=0; i<m_nSubModels; i++)
         func((void *)&m_Offset[i], sizeof(mxs_vector), 1);
   }
   else
   {
      for (i=0; i<m_nSubModels; i++)
         mx_sub_vec(&m_Offset[i], &m_pPosition[i].m_position.loc.vec, &m_pos.m_position.loc.vec);
   }

   // Initialize any pressure plates that are currently in-transit
   if ((g_PhysVersion >= 29) && IsPressurePlate())
   {
      int state;

      func((void *)&state, sizeof(int), 1);

      if ((state == kPPS_Activating) ||
          (state == kPPS_Deactivating))
      {
         mxs_vector limit;

         func((void *)&limit, sizeof(mxs_vector), 1);

         ClearTransLimits();
         AddTransLimit(limit, PressurePlateCallback);
      }
   }

   // Initialize submodel constraint lists
   LGALLOC_PUSH_CREDIT();
   m_SubModConstraintList.SetSize(m_nSubModels);
   LGALLOC_POP_CREDIT();

   // Allocate and Read dynamics and control info, if applicable
   func(&flag, sizeof(int), 1);
   if (flag == 0x0001)
   {
      if (g_PhysVersion <= 11)
         m_DynamicsData.LoadV11(func);
      else
         func(&m_DynamicsData, sizeof(cPhysDynData), 1);
      m_pDynamicsData.SetSize(0);
   }
   else
   if (flag == 0x0002)
   {
      if (g_PhysVersion >= 7)
      {
         if (g_PhysVersion <= 11)
            m_DynamicsData.LoadV11(func);
         else
            func(&m_DynamicsData, sizeof(cPhysDynData), 1);
      }
      if (g_PhysVersion <= 11)
      {
         for (i=0; i<m_nSubModels; i++)
            m_pDynamicsData[i].LoadV11(func);
      }
      else
      {
         if (g_PhysVersion >= 28)
         {
            int size;

            func(&size, sizeof(int), 1);
            m_pDynamicsData.SetSize(size);

            for (i=0; i<size; i++)
               func(&m_pDynamicsData[i], sizeof(cPhysDynData), 1);
         }
         else
         {
            if (m_pType[0] == kPMT_OBB)
            {
               cPhysDynData dummy;

               m_pDynamicsData.SetSize(0);
               for (i=0; i<m_nSubModels; i++)
                  func(&dummy, sizeof(cPhysDynData), 1);
            }
            else
            {
               m_pDynamicsData.SetSize(m_nSubModels);
               for (i=0; i<m_nSubModels; i++)
                  func(&m_pDynamicsData[i], sizeof(cPhysDynData), 1);
            }
         }
      }
   }

   func(&flag, sizeof(int), 1);
   if (flag == 0x0001)
   {
      func(&m_ControlData, sizeof(cPhysCtrlData), 1);
      m_pControlData.SetSize(0);
   }
   else
   if (flag == 0x0002)
   {
      if (g_PhysVersion >= 7)
         func(&m_ControlData, sizeof(cPhysCtrlData), 1);
      if (g_PhysVersion >= 28)
      {
         int size;

         func(&size, sizeof(int), 1);
         m_pControlData.SetSize(size);

         for(i=0; i<size; i++)
            func(&m_pControlData[i], sizeof(cPhysCtrlData), 1);
      }
      else
      {
         if (m_pType[0] == kPMT_OBB)
         {
            cPhysCtrlData dummy;

            m_pControlData.SetSize(0);
            for (i=0; i<m_nSubModels; i++)
               func(&dummy, sizeof(cPhysCtrlData), 1);
         }
         else
         {
            m_pControlData.SetSize(m_nSubModels);
            for (i=0; i<m_nSubModels; i++)
               func(&m_pControlData[i], sizeof(cPhysCtrlData), 1);
         }
      }
   }

   // Set up backpointers
   m_DynamicsData.SetModel(this);
   m_DynamicsData.SetSubModId(-1);
   m_ControlData.SetModel(this);
   m_ControlData.SetSubModId(-1);

   for (i=0; i<m_pDynamicsData.Size(); i++)
   {
      m_pDynamicsData[i].SetModel(this);
      m_pDynamicsData[i].SetSubModId(i);
   }

   for (i=0; i<m_pControlData.Size(); i++)
   {
      m_pControlData[i].SetModel(this);
      m_pControlData[i].SetSubModId(i);
   }

   m_AngLimitList.SetSize(0);

#ifdef SHIP
#define DO_SUBMOD_ROT_FIXUP (TRUE)
#else
#define DO_SUBMOD_ROT_FIXUP (!config_is_defined("no_submod_rot_fixup"))
#endif

   if (DO_SUBMOD_ROT_FIXUP)
   {
      // @HACK: if we have rest axes and submodel offsest, make sure we're
      // rotation controlled
      BOOL rot_ctrl = FALSE;
      if (GetRestAxes() != NoAxes)
      {
         for (i=0; i<NumSubModels(); i++)
         {
            if (mx_mag2_vec(&GetSubModOffset(i)) > 0.0)
            {
               rot_ctrl = TRUE;
               break;
            }
         }
      }

      if (rot_ctrl)
      {
         cPhysCtrlData *pControls = GetControls();
         if (pControls)
            pControls->ControlRotation(GetRotation());
      }
   }
}



void cPhysModel::UpdateDiskInfo()
{
   bDiskCheck = TRUE;
   // If this is an AI, or a pathable object, then set "disk" bit
   if (g_pObjPathableProperty->IsRelevant(m_objID) ||
       g_pAIProperty->IsRelevant(m_objID))
      m_flags |= kPMF_DiskCheck;
}


///////////////////////////////////////

cPhysModel::~cPhysModel()
{
   int i;

   delete[] m_pType;
   delete[] m_pForceList;
   delete[] m_pPosition;

   m_pDynamicsData.SetSize(0);
   m_pControlData.SetSize(0);

   for (i=0; i<m_AngLimitList.Size(); i++)
      delete m_AngLimitList[i];
   m_AngLimitList.SetSize(0);

   for (i=0; i<m_TransLimitList.Size(); i++)
      delete m_TransLimitList[i];
   m_TransLimitList.SetSize(0);
}

///////////////////////////////////////

void cPhysModel::Write(PhysReadWrite func) const
{
   int i;
   int flag;

   // Write General info
   func((void *)&m_objID, sizeof(ObjID), 1);
   func((void *)&m_nSubModels, sizeof(int), 1);
   func((void *)&m_flags, sizeof(unsigned), 1);

   func((void *)&m_gravity, sizeof(mxs_real), 1);

   func((void *)m_pType, sizeof(ePhysModelType), m_nSubModels);

   func((void *)&m_base_friction, sizeof(mxs_real), 1);

   func((void *)&m_medium, sizeof(int), 1);

   for (i=0; i<m_nSubModels; i++)
   {
      func((void *)&m_springInfo[i].tension, sizeof(mxs_real), 1);
      func((void *)&m_springInfo[i].damping, sizeof(mxs_real), 1);
   }

   func((void *)&m_pointVsTerrain, sizeof(BOOL), 1);
   func((void *)&m_passThruDT, sizeof(BOOL), 1);

   func((void *)&m_nAttachments, sizeof(int), 1);
   func((void *)&m_isAttached, sizeof(BOOL), 1);

   func((void *)&m_rotAxes, sizeof(ePhysAxes), 1);
   func((void *)&m_restAxes, sizeof(ePhysAxes), 1);

   func((void *)&m_climbingObj, sizeof(ObjID), 1);
   func((void *)&m_ropeSegment, sizeof(int), 1);
   func((void *)&m_ropePct, sizeof(mxs_real), 1);
   func((void *)&m_mantlingState, sizeof(int), 1);
   func((void *)&m_mantlingTargVec, sizeof(mxs_vector), 1);

   func((void *)&m_referenceFrameObj, sizeof(ObjID), 1);

   func((void *)&m_lastSquishTime, sizeof(long), 1);

   // Write positions for each submodel
   func(m_pPosition, sizeof(cPhysPos), m_nSubModels);

   // Write overall model position & cog offset
   func((void *)&m_pos, sizeof(cPhysPos), 1);
   func((void *)&m_cog, sizeof(mxs_vector), 1);

   for (i=0; i<m_nSubModels; i++)
      func((void *)&m_Offset[i], sizeof(mxs_vector), 1);

   if (IsPressurePlate())
   {
      int state = GetPPlateState();

      func((void *)&state, sizeof(int), 1);

      if ((state == kPPS_Activating) ||
          (state == kPPS_Deactivating))
      {
         mxs_vector trans_limit;
         int        trans_limit_size;

         GetTransLimits(&trans_limit, &trans_limit_size, 1);

         func((void *)&trans_limit, sizeof(mxs_vector), 1);
      }
   }

   // Write dynamics and control info, if applicable
   if (IsMoveable())
   {
      if (m_pDynamicsData.Size())
      {
         int size = m_pDynamicsData.Size();

         flag = 0x0002;
         func(&flag, sizeof(int), 1);
         func((void *)&m_DynamicsData, sizeof(cPhysDynData), 1);
         func(&size, sizeof(int), 1);
         for (i=0; i<size; i++)
            func((void *)&m_pDynamicsData[i], sizeof(cPhysDynData), 1);
      }
      else
      {
         flag = 0x0001;
         func(&flag, sizeof(int), 1);
         func((void *)&m_DynamicsData, sizeof(cPhysDynData), 1);
      }
   }
   else
   {
      flag = 0x0000;
      func(&flag, sizeof(int), 1);
   }

   if (IsControllable())
   {
      if (m_pControlData.Size())
      {
         int size = m_pControlData.Size();

         flag = 0x0002;
         func(&flag, sizeof(int), 1);
         func((void *)&m_ControlData, sizeof(cPhysCtrlData), 1);
         func(&size, sizeof(int), 1);
         for (i=0; i<size; i++)
            func((void *)&m_pControlData[i], sizeof(cPhysCtrlData), 1);
      }
      else
      {
         flag = 0x0001;
         func(&flag, sizeof(int), 1);
         func((void *)&m_ControlData, sizeof(cPhysCtrlData), 1);
      }
   }
   else
   {
      flag = 0x0000;
      func(&flag, sizeof(int), 1);
   }
}

///////////////////////////////////////

void cPhysModel::SetLocationVec(tPhysSubModId subModId, const mxs_vector & locVec)
{
   Assert_(!_isnan(locVec.x) && !_isnan(locVec.y) && !_isnan(locVec.z));
   AssertSubModIdInRange(subModId);
   m_pPosition[subModId].m_position.loc.vec = locVec;
   m_pPosition[subModId].m_targetposition.loc.vec = locVec;
   m_pPosition[subModId].m_endposition.loc.vec = locVec;
   UpdateChangedLocation(&(m_pPosition[subModId].m_position.loc));
}

///////////////////////////////////////

void cPhysModel::SetCurrentLocationVec(tPhysSubModId subModId, const mxs_vector & locVec)
{
   Assert_(!_isnan(locVec.x) && !_isnan(locVec.y) && !_isnan(locVec.z));
   AssertSubModIdInRange(subModId);
   m_pPosition[subModId].m_position.loc.vec = locVec;
   UpdateChangedLocation(&(m_pPosition[subModId].m_position.loc));
}

///////////////////////////////////////

void cPhysModel::SetLocationVec(const mxs_vector & locVec, BOOL update)
{
   Assert_(!_isnan(locVec.x) && !_isnan(locVec.y) && !_isnan(locVec.z));
   m_pos.m_position.loc.vec = locVec;
   m_pos.m_targetposition.loc.vec = locVec;
   m_pos.m_endposition.loc.vec = locVec;
   UpdateChangedLocation(&m_pos.m_position.loc);

   if (update)
      ObjTranslate(GetObjID(), &locVec);
}

///////////////////////////////////////

void cPhysModel::SetCurrentLocationVec(const mxs_vector & locVec, BOOL update)
{
   Assert_(!_isnan(locVec.x) && !_isnan(locVec.y) && !_isnan(locVec.z));
   m_pos.m_position.loc.vec = locVec;
   UpdateChangedLocation(&m_pos.m_position.loc);

   if (update)
      ObjTranslate(GetObjID(), &locVec);
}

///////////////////////////////////////

void cPhysModel::SetRotation(const mxs_angvec & rot, BOOL update)
{
   m_pos.m_position.fac = m_pos.m_targetposition.fac = m_pos.m_endposition.fac = rot;

   if (update)
     ObjRotate(GetObjID(), &rot);
}

///////////////////////////////////////

const mxs_vector cPhysModel::GetCOG() const
{
   mxs_vector cog;
   mxs_matrix orien;

   mx_ang2mat(&orien, &m_pos.m_position.fac);
   mx_mat_mul_vec(&cog, &orien, &m_cog);
   mx_addeq_vec(&cog, &m_pos.m_position.loc.vec);

   return cog;
}

///////////////////////////////////////

static BOOL AttachConstrainLock = FALSE;

void cPhysModel::AddConstraint(ObjID cause, const mxs_vector &dir, mxs_real mag)
{
   if (mag == 0)
      return;

   for (int i=0; i<m_VelConstraintList.Size(); i++)
   {
      if (EqualVectors(m_VelConstraintList[i].dir, dir))
         return;
   }

   tVelocityConstraint vel_const;

   vel_const.cause = cause;
   vel_const.dir = dir;
   vel_const.mag = mag;

   LGALLOC_PUSH_CREDIT();
   m_VelConstraintList.Append(vel_const);
   LGALLOC_POP_CREDIT();

   // And constrain any attachments
   if (AttachConstrainLock == TRUE)
      return;

   if (IsAttached())
   {
      ILinkQuery *query = g_pPhysAttachRelation->Query(ObjID(), LINKOBJ_WILDCARD);

      if (!query->Done())
      {
         sLink link;
         query->Link(&link);

         cPhysModel *pModelDest = g_PhysModels.GetActive(link.dest);
         if (pModelDest != NULL)
         {
            AttachConstrainLock = TRUE;
            pModelDest->AddConstraint(cause, dir, mag);
            AttachConstrainLock = FALSE;
         }
      }
      SafeRelease(query);
   }
}

////////////////////////////////////////

void cPhysModel::AddConstraint(ObjID cause, const mxs_vector &norm)
{
   for (int i=0; i<m_ConstraintList.Size(); i++)
   {
      if (EqualVectors(m_ConstraintList[i].dir, norm))
         return;
   }

   tConstraint constraint;

   constraint.cause = cause;
   constraint.dir = norm;

   LGALLOC_PUSH_CREDIT();
   m_ConstraintList.Append(constraint);
   LGALLOC_POP_CREDIT();

   // And constrain any attachments
   if (AttachConstrainLock == TRUE)
      return;

   if (IsAttached())
   {
      ILinkQuery *query = g_pPhysAttachRelation->Query(ObjID(), LINKOBJ_WILDCARD);

      if (!query->Done())
      {
         sLink link;
         query->Link(&link);

         cPhysModel *pModelDest = g_PhysModels.GetActive(link.dest);
         if (pModelDest != NULL)
         {
            AttachConstrainLock = TRUE;
            pModelDest->AddConstraint(cause, norm);
            AttachConstrainLock = FALSE;
         }
      }
      SafeRelease(query);
   }
}

///////////////////////////////////////

void cPhysModel::AddConstraint(ObjID cause, tPhysSubModId i, const mxs_vector &norm)
{
   BOOL in_model = FALSE;
   BOOL in_submodel = FALSE;
   int  j;

   for (j=0; j<m_ConstraintList.Size(); j++)
   {
      if (EqualVectors(m_ConstraintList[j].dir, norm))
      {
         in_model = TRUE;
         break;
      }
   }

   for (j=0; j<m_SubModConstraintList[i].Size(); j++)
   {
      if (EqualVectors(m_SubModConstraintList[i][j].dir, norm))
      {
         in_submodel = TRUE;
         break;
      }
   }

   if (in_model && in_submodel)
      return;

   tConstraint constraint;

   constraint.cause = cause;
   constraint.dir = norm;

   LGALLOC_PUSH_CREDIT();
   m_ConstraintList.Append(constraint);
   m_SubModConstraintList[i].Append(constraint);
   LGALLOC_POP_CREDIT();

   // And constrain any attachments
   if (AttachConstrainLock == TRUE)
      return;

   if (IsAttached())
   {
      ILinkQuery *query = g_pPhysAttachRelation->Query(ObjID(), LINKOBJ_WILDCARD);

      if (!query->Done())
      {
         sLink link;
         query->Link(&link);

         cPhysModel *pModelDest = g_PhysModels.GetActive(link.dest);
         if (pModelDest != NULL)
         {
            AttachConstrainLock = TRUE;
            pModelDest->AddConstraint(cause, norm);
            AttachConstrainLock = FALSE;
         }
      }
      SafeRelease(query);
   }
}

///////////////////////////////////////

void cPhysModel::ApplyConstraints(mxs_vector *vec)
{
   mxs_vector mid;
   mxs_vector start;

   Assert_(gInsideMT);

   if (IsAttached())
      return;

   mx_copy_vec(&start, vec);

   for (int i=0; i<m_VelConstraintList.Size(); i++)
   {
      mxs_real   dp;
      mxs_vector dir;
      mxs_vector constraint;

      mx_copy_vec(&dir, &m_VelConstraintList[i].dir);

      mx_scale_vec(&constraint, &dir, m_VelConstraintList[i].mag);

      dp = mx_dot_vec(vec, &m_VelConstraintList[i].dir);

      if (dp < m_VelConstraintList[i].mag)
      {
         PhysRemNormComp(vec, dir);
         mx_scaleeq_vec(&dir, -1.0);
         PhysRemNormComp(vec, dir);
         mx_addeq_vec(vec, &constraint);
      }
   }

   mx_copy_vec(&mid, vec);

   if (m_ConstraintList.Size() > 0)
      PhysConstrain(vec, m_ConstraintList, m_ConstraintList.Size());

   if (IsPlayer())
   {
      for (int i=0; i<m_VelConstraintList.Size(); i++)
      {
         if (m_VelConstraintList[i].mag > 0)
         {
            mxs_vector constraint;
            mx_scale_vec(&constraint, &m_VelConstraintList[i].dir, m_VelConstraintList[i].mag);

            if (mx_dot_vec(vec, &constraint) <= 0)
            {
               cPhysModel *pSquishModel = g_PhysModels.Get(m_VelConstraintList[i].cause);

               mxs_real mass = pSquishModel->GetDynamics()->GetMass();
               mxs_real vel  = mx_mag_vec(&pSquishModel->GetDynamics()->GetVelocity());

               Squish((mass * vel) / 100);
               pSquishModel->SetSquishingState(TRUE);

               // Don't move, but retain momentum
               pSquishModel->SetTargetLocation(pSquishModel->GetLocationVec());
               pSquishModel->SetEndLocationVec(pSquishModel->GetLocationVec());
               SetEndLocationVec(GetLocationVec());
            }
         }
      }
   }
}

///////////////////////////////////////

void cPhysModel::ApplyConstraints(tPhysSubModId i, mxs_vector *vec)
{
   if (m_SubModConstraintList[i].Size() > 0)
      PhysConstrain(vec, m_SubModConstraintList[i], m_SubModConstraintList[i].Size());
}

///////////////////////////////////////

void cPhysModel::GetConstraints(mxs_vector vec_list[], int *num_vecs)
{
   int i;

   for (i=0; (i<m_ConstraintList.Size()) && (i<*num_vecs); i++)
      mx_copy_vec(&vec_list[i], &m_ConstraintList[i].dir);

   *num_vecs = i;
}

///////////////////////////////////////

BOOL cPhysModel::ApplyForces(tPhysSubModId subModId, sPhysForce *flist, int nforces, mxs_real dt,
                             mxs_vector &start_loc, mxs_vector *end_loc, mxs_vector *force,
                             mxs_real *ctime, BOOL terrain_check, BOOL object_check)
{
   mx_copy_vec(end_loc, (mxs_vector *)&GetEndLocationVec(subModId));
   mx_zero_vec(force);
   *ctime = 0.0;
   return FALSE;
}

///////////////////////////////////////

BOOL cPhysModel::IsControlled()
{
   if (!IsControllable())
      return FALSE;

   if (m_ControlData.GetType() != kPCT_NoControl)
      return TRUE;

   for (int i=0; i<m_pControlData.Size(); i++)
   {
      if (m_pControlData[i].GetType() != kPCT_NoControl)
         return TRUE;
   }

   return FALSE;
}

///////////////////////////////////////

BOOL cPhysModel::IsVelocityControlled()
{
   if (!IsControllable())
      return FALSE;

   if (m_ControlData.GetType() & kPCT_VelControl)
      return TRUE;

   for (int i=0; i<m_pControlData.Size(); i++)
   {
      if (m_pControlData[i].GetType() & kPCT_VelControl)
         return TRUE;
   }

   return FALSE;
}

///////////////////////////////////////

BOOL cPhysModel::IsAxisVelocityControlled()
{
   if (!IsControllable())
      return FALSE;

   if ((m_ControlData.GetType() & kPCT_VelControl) && m_ControlData.AxisControlled())
      return TRUE;

   for (int i=0; i<m_pControlData.Size(); i++)
   {
      if ((m_pControlData[i].GetType() & kPCT_VelControl) && m_pControlData[i].AxisControlled())
         return TRUE;
   }

   return FALSE;
}

///////////////////////////////////////

BOOL cPhysModel::IsRotationalVelocityControlled()
{
   if (!IsControllable())
      return FALSE;

   if (m_ControlData.GetType() & kPCT_RotVelControl)
      return TRUE;

   if (!IsPlayer())
   {
      for (int i=0; i<m_pControlData.Size(); i++)
      {
         if (m_pControlData[i].GetType() & kPCT_RotVelControl)
            return TRUE;
      }
   }

   return FALSE;
}

///////////////////////////////////////

BOOL cPhysModel::IsLocationControlled()
{
   if (!IsControllable())
      return FALSE;

   if (m_ControlData.GetType() & kPCT_LocControl)
      return TRUE;

   for (int i=0; i<m_pControlData.Size(); i++)
   {
      if (m_pControlData[i].GetType() & kPCT_LocControl)
         return TRUE;
   }

   return FALSE;
}

///////////////////////////////////////

BOOL cPhysModel::IsRotationControlled()
{
   if (!IsControllable())
      return FALSE;

   if (m_ControlData.GetType() & kPCT_RotControl)
      return TRUE;

   for (int i=0; i<m_pControlData.Size(); i++)
   {
      if (m_pControlData[i].GetType() & kPCT_RotControl)
         return TRUE;
   }

   return FALSE;
}

///////////////////////////////////////

void cPhysModel::SetSleep(BOOL state)
{
   if (IsCreature() && !IsRope() && CreatureSelfPropelled(GetObjID()) && (state == FALSE))
      return;

   if (IsWeapon(0) && (state == FALSE))
      return;

#ifdef NEW_NETWORK_ENABLED
   // Tell the other players that this object is or isn't moving now
   // We put this call here, because the networking code doesn't want to
   // ignore the special cases below...
   if (state != IsSleeping())
   {
      AutoAppIPtr(NetManager);
      if (pNetManager->Networking()) {
         AutoAppIPtr(ObjectNetworking);
         // If the object isn't hosted here, then we aren't in charge of
         // telling the rest of the world about its state changes.  Also,
         // AI's base their network sleepiness on their Efficiency mode.
         if (pObjectNetworking->ObjHostedHere(GetObjID())
             && !ObjIsAI(GetObjID()))
         {
            AutoAppIPtr(NetManager);
            PhysNetSetSleep(GetObjID(), state);
         }
      }
   }
#endif

   if (IsPressurePlate() && (GetPPlateState() != kPPS_Inactive) && (state == TRUE))
      return;

   if (ObjectPassThru() && (state == TRUE))
      return;

   if (state != IsSleeping())
   {
      SetFlagState(kPMF_Sleeping, state);

      if (state)
      {
         // We don't want to make non physical if we're in contact with another
         // object, because it could move after we've become non-physical and
         // then we'd just be floating, which isn't so good.
         if (RemovesOnSleep() && !InObjectContact())
         {
            PhysDeregisterModel(GetObjID());
            return;
         }

         // Blow away firer link if we had one
         SetProjectileFirer(GetObjID(), OBJ_NULL);

         g_PhysModels.StopMoving(this);
         PhysMessageFellAsleep(GetObjID());

         SetCollisionChecked(FALSE);
         SetSquishingState(FALSE);

         m_DynamicsData.SetFrozen(FALSE);
         m_DynamicsData.SetCurrentTime(0);

         m_DynamicsData.ZeroCollisionCount();
         m_DynamicsData.ZeroAcceleration();

         for (int i=0; i<m_pDynamicsData.Size(); i++)
         {
            m_pDynamicsData[i].ZeroCollisionCount();
            m_pDynamicsData[i].ZeroAcceleration();
         }
      }
      else
      {
         WakeUpContacts(this);

         SetRest(FALSE);
         g_PhysModels.StartMoving(this);
         PhysMessageWokeUp(GetObjID());
      }
   }
}

///////////////////////////////////////

BOOL cPhysModel::IsTranslating() const
{
   BOOL moving = FALSE;

   if (IsMoveable())
   {
      if (!IsZeroVector(GetDynamics()->GetVelocity()))
         moving = TRUE;

      if (!IsZeroVector(GetDynamics()->GetRotationalVelocity()) &&
          !IsZeroVector(m_cog))
         moving = TRUE;

      for (int i=0; i<NumSubModels(); i++)
      {
         if (!IsZeroVector(GetDynamics(i)->GetVelocity()))
            moving = TRUE;

         if (!IsZeroVector(GetDynamics(i)->GetRotationalVelocity()) &&
             !IsZeroVector(m_cog))
            moving = TRUE;
      }
   }

   return moving;
}

///////////////////////////////////////

void cPhysModel::CreateTerrainContact(tPhysSubModId subModId, const cFacePoly *poly, ObjID objID)
{
   cFaceContactList *pFaceContactList;
   cFaceContact     *pFaceContact;

   if (!GetFaceContacts(subModId, &pFaceContactList) ||
       !pFaceContactList->Find(poly, &pFaceContact))
   {
      m_nFaceContacts++;
      g_PhysContactLinks.CreateTerrainLink(m_objID, subModId, poly, objID);

      PhysMessageContactCreate(this, subModId, kContactFace, OBJ_NULL, -1);
   }
}

void cPhysModel::CreateTerrainContact(tPhysSubModId subModId, const mxs_vector &start, const mxs_vector &end)
{
   cEdgeContactList *pEdgeContactList;
   cEdgeContact     *pEdgeContact;

   if (!GetEdgeContacts(subModId, &pEdgeContactList) ||
       !pEdgeContactList->Find(start, end, &pEdgeContact))
   {
      m_nEdgeContacts++;
      g_PhysContactLinks.CreateTerrainLink(m_objID, subModId, start, end);

      PhysMessageContactCreate(this, subModId, kContactEdge, OBJ_NULL, -1);
   }
}

void cPhysModel::CreateTerrainContact(tPhysSubModId subModId, const mxs_vector &point)
{
   cVertexContactList *pVertexContactList;
   cVertexContact     *pVertexContact;

   if (!GetVertexContacts(subModId, &pVertexContactList) ||
       !pVertexContactList->Find(point, &pVertexContact))
   {
      m_nVertexContacts++;
      g_PhysContactLinks.CreateTerrainLink(m_objID, subModId, point);

      PhysMessageContactCreate(this, subModId, kContactVertex, OBJ_NULL, -1);
   }
}

///////////////////////////////////////

void cPhysModel::DestroyTerrainContact(tPhysSubModId subModId, const cFacePoly *poly)
{
   m_nFaceContacts--;
   g_PhysContactLinks.DestroyTerrainLink(m_objID, subModId, poly);

   PhysMessageContactDestroy(this, subModId, kContactFace, OBJ_NULL, -1);
}

void cPhysModel::DestroyTerrainContact(tPhysSubModId subModId, const mxs_vector &start, const mxs_vector &end)
{
   m_nEdgeContacts--;
   g_PhysContactLinks.DestroyTerrainLink(m_objID, subModId, start, end);

   PhysMessageContactDestroy(this, subModId, kContactEdge, OBJ_NULL, -1);
}

void cPhysModel::DestroyTerrainContact(tPhysSubModId subModId, const mxs_vector &point)
{
   m_nVertexContacts--;
   g_PhysContactLinks.DestroyTerrainLink(m_objID, subModId, point);

   PhysMessageContactDestroy(this, subModId, kContactVertex, OBJ_NULL, -1);
}

///////////////////////////////////////

void cPhysModel::DestroyAllTerrainContacts(tPhysSubModId subModId)
{
   cFaceContactList   *pFaceContactList;
   cEdgeContactList   *pEdgeContactList;
   cVertexContactList *pVertexContactList;

   if (GetFaceContacts(subModId, &pFaceContactList))
   {
      cFaceContact *pFaceContact = pFaceContactList->GetFirst();

      while (pFaceContact != NULL)
      {
         DestroyTerrainContact(subModId, pFaceContact->GetPoly());

         if (!GetFaceContacts(subModId, &pFaceContactList))
            break;
         pFaceContact = pFaceContactList->GetFirst();
      }
   }

   if (GetEdgeContacts(subModId, &pEdgeContactList))
   {
      cEdgeContact *pEdgeContact = pEdgeContactList->GetFirst();

      while (pEdgeContact != NULL)
      {
         DestroyTerrainContact(subModId, pEdgeContact->GetStart(), pEdgeContact->GetEnd());

         if (!GetEdgeContacts(subModId, &pEdgeContactList))
            break;
         pEdgeContact = pEdgeContactList->GetFirst();
      }
   }

   if (GetVertexContacts(subModId, &pVertexContactList))
   {
      cVertexContact *pVertexContact = pVertexContactList->GetFirst();

      while (pVertexContact != NULL)
      {
         DestroyTerrainContact(subModId, pVertexContact->GetPoint());

         if (!GetVertexContacts(subModId, &pVertexContactList))
            break;
         pVertexContact = pVertexContactList->GetFirst();
      }
   }
}
///////////////////////////////////////

BOOL cPhysModel::GetFaceContacts(tPhysSubModId subModId, cFaceContactList **ppFaceContactList) const
{
   if (m_nFaceContacts > 0)
      return g_PhysContactLinks.GetTerrainLinks(m_objID, subModId, ppFaceContactList);

   return FALSE;
}

BOOL cPhysModel::GetEdgeContacts(tPhysSubModId subModId, cEdgeContactList **ppEdgeContactList) const
{
   if (m_nEdgeContacts > 0)
      return g_PhysContactLinks.GetTerrainLinks(m_objID, subModId, ppEdgeContactList);

   return FALSE;
}

BOOL cPhysModel::GetVertexContacts(tPhysSubModId subModId, cVertexContactList **ppVertexContactList) const
{
   if (m_nVertexContacts > 0)
      return g_PhysContactLinks.GetTerrainLinks(m_objID, subModId, ppVertexContactList);

   return FALSE;
}

////////////////////////////////////////////////////////////////////////////////

void cPhysModel::UpdateEndLocation(mxs_real dt)
{
   if (IsSquishing())
   {
      mx_copy_vec(&m_pos.m_endposition.loc.vec, &m_pos.m_position.loc.vec);
      return;
   }

   // Update location
   mx_copy_vec(&m_pos.m_endposition.loc.vec, &m_pos.m_targetposition.loc.vec);

   // Update rotation
   mx_mk_angvec(&m_pos.m_endposition.fac, m_pos.m_targetposition.fac.tx,
                                          m_pos.m_targetposition.fac.ty,
                                          m_pos.m_targetposition.fac.tz);

   if (!EqualVectors(m_pos.m_position.loc.vec, m_pos.m_endposition.loc.vec) ||
       (m_pos.m_position.fac.tx != m_pos.m_endposition.fac.tx) ||
       (m_pos.m_position.fac.ty != m_pos.m_endposition.fac.ty) ||
       (m_pos.m_position.fac.tz != m_pos.m_endposition.fac.tz))
   {
      SetUpdateRefs(TRUE);
   }

   // Update hints
   m_pos.m_endposition.loc.cell = CELL_INVALID;
   m_pos.m_endposition.loc.hint = m_pos.m_position.loc.hint;
}

///////////////////////////////////////

void cPhysModel::UpdateEndLocation(tPhysSubModId i, mxs_real dt)
{
   mxs_vector movement_offset;
   mxs_vector velocity;
   mxs_vector offset;

   if (IsPlayer())
      PlayerMotionGetOffset(i, &offset);
   else
      mx_zero_vec(&offset);

   if ((GetSpringTension(i) > 0 || !IsZeroVector(offset)) && (i < m_pDynamicsData.Size()))
   {
      // Find end location based on velocity
      mx_copy_vec(&velocity, &m_pDynamicsData[i].GetVelocity());
      mx_scale_vec(&movement_offset, &velocity, dt);
   }
   else
      mx_sub_vec(&movement_offset, &m_pPosition[i].m_targetposition.loc.vec, &m_pPosition[i].m_position.loc.vec);

   // Update location
   mx_add_vec(&m_pPosition[i].m_endposition.loc.vec, &m_pPosition[i].m_position.loc.vec, &movement_offset);

   // Update rotation
   mx_mk_angvec(&m_pPosition[i].m_endposition.fac, m_pPosition[i].m_targetposition.fac.tx,
                                                   m_pPosition[i].m_targetposition.fac.ty,
                                                   m_pPosition[i].m_targetposition.fac.tz);
   // Update hints
   m_pPosition[i].m_endposition.loc.cell = CELL_INVALID;
   m_pPosition[i].m_endposition.loc.hint = m_pPosition[i].m_position.loc.hint;

   if (!EqualVectors(m_pPosition[i].m_endposition.loc.vec, m_pPosition[i].m_position.loc.vec))// && IsRope())
   {
      SetUpdateRefs(TRUE);
   }
}

///////////////////////////////////////

const Location & cPhysModel::GetEndLocation(tPhysSubModId subModId) const
{
   if (subModId == -1)
      return m_pos.m_endposition.loc;
   else
      return m_pPosition[subModId].m_endposition.loc;
}

///////////////////////////////////////

const mxs_vector & cPhysModel::GetEndLocationVec(tPhysSubModId subModId) const
{
   if (subModId == -1)
      return m_pos.m_endposition.loc.vec;
   else
      return m_pPosition[subModId].m_endposition.loc.vec;
}

///////////////////////////////////////

void cPhysModel::SetEndLocationVec(tPhysSubModId subModId, const mxs_vector &end_loc)
{
   if (subModId == -1)
   {
      mx_copy_vec((mxs_vector *) &m_pos.m_endposition.loc.vec, &end_loc);
      m_pos.m_endposition.loc.hint = m_pos.m_position.loc.hint;
      m_pos.m_endposition.loc.cell = -1;
   }
   else
      mx_copy_vec((mxs_vector *)  &m_pPosition[subModId].m_endposition.loc.vec, &end_loc);
}

///////////////////////////////////////

const mxs_angvec & cPhysModel::GetEndRotationVec(tPhysSubModId subModId) const
{
   if (subModId == -1)
      return m_pos.m_endposition.fac;
   else
      return m_pPosition[subModId].m_endposition.fac;
}

///////////////////////////////////////

void cPhysModel::SetEndRotationVec(tPhysSubModId subModId, const mxs_angvec &end_ang)
{
   if (subModId == -1)
      m_pos.m_endposition.fac = end_ang;
   else
      m_pPosition[subModId].m_endposition.fac = end_ang;
}

////////////////////////////////////////////////////////////////////////////////

#define sq(x) ((x)*(x))

void cPhysModel::PreventPlayerFall()
{
   Assert_(IsPlayer());

   cFaceContactList *pFaceContactList;

   // get the "correct" speed for constraining our motion
   // get player mode
   ePlayerMode cur_mode=GetPlayerMode();
   float speed=GetTransSpeedScale()*SLOW_MOVE_SPEED;
   float special_muls[2]={0.90,0.60};     // was 0.95
   float height_check=9.0;   // was 8.0

#ifndef SHIP
   config_get_float("player_fall_walk",&special_muls[0]);
   config_get_float("player_fall_crouch",&special_muls[1]);
   config_get_float("player_fall_height",&height_check);
#endif
   if (cur_mode==kPM_Crouch)
      speed*=special_muls[1];
   else
      speed*=special_muls[0];

   if (GetFaceContacts(PLAYER_FOOT, &pFaceContactList) &&
       (mx_mag2_vec(&GetVelocity()) < sq(speed)))
   {
      Location start, end, hit;
      Location targ_start, targ_end;

      // find our forward location
      MakeHintedLocationFromVector(&start, &GetLocationVec(PLAYER_FOOT),
                                   &GetLocation(PLAYER_FOOT));
      MakeHintedLocationFromVector(&targ_start, &GetTargetLocation(PLAYER_FOOT),
                                   &GetLocation(PLAYER_FOOT));

      MakeLocationFromVector(&end, &start.vec);
      end.vec.z -= 0.1;

      MakeLocationFromVector(&targ_end, &targ_start.vec);
      targ_end.vec.z -= height_check;

      // would like to hit water, and stop there, i guess?

      // Raycast down to see what kind of fall we're talking about
      if (PortalRaycast(&targ_start, &targ_end, &hit, 0))
      {
#ifndef SHIP
         if (config_is_defined("player_fall_loud"))
            mprintf("Saved Me\n");
#endif

         m_pos.m_targetposition.loc.vec = m_pos.m_position.loc.vec;
         m_pos.m_endposition.loc.vec = m_pos.m_position.loc.vec;

         for (int i=0; i<NumSubModels(); i++)
         {
            m_pPosition[i].m_targetposition.loc.vec = m_pPosition[i].m_position.loc.vec;
            m_pPosition[i].m_endposition.loc.vec = m_pPosition[i].m_position.loc.vec;
         }
      }
   }
}

DECLARE_TIMER(UM_UpdateTarget, Average);
DECLARE_TIMER(UM_UpdateRope, Average);
DECLARE_TIMER(UM_UpdateEnd, Average);
DECLARE_TIMER(UM_UpdateTargetSub, Average);
DECLARE_TIMER(UM_UpdateSpringSub, Average);
DECLARE_TIMER(UM_UpdateEndSub, Average);
DECLARE_TIMER(UM_PreventFall, Average);
DECLARE_TIMER(UM_UpdateRefs, Average);

void cPhysModel::UpdateModel(mxs_real dt)
{

   TIMER_Start(UM_UpdateTarget);
   UpdateTargetLocation(dt);
   TIMER_MarkStop(UM_UpdateTarget);

   TIMER_Start(UM_UpdateRope);
   UpdateRopeClimbing(dt);
   TIMER_MarkStop(UM_UpdateRope);

   TIMER_Start(UM_UpdateEnd);
   UpdateEndLocation(dt);
   TIMER_MarkStop(UM_UpdateEnd);

   for (int i=0; i<NumSubModels(); i++)
   {
      TIMER_Start(UM_UpdateTargetSub);
      UpdateTargetLocation(i, dt);
      TIMER_MarkStop(UM_UpdateTargetSub);

      TIMER_Start(UM_UpdateSpringSub);
      UpdateSpringMechanics(i, dt);
      TIMER_MarkStop(UM_UpdateSpringSub);

      TIMER_Start(UM_UpdateEndSub);
      UpdateEndLocation(i, dt);
      TIMER_MarkStop(UM_UpdateEndSub);
   }

   TIMER_Start(UM_UpdateRefs);
   if (UpdateRefsPending())
   {
      SetUpdateRefs(FALSE);
      PhysUpdateRefs(this);
   }
   TIMER_MarkStop(UM_UpdateRefs);

   TIMER_Start(UM_PreventFall);
   // If we're trying to prevent the player from falling, do it again
   if (IsPlayer())
      PreventPlayerFall();
   TIMER_MarkStop(UM_PreventFall);

   if (NumAttachments() > 0)
   {
      cAutoLinkQuery query(g_pPhysAttachRelation, LINKOBJ_WILDCARD, GetObjID());

      for (;!query->Done(); query->Next())
      {
         cPhysModel *pAttachModel = g_PhysModels.Get(query.GetSource());
         pAttachModel->UpdateModel(dt);
      }
   }  

}

///////////////////////////////////////

void cPhysModel::SetTargetLocation(const mxs_vector &loc)
{
   int i;

   mx_copy_vec(&m_pos.m_targetposition.loc.vec, &loc);

   UpdateEndLocation(.1);

   for (i=0; i<NumSubModels(); i++)
   {
      UpdateTargetLocation(i, .1);
      UpdateSpringMechanics(i, .1);
      UpdateEndLocation(i, .1);
   }
}

///////////////////////////////////////

void cPhysModel::UpdateTargetLocation(mxs_real dt)
{
   cPhysDynData *pDyn = GetDynamics();
   mxs_matrix facing, rotation, end_facing;
   mxs_vector velocity, rot_vec;
   mxs_real   rot_mag;

   if (IsAttached())
   {
      LinkID linkID = g_pPhysAttachRelation->GetSingleLink(GetObjID(), LINKOBJ_WILDCARD);
      sLink link;

      if (linkID)
      {
         g_pPhysAttachRelation->Get(linkID, &link);
         sPhysAttachData *pAttachData = (sPhysAttachData *)g_pPhysAttachRelation->GetData(linkID);

         cPhysModel *pAttachModel = g_PhysModels.GetActive(link.dest);

         if (pAttachModel)
         {
            mx_add_vec(&m_pos.m_targetposition.loc.vec, &pAttachModel->GetTargetLocation(), &pAttachData->offset);

            return;
         }
      }
   }

   // Setup
   mx_copy_vec(&velocity, &pDyn->GetVelocity());
   mx_copy_vec(&rot_vec, &pDyn->GetRotationalVelocity());

   mx_ang2mat(&facing, &m_pos.m_position.fac);

   if (mx_mag2_vec(&rot_vec) > 0.000001)
   {
      rot_mag = mx_normeq_vec(&rot_vec) * dt;

      mx_mk_rot_vec_mat(&rotation, &rot_vec, mx_rad2ang(rot_mag));
      mx_mul_mat(&end_facing, &rotation, &facing);

      mx_mat2ang(&m_pos.m_targetposition.fac, &end_facing);

#if 0
      if (IsPlayer())
      {
         mprintf("ang = %d\n", mx_rad2ang(rot_mag));
         mprintf("rot_vec = %g %g %g\n", rot_vec.x, rot_vec.y, rot_vec.z);
         mprintf("facing    = %d %d %d\n", m_pos.m_position.fac.tx, m_pos.m_position.fac.ty, m_pos.m_position.fac.tz);
         mprintf("endfacing = %d %d %d\n", m_pos.m_targetposition.fac.tx, m_pos.m_targetposition.fac.ty, m_pos.m_targetposition.fac.tz);
      }
#endif
   }

#if 0
   // Apply constraints
   ApplyConstraints(&velocity);

   pDyn->SetVelocity(velocity);
#endif

   // Update translation and rotation
   mx_scale_add_vec((mxs_vector *) &m_pos.m_targetposition.loc.vec,
                    (mxs_vector *) &m_pos.m_position.loc.vec,
                                   &velocity, dt);

   // Adjust location for rotation not around center point
   if (!IsPlayer() && !IsZeroVector(m_cog) && (rot_mag > 0.0001))
   {
      mxs_matrix rotation;
      mxs_matrix obj_for;
      mxs_matrix both_rot;
      mxs_angvec rot_ang;
      mxs_vector rotvel_offset;
      mxs_vector cog_offset;
      mxs_vector rot_offset;
      mxs_vector add_offset;

      // How much rotated this frame
      mx_scale_vec(&rotvel_offset, &rot_vec, rot_mag);
      mx_mk_angvec(&rot_ang, mx_rad2ang(rotvel_offset.x),
                             mx_rad2ang(rotvel_offset.y),
                             mx_rad2ang(rotvel_offset.z));


      // Set up matrices
      mx_ang2mat(&rotation, &rot_ang);
      mx_ang2mat(&obj_for, &m_pos.m_position.fac);

      mx_mul_mat(&both_rot, &rotation, &obj_for);

      // Find starting offset this frame
      mx_mat_mul_vec(&cog_offset, &obj_for, &m_cog);

      // Find ending offset this frame
      mx_mat_mul_vec(&rot_offset, &both_rot, &m_cog);

      // Find the delta
      mx_sub_vec(&add_offset, &rot_offset, &cog_offset);

      mx_addeq_vec(&m_pos.m_targetposition.loc.vec, &add_offset);
   }

   if (IsAttached())
   {
      LinkID linkID = g_pPhysAttachRelation->GetSingleLink(GetObjID(), LINKOBJ_WILDCARD);
      sLink link;

      if (linkID)
      {
         g_pPhysAttachRelation->Get(linkID, &link);
         sPhysAttachData *pAttachData = (sPhysAttachData *)g_pPhysAttachRelation->GetData(linkID);

         cPhysModel *pAttachModel = g_PhysModels.GetActive(link.dest);

         if (pAttachModel)
         {
            mxs_vector targ;
            mx_add_vec(&targ, &pAttachModel->GetTargetLocation(), &pAttachData->offset);

            mx_subeq_vec(&targ, &m_pos.m_targetposition.loc.vec);

            mprintf("targ: %g %g %g\n", targ.x, targ.y, targ.z);

            Assert_(fabs(targ.x) < 5.0);

         }
      }
   }

}

////////////////////////////////////////

void cPhysModel::ComputeRopeSegPct()
{
   cPhysModel *pRopeModel;

   int node_1 = GetRopeSegment();
   int node_2 = node_1 - 1;

   pRopeModel = g_PhysModels.Get(GetClimbingObj());

   if (pRopeModel == NULL)
   {
      Warning(("cPhysModel::ComputeRopeSegPct: no physics model for obj %d?\n", GetClimbingObj()));
      return;
   }

   mxs_vector segment;
   mxs_vector end_to_obj;

   // @TODO: make sure the rope has been updated

   mx_sub_vec(&segment, &pRopeModel->GetLocationVec(node_2), &pRopeModel->GetLocationVec(node_1));

   // went past node_1?
   mx_sub_vec(&end_to_obj, &GetLocationVec(), &pRopeModel->GetLocationVec(node_1));
   if (mx_dot_vec(&end_to_obj, &segment) < 0)
   {
      if (node_1 < (pRopeModel->NumSubModels() - 1))
      {
         SetRopeSegment(node_1 + 1);
         SetRopeSegmentPct(1.0);

         #ifndef SHIP
         if (config_is_defined("RopeSpew"))
            mprintf(" Obj %d moved from node %d to %d\n", GetObjID(), node_1, node_1 + 1);
         #endif
      }
      else
         BreakClimb(GetObjID(), FALSE, FALSE);
      return;
   }

   // went past node_2?
   mx_sub_vec(&end_to_obj, &GetLocationVec(), &pRopeModel->GetLocationVec(node_2));
   if (mx_dot_vec(&end_to_obj, &segment) > 0)
   {
      if (node_2 > 0)
      {
         SetRopeSegment(node_2);
         SetRopeSegmentPct(0.0);

         #ifndef SHIP
         if (config_is_defined("RopeSpew"))
            mprintf(" Obj %d moved from node %d to %d\n", GetObjID(), node_1, node_2);
         #endif
      }
      return;
   }

   // within the segment
   mxs_vector on_segment;
   mxs_vector seg_norm;
   mxs_vector to_obj;
   mxs_real   seg_len;

   seg_len = mx_norm_vec(&seg_norm, &segment);

   mx_sub_vec(&to_obj, &GetLocationVec(), &pRopeModel->GetLocationVec(node_1));
   mx_scale_vec(&on_segment, &seg_norm, mx_dot_vec(&seg_norm, &to_obj));

   SetRopeSegmentPct(mx_mag_vec(&on_segment) / seg_len);

   #ifndef SHIP
   if (config_is_defined("RopeSpew"))
      mprintf(" Obj %d is %g along segment %d\n", GetObjID(), GetRopeSegmentPct(), node_1);
   #endif
}

////////////////////////////////////////

void cPhysModel::UpdateRopeClimbing(mxs_real dt)
{
   if (!IsRopeClimbing())
      return;

   ObjID rope = GetClimbingObj();
   cPhysModel *pRopeModel = g_PhysModels.Get(rope);

   Assert_(pRopeModel);

   int node = GetRopeSegment();

   mxs_matrix orien;

   // Adjust rotation to match segment
   mxs_vector seg_norm;
   mxs_matrix new_orien;
   mxs_angvec new_rot;

   mx_sub_vec(&seg_norm, &pRopeModel->GetLocationVec(node - 1), &pRopeModel->GetLocationVec(node));
   mx_normeq_vec(&seg_norm);
   mx_ang2mat(&orien, &m_pos.m_targetposition.fac);

   mx_cross_vec(&new_orien.vec[1], &seg_norm, &orien.vec[0]);
   mx_cross_vec(&new_orien.vec[0], &new_orien.vec[1], &seg_norm);
   mx_cross_vec(&new_orien.vec[2], &new_orien.vec[0], &new_orien.vec[1]);

   mx_mat2ang(&new_rot, &new_orien);

   m_pos.m_targetposition.fac = new_rot;

   // Compute target location
   mxs_vector seg;
   mxs_vector loc;
   mxs_vector mov;

   mx_sub_vec(&seg, &pRopeModel->GetLocationVec(node - 1), &pRopeModel->GetLocationVec(node));
   mx_scaleeq_vec(&seg, GetRopeSegmentPct());

   mx_add_vec(&loc, &pRopeModel->GetLocationVec(node), &seg);

   mx_scale_addeq_vec(&loc, &new_orien.vec[0], -1.0);
   mx_scale_addeq_vec(&loc, &new_orien.vec[1], -0.1);

   mx_sub_vec(&mov, &m_pos.m_targetposition.loc.vec, &m_pos.m_position.loc.vec);
   mx_addeq_vec(&loc, &mov);

   mxs_vector new_delta;
   mx_sub_vec(&new_delta, &loc, &m_pos.m_position.loc.vec);
   if (PhysObjValidPos(GetObjID(), &new_delta))
      m_pos.m_targetposition.loc.vec = loc;
   else
   {
      m_pos.m_targetposition.loc.vec = m_pos.m_position.loc.vec;
      m_pos.m_targetposition.fac = m_pos.m_position.fac;
   }
}

////////////////////////////////////////

void cPhysModel::Squish(mxs_real magnitude)
{
   long time = GetSimTime();

   if ((m_lastSquishTime < 0) || (time > (m_lastSquishTime + kSquishPeriod)))
   {
      m_lastSquishTime = time;

      #ifndef SHIP
      if (config_is_defined("SquishSpew"))
         mprintf("Obj %d is squished for %g\n", GetObjID(), magnitude);
      #endif

      AutoAppIPtr_(ObjectSystem, pObjSys);
      AutoAppIPtr_(StimSensors, pStimSens);
      AutoAppIPtr_(Stimulator, pStim);

      StimID stim = pObjSys->GetObjectNamed("BashStim");
      StimSensorID sensid = pStimSens->LookupSensor(GetObjID(), stim);

      if (sensid != SENSORID_NULL)
      {
         sStimEventData evdat = { stim, magnitude, 0, sensid, 0, GetSimTime() };
         sStimEvent event(&evdat);

         pStim->StimulateSensor(sensid, &event);
      }
   }
}

////////////////////////////////////////

void cPhysModel::UpdateTargetLocation(tPhysSubModId i, mxs_real dt)
{
   mxs_matrix obj_rotation;
   mxs_vector submod_offset;
   mxs_vector submod_targ;

   // Offset the submodel target position from the position target
   if (IsPlayer())
   {
      mxs_angvec upright;
      mxs_vector temp_offset, mo_offset;

      mx_mk_angvec(&upright, 0, 0, m_pos.m_position.fac.tz);
      mx_ang2mat(&obj_rotation, &upright);

      PlayerMotionGetOffset(i, &mo_offset);
      mx_add_vec(&temp_offset, &m_Offset[i], &mo_offset);
      mx_mat_mul_vec(&submod_offset, &obj_rotation, &temp_offset);
   }
   else
   {
      if (IsRope())
      {
         sPhysRopeProp *pRopeProp;

         if (!g_pPhysRopeProp->Get(GetObjID(), &pRopeProp))
         {
            Warning(("cPhysModel::UpdateTargetLocation: no rope property on obj %d\n", GetObjID()));
            return;
         }

         mx_scale_vec(&submod_offset, &kGravityDir, (mxs_real)(i) * (pRopeProp->length / 8.0));

         if (i > 0)
         {
            mxs_vector prev_pos;

            mx_sub_vec(&prev_pos, &GetLocationVec(i-1), &GetLocationVec(0));
            mx_scaleeq_vec(&prev_pos, 0.6);
            prev_pos.z = 0;
            mx_addeq_vec(&submod_offset, &prev_pos);
         }

         if (i < (NumSubModels() - 1))
         {
            mxs_vector next_pos;

            mx_sub_vec(&next_pos, &GetLocationVec(i+1), &GetLocationVec(0));
            mx_scaleeq_vec(&next_pos, 0.2);
            next_pos.z = 0;

            mx_addeq_vec(&submod_offset, &next_pos);
         }
      }
      else
      {
         mx_ang2mat(&obj_rotation, &m_pos.m_position.fac);
         mx_mat_mul_vec(&submod_offset, &obj_rotation, &m_Offset[i]);
      }
   }

   mx_add_vec(&submod_targ, &m_pos.m_targetposition.loc.vec, &submod_offset);
   mx_copy_vec(&m_pPosition[i].m_targetposition.loc.vec, &submod_targ);

#if 0
   if (IsCreature())
   {
      mxs_vector model_offset;
      mxs_vector difference;
      mx_sub_vec(&model_offset, &m_pos.m_targetposition.loc.vec, &m_pos.m_position.loc.vec);
      mx_sub_vec(&submod_offset, &m_pPosition[i].m_targetposition.loc.vec, &m_pPosition[i].m_position.loc.vec);
      mx_sub_vec(&difference, &model_offset, &submod_offset);

      Assert_(mx_mag_vec(&difference) < 0.01);
   }
#endif

   // Rotation
   mxs_vector rot_vec;

   mx_copy_vec(&rot_vec, &GetDynamics(i)->GetRotationalVelocity());
   m_pPosition[i].m_targetposition.fac.tx += mx_rad2ang(rot_vec.x * dt);
   m_pPosition[i].m_targetposition.fac.ty += mx_rad2ang(rot_vec.y * dt);
   m_pPosition[i].m_targetposition.fac.tz += mx_rad2ang(rot_vec.z * dt);

#if 0

   mxs_matrix facing;
   mxs_matrix rotation;
   mxs_matrix end_facing;
   mxs_real   rot_mag;

   mx_copy_vec(&rot_vec, &GetDynamics(i)->GetRotationalVelocity());

   if (!IsZeroVector(rot_vec))
   {
      mx_ang2mat(&facing, &m_pPosition[i].m_position.fac);

      rot_mag = mx_normeq_vec(&rot_vec) * dt;

      mx_mk_rot_vec_mat(&rotation, &rot_vec, mx_rad2ang(rot_mag));
      mx_mul_mat(&end_facing, &rotation, &facing);

      mx_mat2ang(&m_pPosition[i].m_targetposition.fac, &end_facing);
   }
#endif
}

////////////////////////////////////////

void cPhysModel::UpdateSpringMechanics(tPhysSubModId i, mxs_real dt)
{
   mxs_vector movement_offset;
   mxs_vector velocity;

   if (i >= m_pDynamicsData.Size())
      return;

   if ((GetSpringTension(i) > 0) && ((GetDynamics()->GetCurrentTime() == 0.0) || IsRope()))
   {
      float damping_factor = 1.0;
      float tension_factor = 1.0;

      if (dt > 0.05)
      {
         dt = 0.05;
         tension_factor = 0.6;
         damping_factor = 0.6;
      }

      if (dt < 0.001)
         dt = 0.001;

#if 0
      if (IsRope())
      {
         mprintf("[%d]\n", i);
         mprintf("pre-tension = %g\n", tension);
         mprintf("pre-damping = %g\n", damping);
      }
#endif

      float tension;
      float damping;

      if (IsRope())
      {
         tension = GetSpringTension(i) * dt * 700 * tension_factor;
         damping = 0.8;
      }
      else
      {
         tension = GetSpringTension(i) / dt;
         damping = GetSpringDamping(i) + ((1.0 - GetSpringDamping(i)) * dt);
      }

#if 0
      if (IsRope())
      {
         mprintf("post-tension = %g\n", _tension);
         mprintf("post-damping = %g\n", _damping);

         if (i == 7)
         {
            char buff[256];

            sprintf(buff, "dt = %2.4f\n", dt);
            mprintf("%s", buff);
         }
         mprintf("\n");
      }
#endif

      mx_sub_vec(&movement_offset, &m_pPosition[i].m_targetposition.loc.vec, &m_pPosition[i].m_position.loc.vec);

      mx_scale_vec(&velocity, &movement_offset, tension);

      velocity.z *= 0.5;
	  
      mx_scale_addeq_vec(&velocity, &m_pDynamicsData[i].GetVelocity(), damping);

	  
#if 0
      if (IsMantling())
      {
         mprintf("movement_offset_scale_2 = %g\n", mx_mag2_vec(&movement_offset));
         mprintf("pre-vel  = %g %g %g\n", m_pDynamicsData[i].GetVelocity().x, m_pDynamicsData[i].GetVelocity().y, m_pDynamicsData[i].GetVelocity().z);
         mprintf("velocity = %g %g %g\n", velocity.x, velocity.y, velocity.z);
         mprintf("velocity_mag_2 = %g\n", mx_mag2_vec(&velocity));
         mprintf("dt = %d\n", (int)(dt * 100000));
         mprintf("\n");
      }
#endif

      ApplyConstraints(i, &velocity);

      //do the capping AFTER constraints, really.
      if (IsPlayer())  //if player, do our spring capping.
      {		  
         float maxmag = kSpringCapMag; //default value	  		 
         if (-1.0 * velocity.z > maxmag)		  			  
            maxmag = -1.0 * velocity.z; //so if I'm falling down I don't					                
                                        //do a Wile E. Coyote bit.  Sigh.  I wish we knew the
		                                  //root cause of the problem with doors and ladders that
	                                     //this is *cough* hacking *cough* to fix.  AMSD      		  
         if (mx_mag_vec(&velocity) > maxmag)	  		  
         {		 			
            ConfigSpew("SpringCapSpew",("Using spring cap\n"));	     			  
            mx_scaleeq_vec(&velocity,maxmag/(mx_mag_vec(&velocity)));	  		  
         }		 	  
      }


      m_pDynamicsData[i].SetVelocity(velocity);
   }
   else
   {
      // Back-compute velocity
      mx_copy_vec(&velocity, &GetVelocity());
#if 0
      mx_sub_vec(&movement_offset, &m_pPosition[i].m_targetposition.loc.vec, &m_pPosition[i].m_position.loc.vec);

      mx_scale_vec(&velocity, &movement_offset, 1 / dt);

      if (IsPlayer() && ((i == 1) || (i == 2)))
         Assert_(velocity.x == GetVelocity().x);

      ApplyConstraints(&velocity);
#endif

      m_pDynamicsData[i].SetVelocity(velocity);
   }
}

////////////////////////////////////////////////////////////////////////////////

void cPhysModel::UpdateMedium()
{
   if (IsPlayer())
   {
      int submod_media_trans[3] = {PLAYER_FOOT, PLAYER_BODY, PLAYER_HEAD};
      int new_portal_medium[3];
      int new_portal_cell[3];
      eMediaState new_medium;
      int new_cell;

      Location new_loc;

      for (int i=0; i<3; i++)
      {
         MakeHintedLocationFromVector(&new_loc,
                                      &GetEndLocationVec(submod_media_trans[i]),
                                      &GetLocation(submod_media_trans[i]));

         // @HACK: offset the player eyes by eye amount
         if (submod_media_trans[i] == PLAYER_HEAD)
            new_loc.vec.z += 0.8;

         if (CellFromLoc(&new_loc) != CELL_INVALID)
         {
            new_portal_cell[i] = new_loc.cell;
            new_portal_medium[i] = WR_CELL(new_loc.cell)->medium;
         }
         else
         {
            new_portal_cell[i] = CELL_INVALID;
            new_portal_medium[i] = NO_MEDIUM;
         }
      }

      if (new_portal_medium[0] == MEDIA_WATER)
      {
         if (new_portal_medium[1] == MEDIA_WATER)
         {
            if (new_portal_medium[2] == MEDIA_WATER)
            {
               new_cell = new_portal_cell[2];
               new_medium = kMS_Liquid_Submerged;
            }
            else
            {
               new_cell = new_portal_cell[1];
               new_medium = kMS_Liquid_Wading;
            }
         }
         else
         {
            new_cell = new_portal_cell[0];
            new_medium = kMS_Liquid_Standing;
         }
      }
      else
      {
         new_cell = new_portal_cell[0];
         new_medium = kMS_Air;
      }

      if (m_medium != new_medium)
      {
         ObjMediaTrans(GetObjID(), new_cell, (eMediaState)m_medium, new_medium);
         m_medium = new_medium;
      }
   }
   else
   {
      eMediaState new_medium;
      int new_portal_medium;

      Location new_loc;
      MakeHintedLocationFromVector(&new_loc, &GetEndLocationVec(), &GetLocation(0));

      if (CellFromLoc(&new_loc) != CELL_INVALID)
         new_portal_medium = WR_CELL(new_loc.cell)->medium;
      else
         new_portal_medium = NO_MEDIUM;

      switch (new_portal_medium)
      {
         case MEDIA_WATER: new_medium = kMS_Liquid_Submerged; break;
         case MEDIA_AIR:   new_medium = kMS_Air; break;
         case MEDIA_SOLID: new_medium = kMS_Invalid; break;

         case NO_MEDIUM: return;
      }

      if (m_medium != new_medium)
      {
         ObjMediaTrans(GetObjID(), new_loc.cell, (eMediaState)m_medium, new_medium);
         m_medium = new_medium;
      }
   }
}

////////////////////////////////////////////////////////////////////////////////

mxs_real cPhysModel::GetPPlateActivationWeight() const
{
   sPhysPPlateProp *pPPlateProp;

   if (!g_pPhysPPlateProp->Get(GetObjID(), &pPPlateProp))
   {
      Warning(("GetPPlateActivationWeight: no pressure plate property on %d\n", GetObjID()));
      return 0.0;
   }

   return pPPlateProp->activation_weight;
}

////////////////////////////////////////

mxs_real cPhysModel::GetPPlateTravel() const
{
   sPhysPPlateProp *pPPlateProp;

   if (!g_pPhysPPlateProp->Get(GetObjID(), &pPPlateProp))
   {
      Warning(("GetPPlateTravel: no pressure plate property on %d\n", GetObjID()));
      return 0.0;
   }

   return pPPlateProp->travel;
}

///////////////////////////////////////

mxs_real cPhysModel::GetPPlateSpeed() const
{
   sPhysPPlateProp *pPPlateProp;

   if (!g_pPhysPPlateProp->Get(GetObjID(), &pPPlateProp))
   {
      Warning(("GetPPlateSpeed: no pressure plate property on %d\n", GetObjID()));
      return 0.0;
   }

   return pPPlateProp->speed;
}

///////////////////////////////////////

mxs_real cPhysModel::GetPPlatePause() const
{
   sPhysPPlateProp *pPPlateProp;

   if (!g_pPhysPPlateProp->Get(GetObjID(), &pPPlateProp))
   {
      Warning(("GetPPlatePause: no pressure plate property on %d\n", GetObjID()));
      return 0.0;
   }

   return pPPlateProp->pause;
}

///////////////////////////////////////

int cPhysModel::GetPPlateState() const
{
   sPhysPPlateProp *pPPlateProp;

   if (!g_pPhysPPlateProp->Get(GetObjID(), &pPPlateProp))
   {
      Warning(("GetPPlateState: no pressure plate property on %d\n", GetObjID()));
      return kPPS_Inactive;
   }

   return pPPlateProp->state;
}

///////////////////////////////////////

void cPhysModel::SetPPlateState(int state)
{
   sPhysPPlateProp *pPPlateProp;

   if (!g_pPhysPPlateProp->Get(GetObjID(), &pPPlateProp))
   {
      Warning(("SetPPlateState: no pressure plate property on %d\n", GetObjID()));
      return;
   }

   pPPlateProp->state = state;
   g_pPhysPPlateProp->Set(GetObjID(), pPPlateProp);
}

///////////////////////////////////////

mxs_real cPhysModel::GetPPlateCurPause() const
{
   sPhysPPlateProp *pPPlateProp;

   if (!g_pPhysPPlateProp->Get(GetObjID(), &pPPlateProp))
   {
      Warning(("GetPPlateCurPause: no pressure plate property on %d\n", GetObjID()));
      return 0.0;
   }

   return pPPlateProp->cur_pause;
}

///////////////////////////////////////

void cPhysModel::SetPPlateCurPause(mxs_real cur_pause)
{
   sPhysPPlateProp *pPPlateProp;

   if (!g_pPhysPPlateProp->Get(GetObjID(), &pPPlateProp))
   {
      Warning(("SetPPlateCurPause: no pressure plate property on %d\n", GetObjID()));
      return;
   }

   pPPlateProp->cur_pause = cur_pause;
   g_pPhysPPlateProp->Set(GetObjID(), pPPlateProp);
}

////////////////////////////////////////////////////////////////////////////////

BOOL cPhysModel::IsRopeClimbing() const
{
   cPhysModel *pModel;

   if ((pModel = g_PhysModels.Get(m_climbingObj)) == NULL)
      return FALSE;

   return pModel->IsRope();
}

////////////////////////////////////////////////////////////////////////////////

void cPhysModel::Activate(void)
{
   if (m_flags & kPMF_Inactive)
   {
      g_PhysModels.ActivateToMoving(this);
      m_flags &= ~kPMF_Inactive;
   }
}

///////////////////////////////////////

void cPhysModel::Deactivate(void)
{
   if (!(m_flags & kPMF_Inactive))
   {
      g_PhysModels.Deactivate(this);
      m_flags |= kPMF_Inactive;
   }
}

///////////////////////////////////////

void cPhysModel::AddTransLimit(const mxs_vector &loc, LimitCallback callback)
{
   sTransLimit *limit = new sTransLimit;

   mx_sub_vec(&limit->norm, &m_pos.m_position.loc.vec, &loc);

   if (mx_mag2_vec(&limit->norm) < 0.0001)
   {
      delete limit;
      return;
   }

   mx_normeq_vec(&limit->norm);
   limit->plane_const = mx_dot_vec(&limit->norm, &loc);

   limit->callback = callback;

   LGALLOC_PUSH_CREDIT();
   m_TransLimitList.Append(limit);
   LGALLOC_POP_CREDIT();
}

///////////////////////////////////////

void cPhysModel::GetTransLimits(mxs_vector *limit_list, int *limit_list_size, int max_list_size) const
{
   int i;
   for (i=0; i<max_list_size && i<m_TransLimitList.Size(); i++)
   {
      // Project our current location onto the plane
      mxs_real plane_dist = mx_dot_vec(&GetLocationVec(), &m_TransLimitList[i]->norm) -
                            m_TransLimitList[i]->plane_const;

      mx_scale_add_vec(&limit_list[i], &GetLocationVec(), &m_TransLimitList[i]->norm, -plane_dist);
   }

   *limit_list_size = i;
}

///////////////////////////////////////

BOOL cPhysModel::CheckTransLimits(const mxs_vector &start, const mxs_vector &end, mxs_vector *limit)
{
   int i;
   BOOL hard_limit = FALSE;
   BOOL limited = FALSE;
   mxs_vector norm;
   mxs_real   plane_const;
   mxs_real   end_dist;


   for (i=0; i<m_TransLimitList.Size(); i++)
   {
      mx_copy_vec(&norm, &m_TransLimitList[i]->norm);
      plane_const = m_TransLimitList[i]->plane_const;

      #ifdef DBG_ON
      if (mx_dot_vec(&start, &norm) - plane_const < 0)
         Warning(("CheckTransLimits: start point on far side of limit for obj %d?\n", GetObjID()));
      #endif

      end_dist = mx_dot_vec(&end, &norm) - plane_const;
      if (end_dist < 0)
      {
         mxs_vector end_loc;

         limited = TRUE;

         mx_copy_vec(&end_loc, &end);

         if (m_TransLimitList[i]->callback(GetObjID()))
         {
            hard_limit = TRUE;

            // project onto plane
            mx_scale_add_vec(limit, &end_loc, &norm, -end_dist);
         }
      }
   }

   if (hard_limit)
   {
      mxs_vector zero;

      mx_zero_vec(&zero);
      GetDynamics()->SetVelocity(zero);
   }

   return limited;
}

///////////////////////////////////////

void cPhysModel::AddAngleLimit(int axis, int angle, LimitCallback callback)
{
   sAngleLimit *limit = new sAngleLimit;

   switch (axis)
   {
      case 0: limit->axis = 2; break;
      case 1: limit->axis = 2; break;
      case 2: limit->axis = 0; break;
   }

   mxs_matrix obj_rotation;
   mxs_matrix axis_rotation;
   mxs_matrix end_rotation;

   mx_ang2mat(&obj_rotation, &GetRotation());
   mx_mk_rot_vec_mat(&axis_rotation, &obj_rotation.vec[axis], angle * MX_ANG_PI / 180);

   mx_mul_mat(&end_rotation, &axis_rotation, &obj_rotation);

   mx_cross_vec(&limit->plane_norm, &end_rotation.vec[limit->axis], &end_rotation.vec[axis]);

   limit->callback = callback;

   LGALLOC_PUSH_CREDIT();
   m_AngLimitList.Append(limit);
   LGALLOC_POP_CREDIT();
}

///////////////////////////////////////

BOOL cPhysModel::CheckAngleLimits(mxs_angvec start, mxs_angvec end, mxs_angvec *limit)
{
   mxs_matrix start_mat, end_mat;
   mxs_real   start_dp, end_dp;
   BOOL       hard_limit = FALSE;
   BOOL       limited = FALSE;
   int i;

   *limit = end;

   mx_ang2mat(&start_mat, &start);
   mx_ang2mat(&end_mat, &end);

   for (i=0; i<m_AngLimitList.Size(); i++)
   {
      start_dp = mx_dot_vec(&start_mat.vec[m_AngLimitList[i]->axis], &m_AngLimitList[i]->plane_norm);
      end_dp = mx_dot_vec(&end_mat.vec[m_AngLimitList[i]->axis], &m_AngLimitList[i]->plane_norm);

      if (((start_dp * end_dp) < 0) || (end_dp == 0.0 && start_dp != 0.0))
      {
         limited = TRUE;

         if (m_AngLimitList[i]->callback)
            hard_limit |= m_AngLimitList[i]->callback(GetObjID());
      }
   }

   if (limited)
   {
      if (IsPlayer())
         *limit = start;
      else
         *limit = m_pos.m_position.fac;
   }

   if (hard_limit)
   {
      mxs_vector zero;

      mx_zero_vec(&zero);
      GetDynamics()->SetRotationalVelocity(zero);
   }

   return limited;
}

///////////////////////////////////////////////////////////////////////////////

#ifndef SHIP

long cPhysModel::InternalSize() const
{
   int i;
   long size = 0;

   size += m_springInfo.Size() * sizeof(tSpringInfo);

   size += m_ConstraintList.Size() * sizeof(tConstraint);
   for (i=0; i<m_SubModConstraintList.Size(); i++)
      size += m_SubModConstraintList[i].Size() * sizeof(tConstraint);
   size += m_VelConstraintList.Size() * sizeof(tVelocityConstraint);

   size += m_nSubModels * sizeof(ePhysModelType);

   size += m_nSubModels * sizeof(cPhysPos);
   size += m_Offset.Size() * sizeof(mxs_vector);

   size += m_AngLimitList.Size() * (sizeof(sAngleLimit *) + sizeof(sAngleLimit));
   size += m_TransLimitList.Size() * (sizeof(sAngleLimit *) + sizeof(sTransLimit));

   size += 8 + m_pDynamicsData.Size() * sizeof(cPhysDynData);
   size += 8 + m_pControlData.Size() * sizeof(cPhysCtrlData);

   return size;
}

#endif

///////////////////////////////////////////////////////////////////////////////
