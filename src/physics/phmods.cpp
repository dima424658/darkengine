/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phmods.cpp,v 1.22 1999/06/15 19:05:11 ccarollo Exp $
//
// Physics model lists
//
// Implementation is that all models are only on one list
// The moving list is just the head of the active list and we keep a pointer to the first
// stationary model on the active list.
//

#include <lg.h>
#include <phystyp2.h>
#include <allocapi.h>

#include <phcore.h>
#include <phmod.h>
#include <phmods.h>
#include <phprop.h>
#include <phmodsph.h>
#include <phmodbsp.h>
#include <phmodobb.h>
#include <objedit.h>

#include <iobjsys.h>
#include <appagg.h>

#include <mprintf.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////

void cPhysModels::Read(PhysReadWrite func, ulong /* partition */ )
{
   int i;
   int model_type;
   int nMoving, nStationary, nInactive;
   int nDeferred;
   cPhysSphereModel *pCurSphereModel;
   cPhysBSPModel *pCurBSPModel;
   cPhysOBBModel *pCurOBBModel;

   //   m_nMovingModels = m_nStationaryModels = m_nInactiveModels = 0;

   // Moving models
   func(&nMoving, sizeof(int), 1);
   for (i=0; i<nMoving; i++) 
   {
      func(&model_type, sizeof(int), 1);
      switch (model_type) {
         case kPMT_Point:
         case kPMT_Sphere:
         case kPMT_SphereHat:
         {
            pCurSphereModel = new cPhysSphereModel(func);
            
            AddToMoving(pCurSphereModel);

            break;
         }
         case kPMT_BSP:
         {
            pCurBSPModel = new cPhysBSPModel(func);
            AddToMoving(pCurBSPModel);
            break;
         }
         case kPMT_OBB:
         {
            pCurOBBModel = new cPhysOBBModel(func);
            AddToMoving(pCurOBBModel);
            break;
         }
         default:            
         {
            Error(1, "Invalid PhysModel type: %d\n", model_type);
            break;
         }
      }

      
   }

   // Stationary models
   func(&nStationary, sizeof(int), 1);
   for (i=0; i<nStationary; i++) {
      func(&model_type, sizeof(int), 1);
      switch (model_type) {
         case kPMT_Point:
         case kPMT_Sphere:  
         case kPMT_SphereHat:
         {
            pCurSphereModel = new cPhysSphereModel(func);
            AddToStationary(pCurSphereModel);
            break;
         }
         case kPMT_BSP:
         {
            pCurBSPModel = new cPhysBSPModel(func);
            AddToStationary(pCurBSPModel);
            break;
         }
         case kPMT_OBB:
         {
            pCurOBBModel = new cPhysOBBModel(func);
            AddToStationary(pCurOBBModel);
            break;
         }
         default:
         {
            Error(1, "Invalid PhysModel type: %d\n", model_type);
            break;
         }
      }
   }

   // Inactive models
   func(&nInactive, sizeof(int), 1);
   for (i=0; i<nInactive; i++) {
      func(&model_type, sizeof(int), 1);
      switch (model_type) {
         case kPMT_Point:
         case kPMT_Sphere:
         case kPMT_SphereHat:
         {
            pCurSphereModel = new cPhysSphereModel(func);
            AddToInactive(pCurSphereModel);
            break;
         }
         case kPMT_BSP:
         {
            pCurBSPModel = new cPhysBSPModel(func);
            AddToInactive(pCurBSPModel);
            break;
         }
         case kPMT_OBB:
         {
            pCurOBBModel = new cPhysOBBModel(func);
            AddToInactive(pCurOBBModel);
            break;
         }
         default:
         {
            Error(1, "Invalid PhysModel type: %d\n", model_type);
            break;
         }
      }
   }

   if (g_PhysVersion >= 23)
   {
      func(&nDeferred, sizeof(int), 1);
      LGALLOC_PUSH_CREDIT();
      m_deferredVelocityList.SetSize(nDeferred);
      LGALLOC_POP_CREDIT();
      for (i=0; i<nDeferred; i++)
         func(&m_deferredVelocityList[i], sizeof(sDeferredVelocity), 1);
   }
   else
      m_deferredVelocityList.SetSize(0);
}


