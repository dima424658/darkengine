/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phclsn.cpp,v 1.27 1998/10/26 23:50:56 CCAROLLO Exp $
//
// Collisions
//

#include <lg.h>
#include <allocapi.h>
#include <matrixs.h>
#include <config.h>
#include <objedit.h>

#include <sphrcsts.h>

#include <phcore.h>
#include <phclsn.h>
#include <phterr.h>
#include <phmodobb.h>
#include <phmodsph.h>

#include <mprintf.h>
#include <wrdbrend.H>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysClsn
//

cPhysClsn::cPhysClsn(cPhysClsn *pClsn)
{
   type = pClsn->GetType();
   result = pClsn->GetResult();
   t0 = pClsn->GetT0();
   dt = pClsn->GetDT();
   clsn_pt = pClsn->GetClsnPt();

   InitSubModelInstance(&instance, pClsn->GetSubModId(), pClsn->GetModel(), pClsn->GetLoc());

   if (pClsn->GetType() & kPC_Object)
   {
      pData = (void *) new sPhysSubModInst;
      InitSubModelInstance((sPhysSubModInst *)pData, pClsn->GetSubModId2(), pClsn->GetModel2(), pClsn->GetLoc2());
   }
   else
   {
      pData = (void *) new sPhysClsnTerr;

      cPhysTerrPoly *pTerrPoly = pClsn->GetTerrainPoly(0);

      while (pTerrPoly)
      {
         cPhysTerrPoly *pNewTerrPoly;

         if (pClsn->GetType() == kPC_TerrainVertex)
            pNewTerrPoly = new cPhysTerrVertex((cPhysTerrVertex *)pTerrPoly);
         else 
            pNewTerrPoly = new cPhysTerrPoly(pTerrPoly);

         LGALLOC_AUTO_CREDIT();
         ((sPhysClsnTerr *)pData)->m_polyList.Append(pNewTerrPoly);
         pTerrPoly = pTerrPoly->GetNext();
      }

      ((sPhysClsnTerr *)pData)->m_normal = pClsn->GetNormal();
   }
}

////////////////////////////////////////

cPhysClsn::cPhysClsn(ePhysClsnType _type, mxs_real _t0, mxs_real _dt,
                     const cPhysSphereModel * pSphereModel1, tPhysSubModId subModId1,
                     const mxs_vector & locVec1,
                     const cPhysSphereModel * pSphereModel2, tPhysSubModId subModId2,
                     const mxs_vector & locVec2)
{
   type   = _type;
   result = kPCR_Nothing;
   t0     = _t0;
   dt     = _dt;

   Assert_(type & kPC_Object);

   InitSubModelInstance(&instance, subModId1, (cPhysModel *)pSphereModel1, locVec1);
   pData = (void *) new sPhysSubModInst;
   InitSubModelInstance((sPhysSubModInst *) pData, subModId2, (cPhysModel *)pSphereModel2, locVec2);

   GenerateClsnPt();
}


cPhysClsn::cPhysClsn(ePhysClsnType _type, mxs_real _t0, mxs_real _dt,
                     const cPhysSphereModel * pSphereModel, tPhysSubModId subModId1, 
                     const mxs_vector & locVec1,
                     const cPhysOBBModel * pOBBModel, int side, 
                     const mxs_vector & locVec2)
{
   type   = (ePhysClsnType)(_type | kPC_OBBObject);
   result = kPCR_Nothing;
   t0     = _t0;
   dt     = _dt;

   Assert_(type & kPC_Object);

   InitSubModelInstance(&instance, subModId1, (cPhysModel *)pSphereModel, locVec1);
   pData = (void *) new sPhysSubModInst;
   InitSubModelInstance((sPhysSubModInst *) pData, side, (cPhysModel *)pOBBModel, locVec2);

   GenerateClsnPt();
}

///////////////////////////////////////

inline void cPhysClsn::CreateTerrainInfo(ePhysClsnType type, sSphrContactData * pContactData,
                                         cPhysTerrPolyList * pList)
{
   cPhysTerrPoly *pTerrPoly;
   if (type == kPC_TerrainVertex)
      pTerrPoly = new cPhysTerrVertex(pContactData->cell_index,
                                        pContactData->polygon_index,
                                        pContactData->vertex_offset,
                                        pContactData->vertex_in_poly);
   else
      pTerrPoly = new cPhysTerrPoly(pContactData->cell_index,
                                      pContactData->polygon_index);
   LGALLOC_AUTO_CREDIT();
   pList->Append(pTerrPoly);
}

