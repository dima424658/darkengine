/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aipathdb.h,v 1.27 2000/03/01 13:02:52 bfarquha Exp $
//
//
//

#ifndef __AIPATHDB_H
#define __AIPATHDB_H

#include <aipthbas.h>
#include <aipthzon.h>
#include <aiokbits.h>

#include <dbmem.h>

#pragma pack(1)
#pragma once

#define HUGE_Z    ((float)5.10)

// Anything less than 2 feet high will use SmallCreature links.
#define AIPATHDB_SmallCreatureHeight 0.5


F_DECLARE_INTERFACE(ITagFile);
class cAIPath;

///////////////////////////////////////////////////////////////////////////////

BOOL AIPathFindDBBuild();

void AIPathFindUpdate();

void AIPathFindMTWaypointHit(ObjID movingTerrain, ObjID waypoint);
void AIPathFindMTWaypointLeft(ObjID movingTerrain);

void AIPathFindRead(ITagFile *);
void AIPathFindWrite(ITagFile *);

// Given flee point, return flee region, else -1.
int AIGetFleeRegion(ObjID id);

///////////////////////////////////////////////////////////////////////////////

struct sAICellLineIntersect
{
   unsigned   n;
   mxs_vector intersects[2];
};

///////////////////////////////////////////////////////////////////////////////

struct sAIExternCell
{
   struct sPlane
   {
      mxs_vector normal;
      float      constant;
   };

   sPlane     plane;
   unsigned   nVertices;
   mxs_vector vertices[1];
};

inline sAIExternCell * AINewExternCell(unsigned nVertices)
{
   sAIExternCell * p = (sAIExternCell *)malloc(sizeof(sAIExternCell) + sizeof(mxs_vector) * nVertices);
   p->nVertices      = nVertices;
   return p;
};

inline sAIExternCell * AINewExternCell(const mxs_vector & normal, float constant, unsigned nVertices)
{
   sAIExternCell * p = AINewExternCell(nVertices);
   p->plane.normal   = normal;
   p->plane.constant = constant;
   return p;
};


struct sFleeRegionData
{
   int nRegionId;
   ObjID id;
};

class cFleeRegionsHash : public cHashSet<sFleeRegionData *, const ObjID, cHashFunctions>
{
public:
   virtual tHashSetKey GetKey(tHashSetNode p) const
   {
      return (tHashSetKey) ((sFleeRegionData *)p)->id;
   }
};



struct sZoneDatabase
{
   cZonePairTable                         m_ZonePairTable;
   int                                    m_nZones;

   // array of zones for each cell
   cAIPathArray<tAIPathZone>              m_CellZones;
};



///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIPathDB
//

class cAIPathDB
{
public:

   cAIPathDB();

   void Init();
   void Term();

   ////////////////////////////////////

   unsigned NumCells() const;

   const sAIPathCell *     GetCell(const tAIPathCellID cell) const;
   sAIPathCell *           AccessCell(const tAIPathCellID cell);
   const sAIPathCellLink * GetCellLink(tAIPathCell2CellLinkID linkId) const;

   const mxs_vector & GetCenter(const tAIPathCellID cell, mxs_vector * pCenter) const;
   const mxs_vector & GetCenter(const tAIPathCellID cell) const;

   const sAIPathCellPlane * GetCellPlane(const tAIPathCellID cell) const;
   const sAIPathCellPlane * GetCellPlane(const sAIPathCell * pCell) const;

   ////////////////////////////////////

   const mxs_vector & GetCellVertex(tAIPathCellID cell, int index) const;
   tAIPathVertexID    GetCellVertexID(tAIPathCellID cell, int index) const;
#if 0
   int                GetCellEdgeInfo(tAIPathCellID cell, int index) const;
#endif
   void               GetCellVertexFull(tAIPathCellID cell, int index, mxs_vector * pVertex, tAIPathVertexID * pVertexID) const;
#if 0
   BOOL               IsCellEdgeLinked(tAIPathCellID cell, int index) const;
#endif
   const mxs_vector & GetVertex(tAIPathVertexID id) const;
   int                GetVertexInfo(tAIPathVertexID id) const;

   ////////////////////////////////////

   ObjID GetCellDoor(tAIPathCellID cell);

   ////////////////////////////////////

   sbool IsUnpathable(tAIPathCellID cell);
   sbool IsBelowDoor(tAIPathCellID cell);
   sbool IsBlockedOBB(tAIPathCellID cell);
   sbool IsActiveMovingTerrain(tAIPathCellID cell);

