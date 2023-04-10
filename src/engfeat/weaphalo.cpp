/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/engfeat/weaphalo.cpp,v 1.5 2000/03/17 12:38:18 bfarquha Exp $
//
// Weapon halo header
//

#ifdef THIEF

#include <lg.h>
#include <matrix.h>
#include <config.h>

#include <relation.h>
#include <linkbase.h>
#include <lnkquery.h>

#include <phmods.h>
#include <phmod.h>
#include <phcore.h>

#include <camera.h>
#include <objpos.h>
#include <physapi.h>
#include <playrobj.h> // PlayerCamera()
#include <plycbllm.h> // PlayerArm()

#include <weaphalo.h>

#include <mprintf.h>

// Must be last header
#include <dbmem.h>

static IRelation *pWHInflatedLinks = NULL;
static IRelation *pWHBlockLinks = NULL;

struct sHaloBlockData
{
   int halo_id;
};

static int  haloAxes[kWH_NumModels] = { 2, 1, 2, 1, 1, 1, 1, 1 };
static BOOL haloSign[kWH_NumModels] = { TRUE, FALSE, FALSE, TRUE, TRUE, FALSE, FALSE, TRUE };

////////////////////////////////////////////////////////////////////////////////

void InitWeaponHaloSystem()
{
   sRelationDesc WHInflatedDesc = { "WHInflated", kRelationNoEdit, 0, 0 };
   sRelationDesc WHBlockDesc    = { "WHBlock", kRelationNoEdit, 0, 0 };

   sRelationDataDesc noDataDesc    = { "None", 0 };
   sRelationDataDesc blockDataDesc = LINK_DATA_DESC_FLAGS(sHaloBlockData, kRelationDataAutoCreate);

   pWHInflatedLinks = CreateStandardRelation(&WHInflatedDesc, &noDataDesc, kQCaseSetDestKnown);
   pWHBlockLinks = CreateStandardRelation(&WHBlockDesc, &blockDataDesc, kQCaseSetSourceKnown);
}

////////////////////////////////////////

void TermWeaponHaloSystem()
{
   SafeRelease(pWHInflatedLinks);
   SafeRelease(pWHBlockLinks);
}

////////////////////////////////////////////////////////////////////////////////

BOOL WeaponHaloInflated(ObjID weapon)
{
   BOOL has_links;

   ILinkQuery *query = pWHInflatedLinks->Query(LINKOBJ_WILDCARD, weapon);
   has_links = !query->Done();
   SafeRelease(query);

   return has_links;
}

////////////////////////////////////////

BOOL WeaponSubmodIsHalo(ObjID weapon, int submod)
{
   cPhysModel *pModel;

   if ((pModel = g_PhysModels.GetActive(weapon)) == NULL)
      return FALSE;

   if ((submod >= (pModel->NumSubModels() - kWH_NumModels)) &&
       (WeaponHaloInflated(weapon) || WeaponHaloIsBlocking(weapon)))
      return TRUE;
   else
      return FALSE;
}

////////////////////////////////////////

BOOL WeaponHaloIsBlocking(ObjID objID)
{
   BOOL has_links;

   if (objID == OBJ_NULL)
      return FALSE;

   if (objID == PlayerArm())
      objID = PlayerObject();

   ILinkQuery *query = pWHBlockLinks->Query(objID, LINKOBJ_WILDCARD);
   has_links = !query->Done();
   SafeRelease(query);

   return has_links;
}

////////////////////////////////////////

BOOL WeaponGetBlockingHalo(ObjID objID, int *haloId)
{
   BOOL has_links;
   sHaloBlockData *pData;

   Assert_(haloId);

   ILinkQuery *query = pWHBlockLinks->Query(objID, LINKOBJ_WILDCARD);
   has_links = !query->Done();

   if (has_links)
   {
      pData = (sHaloBlockData *)pWHBlockLinks->GetData(query->ID());
      *haloId = pData->halo_id;
   }
   else
      *haloId = -1;

   SafeRelease(query);
   return has_links;
}

////////////////////////////////////////

