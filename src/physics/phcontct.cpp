/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phcontct.cpp,v 1.45 1999/08/26 12:52:14 dc Exp $
//
//
//

#include <lg.h>
#include <allocapi.h>

#include <textarch.h>
#include <wrdb.h>
#include <wrdbrend.h>

#include <plyrmov.h>

#include <comtools.h>
#include <appagg.h>

#include <iobjsys.h>
#include <objtype.h>
#include <physapi.h>
#include <phcore.h>
#include <phmod.h>
#include <phmodobb.h>
#include <phmodsph.h>
#include <phmods.h>
#include <phcontct.h>
#include <phclimb.h>
#include <phdbg.h>

#include <phmsg.h>
#include <phscrt.h>

#include <hshpptem.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////

cPhysContactLinks g_PhysContactLinks;

///////////////////////////////////////////////////////////////////////////////

void CreateObjectContact(ObjID objID1, tPhysSubModId subModId1, cPhysModel * pModel1,
                         ObjID objID2, tPhysSubModId subModId2, cPhysModel * pModel2)
{
   cPhysSubModelInst *pObjContactList;

   Assert_(objID1 != objID2);

   if (GetObjectContacts(objID1, subModId1, pModel1, &pObjContactList))
   {
      while (pObjContactList)
      {
         if ((pObjContactList->GetObjID() == objID2) &&
             (pObjContactList->GetSubModId() == subModId2))
         {
            return;
         }
         pObjContactList = pObjContactList->GetNext();
      }
   }

   g_PhysContactLinks.CreateObjectLink(objID1, subModId1, pModel1, objID2, subModId2, pModel2);
   pModel1->SetObjectContact(TRUE);
   pModel2->SetObjectContact(TRUE);

   if ((pModel1->GetType(subModId1) == kPMT_OBB) || (pModel2->GetType(subModId2) == kPMT_OBB))
   {
      PhysMessageContactCreate(pModel1, subModId1, kContactOBB, objID2, subModId2);
      PhysMessageContactCreate(pModel2, subModId2, kContactOBB, objID1, subModId1);
   }
   else
   if (((pModel1->GetType(subModId1) == kPMT_SphereHat) && (subModId1 == 1)) ||
       ((pModel2->GetType(subModId2) == kPMT_SphereHat) && (subModId2 == 1)))
   {
      PhysMessageContactCreate(pModel1, subModId1, kContactSphereHat, objID2, subModId2);
      PhysMessageContactCreate(pModel2, subModId2, kContactSphereHat, objID1, subModId1);
   }
   else
   {
      PhysMessageContactCreate(pModel1, subModId1, kContactSphere, objID2, subModId2);
      PhysMessageContactCreate(pModel2, subModId2, kContactSphere, objID1, subModId1);
   }
}

///////////////////////////////////////

void DestroyObjectContact(ObjID objID1, tPhysSubModId subModId1, cPhysModel * pModel1,
                          ObjID objID2, tPhysSubModId subModId2, cPhysModel * pModel2)
{
   g_PhysContactLinks.DestroyObjectLink(objID1, subModId1, objID2, subModId2);

   Assert_((pModel1 != NULL) && (pModel2 != NULL));
   if ((pModel1 == NULL) || (pModel2 == NULL))
      return;

   pModel1->SetObjectContact(FALSE);
   pModel2->SetObjectContact(FALSE);

   if ((pModel1->GetType(subModId1) == kPMT_OBB) || (pModel2->GetType(subModId2) == kPMT_OBB))
   {
      PhysMessageContactDestroy(pModel1, subModId1, kContactOBB, objID2, subModId2);
      PhysMessageContactDestroy(pModel2, subModId2, kContactOBB, objID1, subModId1);
   }
   else
   if (((pModel1->GetType(subModId1) == kPMT_SphereHat) && (subModId1 == 1)) ||
       ((pModel2->GetType(subModId2) == kPMT_SphereHat) && (subModId2 == 1)))
   {
      PhysMessageContactDestroy(pModel1, subModId1, kContactSphereHat, objID2, subModId2);
      PhysMessageContactDestroy(pModel2, subModId2, kContactSphereHat, objID1, subModId1);
   }
   else
   {
      PhysMessageContactDestroy(pModel1, subModId1, kContactSphere, objID2, subModId2);
      PhysMessageContactDestroy(pModel2, subModId2, kContactSphere, objID1, subModId1);
   }

   pModel1->SetSleep(FALSE);
   pModel2->SetSleep(FALSE);

   if (pModel1->IsRopeClimbing() && pModel1->GetClimbingObj() == objID2)
      BreakClimb(objID1, FALSE, TRUE);
   else
   if (pModel2->IsRopeClimbing() && pModel2->GetClimbingObj() == objID1)
      BreakClimb(objID2, FALSE, TRUE);
}

///////////////////////////////////////

BOOL GetObjectContacts(ObjID objID1, tPhysSubModId subModId1, cPhysModel * pModel,
                       cPhysSubModelInst ** ppSubModel)
{
   BOOL valid=FALSE;
     
   if (pModel->InObjectContact())
      valid = g_PhysContactLinks.GetObjectLinks(objID1, subModId1, ppSubModel);
   if(!valid)
      *ppSubModel = NULL;
   return valid;
}

///////////////////////////////////////

void DestroyAllObjectContacts(ObjID objID, tPhysSubModId subModId, cPhysModel * pModel)
{
   static BOOL DestroyAllInProgress = FALSE;

   if (DestroyAllInProgress)
      return;

   DestroyAllInProgress = TRUE;

   cPhysSubModelInst *pSubModel, *pSubModelNext;

   if (GetObjectContacts(objID, subModId, pModel, &pSubModel))
   {
      while (pSubModel != NULL) 
      {
         pSubModelNext = pSubModel->GetNext();
         DestroyObjectContact(objID, subModId, pModel, 
            pSubModel->GetObjID(), pSubModel->GetSubModId(), pSubModel->GetPhysicsModel());
         pSubModel = pSubModelNext;
      }
   }

   DestroyAllInProgress = FALSE;
}

///////////////////////////////////////////////////////////////////////////////

cFacePoly::cFacePoly()
{
   edgePlaneList.SetSize(0);
}

