/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phcontct.h,v 1.17 2000/02/22 20:01:09 toml Exp $
//
// Physics contact links
//
// Terrain links:
//    Current implementation is a hash table mapping ObjIDs to another
//    mapping from subModIds to the actual terrain polygon IDs.
//    The first mapping is implemented with a hash table, the second
//    with a dynamic array. Each element in this array is a list of
//    polygon IDs.
//
// Object links:
//    implemented the same way, we keep two records to make it bidirectional
//
#pragma once

#ifndef __PHCONTCT_H
#define __PHCONTCT_H

#include <objtype.h>
#include <hashpp.h>
#include <matrix.h>

#include <phsubmod.h>
#include <phterr.h>
#include <phystyp2.h>
#include <dbmem.h>

#pragma pack(4)

class cPhysModel;
class cPhysOBBModel;
class cPhysSphereModel;

static const long kFaceContactCode   = 1;
static const long kEdgeContactCode   = 2;
static const long kVertexContactCode = 3;
static const long kObjectContactCode = 4;
static const long kDoneContactCode   = 5;

///////////////////////////////////////////////////////////////////////////////

class cFacePoly;
class cFaceContact;

typedef cDList<cFaceContact, 0>     cFaceContactListBase;
typedef cDListNode<cFaceContact, 0> cFaceContactNode;

class cFacePoly
{
public:
   cFacePoly();
   cFacePoly(const cFacePoly *poly);
   ~cFacePoly();

   mxs_vector normal;
   mxs_real   d;

   cDynArray<cFacePoly *> edgePlaneList;
};

////////////////////////////////////////

class cFaceContact : public cFaceContactNode
{
public:
   cFaceContact(const cFacePoly *poly, ObjID objID);
   cFaceContact(int cellID, int polyID);
   cFaceContact(cPhysOBBModel *pModel, int side, BOOL endLoc = FALSE);
   cFaceContact(cPhysSphereModel *pModel, mxs_real radius, BOOL endLoc = FALSE);
   ~cFaceContact() { delete m_pPoly; }; 

   const mxs_vector &GetNormal() const;
   const mxs_real   &GetPlaneConst() const;

   const cFacePoly  *GetPoly() const;

   ObjID GetObjID() const;

private:

   cFacePoly *m_pPoly;

   ObjID m_objID;
};

inline cFaceContact::cFaceContact(const cFacePoly *poly, ObjID objID)
{
   m_pPoly = new cFacePoly(poly);
   m_objID = objID;
}

inline const mxs_vector &cFaceContact::GetNormal() const
{
   return m_pPoly->normal;
}

inline const mxs_real &cFaceContact::GetPlaneConst() const
{
   return m_pPoly->d;
}

inline const cFacePoly *cFaceContact::GetPoly() const
{
   return m_pPoly;
}

inline ObjID cFaceContact::GetObjID() const
{
   return m_objID;
}

////////////////////////////////////////

class cFaceContactList : public cFaceContactListBase
{
public:
   BOOL Find(const cFacePoly *poly, cFaceContact **ppFaceContact);
};

///////////////////////////////////////////////////////////////////////////////

class cEdgeContact;

typedef cDList<cEdgeContact, 0>     cEdgeContactListBase;
typedef cDListNode<cEdgeContact, 0> cEdgeContactNode;

////////////////////////////////////////

class cEdgeContact : public cEdgeContactNode
{
public:
   cEdgeContact(const mxs_vector &start, const mxs_vector &end);

   const mxs_vector &GetStart() const;
   const mxs_vector &GetEnd() const;

   const mxs_vector &GetVector() const;
   
   mxs_vector  GetNormal(const mxs_vector &pt) const;
   mxs_real    GetDist(const mxs_vector &pt) const;

private:
   mxs_vector m_Start;
   mxs_vector m_End;
   mxs_vector m_Vector;
};

inline cEdgeContact::cEdgeContact(const mxs_vector &start, const mxs_vector &end)
{
   mx_copy_vec(&m_Start, (mxs_vector *) &start);
   mx_copy_vec(&m_End, (mxs_vector *)  &end);
   mx_sub_vec(&m_Vector, (mxs_vector *) &end, (mxs_vector *) &start);
}