///////////////////////////////////////

cPhysClsn::cPhysClsn(ePhysClsnType _type, mxs_real _t0, mxs_real _dt, const mxs_vector &_clsn_pt,
                     const cPhysModel * pModel, tPhysSubModId subModId,
                     const mxs_vector & locVec, sSphrContactData aContactData[],
                     int contactID)
{
   type   = _type;
   result = kPCR_Nothing;
   t0     = _t0;
   dt     = _dt;
   mx_copy_vec((mxs_vector *) &clsn_pt, (mxs_vector *) &_clsn_pt);

   sSphrContactData * pContactData;
   sPhysClsnTerr *    pClsnTerr;

   #ifndef SHIP
   int count = 0;
   #endif

   AssertMsg1((type & kPC_Terrain), "Invalid Terrain Collision Type: %d", type);

   InitSubModelInstance(&instance, subModId, pModel, locVec);

   // generate terrain poly/vertex list
   pClsnTerr    = new sPhysClsnTerr;
   pData        = (void *) pClsnTerr;
   pContactData = &aContactData[contactID];

   CreateTerrainInfo(type, pContactData, &pClsnTerr->m_polyList);

   while (pContactData->next != NO_NEXT_CONTACT_DATA)
   {
      pContactData = &aContactData[pContactData->next];
      CreateTerrainInfo(type, pContactData, &pClsnTerr->m_polyList);
      #ifndef SHIP
      count++;
      #endif
   }

   #ifndef SHIP
   if ((_type == kPC_TerrainEdge) && (count != 1))
      mprintf("Bad number of edge contacts: %d (%s)\n", count + 1, ObjWarnName(pModel->GetObjID()));
   if ((_type == kPC_TerrainVertex) &&  (count < 2))
      mprintf("Not enough vertex contacts: %d (%s)\n", count + 1, ObjWarnName(pModel->GetObjID()));
   #endif

   pClsnTerr->m_polyList.GetNormal(pClsnTerr->m_normal, locVec, ((cPhysSphereModel *)pModel)->GetRadius(subModId));
}

///////////////////////////////////////

cPhysClsn::cPhysClsn(ePhysClsnType _type, mxs_real _t0, mxs_real _dt, const mxs_vector & _clsn_pt,
                     const cPhysModel * pModel, tPhysSubModId subModId, 
                     const mxs_vector & locVec, int cell_id, int poly_id)
{
   cPhysTerrPoly * pTerrPoly;

   type   = _type;
   result = kPCR_Nothing;
   t0     = _t0;
   dt     = _dt;
   mx_copy_vec((mxs_vector *) &clsn_pt, (mxs_vector *) &_clsn_pt);

   sPhysClsnTerr * pClsnTerr;

   Assert_(type & kPC_Terrain);
  
   InitSubModelInstance(&instance, subModId, pModel, locVec);

   pClsnTerr = new sPhysClsnTerr;
   pData     = (void *) pClsnTerr;

   pTerrPoly = new cPhysTerrPoly(cell_id, poly_id);
   LGALLOC_PUSH_CREDIT();
   pClsnTerr->m_polyList.Append(pTerrPoly);
   LGALLOC_POP_CREDIT();

   pClsnTerr->m_polyList.GetNormal(pClsnTerr->m_normal, locVec, ((cPhysSphereModel *)pModel)->GetRadius(subModId));
}

///////////////////////////////////////

cPhysClsn::~cPhysClsn(void)
{
   if (type & kPC_Object)
      delete(sPhysSubModInst *) pData;
   else if (type & kPC_Terrain)
   {
      sPhysClsnTerr * pClsnTerr = (sPhysClsnTerr *)pData;
      while (pClsnTerr->m_polyList.GetFirst())
           delete pClsnTerr->m_polyList.Remove(pClsnTerr->m_polyList.GetFirst());

      delete pClsnTerr;
   }
}

///////////////////////////////////////////////////////////////////////////////

cPhysTerrPoly *cPhysClsn::GetTerrainPoly(int index) const
{
   int i = 0;
   cPhysTerrPoly *pTerrPoly;

   Assert_(type & kPC_Terrain);
  
   pTerrPoly = ((sPhysClsnTerr *)pData)->m_polyList.GetFirst();
   while ((i < index) && (pTerrPoly != NULL))
   {
      pTerrPoly = pTerrPoly->GetNext();
      i++;
   }

   AssertMsg1(pTerrPoly, "Invalid poly index: %d", index);

   return pTerrPoly;
}