   ////////////////////////////////////

   void MarkBegin();
   void Mark(tAIPathCellID cell);
   void Unmark(tAIPathCellID cell);
   BOOL GetMark(tAIPathCellID cell);
   void MarkEnd();

   ////////////////////////////////////

   // wsf: doen't use condition bits for zones.
   BOOL          CanPathfindBetweenZones(eAIPathZoneType ZoneType, tAIPathZone zone1, tAIPathZone zone2, tAIPathOkBits okBits);

   tAIPathZone   GetCellZone(eAIPathZoneType ZoneType, tAIPathCellID cell);
   tAIPathOkBits GetZoneOkBits(eAIPathZoneType ZoneType, tAIPathZone zone1, tAIPathZone zone2);
   void          SetZoneOkBits(eAIPathZoneType ZoneType, tAIPathZone zone1, tAIPathZone zone2, tAIPathOkBits okBits);

   ////////////////////////////////////

   float GetZAtXY(const sAIPathCell * pCell, const mxs_vector & pt) const;
   float GetZAtXY(const tAIPathCellID cell, const mxs_vector & pt) const;

   float GetHighestZ(tAIPathCellID cell) const;
   float GetLowestZ(tAIPathCellID cell) const;

   ////////////////////////////////////

   float ComputeArea(tAIPathCellID cell) const;

   ////////////////////////////////////

   void DeleteCell(tAIPathCellID cell);
   void GetCell(tAIPathCellID cell, sAIExternCell **);
   BOOL AddOBBCell(sAIExternCell *, tAIPathCellID cell = 0, BOOL movable = FALSE);

   int GetLineIntersections(tAIPathCellID cell, const mxs_vector & line1, const mxs_vector & line2, sAICellLineIntersect * pIntersect) const;
   BOOL SplitCellByLine(tAIPathCellID cell, const mxs_vector & line1, const mxs_vector & line2);
   BOOL SplitCell(tAIPathCellID cell, const mxs_vector & pt1, const mxs_vector & pt2);

   // @TBD (toml 08-24-98):    void Compact();

   ////////////////////////////////////

private:

   friend BOOL AIPathFind(const cMxsVector & fromLocation, tAIPathCellID startCell,
                          const cMxsVector & toLocation, tAIPathCellID endCell,
                          cAIPath * pPath, tAIPathOkBits OkBits, tAIPathOkBits okCondBits = 0);

public:
   // array of cells
   cAIPathArray<sAIPathCell>              m_Cells;           // 1..count, zero is invalid
   tAIPathCellID                          m_nCells;

   // array of cell plane info
   cAIPathArray<sAIPathCellPlane>         m_Planes;
   tAIPathCellID                          m_nPlanes;

   // array of links between cells
   cAIPathArray<sAIPathCellLink>          m_Links;  // 1..count, zero is invalid
   tAIPathCell2CellLinkID                 m_nLinks;

   // array of vertices
   cAIPathArray<sAIPathVertex>            m_Vertices;         // 1..count, zero is invalid
   tAIPathVertexID                        m_nVertices;

   // array of vertices owned by cells
   cAIPathArray<sAIPathCell2VertexLink>   m_CellVertices;// 1..count, zero is invalid
   tAIPathCell2VertexLinkID               m_nCellVertices;

   // array of object hints
   cAIPathArray<tAIPathCellID>            m_ObjHints;
   int                                    m_nObjHints;

   // mapping of objects to the cells they've created
   cAIPathArray<sAIPathCellObjMap>        m_CellObjMap;
   int                                    m_nCellObjMaps;

   sZoneDatabase                          m_ZoneDatabases[kAIZone_Num];

   // Cell to door mapping
   cAICellDoorTable                       m_CellToDoorsTable;
   cAIPathArray<sAICellDoor>              m_CellDoors;

   // List of all movable cells
   cDynClassArray<sAIMovableCell>         m_MovableCells;

   // scratch arrays for database searching/traversing
   cAIPathArray<sbool>                    m_Marks;
   cDynArray_<tAIPathCellID, 128>         m_MarkedCells;
   int                                    m_nMarkedCells;
   cAIPathArray<ushort>                   m_BestCostToReach;

   cFleeRegionsHash                       m_FleeRegions;
   int                                    m_nNumFleeRegions;
};

extern cAIPathDB g_AIPathDB;

///////////////////////////////////////////////////////////////////////////////

inline unsigned cAIPathDB::NumCells() const
{
   return m_nCells;
}

///////////////////////////////////////