inline const mxs_vector & cEdgeContact::GetStart() const
{
   return m_Start;
}

inline const mxs_vector & cEdgeContact::GetEnd() const
{
   return m_End;
}

inline const mxs_vector & cEdgeContact::GetVector() const
{
   return m_Vector;
}

////////////////////////////////////////

class cEdgeContactList : public cEdgeContactListBase
{
public:
   BOOL Find(const mxs_vector &start, const mxs_vector &end, cEdgeContact **ppEdgeContact);
};

///////////////////////////////////////////////////////////////////////////////

class cVertexContact;

typedef cDList<cVertexContact, 0>     cVertexContactListBase;
typedef cDListNode<cVertexContact, 0> cVertexContactNode;

////////////////////////////////////////

class cVertexContact : public cVertexContactNode
{
public:
   cVertexContact(const mxs_vector &point);

   const mxs_vector &GetPoint() const;
   const mxs_vector  GetNormal(const mxs_vector &pt) const;

private:
   mxs_vector m_Point;
};

inline cVertexContact::cVertexContact(const mxs_vector &point)
{
   mx_copy_vec(&m_Point, (mxs_vector *) &point);
}

inline const mxs_vector &cVertexContact::GetPoint() const
{
   return m_Point;
}

////////////////////////////////////////

class cVertexContactList : public cVertexContactListBase
{
public:
   BOOL Find(const mxs_vector &point, cVertexContact **ppVertexContact);
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysObjContactLinks
//
// Class to manage all links for a particular object
//

class cPhysObjContactLinks
{
public:
   cPhysObjContactLinks(void);
   ~cPhysObjContactLinks(void);

   // read and write links to disk
   void Write(PhysReadWrite func, ObjID objID) const;

   void CreateTerrainLink(tPhysSubModId subModId, const cFacePoly *poly, ObjID objID);
   void CreateTerrainLink(tPhysSubModId subModId, const mxs_vector &start, const mxs_vector &end);
   void CreateTerrainLink(tPhysSubModId subModId, const mxs_vector &point);

   BOOL DestroyTerrainLink(tPhysSubModId subModId, const cFacePoly *poly);
   BOOL DestroyTerrainLink(tPhysSubModId subModId, const mxs_vector &start, const mxs_vector &end);
   BOOL DestroyTerrainLink(tPhysSubModId subModId, const mxs_vector &point);
 
   BOOL GetTerrainLink(tPhysSubModId subModId, const cFacePoly *poly, cFaceContact **ppFaceContact) const;
   BOOL GetTerrainLink(tPhysSubModId subModId, const mxs_vector &start, const mxs_vector &end, cEdgeContact **ppEdgeContact) const;
   BOOL GetTerrainLink(tPhysSubModId subModId, const mxs_vector &point, cVertexContact **ppVertexContact) const;

   BOOL GetTerrainLinkList(tPhysSubModId subModId, cFaceContactList **ppFaceContactList) const;
   BOOL GetTerrainLinkList(tPhysSubModId subModId, cEdgeContactList **ppEdgeContactList) const;
   BOOL GetTerrainLinkList(tPhysSubModId subModId, cVertexContactList **ppVertexContactList) const;

   void CreateObjectLink(tPhysSubModId subModId, ObjID objID2, tPhysSubModId subModId2, cPhysModel *pModel);
   BOOL DestroyObjectLink(tPhysSubModId subModId, ObjID objID2, tPhysSubModId subModId2);
   void DestroyObjectLinks();
   BOOL GetObjectLink(tPhysSubModId subModId, ObjID objID2,
                      tPhysSubModId subModId2, cPhysSubModelInst **ppSubModel) const;
   BOOL GetObjectLinks(tPhysSubModId subModId, cPhysSubModelInst **ppSubModel) const;

private:
   cDynArray<cFaceContactList *>    m_FaceContactList;
   cDynArray<cEdgeContactList *>    m_EdgeContactList;
   cDynArray<cVertexContactList *>  m_VertexContactList;