///////////////////////////////////////////////////////////////////////////////
      
int cPhysClsn::GetCellID(int index) const
{
   int i = 0;
   cPhysTerrPoly *pTerrPoly;

   Assert_(type & kPC_Terrain);

   pTerrPoly = ((sPhysClsnTerr *)pData)->m_polyList.GetFirst();
   while ((i < index) && (pTerrPoly != NULL))
   {
      pTerrPoly = pTerrPoly->GetNext();
      i++;
   }

   AssertMsg1(pTerrPoly, "Invalid poly index: %d", index);

   return pTerrPoly->GetCellID();
}

///////////////////////////////////////////////////////////////////////////////

int cPhysClsn::GetPolyID(int index) const
{
   int i = 0;
   cPhysTerrPoly *pTerrPoly;

   Assert_(type & kPC_Terrain);

   pTerrPoly = ((sPhysClsnTerr *)pData)->m_polyList.GetFirst();
   while ((i < index) && (pTerrPoly != NULL))
   {
      pTerrPoly = pTerrPoly->GetNext();
      i++;
   }

   AssertMsg1(pTerrPoly, "Invalid poly index: %d", index);

   return pTerrPoly->GetPolyID();
}

///////////////////////////////////////////////////////////////////////////////

int cPhysClsn::GetTexture() const
{
   return WR_CELL(GetCellID())->render_list[GetPolyID()].texture_id; 
}

///////////////////////////////////////////////////////////////////////////////

const mxs_vector cPhysClsn::GetNormal(int index) const
{
   Assert_(type & kPC_Terrain);

   if (index == -1)
   {
      return ((sPhysClsnTerr *)pData)->m_normal;
   }
   else
   {
      cPhysTerrPoly *pTerrPoly;
      int i = 0;

      pTerrPoly = ((sPhysClsnTerr *)pData)->m_polyList.GetFirst();
      while ((i < index) && (pTerrPoly != NULL))
      {
         pTerrPoly = pTerrPoly->GetNext();
         i++;
      }

      AssertMsg1(pTerrPoly, "Invalid poly index: %d", index);

      return pTerrPoly->GetNormal();
   }
}

///////////////////////////////////////////////////////////////////////////////

void cPhysClsn::GenerateClsnPt()
{
   if (type & kPC_Terrain)
      return;
 
   cPhysModel *pModel1 = GetModel();
   cPhysModel *pModel2 = GetModel2();

   ePhysModelType type1 = pModel1->GetType(0);
   ePhysModelType type2 = pModel2->GetType(0);

   tPhysSubModId submod;

   if ((type1 == kPMT_OBB) || (type2 == kPMT_OBB))
   {
      cPhysOBBModel    *pOBBModel;
      cPhysSphereModel *pSphereModel;

      if (type1 == kPMT_OBB)
      {
         pOBBModel = (cPhysOBBModel *)pModel1;
         pSphereModel = (cPhysSphereModel *)pModel2;
         submod = GetSubModId2();
      }
      else
      {
         pOBBModel = (cPhysOBBModel *)pModel2;
         pSphereModel = (cPhysSphereModel *)pModel1;
         submod = GetSubModId();
      }

      mxs_vector dir;

      mx_sub_vec(&dir, &pOBBModel->GetLocationVec(), &pSphereModel->GetLocationVec(submod));
      mx_normeq_vec(&dir);
      mx_scaleeq_vec(&dir, pSphereModel->GetRadius(submod));

      mx_add_vec(&clsn_pt, &pSphereModel->GetLocationVec(submod), &dir);
   }
   else
   if (((GetSubModId() == 1) && (type1 == kPMT_SphereHat)) ||
       ((GetSubModId2() == 1) && (type2 == kPMT_SphereHat)))
   {
      cPhysSphereModel *pSphereModel;
      cPhysSphereModel *pSphereHatModel;

      if (type1 == kPMT_SphereHat)
      {
         pSphereModel = (cPhysSphereModel *)pModel2;
         pSphereHatModel = (cPhysSphereModel *)pModel1;
         submod = GetSubModId2();
      }
      else
      {
         pSphereModel = (cPhysSphereModel *)pModel1;
         pSphereHatModel = (cPhysSphereModel *)pModel2;
         submod = GetSubModId();
      }

      mxs_vector dir;

      mx_sub_vec(&dir, &pSphereHatModel->GetLocationVec(), &pSphereModel->GetLocationVec(submod));
      mx_normeq_vec(&dir);
      mx_scaleeq_vec(&dir, pSphereModel->GetRadius(submod));

      mx_add_vec(&clsn_pt, &pSphereModel->GetLocationVec(submod), &dir);
   }
   else
   {
      Assert_(((type1 == kPMT_Sphere) || (type1 == kPMT_Point) || ((type1 == kPMT_SphereHat) && (GetSubModId() == 0))) && 
              ((type2 == kPMT_Sphere) || (type2 == kPMT_Point) || ((type2 == kPMT_SphereHat) && (GetSubModId2() == 0))));

      mxs_vector dir;

      mx_sub_vec(&dir, &pModel1->GetLocationVec(GetSubModId()), &pModel2->GetLocationVec(GetSubModId2()));
      mx_normeq_vec(&dir);
      mx_scaleeq_vec(&dir, ((cPhysSphereModel *)pModel2)->GetRadius(GetSubModId2()));

      mx_add_vec(&clsn_pt, &pModel2->GetLocationVec(GetSubModId2()), &dir);
   }
}