cFacePoly::cFacePoly(const cFacePoly *poly)
{
   edgePlaneList.SetSize(0);

   // Copy plane info
   mx_copy_vec(&normal, &poly->normal);
   d = poly->d;

   // Copy edge info
   for (int i=0; i<poly->edgePlaneList.Size(); i++)
   {
      cFacePoly *pEdgePoly = new cFacePoly;

      mx_copy_vec(&pEdgePoly->normal, &poly->edgePlaneList[i]->normal);
      pEdgePoly->d = poly->edgePlaneList[i]->d;
      
      LGALLOC_AUTO_CREDIT();
      edgePlaneList.Append(pEdgePoly);
   }
}

cFacePoly::~cFacePoly()
{
   for (int i=0; i<edgePlaneList.Size(); i++)
      delete edgePlaneList[i];

   edgePlaneList.SetSize(0);
}

////////////////////////////////////////

cFaceContact::cFaceContact(int cellID, int polyID)
{
   cFacePoly *pFacePoly = new cFacePoly;
   cFacePoly *pEdgePoly;

   PortalCell        *cell;
   PortalPolygonCore *pgon;

   mxs_vector center;
   mxs_vector edge;
   int        start_vertex;
   int        i;

   // Easy stuff first
   pFacePoly->normal = WR_CELL(cellID)->plane_list[WR_CELL(cellID)->poly_list[polyID].planeid].normal;
   pFacePoly->d = WR_CELL(cellID)->plane_list[WR_CELL(cellID)->poly_list[polyID].planeid].plane_constant;

   m_objID = GetTextureObj(WR_CELL(cellID)->render_list[polyID].texture_id);

   // Build planes from edges
   cell = WR_CELL(cellID);
   pgon = &cell->poly_list[polyID];

   // Find start vertex
   start_vertex = 0;
   for (i=0; i<polyID; i++)
      start_vertex += WR_CELL(cellID)->poly_list[i].num_vertices;

   // Find center of poly
   mx_zero_vec(&center);
   for (i=0; i<pgon->num_vertices; i++)
      mx_addeq_vec(&center, &cell->vpool[cell->vertex_list[start_vertex + i]]);
   mx_scaleeq_vec(&center, 1.0 / pgon->num_vertices);

   // Build an edge for each vertex pair
   for (i=0; i<pgon->num_vertices; i++)
   {
      pEdgePoly = new cFacePoly;

      mx_sub_vec(&edge, &cell->vpool[cell->vertex_list[start_vertex + ((i + 1) % pgon->num_vertices)]],
                        &cell->vpool[cell->vertex_list[start_vertex + i]]);

      // Find the normal
      mx_cross_vec(&pEdgePoly->normal, &pFacePoly->normal, &edge);
      mx_normeq_vec(&pEdgePoly->normal);

      // And the plane constant
      pEdgePoly->d = mx_dot_vec(&pEdgePoly->normal, &cell->vpool[cell->vertex_list[start_vertex + i]]);
     
      // Switch direction if pointing inward
      if ((mx_dot_vec(&pEdgePoly->normal, &center) - pEdgePoly->d) > 0)
      {
         mx_scaleeq_vec(&pEdgePoly->normal, -1.0);
         pEdgePoly->d *= -1;
      }

      // And add to list
      LGALLOC_AUTO_CREDIT();
      pFacePoly->edgePlaneList.Append(pEdgePoly);
   }

   m_pPoly = pFacePoly;
}

cFaceContact::cFaceContact(cPhysOBBModel *pModel, int side, BOOL endLoc)
{
   Assert_(side >= 0);

   cFacePoly *pFacePoly = new cFacePoly;

   mxs_matrix obb_frame;
   mxs_vector obb_loc;
   mxs_vector obb_offset;
   mxs_vector unit;
   mxs_vector pt_on;
   mxs_real   edge_len;

   m_objID = pModel->GetObjID();

   // Calc normal
   if (endLoc)
   {
      mx_ang2mat(&obb_frame, &pModel->GetEndRotationVec());
      mx_mat_mul_vec(&obb_offset, &obb_frame, &pModel->GetOffset());
      mx_add_vec(&obb_loc, &pModel->GetEndLocationVec(), &obb_offset);
   }
   else
   {
      mx_ang2mat(&obb_frame, &pModel->GetRotation());
      mx_mat_mul_vec(&obb_offset, &obb_frame, &pModel->GetOffset());
      mx_add_vec(&obb_loc, &pModel->GetLocationVec(), &obb_offset);
   }

   mx_unit_vec(&unit, (side % 3));

   // Flip if last 3 
   if (side >= 3)
      mx_scaleeq_vec(&unit, -1.0);

   // Rotate into obb's facing
   mx_mat_mul_vec(&pFacePoly->normal, &obb_frame, &unit);

   // Find point on normal
   edge_len = fabs(mx_dot_vec(&pModel->GetEdgeLengths(), &unit)) / 2;
   mx_scale_add_vec(&pt_on, &obb_loc, &pFacePoly->normal, edge_len);

   // Find plane constant
   pFacePoly->d = mx_dot_vec(&pFacePoly->normal, &pt_on);

   // Build edge planes
   for (int i=0; i<6; i++)
   {
      // Ignore this plane, and the one 3 away (pointing opposite direction)
      if ((i % 3) == (side % 3))
         continue;

      cFacePoly *pEdgePoly = new cFacePoly;

      // Calc normal
      mx_unit_vec(&unit, (i % 3));

      // Flip if last 3 
      if (i >= 3)
         mx_scaleeq_vec(&unit, -1.0);

      // Rotate into obb's facing
      mx_mat_mul_vec(&pEdgePoly->normal, &obb_frame, &unit);

      // Find point on normal
      edge_len = fabs(mx_dot_vec(&pModel->GetEdgeLengths(), &unit)) / 2;
      mx_scale_add_vec(&pt_on, &obb_loc, &pEdgePoly->normal, edge_len);

      // Find plane constant
      pEdgePoly->d = mx_dot_vec(&pEdgePoly->normal, &pt_on);

      LGALLOC_AUTO_CREDIT();
      pFacePoly->edgePlaneList.Append(pEdgePoly);
   }

   m_pPoly = pFacePoly;
}

