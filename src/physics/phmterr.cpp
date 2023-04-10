/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phmterr.cpp,v 1.26 1999/08/12 16:30:33 Justin Exp $
//
// Physics moving terrain
//

#include <lg.h>
#include <appagg.h>
#include <comtools.h>

#include <relation.h>
#include <linkman.h>
#include <lnkquery.h>
#include <linkbase.h>

#include <property.h>
#include <propbase.h>

#include <objpos.h>
#include <editobj.h>

#include <aipathdb.h>

#include <physapi.h>
#include <phmods.h>
#include <phmod.h>
#include <phcore.h>
#include <phprop.h>

#include <scrptapi.h>
#include <scrptbas.h>
#include <scrptsrv.h>

#include <netman.h>
#include <iobjnet.h>

#include <phmtprop.h>
#include <phmtscpt.h>
#include <phmterr.h>

// Must be last header
#include <dbmem.h>

static BOOL TerrainHitWaypoint(ObjID objID);

////////////////////////////////////////////////////////////////////////////////

void ResetMovingTerrain()
{
   AutoAppIPtr_(LinkManager, pLinkMan);

   IRelation *pPathNextRel = pLinkMan->GetRelationNamed(TERRAIN_PATH_NEXT_NAME);
   ILinkQuery *query = pPathNextRel->Query(LINKOBJ_WILDCARD, LINKOBJ_WILDCARD);

   for (; !query->Done(); query->Next())
      pPathNextRel->Remove(query->ID());

   SafeRelease(query);
   SafeRelease(pPathNextRel);

   RefreshMovingTerrain();
}

////////////////////////////////////////

void RefreshMovingTerrain()
{
   Assert_(g_pMovingTerrainProperty);

   cPhysModel *pModel;
   sPropertyObjIter iter;
   cMovingTerrainProp *pMTProp;
   ObjID objID;

   g_pMovingTerrainProperty->IterStart(&iter);
   while (g_pMovingTerrainProperty->IterNext(&iter, &objID))
   {
      g_pMovingTerrainProperty->Get(objID, &pMTProp);

      if (pMTProp->active)
      {
         DeactivateMovingTerrain(objID);
         ActivateMovingTerrain(objID);
      }
      else
      {
         ActivateMovingTerrain(objID);
         DeactivateMovingTerrain(objID);
      }

      pModel = g_PhysModels.Get(objID);
      if (pModel != NULL)
         pModel->GetDynamics()->SetMass(100);

   }
   g_pMovingTerrainProperty->IterStop(&iter);
}

////////////////////////////////////////

void PostLoadMovingTerrain()
{
   sPropertyObjIter iter;
   ObjID objID;

   AutoAppIPtr_(LinkManager, pLinkMan);
   IRelation *pPathNextRel = pLinkMan->GetRelationNamed(TERRAIN_PATH_NEXT_NAME);
   IRelation *pPathRel = pLinkMan->GetRelationNamed(TERRAIN_PATH_NAME);
   IRelation *pPathInitRel = pLinkMan->GetRelationNamed(TERRAIN_PATH_INIT_NAME);

   g_pMovingTerrainProperty->IterStart(&iter);
   while (g_pMovingTerrainProperty->IterNext(&iter, &objID))
   {
      if (!IsMovingTerrainActive(objID))
         continue;

      cPhysModel *pModel = g_PhysModels.Get(objID);

      if (pModel != NULL)
      {
         LinkID id = pPathNextRel->GetSingleLink(objID,LINKOBJ_WILDCARD); 

         if (id != LINKID_NULL)
         {
            sLink link; 
            pPathNextRel->Get(id,&link);

            LinkID pathID = pPathRel->GetSingleLink(LINKOBJ_WILDCARD, link.dest);
            sTerrainPath *pTerrainPath = (sTerrainPath *)pLinkMan->GetData(pathID);

            pModel->AddTransLimit(ObjPosGet(link.dest)->loc.vec, TerrainHitWaypoint);
            if (pTerrainPath)
               UpdateMovingTerrainVelocity(objID, link.dest, pTerrainPath->speed);
         }

         // update AI on current status of mt
         // pModel != NULL above should filter out abstract objects
         if (!IsMovingTerrainActive(objID))
         {
            // not active, so look at the next waypoint
            LinkID tPathNextID = pPathNextRel->GetSingleLink(objID,LINKOBJ_WILDCARD); 
            if (tPathNextID == LINKID_NULL)
            {
               Warning(("PostLoadMovingTerrain: %d has no TPathNext link\n", objID));
               continue;
            }
            sLink link; 
            pPathNextRel->Get(tPathNextID,&link);
            ObjID nextWaypointID = link.dest;
            
            // now find (guess) the current waypoint by going back along tpath link
            // this will break in the case that multiple tpath links converge on the waypoint
            LinkID tPathID = pPathRel->GetSingleLink(LINKOBJ_WILDCARD, nextWaypointID);
            if (tPathID == LINKID_NULL)
            {
               Warning(("PostLoadMovingTerrain: %d has no ~TPath link\n", nextWaypointID));
               continue;
            }
            pPathRel->Get(tPathID, &link);
            ObjID currentWaypointID = link.source;
            
            // now get the initial waypoint on this path
            LinkID tPathInitID = pPathInitRel->GetSingleLink(objID,LINKOBJ_WILDCARD); 
            if (tPathInitID == LINKID_NULL)
            {
               Warning(("PostLoadMovingTerrain: %d has no TPathInit link\n", objID));
               continue;
            }
            pPathInitRel->Get(tPathInitID, &link);
            ObjID initWaypointID = link.dest;
            
            // if we're at the initial waypoint, everything's hunky dory
            if (currentWaypointID == initWaypointID)
               continue;
            // otherwise, tell the AI we're moving (to break initial links)
            AIPathFindMTWaypointLeft(objID);
            // then tell the AI that we've arrived at the current waypoint
            AIPathFindMTWaypointHit(objID, currentWaypointID);
         }
         else
         {
            // active, so just need to let the AI know that
            AIPathFindMTWaypointLeft(objID);
         }
      }
   }
}

