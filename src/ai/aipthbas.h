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
#include <darklimit.h>

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
   int32 key;
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

#define MAX_AI_PATH_CELLS       DARK_LIMIT_RUNTIME_AIPATH_INDEX
#define MAX_CELLS               DARK_LIMIT_RUNTIME_AIPATH_SLOTS

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

// Runtime path cells use native-width indices.  The 3.3 and older codecs
// explicitly pack these into their historical 16-bit disk records; NewDark
// 3.4 can therefore be decoded without wrapping large cell-vertex tables.
struct sAIPathCell // 48 bytes
{
                                 // what vertices am I made of?
                                 // what other cells am I connected to?
                                 // normal of path cell
                                 // pathfinding data

/* 0*/   tAIPathCell2VertexLinkID firstVertex; // in pathCell2VertexLink array

/* 4*/   tAIPathCell2CellLinkID firstCell;   // in pathCell2CellLink array

/* 8*/   tAIPathCellID plane;

/*12*/   tAIPathCellID next;            // in the A* search open list
/*16*/   tAIPathCellID bestNeighbor;    // bestNeighbor yet found during the search, index in g_AIPathDB.m_Cells
/*20*/   tAIPathCell2CellLinkID linkFromNeighbor;// link used to connect neighbor to me

/*24*/   ushort vertexCount; // in pathCell2VertexLink array

/*26*/   BYTE pathFlags;

/*27*/   uchar cellCount;   // in pathCell2CellLink array

/*28*/   uchar wrapFlags; // legacy editor-build bookkeeping; normalized to zero on load
         uchar runtimePad[3];

/*32*/   cMxsVector center;

         // extra info needed for each cell, to help decide what motions to
         // play & whatever; all handled in terms of bits

         // constants matching bit field sizes
         #define kAIPathCellNumLightLevels 16
         #define kAIPathCellNumStairSizes 4

         // Each group of fields represents one byte.  4 bytes total.

         uint m_LightLevel : 4;
         uint m_IsRamp : 1;    // Is it slanted?
         uint m_IsStair : 1;   // Are nearby surfaces a little lower?
/*44*/   uint m_StairSize : 2;

         uint m_Volume : 3;
         uint m_CeilingHeight : 3;
         uint m_Doorway : 1;
/*45*/   uint m_Corner : 1;

         uint m_CliffEdge : 1;
         uint m_Water : 1;
/*46*/   uint pad30_2 : 6;

/*47*/   uint pad31 : 8;

/*END 48*/
};

#define AIAddrCellInfo(p)  (((uchar *)(&((p)->center))) + sizeof((p)->center))
#define kSizeofCellInfo    4

///////////////////////////////////////

// link data is precalculated data to pass from one to cell to
// another.  Data is directional in nature.
struct sAIPathCellLink // 16 bytes
{
/* 0*/   tAIPathCellID   dest;     // who does the link point to?
/* 4*/   tAIPathVertexID vertex_1,
/* 8*/                   vertex_2; // what vertices do I head towards to reach the other cell
/*12*/   tAIPathOkBits   okBits;   // which AIs can use the link?
/*13*/   uchar           runtimePad;
/*14*/   ushort          cost;     // how expensive is it?
/*END 16*/
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIPTHBAS_H */