cFaceContact::cFaceContact(cPhysSphereModel *pModel, mxs_real radius, BOOL endLoc)
{
   mxs_matrix obj_frame;

   m_objID = pModel->GetObjID();

   if (endLoc)
      mx_ang2mat(&obj_frame, &pModel->GetEndRotationVec());
   else
      mx_ang2mat(&obj_frame, &pModel->GetRotation());

   if (obj_frame.el[8] < .9)
   {
      m_pPoly = NULL;
      return;
   }

   cFacePoly *pFacePoly = new cFacePoly;
   mxs_vector pt_on;

   mx_copy_vec(&pFacePoly->normal, &obj_frame.vec[2]);
   if (endLoc)
      mx_scale_add_vec(&pt_on, &pModel->GetEndLocationVec(), &pFacePoly->normal, radius);
   else
      mx_scale_add_vec(&pt_on, &pModel->GetLocationVec(), &pFacePoly->normal, radius);

   pFacePoly->d = mx_dot_vec(&pFacePoly->normal, &pt_on);

   for (int i=0; i<6; i++)
   {
      if ((i % 3) == 2)
         continue;

      cFacePoly *pEdgePoly = new cFacePoly;

      mx_copy_vec(&pEdgePoly->normal, &obj_frame.vec[i%3]);
      if (i > 2)
         mx_scaleeq_vec(&pEdgePoly->normal, -1.0);

      if (endLoc)
         mx_scale_add_vec(&pt_on, &pModel->GetEndLocationVec(), &pEdgePoly->normal, radius * 0.9);
      else
         mx_scale_add_vec(&pt_on, &pModel->GetLocationVec(), &pEdgePoly->normal, radius * 0.9);

      pEdgePoly->d = mx_dot_vec(&pEdgePoly->normal, &pt_on);

      LGALLOC_AUTO_CREDIT();
      pFacePoly->edgePlaneList.Append(pEdgePoly);
   }

   m_pPoly = pFacePoly;
}

////////////////////////////////////////

mxs_vector cEdgeContact::GetNormal(const mxs_vector &pt) const
{
   mxs_vector v1;
   mxs_vector cross;
   mxs_vector norm;

   mx_sub_vec(&v1, (mxs_vector *) &pt, (mxs_vector *) &m_Start);
   mx_cross_vec(&cross, (mxs_vector *) &m_Vector, &v1);
   mx_cross_vec(&norm, &cross, (mxs_vector *) &m_Vector);
   mx_normeq_vec(&norm);

   return norm;
}

mxs_real cEdgeContact::GetDist(const mxs_vector &pt) const
{
   mxs_vector v1c, v2c;
   mxs_vector edge;
   mxs_vector edge_norm;

   mx_sub_vec(&v1c, (mxs_vector *) &pt, (mxs_vector *) &m_Start);
   mx_sub_vec(&v2c, (mxs_vector *) &pt, (mxs_vector *) &m_End);

   mx_sub_vec(&edge, (mxs_vector *) &m_Start, (mxs_vector *) &m_End);

   // See if we're past the start vertex
   if (mx_dot_vec(&v1c, &edge) > 0.0)
      return mx_dist_vec((mxs_vector *) &pt, (mxs_vector *) &m_Start);

   // Or the end vertex
   if (mx_dot_vec(&v2c, &edge) < 0.0)
      return mx_dist_vec((mxs_vector *) &pt, (mxs_vector *) &m_End);
      
   // We're between the vertices, so do funky calculation...
   //
   // ||cross|| = 2 x area of triangle formed by edge and point
   // area = (base x height) / 2, so if we divide by the base we're left with
   // the height, or the distance of the point from the line.  Neat-o.
   mx_cross_vec(&edge_norm, &v1c, &edge);
   
   return sqrt(mx_mag2_vec(&edge_norm) / mx_mag2_vec(&edge));
}

////////////////////////////////////////

const mxs_vector cVertexContact::GetNormal(const mxs_vector &pt) const
{
   mxs_vector norm;

   mx_sub_vec(&norm, (mxs_vector *) &pt, (mxs_vector *) &m_Point);
   mx_normeq_vec(&norm);

   return norm;
}

////////////////////////////////////////

// wow this is some nasty code

BOOL cFaceContactList::Find(const cFacePoly *poly, cFaceContact **ppFaceContact)
{
   const cFacePoly *face_poly;
   const cFacePoly *face_epoly, *epoly;
   mxs_vector delta;
   int i;

   *ppFaceContact = GetFirst();
   while (*ppFaceContact)
   {
      face_poly = (*ppFaceContact)->GetPoly();

      mx_sub_vec(&delta, &poly->normal, &face_poly->normal);
      if (mx_mag2_vec(&delta) > 0.0001)
         goto skip_poly;

      if ((poly->d - face_poly->d) > 0.0001)
         goto skip_poly;

      for (i=0; (i<face_poly->edgePlaneList.Size()) && (i<poly->edgePlaneList.Size()); i++)
      {
         face_epoly = face_poly->edgePlaneList[i];
         epoly = poly->edgePlaneList[i];

         mx_sub_vec(&delta, &epoly->normal, &face_epoly->normal);
         if (mx_mag2_vec(&delta) > 0.0001)
            goto skip_poly;

         if ((epoly->d - face_epoly->d) > 0.0001)
           goto skip_poly;
      }
      break;

      skip_poly:
      *ppFaceContact = (*ppFaceContact)->GetNext();
   }

   return (*ppFaceContact != NULL);
}

BOOL cEdgeContactList::Find(const mxs_vector &start, const mxs_vector &end, cEdgeContact **ppEdgeContact)
{
   mxs_vector start_delta, end_delta;

   *ppEdgeContact = GetFirst();
   while (*ppEdgeContact)
   {
      mx_sub_vec(&start_delta, (mxs_vector *)&start, (mxs_vector *)&(*ppEdgeContact)->GetStart());
      mx_sub_vec(&end_delta, (mxs_vector *)&end, (mxs_vector *)&(*ppEdgeContact)->GetEnd());

      if ((mx_mag2_vec(&start_delta) < 0.0001) &&
          (mx_mag2_vec(&end_delta) < 0.0001))
         break;

      *ppEdgeContact = (*ppEdgeContact)->GetNext();
   }

   return (*ppEdgeContact != NULL);
}