mxs_real WeaponHaloQuadDist(ObjID objID, int halo_id, mxs_vector &subPos)
{

   mxs_vector halo_pos;
   mxs_vector position;
   mxs_angvec facing;
   mxs_matrix mat_facing;
   mxs_real   pushout;
   mxs_real   offset;

   // Find "in front" location and heading
   if ((objID == PlayerArm()) || (objID == PlayerObject()))
   {
      // Grab the position/facing from the camera
      CameraGetLocation(PlayerCamera(), &position, &facing);

      pushout = kWH_PlayerPushout;
      config_get_float("halo_player_pushout", &pushout);

      offset = kWH_PlayerOffset;
      config_get_float("halo_player_offset", &offset);

      // The center point should be slightly in front of the camera
      mx_ang2mat(&mat_facing, &facing);
      mx_scale_addeq_vec(&position, &mat_facing.vec[0], pushout);
   }
   else
   {
      // Grab the position/facing from the AI object
      position = ObjPosGet(objID)->loc.vec;
      facing = ObjPosGet(objID)->fac;

      pushout = kWH_AIPushout;
      config_get_float("halo_ai_pushout", &pushout);

      offset = kWH_AIOffset;
      config_get_float("halo_ai_offset", &offset);

      // The center point should be slightly in front of the AI
      mx_ang2mat(&mat_facing, &facing);
      mx_scale_addeq_vec(&position, &mat_facing.vec[0], pushout);
   }

   mx_scale_add_vec(&halo_pos, &position, &mat_facing.vec[haloAxes[halo_id]], haloSign[halo_id] ? offset : -offset);

   return mx_dist_vec(&halo_pos, &subPos);
}

////////////////////////////////////////////////////////////////////////////////

void WeaponHaloInit(ObjID objID, ObjID weapon, int numSubModels)
{
   if (objID == PlayerArm())
      objID = PlayerObject();

   // Check if we should already be inflated
   if (WeaponHaloInflated(weapon))
      WeaponHaloInflate(objID, weapon);
}

////////////////////////////////////////

void WeaponHaloTerm(ObjID objID, ObjID weapon)
{
   if (objID == PlayerArm())
      objID = PlayerObject();

   // Deflate, if we haven't already
   if (WeaponHaloInflated(weapon))
      WeaponHaloDeflate(objID, weapon);

   // Remove any blocking links
   if (WeaponHaloIsBlocking(objID))
      WeaponHaloUnsetBlock(objID, weapon);
}

////////////////////////////////////////

// halo is made of 8 spheres - 4 in front of, in cross pattern, and
// 4 in front, slightly closer bug larger, in 'x' pattern.