inline const sAIPathCell * cAIPathDB::GetCell(const tAIPathCellID cell) const
{
   return &m_Cells[cell];
}

///////////////////////////////////////

inline const sAIPathCellLink * cAIPathDB::GetCellLink(tAIPathCell2CellLinkID linkId) const
{
   return &m_Links[linkId];
}

///////////////////////////////////////

inline sAIPathCell * cAIPathDB::AccessCell(const tAIPathCellID cell)
{
   return &m_Cells[cell];
}

///////////////////////////////////////

inline const mxs_vector & cAIPathDB::GetCenter(const tAIPathCellID cell, mxs_vector * pCenter) const
{
   return (*pCenter = m_Cells[cell].center);
}

///////////////////////////////////////

inline const mxs_vector & cAIPathDB::GetCenter(const tAIPathCellID cell) const
{
   return m_Cells[cell].center;
}

///////////////////////////////////////

inline const sAIPathCellPlane * cAIPathDB::GetCellPlane(const tAIPathCellID cell) const
{
   return &m_Planes[m_Cells[cell].plane];
}

///////////////////////////////////////

inline const sAIPathCellPlane * cAIPathDB::GetCellPlane(const sAIPathCell * pCell) const
{
   return &m_Planes[pCell->plane];
}

///////////////////////////////////////

inline float cAIPathDB::GetZAtXY(const sAIPathCell * pCell, const mxs_vector & pt) const
{
   const sAIPathCellPlane * pPlane = &m_Planes[pCell->plane];
   return ((-pPlane->constant - pPlane->normal.x * pt.x - pPlane->normal.y * pt.y) / pPlane->normal.z);
}

///////////////////////////////////////

inline float cAIPathDB::GetZAtXY(const tAIPathCellID cell, const mxs_vector & pt) const
{
   return GetZAtXY(&m_Cells[cell], pt);
}

///////////////////////////////////////

// Normalize a vertex index.
#define cAIPathDB_NormalizeVertexIndex(i) \
   { \
      const int _nVertices = m_Cells[cell].vertexCount; \
      \
      if (index == _nVertices) \
         index = 0; \
      else if (index > _nVertices) \
         index %= _nVertices; \
      else if (index < 0) \
         index = _nVertices + (index % _nVertices); \
   }

///////////////////////////////////////

inline const mxs_vector & cAIPathDB::GetCellVertex(tAIPathCellID cell, int index) const
{
   cAIPathDB_NormalizeVertexIndex(index);
   return m_Vertices[m_CellVertices[m_Cells[cell].firstVertex + index].id].pt;
}

///////////////////////////////////////

inline tAIPathVertexID cAIPathDB::GetCellVertexID(tAIPathCellID cell, int index) const
{
   cAIPathDB_NormalizeVertexIndex(index);
   return m_CellVertices[m_Cells[cell].firstVertex + index].id;
}

///////////////////////////////////////

inline void cAIPathDB::GetCellVertexFull(tAIPathCellID cell, int index, mxs_vector * pVertex, tAIPathVertexID * pVertexID) const
{
   cAIPathDB_NormalizeVertexIndex(index);
   *pVertexID = m_CellVertices[m_Cells[cell].firstVertex + index].id;
   *pVertex   = m_Vertices[*pVertexID].pt;
}

///////////////////////////////////////
#if 0
inline int cAIPathDB::GetCellEdgeInfo(tAIPathCellID cell, int index) const
{
   cAIPathDB_NormalizeVertexIndex(index);
   return m_CellVertices[m_Cells[cell].firstVertex + index].edgeInfo;
}
#endif
///////////////////////////////////////
#if 0
inline BOOL cAIPathDB::IsCellEdgeLinked(tAIPathCellID cell, int index) const
{
   return m_CellVertices[m_Cells[cell].firstVertex + index].hasLinks;
}
#endif
///////////////////////////////////////

inline const mxs_vector & cAIPathDB::GetVertex(tAIPathVertexID id) const
{
   return m_Vertices[id].pt;
}

///////////////////////////////////////

inline int cAIPathDB::GetVertexInfo(tAIPathVertexID id) const
{
   return m_Vertices[id].ptInfo;
}

///////////////////////////////////////

inline ObjID cAIPathDB::GetCellDoor(tAIPathCellID cell)
{
   sAICellDoor * pCellDoor = m_CellToDoorsTable.Search(cell);
   if (!pCellDoor)
      return OBJ_NULL;
   return pCellDoor->door;
}

///////////////////////////////////////