void cPhysModels::Write(PhysReadWrite func, eObjPartition part)
{
   cPhysModel *pCurModel;
   int         model_type;
   int num_models; 
   int num_deferred;

   AutoAppIPtr_(ObjectSystem,pObjSys); 

   // Moving models

   // count the number of moving models we're actually saving 
   num_models = 0; 
   for (pCurModel = GetFirstMoving(); pCurModel != NULL; pCurModel = GetNextMoving(pCurModel)) 
   {
      if (pObjSys->IsObjSavePartition(pCurModel->GetObjID(),part))
         num_models++; 
   }


   func(&num_models, sizeof(int), 1);
   for (pCurModel = GetFirstMoving(); pCurModel != NULL; pCurModel = GetNextMoving(pCurModel)) 
   {
      if (!pObjSys->IsObjSavePartition(pCurModel->GetObjID(),part))
         continue; 

      // Write the model type so that it knows what's coming when reading
      // @TODO: This needs to be fixed when/if we mix submodel types
      model_type = (int)pCurModel->GetType(0);
      func(&model_type, sizeof(int), 1);
      // This will call the appropriate descendant's write function
      pCurModel->Write(func);
   }


   // Stationary models

   // count the number of stationary models we're actually saving 
   num_models = 0; 
   for (pCurModel = GetFirstStationary(); pCurModel != NULL; pCurModel = GetNextStationary(pCurModel)) 
   {
      if (pObjSys->IsObjSavePartition(pCurModel->GetObjID(),part))
         num_models++; 
   }

   func(&num_models, sizeof(int), 1);
   for (pCurModel = GetFirstStationary(); pCurModel != NULL; pCurModel = GetNextStationary(pCurModel)) 
   {
      if (!pObjSys->IsObjSavePartition(pCurModel->GetObjID(),part))
         continue; 

      model_type = (int)pCurModel->GetType(0);
      func(&model_type, sizeof(int), 1);
      pCurModel->Write(func);

   }

   // Inactive models

   // count how many...
   num_models = 0; 
   for (pCurModel = GetFirstInactive(); pCurModel != NULL; pCurModel = GetNextInactive(pCurModel)) 
   {
      if (pObjSys->IsObjSavePartition(pCurModel->GetObjID(),part))
         num_models++; 
   }

   func(&num_models, sizeof(int), 1);

   for (pCurModel = GetFirstInactive(); pCurModel != NULL; pCurModel = GetNextInactive(pCurModel)) 
   {
      if (!pObjSys->IsObjSavePartition(pCurModel->GetObjID(),part))
         continue; 

      model_type = (int)pCurModel->GetType(0);
      func(&model_type, sizeof(int), 1);
      pCurModel->Write(func);
   }

   num_deferred = m_deferredVelocityList.Size();
   func(&num_deferred, sizeof(int), 1);
   for (int i=0; i<num_deferred; i++)
      func(&m_deferredVelocityList[i], sizeof(sDeferredVelocity), 1);
}

///////////////////////////////////////////////////////////////////////////////

void cPhysModels::Remove(cPhysModel * pModel)
{
   DecNum(pModel);

   if (pModel->IsActive())
   {
      if (m_pFirstStationary == pModel)
         m_pFirstStationary = m_pFirstStationary->GetNext();
      m_activeList.Remove(pModel);
      m_activeTable.Remove(pModel);
   }
   else
   {
      m_inactiveList.Remove(pModel);
      m_inactiveTable.Remove(pModel);
   }
}

///////////////////////////////////////

void cPhysModels::DestroyAllInList(cPhysModelList * pList)
{
   cPhysModel * pModel;
   cPhysModel * pNextModel;

   pModel = pList->GetFirst();

   while (pModel)
   {
#if 0
//      if (pModel->IsHigh())
         g_pHighModelProperty->Delete(pModel->GetObjID());
//      else
         g_pLowModelProperty->Delete(pModel->GetObjID());
#endif

      pNextModel = pModel->GetNext();
      pList->Remove(pModel);
      delete pModel;
      pModel = pNextModel;
   }
}

///////////////////////////////////////

void cPhysModels::DestroyAll()
{
   m_activeTable.SetEmpty();
   m_inactiveTable.SetEmpty();
   DestroyAllInList(&m_activeList);
   DestroyAllInList(&m_inactiveList);
   m_nMovingModels = m_nStationaryModels = m_nInactiveModels = 0;
   m_pFirstStationary = NULL;
}

///////////////////////////////////////

void cPhysModels::AddToStationary(cPhysModel * pModel)
{
   ConfigSpew("ModelState", ("Object %d becomes stationary\n", pModel->GetObjID()));
   if (m_pFirstStationary == NULL)
      m_pFirstStationary = pModel;

   LGALLOC_PUSH_CREDIT();
   m_activeList.Append(pModel);
   LGALLOC_POP_CREDIT();

   m_activeTable.Insert(pModel);
   pModel->SetStationary(TRUE);
   m_nStationaryModels++;
}

///////////////////////////////////////

void cPhysModels::Deactivate(cPhysModel * pModel)
{
#ifdef DBG_ON
   if (!pModel->IsActive())
      Warning(("cPhysModels::Deactivate: model is not currently active\n"));
#endif
   if (m_pFirstStationary == pModel)
      m_pFirstStationary = m_pFirstStationary->GetNext();
   m_activeList.Remove(pModel);
   m_activeTable.Remove(pModel);
   DecNum(pModel);
   AddToInactive(pModel);
}

///////////////////////////////////////

void cPhysModels::ActivateToMoving(cPhysModel * pModel)
{
#ifdef DBG_ON
   if (pModel->IsActive())
      Warning(("cPhysModels::ActivateToMoving: model is already active\n"));
#endif
   m_inactiveList.Remove(pModel);
   m_inactiveTable.Remove(pModel);
   DecNum(pModel);
   AddToMoving(pModel);
}