////////////////////////////////////////

void UpdateMovingTerrainVelocity(ObjID objID, ObjID next_node, float speed)
{
   cPhysModel *pModel = g_PhysModels.Get(objID);
   mxs_vector direction;
   
   if (pModel == NULL)
      return;

   mx_sub_vec(&direction, &ObjPosGet(next_node)->loc.vec, &ObjPosGet(objID)->loc.vec);

   if (mx_mag2_vec(&direction) > 0.0001)
   {
      mx_normeq_vec(&direction);
      mx_scaleeq_vec(&direction, speed);
   }
   else
      mx_zero_vec(&direction);

   pModel->ClearTransLimits();
   pModel->AddTransLimit(ObjPosGet(next_node)->loc.vec, TerrainHitWaypoint);
   pModel->GetControls()->SetControlVelocity(direction);
}

////////////////////////////////////////

BOOL IsMovingTerrain(ObjID objID)
{
   cPhysModel *pModel = g_PhysModels.Get(objID);

   return (pModel && pModel->IsMovingTerrain());
}

////////////////////////////////////////////////////////////////////////////////

void UpdateMovingTerrain(int ms)
{
   Assert_(g_pMovingTerrainProperty);

   AutoAppIPtr_(LinkManager, pLinkMan);
   ILinkQuery *nextQuery, *query;
   sLink nextLink;
   sLink link;
   ObjID cur_node;
   ObjID next_node;

   IRelation *pPathRel = pLinkMan->GetRelationNamed(TERRAIN_PATH_NAME);
   IRelation *pPathNextRel = pLinkMan->GetRelationNamed(TERRAIN_PATH_NEXT_NAME);
 
   if (pPathNextRel != NULL)
   {
      nextQuery = pPathNextRel->Query(LINKOBJ_WILDCARD, LINKOBJ_WILDCARD);

      for (; !nextQuery->Done(); nextQuery->Next())
      {
         nextQuery->Link(&nextLink);

         if (!IsMovingTerrainActive(nextLink.source))
            continue;

         // Find link TO this node
         if (pPathRel != NULL)
         {
            query = pPathRel->Query(LINKOBJ_WILDCARD, nextLink.dest);

            if (!query->Done())
            {
               sTerrainPath *pTerrainPath = (sTerrainPath *)pLinkMan->GetData(query->ID());
               query->Link(&link);

               cur_node = link.source;
               next_node = link.dest;

               if (pTerrainPath->cur_paused >= 0)
               {
                  pTerrainPath->cur_paused += ms;

                  // Done pausing, go go go
                  if (pTerrainPath->cur_paused >= pTerrainPath->pause)
                  {
                     cPhysModel *pModel = g_PhysModels.Get(nextLink.source);
                     if (pModel != NULL)
                     {
                        mxs_vector direction;

                        mx_sub_vec(&direction, &ObjPosGet(next_node)->loc.vec, &ObjPosGet(nextLink.source)->loc.vec);

                        if (mx_mag2_vec(&direction) > 0.01)
                        {
                           mx_normeq_vec(&direction);
                           mx_scaleeq_vec(&direction, pTerrainPath->speed);

                           pModel->GetControls()->SetControlVelocity(direction);
                           pModel->ClearTransLimits();
                           pModel->AddTransLimit(ObjPosGet(next_node)->loc.vec, TerrainHitWaypoint);

                           AIPathFindMTWaypointLeft(nextLink.source);
                        }

                        pTerrainPath->cur_paused = -1;
                     }
                     else
                        Warning(("UpdateMovingTerrain: obj %d not physical?\n", nextLink.source));
                  }

                  pLinkMan->SetData(query->ID(), (void *)pTerrainPath);
               }
            }
            else
               Warning(("UpdateMovingTerrain: no link TO node %d?\n", nextLink.dest));

            SafeRelease(query);
         }
      }
        
      SafeRelease(nextQuery);
   }

   SafeRelease(pPathNextRel);
   SafeRelease(pPathRel);
}