// We'd like to have AIs only have four spheres, and the player more, but in meantime, lets just double up on the
// AI spheres. Slightly inefficient, but not badly.
void WeaponHaloUpdate(ObjID objID, ObjID weapon, int numSubModels)
{
   int i;

   if (!PhysObjHasPhysics(weapon))
   {
      WeaponHaloDeflate(objID, weapon);
      return;
   }

   // @TBD: this might want to be faster...
   if (!WeaponHaloInflated(weapon) && PhysObjHasPhysics(weapon))
   {
      for (i=0; i<kWH_NumModels; i++)
      {
         PhysSetSubModLocation(weapon, numSubModels + i, &ObjPosGet(objID)->loc.vec);
         PhysSetSubModRadius(weapon, numSubModels + i, 0);
      }

      return;
   }

   mxs_vector halo_pos;
   mxs_angvec facing;
   mxs_matrix mat_facing;

   mxs_vector position[2];

   mxs_real pushout[2];
   mxs_real offset[2];
   mxs_real radius[2];

   float fSet2Offset;
   BOOL bIsAI;

   // Find "in front" location and heading

   if (objID == PlayerObject() || objID == PlayerArm())
   {
      bIsAI = FALSE;

      offset[0] = kWH_PlayerOffset;
      config_get_float("halo_player_offset", &offset[0]);

      radius[0] = kWH_PlayerRadius;
      config_get_float("halo_player_radius", &radius[0]);

      offset[1] = kWH_PlayerOffset2;
      config_get_float("halo_player_offset2", &offset[1]);

      radius[1] = kWH_PlayerRadius2;
      config_get_float("halo_player_radius2", &radius[1]);

      fSet2Offset = kWH_PlayerSet2Offset;
      config_get_float("halo_player_set2offset", &fSet2Offset);

      // Grab the position/facing from the camera
      CameraGetLocation(PlayerCamera(), &position[0], &facing);

      pushout[0] = kWH_PlayerPushout;
      config_get_float("halo_player_pushout", &pushout[0]);

      pushout[1] = kWH_PlayerPushout2;
      config_get_float("halo_player_pushout2", &pushout[1]);

      // The center point should be slightly in front of the camera
      mx_ang2mat(&mat_facing, &facing);
      mx_scale_add_vec(&position[1], &position[0], &mat_facing.vec[0], pushout[1]);
      mx_scale_addeq_vec(&position[0], &mat_facing.vec[0], pushout[0]);
   }
   else
   {
      bIsAI = TRUE;

      offset[0] = kWH_AIOffset;
      config_get_float("halo_ai_offset", &offset[0]);

      radius[0] = kWH_AIRadius;
      config_get_float("halo_ai_radius", &radius[0]);

      offset[1] = offset[0];
      radius[1] = radius[0];

      // Grab the position/facing from the AI object
      position[0] = ObjPosGet(objID)->loc.vec;
      facing = ObjPosGet(objID)->fac;

      pushout[0] = kWH_AIPushout;
      config_get_float("halo_ai_pushout", &pushout[0]);
      pushout[1] = pushout[0];

      // The center point should be slightly in front of the AI
      mx_ang2mat(&mat_facing, &facing);
      mx_scale_addeq_vec(&position[0], &mat_facing.vec[0], pushout[0]);
      position[1] = position[0];
   }


   // Set the locations and radii
   for (i=0; i<kWH_NumModels; i++)
   {
      if (bIsAI)
      {
         if (i == kWH_Set2Start) // Already Doubled up
            break;

         mx_scale_add_vec(&halo_pos, &position[0], &mat_facing.vec[haloAxes[i]], haloSign[i] ? offset[0] : -offset[0]);
         PhysSetSubModLocation(weapon, numSubModels + i, &halo_pos);
         PhysSetSubModRadius(weapon, numSubModels + i, radius[0]);
         PhysSetSubModLocation(weapon, numSubModels + i + kWH_Set2Start, &halo_pos);
         PhysSetSubModRadius(weapon, numSubModels + i + kWH_Set2Start, radius[0]);
      }
      else
      {
         if (i >= kWH_Set2Start) // Make 'X' pattern
         {
            mx_scale_add_vec(&halo_pos, &position[1], &mat_facing.vec[haloAxes[i]], haloSign[i] ? offset[1] : -offset[1]);
            if (i >= kWH_Set2Part2Start)
               halo_pos.z += fSet2Offset;
            else
               halo_pos.z -= fSet2Offset;

            PhysSetSubModLocation(weapon, numSubModels + i, &halo_pos);
            PhysSetSubModRadius(weapon, numSubModels + i, radius[1]);
         }
         else // Make Cross pattern
         {
            mx_scale_add_vec(&halo_pos, &position[0], &mat_facing.vec[haloAxes[i]], haloSign[i] ? offset[0] : -offset[0]);
            PhysSetSubModLocation(weapon, numSubModels + i, &halo_pos);
            PhysSetSubModRadius(weapon, numSubModels + i, radius[0]);
         }
      }
   }
}

////////////////////////////////////////

void WeaponHaloInflate(ObjID objID, ObjID weapon)
{
   // Create the inflation link
   pWHInflatedLinks->Add(objID, weapon);
}

////////////////////////////////////////

void WeaponHaloDeflate(ObjID objID, ObjID weapon)
{
   // Destroy the inflation link
   ILinkQuery *query = pWHInflatedLinks->Query(LINKOBJ_WILDCARD, weapon);
   for (; !query->Done(); query->Next())
      pWHInflatedLinks->Remove(query->ID());
   SafeRelease(query);
}

////////////////////////////////////////

void WeaponHaloSetBlock(ObjID objID, ObjID weapon, int submod)
{
   cPhysModel *pModel;
   sHaloBlockData *pData = new sHaloBlockData;

   if ((pModel = g_PhysModels.GetActive(weapon)) == NULL)
   {
      Warning(("Attempt to set block w/o physics model?\n"));
      delete pData;
      return;
   }

   pData->halo_id = kWH_NumModels - (pModel->NumSubModels() - submod);

   pWHBlockLinks->AddFull(objID, weapon, pData);
}

////////////////////////////////////////

void WeaponHaloUnsetBlock(ObjID objID, ObjID weapon)
{
   ILinkQuery *query = pWHBlockLinks->Query(objID, LINKOBJ_WILDCARD);
   for (; !query->Done(); query->Next())
      pWHBlockLinks->Remove(query->ID());
   SafeRelease(query);
}

////////////////////////////////////////

#endif