   cPhysSubModelListArray m_subModelListArray;

   // @TBD: Needed?
   int                    m_linkCount;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysContactLinks
//
// Class to manage all links for all objects
//

class cPhysContactLinks
{
public:

   // read and write links to disk
   void Write(PhysReadWrite func) const;
   void Read(PhysReadWrite func);

   void Reset();

   void WriteDone(PhysReadWrite func) const;

   // create, destroy, query terrain link
   void CreateTerrainLink(ObjID objID, tPhysSubModId subModId, const cFacePoly *poly, ObjID contactObj);
   void CreateTerrainLink(ObjID objID, tPhysSubModId subModId, const mxs_vector &start, const mxs_vector &end);
   void CreateTerrainLink(ObjID objID, tPhysSubModId subModId, const mxs_vector &point);

   void DestroyTerrainLink(ObjID objID, tPhysSubModId subModId, const cFacePoly *poly);
   void DestroyTerrainLink(ObjID objID, tPhysSubModId subModId, const mxs_vector &start, const mxs_vector &end);
   void DestroyTerrainLink(ObjID objID, tPhysSubModId subModId, const mxs_vector &point);

   void DestroyTerrainLinks(ObjID objID);

   BOOL GetTerrainLinks(ObjID objID, tPhysSubModId subModId, cFaceContactList **ppFaceContactList) const;
   BOOL GetTerrainLinks(ObjID objID, tPhysSubModId subModId, cEdgeContactList **ppEdgeContactList) const;
   BOOL GetTerrainLinks(ObjID objID, tPhysSubModId subModId, cVertexContactList **ppVertexContactList) const;

   // create, destroy, query object link
   void CreateObjectLink(ObjID objID1, tPhysSubModId subModId1, cPhysModel * pModel,
                          ObjID objID2, tPhysSubModId subModId2, cPhysModel * pModel2);
   void DestroyObjectLink(ObjID objID1, tPhysSubModId subModId1, ObjID objID2,
                           tPhysSubModId subModId2);
   void DestroyObjectLinks(ObjID objID);
   BOOL GetObjectLinks(ObjID objID, tPhysSubModId subModId, cPhysSubModelInst ** ppSubModel) const;

private:
   void CreateOneWayObjectLink(ObjID objID, tPhysSubModId subModId, ObjID objID2,
                                tPhysSubModId subModId2, cPhysModel * pModel2);
   void DestroyOneWayObjectLink(ObjID objID1, tPhysSubModId subModId1, ObjID objID2,
                                 tPhysSubModId subModId2);

   // hash table maps objIDs to an object contact class
   typedef cHashTableFunctions<ObjID> DefaultHashFunctions;
   typedef cHashTable<ObjID, cPhysObjContactLinks *, DefaultHashFunctions> cContactLinkHash;

   cContactLinkHash m_contactLinkHash;
};

///////////////////////////////////////////////////////////////////////////////
//
// API for object/object contact
//
// modifies the models as well as making the link
//

extern void CreateObjectContact(ObjID objID1, tPhysSubModId subModId1, cPhysModel * pModel1,
                                 ObjID objID2, tPhysSubModId subModId2, cPhysModel * pModel2);

///////////////////////////////////////

extern void DestroyObjectContact(ObjID objID1, tPhysSubModId subModId1, cPhysModel * pModel1,
                                  ObjID objID2, tPhysSubModId subModId2, cPhysModel * pModel2);

///////////////////////////////////////

extern BOOL GetObjectContacts(ObjID objID1, tPhysSubModId subModId1, cPhysModel * pModel,
                               cPhysSubModelInst ** ppSubModel);

///////////////////////////////////////

extern void DestroyAllObjectContacts(ObjID objID, tPhysSubModId subModId, cPhysModel * pModel);

///////////////////////////////////////////////////////////////////////////////
//
// Globals
//

extern cPhysContactLinks g_PhysContactLinks;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysObjContactLinks, inline functions
//

inline cPhysObjContactLinks::cPhysObjContactLinks(void)
 : m_linkCount(0)
{
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#include <undbmem.h>

#endif /* !__PHCONTCT_H */