////////////////////////////////////////

static BOOL InternalTerrainHitWaypoint(ObjID objID)
{
   Assert_(g_pMovingTerrainProperty);

   AutoAppIPtr_(LinkManager, pLinkMan);
   ILinkQuery *query;
   sLink  link;
   LinkID linkID;
   LinkID nextLinkID;
   ObjID  cur_node = -1;
   ObjID  next_node = -1;
   BOOL   hard_limit = TRUE;

   IRelation *pPathRel = pLinkMan->GetRelationNamed(TERRAIN_PATH_NAME);
   IRelation *pPathNextRel = pLinkMan->GetRelationNamed(TERRAIN_PATH_NEXT_NAME);

   if (pPathNextRel != NULL)
   {
      // Get current path node
      query = pPathNextRel->Query(objID, LINKOBJ_WILDCARD);

      if (!query->Done())
      {
         query->Link(&link);
         nextLinkID = query->ID();

         cur_node = link.dest;
      }
      else
         Warning(("Moving terrain obj %d does not have a next link?\n", objID));

      SafeRelease(query);

      // Get next path node
      query = pPathRel->Query(cur_node, LINKOBJ_WILDCARD);

      if (!query->Done())
      {
         query->Link(&link);
         linkID = query->ID();

         next_node = link.dest;
      }

      SafeRelease(query);

   }

   // @TODO: probably want installable callbacks
   AIPathFindMTWaypointHit(objID, cur_node);

   pPathNextRel->Remove(nextLinkID);

   cPhysModel *pModel = g_PhysModels.Get(objID);      
   if (pModel != NULL)
   {
      // Got the link and both ends
      if ((cur_node != -1) && (next_node != -1))
      {
         pPathNextRel->Add(objID, next_node);

         sTerrainPath *pTerrainPath = (sTerrainPath *)pLinkMan->GetData(linkID);

         if (IsMovingTerrainActive(objID))
         {
            // Change movement to toward next node
            if (pTerrainPath != NULL)
            {
               hard_limit = pTerrainPath->path_limit;
               
               if (pTerrainPath->pause > 0)
               {
                  pTerrainPath->cur_paused = 0;
                  
                  // Let go
                  pModel->GetControls()->StopControlVelocity();
                  pModel->ClearTransLimits();
               }
               else
               {
                  mxs_vector direction;
                  
                  mx_sub_vec(&direction, &ObjPosGet(next_node)->loc.vec, &ObjPosGet(objID)->loc.vec);

                  if (mx_mag2_vec(&direction) > 0.01)
                  {
                     mx_normeq_vec(&direction);
                     mx_scaleeq_vec(&direction, pTerrainPath->speed);
                  
                     pModel->GetControls()->SetControlVelocity(direction);
                     pModel->ClearTransLimits();
                     pModel->AddTransLimit(ObjPosGet(next_node)->loc.vec, TerrainHitWaypoint);

                     // @TODO: Probably want installable callbacks
                     AIPathFindMTWaypointLeft(objID);
                  }    

                  pTerrainPath->cur_paused = -1;
               }
               
               pLinkMan->SetData(linkID, (void *)pTerrainPath);
            }
            else
               Warning(("No terrain path data on terrain path link id %d?\n", linkID));
         }
         else
         {
            if (pTerrainPath)
               hard_limit = pTerrainPath->path_limit;
            pModel->GetControls()->StopControlVelocity();
            pModel->ClearTransLimits();
         }
      }
      else
      {
         // Let go 
         pModel->GetControls()->StopControlVelocity();
         pModel->ClearTransLimits();
      }
   }
   else
      Warning(("No physics on moving terrain obj %d\n", objID));

   SafeRelease(pPathRel);
   SafeRelease(pPathNextRel);

   UpdatePhysProperty(objID, PHYS_STATE | PHYS_CONTROL);

   // No hard limits
   return hard_limit;
}