///////////////////////////////////////

void cPhysModels::StopMoving(cPhysModel * pModel)
{
#ifdef DBG_ON
   if (!(pModel->IsActive()))
      Warning(("cPhysModels::StopMoving: model is not currently active\n"));
#endif
   m_activeList.Remove(pModel);
   m_activeTable.Remove(pModel);
   DecNum(pModel);
   AddToStationary(pModel);
}

///////////////////////////////////////

void cPhysModels::StartMoving(cPhysModel * pModel)
{
#ifdef DBG_ON
   if (!(pModel->IsActive()))
      Warning(("cPhysModels::StartMoving: model is not currently active\n"));
#endif
   if (m_pFirstStationary == pModel)
      m_pFirstStationary = m_pFirstStationary->GetNext();
   m_activeList.Remove(pModel);
   m_activeTable.Remove(pModel);
   DecNum(pModel);
   AddToMoving(pModel);
}

///////////////////////////////////////////////////////////////////////////////

void cPhysModels::AddDeferredVelocity(ObjID objID, const mxs_vector &velocity)
{
   sDeferredVelocity defer;

   defer.objID = objID;
   defer.submod = -1;
   defer.velocity = velocity;

   LGALLOC_AUTO_CREDIT();
   m_deferredVelocityList.Append(defer);
}

////////////////////////////////////////

void cPhysModels::AddDeferredVelocity(ObjID objID, int subModId, const mxs_vector &velocity)
{
   sDeferredVelocity defer;

   defer.objID = objID;
   defer.submod = subModId;
   defer.velocity = velocity;

   LGALLOC_AUTO_CREDIT();
   m_deferredVelocityList.Append(defer);
}

////////////////////////////////////////

void cPhysModels::ApplyDeferredVelocities()
{
   cPhysModel *pModel;
   mxs_vector  velocity;

   for (int i=0; i<m_deferredVelocityList.Size(); i++)
   {
      pModel = g_PhysModels.Get(m_deferredVelocityList[i].objID);

      if (pModel == NULL)
      {
         Warning(("ApplyDeferredVelocities: %s has no physics model\n", ObjWarnName(m_deferredVelocityList[i].objID)));
         continue;
      }

      mx_copy_vec(&velocity, &pModel->GetVelocity(m_deferredVelocityList[i].submod));
      mx_addeq_vec(&velocity, &m_deferredVelocityList[i].velocity);
      pModel->GetDynamics(m_deferredVelocityList[i].submod)->SetVelocity(velocity);

      pModel->SetSleep(FALSE);
   }

   m_deferredVelocityList.SetSize(0);
}

///////////////////////////////////////////////////////////////////////////////

#ifndef SHIP

void cPhysModels::SpewStats()
{
   cPhysModel *pModel;

   long total_size;

   // Sum sizes of main structures
   long contact_size = 0;
   long external_size = 0;
   long internal_size = 0;

   int obb_count = 0;
   int sphere_count = 0;

   int model_count = 0;

   pModel = m_activeList.GetFirst();
   while (pModel)
   {
      switch (pModel->GetType(0))
      {
         case kPMT_Sphere:
         case kPMT_Point:
         case kPMT_SphereHat:
         {
            sphere_count++;
            external_size += sizeof(cPhysSphereModel);
            internal_size += ((cPhysSphereModel *)pModel)->InternalSize();
            break;
         }
         case kPMT_OBB:
         {
            obb_count++;
            external_size += sizeof(cPhysOBBModel);
            internal_size += ((cPhysOBBModel *)pModel)->InternalSize();
            break;
         }
      }

      model_count++;
      pModel = pModel->GetNext();
   }

   pModel = m_inactiveList.GetFirst();
   while (pModel)
   {
      switch (pModel->GetType(0))
      {
         case kPMT_Sphere:
         case kPMT_Point:
         case kPMT_SphereHat:
         {
            sphere_count++;
            external_size += sizeof(cPhysSphereModel);
            internal_size += ((cPhysSphereModel *)pModel)->InternalSize();
            break;
         }
         case kPMT_OBB:
         {      
            obb_count++;
            external_size += sizeof(cPhysOBBModel);
            internal_size += ((cPhysOBBModel *)pModel)->InternalSize();
            break;
         }
      }

      model_count++;
      pModel = pModel->GetNext();
   }

   // Total it up
   total_size = external_size + internal_size;

   mprintf(" Models: %d (%d sphere, %d obb)\n", model_count, sphere_count, obb_count);
   mprintf("   external: %dk\n", external_size >> 10);
   mprintf("   internal: %dk\n", internal_size >> 10);
   mprintf("Total:  %dk\n", total_size >> 10);
}

void cPhysModels::HackListRemove(cPhysModel *pModel)
{
   if (pModel->IsActive())
   {
      if (m_pFirstStationary == pModel)
         m_pFirstStationary = m_pFirstStationary->GetNext();
      m_activeList.Remove(pModel);
   }
   else
      m_inactiveList.Remove(pModel);
}

#endif

///////////////////////////////////////////////////////////////////////////////