BOOL cVertexContactList::Find(const mxs_vector &point, cVertexContact **ppVertexContact)
{
   mxs_vector point_delta;

   *ppVertexContact = GetFirst();
   while (*ppVertexContact)
   {
      mx_sub_vec(&point_delta, (mxs_vector *)&point, (mxs_vector *)&(*ppVertexContact)->GetPoint());

      if (mx_mag2_vec(&point_delta) < 0.0001)
         break;
 
      *ppVertexContact = (*ppVertexContact)->GetNext();
   }

   return (*ppVertexContact != NULL);
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysObjContactLinks
//

///////////////////////////////////////

cPhysObjContactLinks::~cPhysObjContactLinks(void)
{
   int i;

   // Clear terrain face contacts
   for (i=0; i<m_FaceContactList.Size(); i++)
   {
      if (m_FaceContactList[i] != NULL)
      {
         cFaceContact* face_contact = m_FaceContactList[i]->GetFirst();
         while (face_contact)
         {
            m_FaceContactList[i]->Remove(face_contact);
            delete face_contact;
            face_contact = m_FaceContactList[i]->GetFirst();
         }
         delete m_FaceContactList[i];
      }
   }
   m_FaceContactList.SetSize(0);

   // Clear terrain edge contacts
   for (i=0; i<m_EdgeContactList.Size(); i++)
   {
      if (m_EdgeContactList[i] != NULL)
      {
         cEdgeContact* edge_contact = m_EdgeContactList[i]->GetFirst();
         while (edge_contact)
         {
            m_EdgeContactList[i]->Remove(edge_contact);
            delete edge_contact;
            edge_contact = m_EdgeContactList[i]->GetFirst();
         }
         delete m_EdgeContactList[i];
      }
   }
   m_EdgeContactList.SetSize(0);

   // Clear terrain vertex contacts
   for (i=0; i<m_VertexContactList.Size(); i++)
   {
      if (m_VertexContactList[i] != NULL)
      {
         cVertexContact* vertex_contact = m_VertexContactList[i]->GetFirst();
         while (vertex_contact)
         {
            m_VertexContactList[i]->Remove(vertex_contact);
            delete vertex_contact;
            vertex_contact = m_VertexContactList[i]->GetFirst();
         }
         delete m_VertexContactList[i];
      }
   }
   m_VertexContactList.SetSize(0);

   // Clear submodel contacts
   for (i = 0; i < m_subModelListArray.Size(); i++)
      if (m_subModelListArray[i] != NULL)
         delete m_subModelListArray[i];
}

///////////////////////////////////////

static void WriteFaceContact(PhysReadWrite func, ObjID objID, int i, cFaceContact *pFaceContact) 
{
   func((void *)&kFaceContactCode, sizeof(long), 1);
   func(&objID, sizeof(ObjID), 1);
   func(&i, sizeof(int), 1);

   // contacting object
   ObjID contact_obj = pFaceContact->GetObjID();

   // just save the archetype (singified by <= 0), if its a texture object
   if (IsTextureObj(contact_obj))
      contact_obj = -GetObjTextureIdx(contact_obj);

   func(&contact_obj, sizeof(ObjID), 1);

   // face poly
   const cFacePoly *poly = pFaceContact->GetPoly();

   func((void *)&poly->normal, sizeof(mxs_vector), 1);
   func((void *)&poly->d, sizeof(mxs_real), 1);

   int size = poly->edgePlaneList.Size();
   func(&size, sizeof(int), 1);

   for (int j=0; j<size; j++)
   {
      func((void *)&poly->edgePlaneList[j]->normal, sizeof(mxs_vector), 1);
      func((void *)&poly->edgePlaneList[j]->d, sizeof(mxs_real), 1);
   }
}

static void WriteEdgeContact(PhysReadWrite func, ObjID objID, int i, cEdgeContact *pEdgeContact) 
{
   func((void *)&kEdgeContactCode, sizeof(long), 1);
   func(&objID, sizeof(ObjID), 1);
   func(&i, sizeof(int), 1);

   func((void *)&pEdgeContact->GetStart(), sizeof(mxs_vector), 1);
   func((void *)&pEdgeContact->GetEnd(), sizeof(mxs_vector), 1);
}

static void WriteVertexContact(PhysReadWrite func, ObjID objID, int i, cVertexContact *pVertexContact) 
{
   func((void *)&kVertexContactCode, sizeof(long), 1);
   func(&objID, sizeof(ObjID), 1);
   func(&i, sizeof(int), 1);

   func((void *)&pVertexContact->GetPoint(), sizeof(mxs_vector), 1);
}

static void WriteObjectContact(PhysReadWrite func, ObjID objID, int i, cPhysSubModelInst *pObjContact)
{
   func((void *)&kObjectContactCode, sizeof(long), 1);
   func(&objID, sizeof(ObjID), 1);
   func(&i, sizeof(int), 1);

   ObjID contact_obj = pObjContact->GetObjID();
   int   contact_submod = pObjContact->GetSubModId();

   AssertMsg2(g_PhysModels.Get(contact_obj) != NULL, "ContactObj %s no phys models in write (obj %s)", ObjWarnName(contact_obj), ObjWarnName(objID));

   func(&contact_obj, sizeof(ObjID), 1);
   func(&contact_submod, sizeof(ObjID), 1);
}

///////////////////////////////////////

void cPhysObjContactLinks::Write(PhysReadWrite func, ObjID objID) const
{
   int i;

   // write face contacts
   for (i=0; i<m_FaceContactList.Size(); i++)
   {
      cFaceContactList *pFaceContactList;
      cFaceContact     *pFaceContact;

      pFaceContactList = m_FaceContactList[i];

      if (pFaceContactList)
      {
         pFaceContact = pFaceContactList->GetFirst();
         while (pFaceContact)
         {
            WriteFaceContact(func, objID, i, pFaceContact);

            pFaceContact = pFaceContact->GetNext();
         }
      }
   }

   // write edge contacts
   for (i=0; i<m_EdgeContactList.Size(); i++)
   {
      cEdgeContactList *pEdgeContactList;
      cEdgeContact     *pEdgeContact;

      pEdgeContactList = m_EdgeContactList[i];

      if (pEdgeContactList)
      {
         pEdgeContact = pEdgeContactList->GetFirst();
         while (pEdgeContact)
         {
            WriteEdgeContact(func, objID, i, pEdgeContact);

            pEdgeContact = pEdgeContact->GetNext();
         }
      }
   }

   // write vertex contacts
   for (i=0; i<m_VertexContactList.Size(); i++)
   {
      cVertexContactList *pVertexContactList;
      cVertexContact     *pVertexContact;

      pVertexContactList = m_VertexContactList[i];

      if (pVertexContactList)
      {
         pVertexContact = pVertexContactList->GetFirst();
         while (pVertexContact)
         {
            WriteVertexContact(func, objID, i, pVertexContact);

            pVertexContact = pVertexContact->GetNext();
         }
      }
   }

   // write object contacts
   for (i=0; i<m_subModelListArray.Size(); i++)
   {
      cPhysSubModelList *pSubModelList;
      cPhysSubModelInst *pSubModel;

      pSubModelList = m_subModelListArray[i];

      if (pSubModelList)
      {
         pSubModel = pSubModelList->GetFirst();
         while (pSubModel)
         {
            WriteObjectContact(func, objID, i, pSubModel);

            pSubModel = pSubModel->GetNext();
         }
      }
   }
}

///////////////////////////////////////

void cPhysObjContactLinks::CreateTerrainLink(tPhysSubModId subModId, const cFacePoly *poly, ObjID objID)
{
   cFaceContact *pFaceContact;

   LGALLOC_PUSH_CREDIT();
   while (!m_FaceContactList.IsValidIndex(subModId))
      m_FaceContactList.Append(NULL);
   LGALLOC_POP_CREDIT();

   pFaceContact = new cFaceContact(poly, objID);

   if (m_FaceContactList[(int) subModId] == NULL)
      m_FaceContactList[(int) subModId] = new cFaceContactList;

   LGALLOC_PUSH_CREDIT();
   m_FaceContactList[(int) subModId]->Append(pFaceContact);
   LGALLOC_POP_CREDIT();

   m_linkCount++;
}

void cPhysObjContactLinks::CreateTerrainLink(tPhysSubModId subModId, const mxs_vector &start, const mxs_vector &end)
{
   cEdgeContact *pEdgeContact;

   LGALLOC_PUSH_CREDIT();
   while (!m_EdgeContactList.IsValidIndex(subModId))
      m_EdgeContactList.Append(NULL);
   LGALLOC_POP_CREDIT();

   pEdgeContact = new cEdgeContact(start, end);

   if (m_EdgeContactList[(int) subModId] == NULL)
      m_EdgeContactList[(int) subModId] = new cEdgeContactList;

   LGALLOC_PUSH_CREDIT();
   m_EdgeContactList[(int) subModId]->Append(pEdgeContact);
   LGALLOC_POP_CREDIT();

   m_linkCount++;
}

void cPhysObjContactLinks::CreateTerrainLink(tPhysSubModId subModId, const mxs_vector &point)
{
   cVertexContact *pVertexContact;

   LGALLOC_PUSH_CREDIT();
   while (!m_VertexContactList.IsValidIndex(subModId))
      m_VertexContactList.Append(NULL);
   LGALLOC_POP_CREDIT();

   pVertexContact = new cVertexContact(point);

   if (m_VertexContactList[(int) subModId] == NULL)
      m_VertexContactList[(int) subModId] = new cVertexContactList;

   LGALLOC_PUSH_CREDIT();
   m_VertexContactList[(int) subModId]->Append(pVertexContact);
   LGALLOC_POP_CREDIT();

   m_linkCount++;
}

///////////////////////////////////////
//
// Destroy a terrain link
// return TRUE if no more links for the object
//

BOOL cPhysObjContactLinks::DestroyTerrainLink(tPhysSubModId subModId, const cFacePoly *poly)
{
   cFaceContact *pFaceContact;

   if (GetTerrainLink(subModId, poly, &pFaceContact))
   {
      cFaceContactList *pFaceContactList = m_FaceContactList[(int) subModId];

      AssertMsg1(pFaceContactList, "Attempt to destroy non-existant link with subobj %d", subModId);

      pFaceContactList->Remove(pFaceContact);
      delete pFaceContact;

      if (pFaceContactList->GetFirst() == NULL)
      {
         delete pFaceContactList;
         m_FaceContactList[(int) subModId] = NULL;
      }

      if (--m_linkCount == 0)
         return TRUE;
   }

   return FALSE;
}

BOOL cPhysObjContactLinks::DestroyTerrainLink(tPhysSubModId subModId, const mxs_vector &start, const mxs_vector &end)
{
   cEdgeContact *pEdgeContact;

   if (GetTerrainLink(subModId, start, end, &pEdgeContact))
   {
      cEdgeContactList *pEdgeContactList = m_EdgeContactList[(int) subModId];

      AssertMsg1(pEdgeContactList, "Attempt to destroy non-existant link with subobj %d", subModId);

      pEdgeContactList->Remove(pEdgeContact);
      delete pEdgeContact;

      if (pEdgeContactList->GetFirst() == NULL)
      {
         delete pEdgeContactList;
         m_EdgeContactList[(int) subModId] = NULL;
      }

      if (--m_linkCount == 0)
         return TRUE;
   }

   return FALSE;
}

BOOL cPhysObjContactLinks::DestroyTerrainLink(tPhysSubModId subModId, const mxs_vector &point)
{
   cVertexContact *pVertexContact;

   if (GetTerrainLink(subModId, point, &pVertexContact))
   {
      cVertexContactList *pVertexContactList = m_VertexContactList[(int) subModId];

      AssertMsg1(pVertexContactList, "Attempt to destroy non-existant link with subobj %d", subModId);

      pVertexContactList->Remove(pVertexContact);
      delete pVertexContact;

      if (pVertexContactList->GetFirst() == NULL)
      {
         delete pVertexContactList;
         m_VertexContactList[(int) subModId] = NULL;
      }

      if (--m_linkCount == 0)
         return TRUE;
   }

   return FALSE;
}



///////////////////////////////////////
//
// Get a terrain link
//

BOOL cPhysObjContactLinks::GetTerrainLink(tPhysSubModId subModId, const cFacePoly *poly, 
                                          cFaceContact **ppFaceContact) const
{
   AssertMsg(m_FaceContactList.IsValidIndex(subModId), "Invalid terrain link index");
   AssertMsg(m_FaceContactList[subModId], "Nonexistant terrain link");

   return m_FaceContactList[(int) subModId]->Find(poly, ppFaceContact);
}

BOOL cPhysObjContactLinks::GetTerrainLink(tPhysSubModId subModId, const mxs_vector &start,
                                          const mxs_vector &end, cEdgeContact **ppEdgeContact) const
{
   AssertMsg(m_EdgeContactList.IsValidIndex(subModId), "Invalid terrain link index");
   AssertMsg(m_EdgeContactList[subModId], "Nonexistant terrain link");

   return m_EdgeContactList[(int) subModId]->Find(start, end, ppEdgeContact);
}

BOOL cPhysObjContactLinks::GetTerrainLink(tPhysSubModId subModId, const mxs_vector & point,
                                          cVertexContact **ppVertexContact) const
{
   AssertMsg(m_VertexContactList.IsValidIndex(subModId), "Invalid terrain link index");
   AssertMsg(m_VertexContactList[subModId], "Nonexistant terrain link");

   return m_VertexContactList[(int) subModId]->Find(point, ppVertexContact);
}

///////////////////////////////////////
//
// get terrain link list
//

BOOL cPhysObjContactLinks::GetTerrainLinkList(tPhysSubModId subModId, cFaceContactList **ppFaceContactList) const
{
   if (!m_FaceContactList.IsValidIndex(subModId))
   {
      *ppFaceContactList = NULL;
      return FALSE;
   }

   *ppFaceContactList = m_FaceContactList[subModId];
   return (m_FaceContactList[subModId] != NULL);
}

BOOL cPhysObjContactLinks::GetTerrainLinkList(tPhysSubModId subModId, cEdgeContactList **ppEdgeContactList) const
{
   if (!m_EdgeContactList.IsValidIndex(subModId))
   {
      *ppEdgeContactList = NULL;
      return FALSE;
   }

   *ppEdgeContactList = m_EdgeContactList[subModId];
   return (m_EdgeContactList[subModId] != NULL);
}

BOOL cPhysObjContactLinks::GetTerrainLinkList(tPhysSubModId subModId, cVertexContactList **ppVertexContactList) const
{
   if (!m_VertexContactList.IsValidIndex(subModId))
   {
      *ppVertexContactList = NULL;
      return FALSE;
   }

   *ppVertexContactList = m_VertexContactList[subModId];
   return (m_VertexContactList[subModId] != NULL);
}

///////////////////////////////////////
//
// Create a single object link, alloc new model info and maybe a new list
//

void cPhysObjContactLinks::CreateObjectLink(tPhysSubModId subModId, ObjID objID2, tPhysSubModId subModId2,
                                        cPhysModel * pModel)
{
   cPhysSubModelInst *pSubModel;

   LGALLOC_PUSH_CREDIT();
   if (!m_subModelListArray.IsValidIndex(subModId))
      m_subModelListArray.SetSize(subModId + 1);
   LGALLOC_POP_CREDIT();

   pSubModel = new cPhysSubModelInst(objID2, subModId2, pModel);

   if (m_subModelListArray[(int) subModId] == NULL)
      m_subModelListArray[(int) subModId] = new cPhysSubModelList;

   LGALLOC_PUSH_CREDIT();
   m_subModelListArray[(int) subModId]->Append(pSubModel);
   LGALLOC_POP_CREDIT();

   m_linkCount++;
}

///////////////////////////////////////
//
// Destroy an object-object link, free the data structure, may free the list
//

BOOL cPhysObjContactLinks::DestroyObjectLink(tPhysSubModId subModId, ObjID objID2, tPhysSubModId subModId2)
{
   cPhysSubModelInst *pSubModel;

   if (GetObjectLink(subModId, objID2, subModId2, &pSubModel))
   {
      cPhysSubModelList *pModelList = m_subModelListArray[(int) subModId];

      AssertMsg1(pModelList, "Attempt to destroy non-existant link with subobj %d", subModId);

      pModelList->Remove(pSubModel);
      delete pSubModel;

      if (pModelList->GetFirst() == NULL)
      {
         m_subModelListArray[(int) subModId] = NULL;
         delete pModelList;
      }
      if (--m_linkCount == 0)
         return TRUE;
   }

   return FALSE;
}

///////////////////////////////////////

BOOL cPhysObjContactLinks::GetObjectLink(tPhysSubModId subModId, ObjID objID2,
                                         tPhysSubModId subModId2, cPhysSubModelInst ** ppSubModel) const
{
   if (m_subModelListArray.IsValidIndex(subModId))
   {
      if (m_subModelListArray[(int) subModId] != NULL)
      {
         if (m_subModelListArray[(int) subModId]->Find(objID2, subModId2, ppSubModel))
            return TRUE;
      }
   }
   return FALSE;
}

///////////////////////////////////////

BOOL cPhysObjContactLinks::GetObjectLinks(tPhysSubModId subModId, cPhysSubModelInst ** ppSubModel) const
{
   if (m_subModelListArray.IsValidIndex(subModId))
   {
      if (m_subModelListArray[(int) subModId] != NULL)
      {
         *ppSubModel = m_subModelListArray[(int) subModId]->GetFirst();
         return TRUE;
      }
   }
   return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysContactLinks
//


///////////////////////////////////////
//
// Read links from disk
//

void cPhysContactLinks::Read(PhysReadWrite func)
{
   ObjID objID;
   int submod;
   long code;

   AutoAppIPtr_(ObjectSystem, pObjSys);

   while (TRUE)
   {
      func(&code, sizeof(long), 1);

      if (code == kDoneContactCode)
         return;

      func(&objID, sizeof(ObjID), 1);
      func(&submod, sizeof(int), 1);

      objID = pObjSys->RemapOnLoad(objID);

      switch (code)
      {
         case kFaceContactCode:
         {
            cFacePoly    *pFacePoly;

            // contacting object
            ObjID contact_obj;
            func(&contact_obj, sizeof(ObjID), 1);

            contact_obj = pObjSys->RemapOnLoad(contact_obj);

            // if it's a texture index, create the texture object
            if (contact_obj <= 0)
               contact_obj = GetTextureObj(-contact_obj);

            // face poly
            pFacePoly = new cFacePoly;

            func((void *)&pFacePoly->normal, sizeof(mxs_vector), 1);
            func((void *)&pFacePoly->d, sizeof(mxs_real), 1);

            int size;
            func(&size, sizeof(int), 1);
            
            LGALLOC_PUSH_CREDIT();
            pFacePoly->edgePlaneList.SetSize(size);
            LGALLOC_POP_CREDIT();

            for (int i=0; i<size; i++)
            {
               pFacePoly->edgePlaneList[i] = new cFacePoly;

               func((void *)&pFacePoly->edgePlaneList[i]->normal, sizeof(mxs_vector), 1);
               func((void *)&pFacePoly->edgePlaneList[i]->d, sizeof(mxs_real), 1);
            }

            ///////

            cPhysObjContactLinks *pObjContactLinks;

            if (!m_contactLinkHash.Lookup(objID, &pObjContactLinks))
            {
               pObjContactLinks = new cPhysObjContactLinks;
               m_contactLinkHash.Insert(objID, pObjContactLinks);
            }

            pObjContactLinks->CreateTerrainLink(submod, pFacePoly, contact_obj);
            delete pFacePoly;

            cPhysModel *pModel = g_PhysModels.Get(objID);
            if (pModel)
            {
               pModel->IncTerrainFaceContact();

               if (pModel->IsPlayer() && (submod == PLAYER_FOOT))
                  g_pPlayerMovement->SetGroundObj(contact_obj);
            }

            break;
         }

         case kEdgeContactCode:
         {
            mxs_vector start, end;
            
            func(&start, sizeof(mxs_vector), 1);
            func(&end, sizeof(mxs_vector), 1);

            ///////

            cPhysObjContactLinks *pObjContactLinks;

            if (!m_contactLinkHash.Lookup(objID, &pObjContactLinks))
            {
               pObjContactLinks = new cPhysObjContactLinks;
               m_contactLinkHash.Insert(objID, pObjContactLinks);
            }

            pObjContactLinks->CreateTerrainLink(submod, start, end);

            cPhysModel *pModel = g_PhysModels.Get(objID);
            if (pModel)
               pModel->IncTerrainEdgeContact();

            break;
         }

         case kVertexContactCode:
         {
            cVertexContact *pVertexContact;
            mxs_vector point;

            func(&point, sizeof(mxs_vector), 1);

            pVertexContact = new cVertexContact(point);

            ///////

            cPhysObjContactLinks *pObjContactLinks;

            if (!m_contactLinkHash.Lookup(objID, &pObjContactLinks))
            {
               pObjContactLinks = new cPhysObjContactLinks;
               m_contactLinkHash.Insert(objID, pObjContactLinks);
            }

            pObjContactLinks->CreateTerrainLink(submod, point);

            cPhysModel *pModel = g_PhysModels.Get(objID);
            if (pModel)
               pModel->IncTerrainVertexContact();

            break;
         }

         case kObjectContactCode:
         {
            ObjID contact_obj;
            int   contact_submod;

            func(&contact_obj, sizeof(ObjID), 1);
            func(&contact_submod, sizeof(int), 1);

            contact_obj = pObjSys->RemapOnLoad(contact_obj);

            cPhysModel *pModel = g_PhysModels.Get(contact_obj); // for caching the ptr

            if (pModel == NULL)
            {
               Warning(("Attempt to load contact with non-physical object %s!\n", ObjWarnName(contact_obj)));
               break;
            }

            ///////

            cPhysObjContactLinks *pObjContactLinks;

            if (!m_contactLinkHash.Lookup(objID, &pObjContactLinks))
            {
               pObjContactLinks = new cPhysObjContactLinks;
               m_contactLinkHash.Insert(objID, pObjContactLinks);
            }

            pObjContactLinks->CreateObjectLink(submod, contact_obj, contact_submod, pModel);

            if (pModel)
               pModel->SetObjectContact(TRUE);

            break;
         }
      }
   }
}


///////////////////////////////////////
//
// Write links to disk
//

void cPhysContactLinks::Write(PhysReadWrite func) const
{
   cContactLinkHash::cIter hashIter = m_contactLinkHash.Iter();

   for (; !hashIter.Done(); hashIter.Next())
   {
      cPhysObjContactLinks *links = (cPhysObjContactLinks *)hashIter.Value();

      links->Write(func, hashIter.Key());
   }

   WriteDone(func);
}

void cPhysContactLinks::WriteDone(PhysReadWrite func) const
{
   // we're done
   func((void *)&kDoneContactCode, sizeof(long), 1);
}


///////////////////////////////////////
//
// Reset contacts
//

void cPhysContactLinks::Reset()
{
   cContactLinkHash::cIter hashIter = m_contactLinkHash.Iter();

   for (; !hashIter.Done(); hashIter.Next())
      delete ((cPhysObjContactLinks *)hashIter.Value());

   m_contactLinkHash.Clear();

}

///////////////////////////////////////
//
// Create a new terrain link, insert into hash
//

void cPhysContactLinks::CreateTerrainLink(ObjID objID, tPhysSubModId subModId, const cFacePoly *poly, ObjID contactObj)
{
   cPhysObjContactLinks *pObjContactLinks;

   PHYSICS_CONTACT_SPEW(objID, ("Obj %d(%d) contacts face\n", objID, subModId));

   if (!m_contactLinkHash.Lookup(objID, &pObjContactLinks))
   {
      pObjContactLinks = new cPhysObjContactLinks;
      m_contactLinkHash.Insert(objID, pObjContactLinks);
   }

   pObjContactLinks->CreateTerrainLink(subModId, poly, contactObj);
}

void cPhysContactLinks::CreateTerrainLink(ObjID objID, tPhysSubModId subModId, 
                                          const mxs_vector &start, const mxs_vector &end)
{
   cPhysObjContactLinks *pObjContactLinks;

   PHYSICS_CONTACT_SPEW(objID, ("Obj %d(%d) contacts edge\n", objID, subModId));

   if (!m_contactLinkHash.Lookup(objID, &pObjContactLinks))
   {
      pObjContactLinks = new cPhysObjContactLinks;
      m_contactLinkHash.Insert(objID, pObjContactLinks);
   }

   pObjContactLinks->CreateTerrainLink(subModId, start, end);
}

void cPhysContactLinks::CreateTerrainLink(ObjID objID, tPhysSubModId subModId, const mxs_vector &point)
{
   cPhysObjContactLinks *pObjContactLinks;

   PHYSICS_CONTACT_SPEW(objID, ("Obj %d(%d) contacts vertex\n", objID, subModId));

   if (!m_contactLinkHash.Lookup(objID, &pObjContactLinks))
   {
      pObjContactLinks = new cPhysObjContactLinks;
      m_contactLinkHash.Insert(objID, pObjContactLinks);
   }

   pObjContactLinks->CreateTerrainLink(subModId, point);
}


///////////////////////////////////////
//
// Delete terrain link, remove from hash if no more links for object
//

void cPhysContactLinks::DestroyTerrainLink(ObjID objID, tPhysSubModId subModId, const cFacePoly *poly)
{
   cPhysObjContactLinks *pObjContactLinks;

   PHYSICS_CONTACT_SPEW(objID, ("Obj %d(%d) leaves face\n", objID, subModId));

   if (m_contactLinkHash.Lookup(objID, &pObjContactLinks))
   {
      if (pObjContactLinks->DestroyTerrainLink(subModId, poly))
      {
         delete pObjContactLinks;
         m_contactLinkHash.Delete(objID);
      }
   }
}

void cPhysContactLinks::DestroyTerrainLink(ObjID objID, tPhysSubModId subModId, 
                                           const mxs_vector &start, const mxs_vector &end)
{
   cPhysObjContactLinks *pObjContactLinks;

   PHYSICS_CONTACT_SPEW(objID, ("Obj %d(%d) leaves edge\n", objID, subModId));

   if (m_contactLinkHash.Lookup(objID, &pObjContactLinks))
   {
      if (pObjContactLinks->DestroyTerrainLink(subModId, start, end))
      {
         delete pObjContactLinks;
         m_contactLinkHash.Delete(objID);
      }
   }
}

void cPhysContactLinks::DestroyTerrainLink(ObjID objID, tPhysSubModId subModId, const mxs_vector &point)
{
   cPhysObjContactLinks *pObjContactLinks;

   PHYSICS_CONTACT_SPEW(objID, ("Obj %d(%d) leaves vertex\n", objID, subModId));

   if (m_contactLinkHash.Lookup(objID, &pObjContactLinks))
   {
      if (pObjContactLinks->DestroyTerrainLink(subModId, point))
      {
         delete pObjContactLinks;
         m_contactLinkHash.Delete(objID);
      }
   }
}

///////////////////////////////////////

void cPhysContactLinks::DestroyTerrainLinks(ObjID objID)
{
   cPhysObjContactLinks *pObjContactLinks;

   if (m_contactLinkHash.Lookup(objID, &pObjContactLinks))
   {
      delete pObjContactLinks;
      m_contactLinkHash.Delete(objID);
   }
}

///////////////////////////////////////

BOOL cPhysContactLinks::GetTerrainLinks(ObjID objID, tPhysSubModId subModId, cFaceContactList **ppFaceContactList) const
{
   cPhysObjContactLinks *pObjContactLinks;

   if (m_contactLinkHash.Lookup(objID, &pObjContactLinks))
      return pObjContactLinks->GetTerrainLinkList(subModId, ppFaceContactList);

   *ppFaceContactList = NULL;
   return FALSE;
}

BOOL cPhysContactLinks::GetTerrainLinks(ObjID objID, tPhysSubModId subModId, cEdgeContactList **ppEdgeContactList) const
{
   cPhysObjContactLinks *pObjContactLinks;

   if (m_contactLinkHash.Lookup(objID, &pObjContactLinks))
      return pObjContactLinks->GetTerrainLinkList(subModId, ppEdgeContactList);

   *ppEdgeContactList = NULL;
   return FALSE;
}

BOOL cPhysContactLinks::GetTerrainLinks(ObjID objID, tPhysSubModId subModId, cVertexContactList **ppVertexContactList) const
{
   cPhysObjContactLinks *pObjContactLinks;

   if (m_contactLinkHash.Lookup(objID, &pObjContactLinks))
      return pObjContactLinks->GetTerrainLinkList(subModId, ppVertexContactList);

   *ppVertexContactList = NULL;
   return FALSE;
}

///////////////////////////////////////
//
// Create object->object link
//

void cPhysContactLinks::CreateOneWayObjectLink(ObjID objID, tPhysSubModId subModId, ObjID objID2,
                                               tPhysSubModId subModId2, cPhysModel * pModel2)
{
   cPhysObjContactLinks *pObjContactLinks;

   if (!m_contactLinkHash.Lookup(objID, &pObjContactLinks))
   {
      pObjContactLinks = new cPhysObjContactLinks;
      m_contactLinkHash.Insert(objID, pObjContactLinks);
   }

   pObjContactLinks->CreateObjectLink(subModId, objID2, subModId2, pModel2);
}

///////////////////////////////////////
//
// Create object<->object link
//

void cPhysContactLinks::CreateObjectLink(ObjID objID, tPhysSubModId subModId, cPhysModel * pModel,
                                         ObjID objID2, tPhysSubModId subModId2, cPhysModel * pModel2)
{
#ifdef DBG_ON
   if (g_PhysicsDebug.TestDebugObj(objID, kPhysicsDebugContact) ||
       g_PhysicsDebug.TestDebugObj(objID2, kPhysicsDebugContact))
      mprintf("Obj %d(%d) contacts Obj %d(%d)\n", objID, subModId, objID2, subModId2);
#endif

   // create links both ways
   CreateOneWayObjectLink(objID, subModId, objID2, subModId2, pModel2);
   CreateOneWayObjectLink(objID2, subModId2, objID, subModId, pModel);
}

///////////////////////////////////////

void cPhysContactLinks::DestroyOneWayObjectLink(ObjID objID, tPhysSubModId subModId, ObjID objID2,
                                                tPhysSubModId subModId2)
{
   cPhysObjContactLinks *pObjContactLinks;

   if (m_contactLinkHash.Lookup(objID, &pObjContactLinks))
   {
      if (pObjContactLinks->DestroyObjectLink(subModId, objID2, subModId2))
      {
         delete pObjContactLinks;
         m_contactLinkHash.Delete(objID);
      }
   }
}

///////////////////////////////////////
//
// Destroy object link in both directions
//

void cPhysContactLinks::DestroyObjectLink(ObjID objID, tPhysSubModId subModId, ObjID objID2,
                                          tPhysSubModId subModId2)
{
#ifdef DBG_ON
   if (g_PhysicsDebug.TestDebugObj(objID, kPhysicsDebugContact) ||
       g_PhysicsDebug.TestDebugObj(objID2, kPhysicsDebugContact))
      mprintf("Obj %d(%d) leaves Obj %d(%d)\n", objID, subModId, objID2, subModId2);
#endif

   DestroyOneWayObjectLink(objID, subModId, objID2, subModId2);
   DestroyOneWayObjectLink(objID2, subModId2, objID, subModId);
}

///////////////////////////////////////

void cPhysContactLinks::DestroyObjectLinks(ObjID objID)
{
   cPhysObjContactLinks *pObjContactLinks;

   if (m_contactLinkHash.Lookup(objID, &pObjContactLinks))
   {
      delete pObjContactLinks;
      m_contactLinkHash.Delete(objID);
   }
}

///////////////////////////////////////

BOOL cPhysContactLinks::GetObjectLinks(ObjID objID, tPhysSubModId subModId,
                                       cPhysSubModelInst ** ppSubModel) const
{
   cPhysObjContactLinks *pObjContactLinks;
   if (m_contactLinkHash.Lookup(objID, &pObjContactLinks))
      return pObjContactLinks->GetObjectLinks(subModId, ppSubModel);
   return FALSE;
}


///////////////////////////////////////////////////////////////////////////////