////////////////////////////////////////

static void SendTerrainWaypointMessages(ObjID objID, IRelation *pPathNextRel)
{
   AutoAppIPtr(ScriptMan);

   ILinkQuery *query = pPathNextRel->Query(objID, LINKOBJ_WILDCARD);

   if (!query->Done())
   {
      sLink link;
      query->Link(&link);
      
      sMovingTerrainMsg moving_terrain_message(objID, link.dest);
      pScriptMan->SendMessage(&moving_terrain_message);
      
      sWaypointMsg waypoint_message(objID, link.dest);
      pScriptMan->SendMessage(&waypoint_message);
   }

   SafeRelease(query);
}

////////////////////////////////////////

static BOOL TerrainHitWaypoint(ObjID objID)
{
   AutoAppIPtr(LinkManager);

   IRelation *pPathNextRel = pLinkManager->GetRelationNamed(TERRAIN_PATH_NEXT_NAME);

   if (pPathNextRel != NULL)
   {
      SendTerrainWaypointMessages(objID, pPathNextRel);
   }

   SafeRelease(pPathNextRel);
 
   return InternalTerrainHitWaypoint(objID);
}

////////////////////////////////////////////////////////////////////////////////

void ActivateMovingTerrain(ObjID objID)
{
   AutoAppIPtr_(LinkManager, pLinkMan);

   if (!OBJ_IS_CONCRETE(objID))
      return;

   IRelation *pPathInitRel = pLinkMan->GetRelationNamed(TERRAIN_PATH_INIT_NAME);
   IRelation *pPathNextRel = pLinkMan->GetRelationNamed(TERRAIN_PATH_NEXT_NAME);
   IRelation *pPathRel = pLinkMan->GetRelationNamed(TERRAIN_PATH_NAME);

   ILinkQuery *pNextQuery;
   ILinkQuery *pInitQuery;
   ILinkQuery *pQuery;

   ObjID from_node = -1;
   ObjID to_node = -1;

   BOOL  initializing = FALSE;

   pNextQuery = pPathNextRel->Query(objID, LINKOBJ_WILDCARD);

   // Find the "from" node
   if (pNextQuery->Done())
   {
      initializing = TRUE;

      // Initializing
      pInitQuery = pPathInitRel->Query(objID, LINKOBJ_WILDCARD);

      if (!pInitQuery->Done())
      {
         sLink link;
         pInitQuery->Link(&link);
      
         from_node = link.dest;
         to_node = link.dest;

         // Update next link
         pPathNextRel->Add(objID, link.dest);
      }
      else
         Warning(("No init link for moving terrain obj %d\n", objID));

      SafeRelease(pInitQuery);
   }
   else
   {
      // Next already exists, but we have to back up one along path to find from node
      sLink link;
      pNextQuery->Link(&link);

      pQuery = pPathRel->Query(LINKOBJ_WILDCARD, link.dest);

      to_node = link.dest;

      if (!pQuery->Done())
      {
         pQuery->Link(&link);

         from_node = link.source;

         // Update next link
         pPathNextRel->Remove(pNextQuery->ID());
         pPathNextRel->Add(objID, link.source);
      }
      else
         Warning(("Moving terrain moving toward link with no previous path point\n"));

      SafeRelease(pQuery);
   }

   SafeRelease(pNextQuery);

   if (from_node != -1)
   {
      if (initializing)
      {
         PhysSetModLocation(objID, &ObjPosGet(from_node)->loc.vec);
         ObjPosUpdate(objID, &ObjPosGet(from_node)->loc.vec, &ObjPosGet(from_node)->fac);
      }
      InternalTerrainHitWaypoint(objID);      
   }
   else
      Warning(("Unable to activate moving terrain obj %d\n", objID));

   UpdatePhysProperty(objID, PHYS_STATE | PHYS_CONTROL);

   SafeRelease(pPathRel);
   SafeRelease(pPathNextRel);
   SafeRelease(pPathInitRel);
}

////////////////////////////////////////

void DeactivateMovingTerrain(ObjID objID)
{
   cPhysModel *pModel = g_PhysModels.Get(objID);

   // Let go physically
   if (pModel != NULL)
   {
      pModel->GetControls()->StopControlVelocity();
      pModel->ClearTransLimits();
   }

   UpdatePhysProperty(objID, PHYS_STATE | PHYS_CONTROL);
}













