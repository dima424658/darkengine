/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aipthbas.h,v 1.14 2000/01/31 18:20:33 bfarquha Exp $
//
//
//

#ifndef __AIPTHBAS_H
#define __AIPTHBAS_H

#include <pool.h>
#include <dynarray.h>
#include <hashset.h>

#include <aitype.h>
#include <aipthtyp.h>

#pragma pack(1)
#pragma once

///////////////////////////////////////////////////////////////////////////////
//
// Array type used for path database elements
//

template <class T>
class cAIPathArray : public cDynArray_<T, 1>
{
public:
   cAIPathArray()
   {
   }
};

///////////////////////////////////////////////////////////////////////////////
//
// Hash table used for mapping zone-pairs to okbits
//

struct sZoneOkBitsMap
{
   long key;
   tAIPathOkBits okBits;
   char pad[2];

   DECLARE_POOL();
};

template <class NODE>
class cIntHashSet : public cHashSet <NODE, int, cHashFunctions>
{
public:
   cIntHashSet(unsigned n = cHashSetBase::kDefaultSize)     : cHashSet<NODE, int, cHashFunctions>(n) { }
   cIntHashSet(const cHashSet<NODE, int, cHashFunctions> &) : cHashSet<NODE, int, cHashFunctions>(*this) { }
};

class cZonePairTable : public cIntHashSet<sZoneOkBitsMap *>
{
   virtual tHashSetKey GetKey(tHashSetNode p) const
   {
      return (tHashSetKey) ((sZoneOkBitsMap *)p)->key;
   }
};

///////////////////////////////////////////////////////////////////////////////
//
// Hash table used for mapping cells to doors
//

struct sAICellDoor
{
   tAIPathCellID cell;
   ObjID         door;
};

class cAICellDoorTable : public cHashSet<sAICellDoor *, tAIPathCellID, cHashFunctions>
{
   virtual tHashSetKey GetKey(tHashSetNode p) const
   {
      return (tHashSetKey) ((sAICellDoor *)p)->cell;
   }
};

///////////////////////////////////////////////////////////////////////////////
//
// Pathfind Database (very private!)
//

#define MAX_CELLS                (0xfffe)       // in g_AIPathDB.m_Cells array

#define PATH_EDGE_OPEN  0
#define PATH_EDGE_WALL  1
#define PATH_EDGE_CLIFF 2

///////////////////////////////////////

struct sAIPathVertex // 16 bytes
{
/* 0*/   cMxsVector pt;
/*12*/   int        ptInfo;   // what is this pt like?
/*END 16 */
};

///////////////////////////////////////

struct sAIPathCell2VertexLink // 4 bytes
{
#if 0
/* 0*/   uchar                 __edgeInfo; // what is the edge beween this vertex and the next like?
/* 1*/   uchar                 __hasLinks;
/* 2*/   tAIPathVertexIDPacked id;
#else
/* 4*/   tAIPathVertexID       id;
#endif
/*END 4*/
};

///////////////////////////////////////

struct sAIPathCellPlane // 16 bytes
{
   mxs_vector normal;
   float      constant;
};

///////////////////////////////////////

struct sAIPathCellObjMap // 16 bytes
{
   ObjID         objID;
   tAIPathCellID cellID;
   BOOL          prevPropState;
   void         *data;
};

///////////////////////////////////////

struct sAILinkData // 4 bytes
{
   tAIPathCell2CellLinkIDPacked  id;
   tAIPathOkBits                 bits;
   uchar pad;
};

struct sAIWaypointLinks // 12 bytes
{
   ObjID  waypointObj;
   cDynArray<sAILinkData> linkList;
};

struct sAIMovableCell // 28 bytes
{
   ObjID          movingObj;
   tAIPathCellID  cellID;

   cMxsVector     objVec;

   cDynClassArray<sAIWaypointLinks>  waypointLinkList;
};

///////////////////////////////////////

#define kAIPF_Unpathable  ((BYTE)(0x01))
#define kAIPF_BelowDoor   ((BYTE)(0x02))
#define kAIPF_BlockingOBB ((BYTE)(0x04))
#define kAIPF_ActiveMovingTerrain ((BYTE)(0x08))

///////////////////////////////////////

#define IDWRAP_Cell2Cell 0x01

struct sAIPathCell // 32 bytes
{
                                 // what vertices am I made of?
                                 // what other cells am I connected to?
                                 // normal of path cell
                                 // pathfinding data

/* 0*/   tAIPathCell2VertexLinkIDPacked firstVertex; // in pathCell2VertexLink array

/* 2*/   tAIPathCell2CellLinkIDPacked firstCell;   // in pathCell2CellLink array

/* 4*/   tAIPathCellIDPacked plane;

/* 6*/   tAIPathCellIDPacked  next;            // in the A* search open list
/* 8*/   tAIPathCellIDPacked  bestNeighbor;    // bestNeighbor yet found during the search, index in g_AIPathDB.m_Cells
/*10*/   tAIPathCell2CellLinkIDPacked  linkFromNeighbor;// link used to connect neighbor to me

/*12*/   uchar vertexCount; // in pathCell2VertexLink array

/*13*/   BYTE pathFlags;

/*14*/   uchar cellCount;   // in pathCell2CellLink array

/*15*/   uchar wrapFlags; // set if any ID's have wrapped. Only used in processing, not in game!

/*16*/   cMxsVector center;

         // extra info needed for each cell, to help decide what motions to
         // play & whatever; all handled in terms of bits

         // constants matching bit field sizes
         #define kAIPathCellNumLightLevels 16
         #define kAIPathCellNumStairSizes 4

         // Each group of fields represents one byte.  4 bytes total.

         uint m_LightLevel : 4;
         uint m_IsRamp : 1;    // Is it slanted?
         uint m_IsStair : 1;   // Are nearby surfaces a little lower?
/*28*/   uint m_StairSize : 2;

         uint m_Volume : 3;
         uint m_CeilingHeight : 3;
         uint m_Doorway : 1;
/*29*/   uint m_Corner : 1;

         uint m_CliffEdge : 1;
         uint m_Water : 1;
/*30*/   uint pad30_2 : 6;

/*31*/   uint pad31 : 8;

/*END 32*/
};

#define AIAddrCellInfo(p)  (((uchar *)(&((p)->center))) + sizeof((p)->center))
#define kSizeofCellInfo    4

///////////////////////////////////////

// link data is precalculated data to pass from one to cell to
// another.  Data is directional in nature.
struct sAIPathCellLink // 8 bytes
{
/* 0*/   tAIPathCellIDPacked   dest;     // who does the link point to?
/* 2*/   tAIPathVertexIDPacked vertex_1,
/* 4*/                         vertex_2; // what vertices do I head towards to reach the other cell (index in g_AIPathDB.m_Vertices array)
/* 6*/   tAIPathOkBits         okBits;   // which AIs can use the link?
/* 7*/   uchar                 cost;     // how expensive is it?
/*END 8*/
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIPTHBAS_H */