///////////////////////////////////////////////////////////////////////////////

void cPhysClsns::Insert(cPhysClsn *pNewClsn)
{
   cPhysClsn *pClsn;
   cPhysClsn *pLastClsn = NULL;

   pClsn = g_PhysClsns.GetFirst();

   if (pClsn != NULL)
   {
      // Advance pointer till just past insertion point
      while ((pClsn != NULL) && (pNewClsn->GetTime() > pClsn->GetTime()))
      {
         pLastClsn = pClsn;
         pClsn = pClsn->GetNext();
      }

      if (pClsn != NULL) 
         // Insert before "just past" value 
         g_PhysClsns.InsertBefore(pClsn, pNewClsn);
      else
         // Or after the previous one, if we hit the end of the list
         g_PhysClsns.InsertAfter(pLastClsn, pNewClsn);
   }
   else
      // Only thing in list, so just prepend it
      g_PhysClsns.Prepend(pNewClsn);
}

///////////////////////////////////////////////////////////////////////////////

void cPhysClsns::Sort(void)
{
   BOOL       sorted = FALSE;
   cPhysClsn *pClsn;
   cPhysClsn *pNextClsn = NULL;

   while (!sorted)
   {
      sorted = TRUE;
      pClsn = g_PhysClsns.GetFirst();

      if (pClsn != NULL)
         pNextClsn = pClsn->GetNext();

      while (pNextClsn != NULL)
      {
         if (pClsn->GetTime() > pNextClsn->GetTime())
         {
            g_PhysClsns.Remove(pNextClsn);
            g_PhysClsns.InsertBefore(pClsn, pNextClsn);
            sorted = FALSE;
         }
         else
            pClsn = pClsn->GetNext();
         pNextClsn = pClsn->GetNext();
      }
   }
}

///////////////////////////////////////

#ifdef DBG_ON
void cPhysClsn::MonoPrint(void) const
{
   switch (type)
   {
      case kPC_StationaryObject:
         mprintf("Obj %d(%d) collides with stationary obj %d(%d)\n",
                 GetObjID(), GetSubModId(), GetObjID2(), GetSubModId2());
         break;
      case kPC_MoveableObject:
         mprintf("Obj %d(%d) collides with moveable obj %d(%d)\n",
                 GetObjID(), GetSubModId(), GetObjID2(), GetSubModId2());
         break;
      case kPC_TerrainFace:
         mprintf("Obj %d(%d) collides with terrain face %d(%d)\n",
                 GetObjID(), GetSubModId(), GetCellID(), GetPolyID());
         break;
      case kPC_TerrainEdge:
         mprintf("Obj %d(%d) collides with terrain edge %d(%d)\n",
                 GetObjID(), GetSubModId(), GetCellID(), GetPolyID());
         break;
      case kPC_TerrainVertex:
         mprintf("Obj %d(%d) collides with terrain vertex %d(%d)\n",
                 GetObjID(), GetSubModId(), GetCellID(), GetPolyID());
         break;
      default:
         mprintf("unknown collision type\n");
         break;
   }
}
#endif

///////////////////////////////////////////////////////////////////////////////