inline sbool cAIPathDB::IsUnpathable(tAIPathCellID cell)
{
   return (m_Cells[cell].pathFlags & kAIPF_Unpathable);
}

///////////////////////////////////////

inline sbool cAIPathDB::IsBelowDoor(tAIPathCellID cell)
{
   return (m_Cells[cell].pathFlags & kAIPF_BelowDoor);
}

///////////////////////////////////////

inline sbool cAIPathDB::IsBlockedOBB(tAIPathCellID cell)
{
   return (m_Cells[cell].pathFlags & kAIPF_BlockingOBB);
}

///////////////////////////////////////

inline sbool cAIPathDB::IsActiveMovingTerrain(tAIPathCellID cell)
{
   return (m_Cells[cell].pathFlags & kAIPF_ActiveMovingTerrain);
}

///////////////////////////////////////

inline void cAIPathDB::MarkBegin()
{
   m_nMarkedCells = 0;
}

///////////////////////////////////////

inline void cAIPathDB::Mark(tAIPathCellID cell)
{
   m_Marks[cell] = TRUE;
   if (m_MarkedCells.Size() == m_nMarkedCells)
      m_MarkedCells.Grow(16);
   m_MarkedCells[m_nMarkedCells] = cell;
   m_nMarkedCells++;
}

///////////////////////////////////////

inline void cAIPathDB::Unmark(tAIPathCellID cell)
{
   m_Marks[cell] = FALSE;
}

///////////////////////////////////////

inline BOOL cAIPathDB::GetMark(tAIPathCellID cell)
{
   return m_Marks[cell];
}

///////////////////////////////////////

inline void cAIPathDB::MarkEnd()
{
   register int                         i   = m_nMarkedCells;
   register sbool * const               pm  = m_Marks.AsPointer();
   register const tAIPathCellID * const pid = m_MarkedCells.AsPointer();

   while (--i >= 0)
      *(pm + *(pid + i)) = FALSE;
}

///////////////////////////////////////////////////////////////////////////////

inline BOOL cAIPathDB::CanPathfindBetweenZones(eAIPathZoneType ZoneType, tAIPathZone zone1, tAIPathZone zone2, tAIPathOkBits okBits)
{
   if (zone1 == zone2)
      return TRUE;

   if (zone1 == AI_ZONE_ALL || zone2 == AI_ZONE_ALL)
      return TRUE;

   if (zone1 == AI_ZONE_SOLO || zone2 == AI_ZONE_SOLO)
      return FALSE;

   // the okbits must satisfy all okbits between the zones
   tAIPathOkBits zoneOkBits = GetZoneOkBits(ZoneType, zone1, zone2);

   if ((zoneOkBits != 0) && ((zoneOkBits & okBits & kNoConditionMask) == zoneOkBits))
      return TRUE;
   else
      return FALSE;
}

////////////////////////////////////////

inline tAIPathZone cAIPathDB::GetCellZone(eAIPathZoneType ZoneType, tAIPathCellID cell)
{
   if (cell < m_ZoneDatabases[ZoneType].m_CellZones.Size())
      return m_ZoneDatabases[ZoneType].m_CellZones[cell];
   else
      return 0;
}

////////////////////////////////////////

inline tAIPathOkBits cAIPathDB::GetZoneOkBits(eAIPathZoneType ZoneType, tAIPathZone zone1, tAIPathZone zone2)
{
   if (zone1 == AI_ZONE_SOLO || zone2 == AI_ZONE_SOLO)
      return 0;

   int key = (zone1 << 16) | zone2;

   sZoneOkBitsMap *pMap = m_ZoneDatabases[ZoneType].m_ZonePairTable.Search(key);

   if (pMap == NULL)
      return 0;
   else
      return pMap->okBits;
}

////////////////////////////////////////

inline void cAIPathDB::SetZoneOkBits(eAIPathZoneType ZoneType, tAIPathZone zone1, tAIPathZone zone2, tAIPathOkBits okBits)
{
   if (zone1 == AI_ZONE_SOLO || zone2 == AI_ZONE_SOLO)
      return;

   sZoneOkBitsMap *pMap = new sZoneOkBitsMap;

   pMap->key = (zone1 << 16) | zone2;
   pMap->okBits = okBits;

   m_ZoneDatabases[ZoneType].m_ZonePairTable.RemoveByKey(pMap->key);
   m_ZoneDatabases[ZoneType].m_ZonePairTable.Insert(pMap);
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#include <undbmem.h>

#endif /* !__AIPATHDB_H */
