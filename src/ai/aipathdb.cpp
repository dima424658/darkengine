/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aipathdb.cpp,v 1.97 2000/03/08 22:11:02 toml Exp $
//
// @Note (toml 08-20-98): this database is in transition from a global to
// a non-global structure so that optimizations I'm thinking of in the future
// can use "scratch" databases. Because of this, there is a somewhat
// confusing mix of styles in here.
//

// @TODO: after ship T2, change all the fabs() added to this file to ffabsf()! (toml 3-8-00)

// #define PROFILE_ON 1

#include <stdlib.h>
#include <math.h>

#include <lg.h>
#include <allocapi.h>
#include <config.h>
#include <cfgdbg.h>
#include <objpos.h>
#include <objsys.h>
#include <objlight.h>
#include <objedit.h>
#include <tagfile.h>
#include <vernum.h>

#include <appagg.h>
#include <gshelapi.h>

#include <status.h>

#include <vec2.h>
#include <mprintf.h>
#include <dynarray.h>
#include <timings.h>
#include <hashset.h>
#include <hshsttem.h>

#include <fltang.h>

#include <wr.h>
#include <portal.h> // just for raycasting!

#include <sphrcst.h>
#include <media.h>
#include <phmterr.h>
#include <aiphycst.h>

#include <roomsys.h>

#include <aiavoid.h>
#include <aitype.h>
#include <aicretsz.h>
#include <aidist.h>
#include <aiutils.h>
#include <aipthtyp.h>
#include <aipathdb.h>
#include <aipthedg.h>
#include <aipthloc.h>
#include <aipthobb.h>
#include <aipthzon.h>
#include <aiokbits.h>
#include <aipthcst.h>
#include <rpaths.h>

#include <osysbase.h>

#include <propbase.h>
#include <propface.h>

// For flee point regions:
#include <aiflee.h>

#include <filevar.h>
#include <sdesbase.h>
#include <sdesc.h>


// Must be last header
#include <dbmem.h>

/////////////////////////////////////////////

// wsf: max z delta allowed for motion capture, generally. Used for jumping on/off tables.
// as far as I can tell, the MaxAscend stuff in the mocap data is set to 1.5 ft, so is
// useless to us as a measure of how far we can make LVLs (large vertical leaps).
//
// After looking at table and bed heights, I'm going to try a vertical delta of 3.1.
//
// wsf: update. Physics cannot slide onto table if the table is more than, oh, about 2.6 ft, at
// least for the sword guard case. So we'll actually use THAT number.
//
// We put it in here instead of aipathdb.cpp so that code that might want to know this number
// (like hand to hand combat) can use it.
//
// #define LVL_SIZE ((float)2.61)

// OK, with recent physics changes, let's use this:

//
// OBJ_LVL_SIZE is used for objects only, since physics can *barely* crawl over objects,
// but can't handle the terrain the same way - the spherecast stuff in 'render' is indecipherable
// to me. So, we have seperate criteria for terrain-generated cells, unfortunately. We need
// to determine what the height is - it's probably around 2.6. We'll need a transient database
// of cell->object mappings, so we can query to see if a cell was contributed by an object.
//

#define OBJ_LVL_SIZE ((float)3.250)

// After some in-game testing, this number seems to be at the limits of physics. We can go
// a little higher, but the AI will do a lot of dancing before it comes up with a solution:
#define TERR_LVL_SIZE ((float)2.410)



/////////////////////////////////////


/////////////////////////////////////////////////
//
// Structure to hold all the data for builing a link between two cells.
// The structure exists to reduce duplicate calculations, and a global is used
// to reduce function call overhead.
//

enum eAIEdgeConnectKind
{
   kNone,
   kPerfect,
   kCell1EdgeContained, // i.e., (2b-1a-1b-2a), connect 1a-1b
   kCell2EdgeContained, // i.e., (1a-2b-2a-1b), connect 2b-2a
   kCell2SkewedLeft,    // i.e., (2b-1a-2a-1b), connect 1a-2a
   kCell2SkewedRight    // i.e., (1a-2b-1b-2a), connect 2b-1b
};

struct sAIEdgeConnectData
{
   tAIPathCellID   cell1;
   tAIPathVertexID vertex1a;
   tAIPathVertexID vertex1b;

   tAIPathCellID   cell2;
   tAIPathVertexID vertex2a;
   tAIPathVertexID vertex2b;

   float        distSq;
   int          kind;
};

static sAIEdgeConnectData g_CurConnData;

/////////////////////////////////////



struct sAIPathOptions
{
   BOOL bWaterIsPathable;
};

// Here's my descriptor, which identifies my stuff to the tag file & editor
sFileVarDesc g_AIPathFileVarDesc =
{
   kMissionVar,         // Where do I get saved?
   "AIPATHVAR",        // Tag file tag
   "AI Path options",           // friendly name
   FILEVAR_TYPE(sAIPathOptions),// Type (for editing)
   { 1, 0},             // version
   { 1, 0},             // last valid version
};

// the class for actual global variable
class cAIPathOptions : public cFileVar<sAIPathOptions,&g_AIPathFileVarDesc>
{};

// the thing itself
static cAIPathOptions g_AIPathOpts;

static sFieldDesc g_aPathOptionsField[] =
{
   { "Pathable Water", kFieldTypeBool,
     FieldLocation(sAIPathOptions, bWaterIsPathable), },
};

static sStructDesc g_SDesc = StructDescBuild(sAIPathOptions, kStructFlagNone,
                                             g_aPathOptionsField);

////////////////////////////

struct sAIGamesysPathOptions
{
   sAIGamesysPathOptions() : largeDoorSize(5.0) {}
   
   float largeDoorSize;
};

// Here's my descriptor, which identifies my stuff to the tag file & editor
sFileVarDesc g_AIGamesysPathFileVarDesc =
{
   kGameSysVar,         // Where do I get saved?
   "AIGPTHVAR",        // Tag file tag
   "AI Gamesys Path options",           // friendly name
   FILEVAR_TYPE(sAIGamesysPathOptions),// Type (for editing)
   { 1, 0},             // version
   { 1, 0},             // last valid version
};

// the class for actual global variable
class cAIGamesysPathOptions : public cFileVar<sAIGamesysPathOptions,&g_AIGamesysPathFileVarDesc>
{};

// the thing itself
static cAIGamesysPathOptions g_AIGamesysPathOpts;

static sFieldDesc g_aGamesysPathOptionsField[] =
{
   { "Large Door Size", kFieldTypeFloat,
     FieldLocation(sAIGamesysPathOptions, largeDoorSize), },
};


float * g_pLargeDoorSize = &g_AIGamesysPathOpts.largeDoorSize;

static sStructDesc g_GamesysSDesc = StructDescBuild(sAIGamesysPathOptions, kStructFlagNone, g_aGamesysPathOptionsField);

////////////////////////////



#ifdef EDITOR

#define PumpWindowsEvents()  \
   { \
      AutoAppIPtr(GameShell); \
      pGameShell->PumpEvents(kPumpAll); \
   }



///////////////////////////////////////////////////////////////////////////////

float TriArea(const mxs_vector & v1, const mxs_vector & v2, const mxs_vector & v3)
{
   float a, b, c, s;
   a = Vec2Dist((Vec2 *)&v1, (Vec2 *)&v2);
   b = Vec2Dist((Vec2 *)&v2, (Vec2 *)&v3);
   c = Vec2Dist((Vec2 *)&v3, (Vec2 *)&v1);

   s = (a + b + c) / 2.0;

   return sqrt(s*(s-a)*(s-b)*(s-c));
}

///////////////////////////////////////////////////////////////////////////////

inline BOOL Same(const mxs_vector & v1, const mxs_vector & v2, float eps)
{
   return (afle(v1.x-v2.x, eps) &&
           afle(v1.y-v2.y, eps) &&
           afle(v1.z-v2.z, eps));
}

///////////////////////////////////////////////////////////////////////////////

inline float DistSqPtLine(const mxs_vector & pt, const mxs_vector & line1, const mxs_vector & line2)
{
   mxs_vector  dist;
   mxs_vector  norm;

   mx_sub_vec(&norm, &line2, &line1);
   mx_normeq_vec(&norm);

   mx_sub_vec(&dist, &pt, &line1);
   mx_scaleeq_vec(&norm, mx_dot_vec(&dist, &norm));

   return mx_dist2_vec(&dist, &norm);
}

#define DistSqXYPtSeg(p, s, e) Vec2PointSegmentSquared((Vec2 *)&(p), (Vec2 *)&(s), (Vec2 *)&(e))
#define DistXYPtLine(p, s, e)  Vec2PointLineDist((Vec2 *)&(p), (Vec2 *)&(s), (Vec2 *)&(e))

///////////////////////////////////////////////////////////////////////////////

static int CellCompareFunc(const tAIPathCellID * p1, const tAIPathCellID * p2)
{
   return *((int *)p1) - *((int *)p2);
}

///////////////////////////////////////////////////////////////////////////////

#ifndef SHIP
BOOL g_fAIPDB_fUseLVLs = TRUE;
BOOL g_fAIPDB_fUseHugeZ = TRUE;
BOOL g_fAIPDB_EnableHighStrike = TRUE;
#endif

BOOL g_fAIPDB_QuickShare      = TRUE;
BOOL g_fAIPDB_ExhaustShare    = TRUE;
BOOL g_fAIPDB_AllSteps        = FALSE;
BOOL g_fAIPDB_NoSteps         = FALSE;
BOOL g_fAIPDB_FullyContained  = TRUE;
BOOL g_fAIPDB_Skewed          = TRUE;
BOOL g_fAIPDB_CalcOkBits      = TRUE;
BOOL g_fAIPDB_CellInfo        = TRUE;

BOOL g_fAIPDB_BlockingObjects = TRUE;
BOOL g_fAIPDB_DoorMarks       = TRUE;

BOOL g_fAIPDB_PostProcessNarrow = FALSE;

BOOL g_fAIPDB_CellsOnly       = FALSE;

ObjID g_AIPDB_MarkOnlyObj;

int g_AIQualityWatch = 0;

#endif  // !EDITOR

///////////////////////////////////////////////////////////////////////////////
//
// PATHFINDING DATABASE DATA
//

#define MAX_LINKS_FROM_CELL      24
#define MAX_VERTICES             0xfffe          // in g_AIPathDB.m_Vertices array

// How many we can actually store:
#define MAX_CELL2CELL_LINKS      0xfffe          // in g_AIPathCell2Cell array

// How many we can process. We want to allow a little play, since we links get postprocessed and
// whittled down to below MAX_CELL2CELL_LINKS:
#define MAX_PROCESSED_CELL2CELL_LINKS 0x0001fffe


#define MAX_CELL2VERTEX_LINKS    0xfffe          // in g_AIPathDB.m_nVertices array

#ifdef EDITOR
const int PATH_PT_OPEN = 0;   // cell vertex is in the open
const int PATH_PT_WALL = 1;   // cell vertex is in at the edge of a wall
const int PATH_PT_CLIFF = 2;  // cell vertex is at the edge of a cliff
#endif

cAIPathDB g_AIPathDB;

BOOL g_fAIPathFindInited = FALSE;

///////////////////////////////////////

#ifdef EDITOR

///////////////////////////////////////

BOOL g_fAIEnforceDBQuality = TRUE;

///////////////////////////////////////////////////////////////////////////////

// temporary info used in setting up OkBits & cell flags

#define kAdjunctNoStair -1

struct sAIPathCellAdjunct
{
   // Out ceiling heights are not guaranteed to be true over their
   // cells' entire cylendar since they are based on sampling at only
   // certain points.
   float m_fMaxFloorZ;
   float m_fMinCeilingZ;

   // This represents whether we can place a trail of person-sized
   // cigars running from the cell center in each of 16 directions.
   // We use the result for identifying corners and doorways.
   int m_iBlockedDirectionFlags;

   BOOL m_bRamp;
   int m_iStairSize;

   uchar m_chSwimMedium;        // medium at depth needed for swimming
   uchar m_chWaistMedium;       // at waist height
   uchar m_chFlyMedium;         // very close to ceiling
   uchar m_m_m_m;               // padding
};

// temporary data to help generate heuristics--we destroy these before
// we finish generating the pathfinding database

// temporary data per cell
sAIPathCellAdjunct       *g_AIPathCellAdjunct;


///////////////////////////////////////////////////////////////////////////////

inline void SetCellEdgeInfo(tAIPathCellID cell, int index, int edgeInfo)
{
#if 0
   g_AIPathDB.m_CellVertices[g_AIPathDB.m_Cells[cell].firstVertex + index].edgeInfo
      = (uchar) edgeInfo;
#endif
}

inline void SetCellEdgeLinked(tAIPathCellID cell, int index, int hasLinks)
{
#if 0
   g_AIPathDB.m_CellVertices[g_AIPathDB.m_Cells[cell].firstVertex + index].hasLinks
      = (uchar) hasLinks;
#endif
}

///////////////////////////////////////////////////////////////////////////////
//
// PATHFNSTANTS--NOW TURNED INTO VARIABLES FOR YOUR DEBUGGING PLEASURE!
// BUT STILL, BIZARRELY, IN ALL CAPS!
//


// connecting cells together...

// considered close enough to go both ways
static float CLOSE_ENOUGH_HORIZONTALLY = 0.25;
// considered close enough to go both ways
static float CLOSE_ENOUGH_HORIZONTALLY_SQ = 0.0625;

// calculating travel costs...

// cost to travel MAX_TRAVEL_DISTANCE
static float MAX_TRAVEL_COST           = 100.0;
// distance at which MAX_TRAVEL_COST is used
static float MAX_TRAVEL_DISTANCE       = 10.0;

// vertical motion between cells

// how far can non-flying AI's drop when not stressed?
static float VERTICAL_DROP_LIMIT       = 1.5;
// how far can non-flying AI's rise not stressed?
static float VERTICAL_RISE_LIMIT       = 1.5;

// When we're around vertical differences less than this we'll walk normally.
static float MIN_STAIR_HEIGHT          = .25;
// We find stairs by raycasting down, from points this far away from
// the edges of our cells.
static float STAIR_SAMPLE_DISTANCE     = 1.5;
// distance a walker can pull itself up
static float HOIST_HEIGHT              =  4.0;
// distance they're willing to jump down
static float JUMP_HEIGHT               = 5.0;

// other cell attributes

// for deciding whether a guard can wade into water
static float WADING_HEIGHT             = 3.0;
// for whether a swimming creature has enough room
static float SWIM_DEPTH                = 2.0;
// distance a flier needs between floor (or water) and ceiling
static float FLY_HEIGHT                = 10.0;
// limit for finding cliffs
static float DROP_DISTANCE             = 8.0;
// We find out how well lit a hypothetical object is when it's this
// far above the center of each pathfinding cell.
static float LIGHT_HEIGHT              = 4.0;
// This is cos(10).
static float MIN_RAMP_SLANT            = 0.9848;
// When we're deciding whether the cell is in a corner, we cast out
// this far in 16 directions and look for a block of hits and a block
// of misses.
static float BLOCK_CHECK_DISTANCE      = 5.0;


float g_AIPlaneZNormAccept = 0.465;
float g_AIPlaneZNormPath   = 0.7;

#endif EDITOR

///////////////////////////////////////////////////////////////////////////////

cAIPathDB::cAIPathDB()
 : m_MarkedCells(0)
{
}

///////////////////////////////////////

void cAIPathDB::Init()
{
   int i;

   m_nCells = 0;
   m_nPlanes = 0;
   m_nVertices = 0;
   m_nLinks = 0;
   m_nCellVertices = 0;

   m_nObjHints = 0;
   m_nCellObjMaps = 0;

   LGALLOC_PUSH_CREDIT();

   m_Cells.SetSize(2);
   m_Planes.SetSize(2);
   m_Vertices.SetSize(2);
   m_Links.SetSize(2);
   m_CellVertices.SetSize(2);

   m_CellObjMap.SetSize(0);
   m_nCellObjMaps = 0;

   for (i = 0; i < kAIZone_Num; i++)
   {
      m_ZoneDatabases[i].m_ZonePairTable.SetEmpty();
      m_ZoneDatabases[i].m_nZones = 1;
   }

   m_Marks.SetSize(2);
   m_BestCostToReach.SetSize(2);

#ifdef THIEF
   AutoAppIPtr_(StructDescTools, pTools);
   pTools->Register(&g_SDesc);
   pTools->Register(&g_GamesysSDesc);
   
#endif

   LGALLOC_POP_CREDIT();

   AICreatureSizeInit();
}

///////////////////////////////////////

void cAIPathDB::Term()
{
   if (m_Cells)
   {
      int i;

      m_Cells.SetSize(2);
      m_Planes.SetSize(2);
      m_Vertices.SetSize(2);
      m_Links.SetSize(2);
      m_CellVertices.SetSize(2);

      m_Marks.SetSize(2);
      m_BestCostToReach.SetSize(2);

      m_ObjHints.SetSize(2);

      m_CellObjMap.SetSize(0);
      m_nCellObjMaps = 0;

      for (i = 0; i < kAIZone_Num; i++)
      {
         m_ZoneDatabases[i].m_ZonePairTable.DestroyAll();
         m_ZoneDatabases[i].m_ZonePairTable.SetEmpty();
         m_ZoneDatabases[i].m_nZones = 1;
      }

      m_nPlanes = 0;
      m_nCells = 0;
      m_nVertices = 0;
      m_nLinks = 0;
      m_nCellVertices = 0;

      m_CellToDoorsTable.SetEmpty();
      m_CellDoors.SetSize(0);

      m_MovableCells.SetSize(0);

      m_FleeRegions.DestroyAll();

      g_fAIPathFindInited = FALSE;
   }
}

#ifdef EDITOR

///////////////////////////////////////

float cAIPathDB::GetHighestZ(tAIPathCellID cell) const
{
   int       iFirstVertex = m_Cells[cell].firstVertex;
   const int iVertexLimit = iFirstVertex + m_Cells[cell].vertexCount;

   float zMax;
   float zCur;

   zMax = m_Vertices[m_CellVertices[iFirstVertex].id].pt.z;
   iFirstVertex++;

   for (int i = iFirstVertex; i < iVertexLimit; i++)
   {
      zCur = m_Vertices[m_CellVertices[i].id].pt.z;

      if (zCur > zMax)
         zMax = zCur;
   }
   return zMax;
}

///////////////////////////////////////

float cAIPathDB::GetLowestZ(tAIPathCellID cell) const
{
   int       iFirstVertex = m_Cells[cell].firstVertex;
   const int iVertexLimit = iFirstVertex + m_Cells[cell].vertexCount;

   float zMin;
   float zCur;

   zMin = m_Vertices[m_CellVertices[iFirstVertex].id].pt.z;
   iFirstVertex++;

   for (int i = iFirstVertex; i < iVertexLimit; i++)
   {
      zCur = m_Vertices[m_CellVertices[i].id].pt.z;

      if (zCur < zMin)
         zMin = zCur;
   }
   return zMin;
}

///////////////////////////////////////

float cAIPathDB::ComputeArea(tAIPathCellID cell) const
{
   const int          iLimit = m_Cells[cell].vertexCount - 1;
   float              result = 0.0;
   const mxs_vector & v0     = GetCellVertex(cell, 0);

   for (int i = 1; i < iLimit; i++)
   {
      result += TriArea(v0, GetCellVertex(cell, i), GetCellVertex(cell, i + 1));
   }

   return result;
}

///////////////////////////////////////////////////////////////////////////////
//
// DATABASE CREATION SUBROUTINES
//

static int __fastcall AIFindFirstPlane(const cMxsVector & normal, float constant)
{
   tAIPathCellID i;

   for (i = 1; i <= g_AIPathDB.m_nPlanes; i++)
      if (normal == g_AIPathDB.m_Planes[i].normal && constant == g_AIPathDB.m_Planes[i].constant)
      {
         return i;
      }

   return 0;
}


///////////////////////////////////////

static int __fastcall AICreatePlane(const cMxsVector & normal, float constant)
{
   // plane does not yet exist, increase array count.
   g_AIPathDB.m_nPlanes++;
   if (g_AIPathDB.m_nPlanes >= MAX_CELLS-1)
   {
      mprintf("ERROR: Too many planes in pathfinding database!");
      return 0;
   }

   // create the plane.
   g_AIPathDB.m_Planes[g_AIPathDB.m_nPlanes].normal = normal;
   g_AIPathDB.m_Planes[g_AIPathDB.m_nPlanes].constant = constant;
   return g_AIPathDB.m_nPlanes;
}

///////////////////////////////////////

static int __fastcall AICreateOrFindPlane(const cMxsVector & normal, float constant)
{
   // find/create the plane's id
   int i = AIFindFirstPlane(normal, constant);
   if (!i)
      return AICreatePlane(normal, constant);
   return i;
}

///////////////////////////////////////

static BOOL __fastcall AIPathVertexAlreadyExists(const cMxsVector & vertex, tAIPathVertexID * pId)
{
   tAIPathVertexID i;

   for (i = 1; i <= g_AIPathDB.m_nVertices; i++)
   {
//      if (vertex.IsIdent(g_AIPathDB.m_Vertices[i].pt))
      if (vertex == g_AIPathDB.m_Vertices[i].pt)
      {
         *pId = i;
         return TRUE;
      }
   }

   return FALSE;
}

///////////////////////////////////////

static BOOL __fastcall AICreateOrFindPathVertex(const cMxsVector & vertex, tAIPathVertexID * pId)
{
   // find/create the newVertex's id
   if (!AIPathVertexAlreadyExists(vertex, pId))
   {
      // vertex does not yet exist, increase array count.
      g_AIPathDB.m_nVertices++;
      if (g_AIPathDB.m_nVertices >= MAX_VERTICES-1)
      {
         mprintf("ERROR: Too many verticies in pathfinding database!");
         return FALSE;
      }

      // create the vertex.
      g_AIPathDB.m_Vertices[g_AIPathDB.m_nVertices].pt = vertex;
      g_AIPathDB.m_Vertices[g_AIPathDB.m_nVertices].ptInfo = PATH_PT_OPEN;
      *pId = g_AIPathDB.m_nVertices;
   }
   return TRUE;
}

///////////////////////////////////////

static void PathFindMinimumRaycastAtLocation(const cMxsVector &sampleSourcePt,
                                             float &heightSq, float &widthSq)
{
   const float SAMPLE_DISTANCE = 1.0;
   const int   SAMPLE_SIZE = 8;
   floatang    sampleDirection;
   float       widthOne, widthTwo;
   cMxsVector    sampleEndPt;
   Location    sampleSourceLoc, sampleEndLoc, hitLoc;
   cMxsVector    hitPt;
   int         i;

   MakeLocation(&sampleSourceLoc,
                sampleSourcePt.x, sampleSourcePt.y, sampleSourcePt.z);
   int sHint = ComputeCellForLocation(&sampleSourceLoc);
   if (sHint == -1)
   {
      // not in portal database
      widthSq = 0;
      heightSq = 0;
      return;
   }

   ///////////////////////////////////////////////////////////
   // get the worst case width by sampling horizontally around the
   // sample pt.

   widthSq = 2*SAMPLE_DISTANCE*SAMPLE_DISTANCE;
   for (i = 0; i < SAMPLE_SIZE; i++)
   {
      sampleDirection.value = PI * i / SAMPLE_SIZE;

      ProjectFromLocationOnZPlane(sampleSourcePt, SAMPLE_DISTANCE,
                                  sampleDirection, &sampleEndPt);
      MakeLocation(&sampleEndLoc, sampleEndPt.x, sampleEndPt.y, sampleEndPt.z);
      if (!PortalRaycast(&sampleSourceLoc, &sampleEndLoc, &hitLoc, 0))
      {
         hitPt.x = hitLoc.vec.x;
         hitPt.y = hitLoc.vec.y;
         hitPt.z = hitLoc.vec.z;

         widthOne = AIDistanceSq(sampleSourcePt, hitPt);
      }
      else
         widthOne = SAMPLE_DISTANCE * SAMPLE_DISTANCE;

      if (widthOne < widthSq)
         widthSq = widthOne;

      ProjectFromLocationOnZPlane(sampleSourcePt, SAMPLE_DISTANCE,
                                  sampleDirection + floatang(PI), &sampleEndPt);
      MakeLocation(&sampleEndLoc, sampleEndPt.x, sampleEndPt.y, sampleEndPt.z);
      if (!PortalRaycast(&sampleSourceLoc, &sampleEndLoc, &hitLoc, 0))
      {
         hitPt.x = hitLoc.vec.x;
         hitPt.y = hitLoc.vec.y;
         hitPt.z = hitLoc.vec.z;

         widthTwo = AIDistanceSq(sampleSourcePt, hitPt);
      }
      else
         widthTwo = SAMPLE_DISTANCE * SAMPLE_DISTANCE;

      if (widthTwo < widthSq)
         widthSq = widthTwo;
   }

   ///////////////////////////////////////////////////////////
   // get the height available at the location by sampling up.

   MakeLocation(&sampleEndLoc,
                sampleSourcePt.x, sampleSourcePt.y, sampleSourcePt.z + 10);
   if (sHint != -1 && !PortalRaycast(&sampleSourceLoc,
                                     &sampleEndLoc, &hitLoc, 0))
   {
      hitPt.x = hitLoc.vec.x;
      hitPt.y = hitLoc.vec.y;
      hitPt.z = hitLoc.vec.z;

      heightSq = AIDistanceSq(sampleSourcePt, hitPt);
   }
   else
      heightSq = 100;
}

///////////////////////////////////////

// it's all in a name
static const mxs_vector & __fastcall ComputeCellCenter(const tAIPathCellID cell, mxs_vector * pCenter)
{
   tAIPathVertexID id;
   tAIPathCell2VertexLinkID i;
   register cMxsVector * pPoint;
   const int lastVertex = g_AIPathDB.m_Cells[cell].firstVertex + g_AIPathDB.m_Cells[cell].vertexCount;

   ((cMxsVector *)pCenter)->Set(0, 0, 0);

   for (i = g_AIPathDB.m_Cells[cell].firstVertex; i < lastVertex; i++)
   {
      id = g_AIPathDB.m_CellVertices[i].id;
      pPoint = &(g_AIPathDB.m_Vertices[id].pt);
      pCenter->x += pPoint->x;
      pCenter->y += pPoint->y;
      pCenter->z += pPoint->z;
   }

   const float vertexCount = (float)(g_AIPathDB.m_Cells[cell].vertexCount);

   pCenter->x = pCenter->x / vertexCount;
   pCenter->y = pCenter->y / vertexCount;
   pCenter->z = pCenter->z / vertexCount;

   return *pCenter;
}

///////////////////////////////////////

// This finds the greatest height we can fall from the given point, up
// to DROP_DISTANCE.  We return 0.0 if we're not near any cliffs.
static float PathFindCliffHeight(const cMxsVector &sampleSourcePt)
{
   const float   SAMPLE_DISTANCE = 1.0;
   const int     SAMPLE_SIZE = 16;
   float         fGreatestDrop = 0;
   floatang      sampleDirection;
   cMxsVector      sampleEndPt;
   Location      sampleSourceLoc, sampleEndLoc, dropLoc, hitLoc;
   int           i;

   MakeLocation(&sampleSourceLoc,
                sampleSourcePt.x, sampleSourcePt.y, sampleSourcePt.z);
   int sHint = ComputeCellForLocation(&sampleSourceLoc);

   /////////////////////////////////////////////////
   // get the worst case width by sampling horizontally around the
   // sample pt.

   for (i = 0; i < SAMPLE_SIZE; i++)
   {
      sampleDirection.value = TWO_PI * i / SAMPLE_SIZE;

      ProjectFromLocationOnZPlane(sampleSourcePt, SAMPLE_DISTANCE,
                                  sampleDirection, &sampleEndPt);
      MakeLocation(&sampleEndLoc, sampleEndPt.x, sampleEndPt.y, sampleEndPt.z);
      if (sHint != -1 && PortalRaycast(&sampleSourceLoc, &sampleEndLoc,
                                       &hitLoc, 0))
      {
         dropLoc = sampleEndLoc;
         dropLoc.vec.z -= DROP_DISTANCE;
         if (ComputeCellForLocation(&sampleEndLoc) != -1)
         {
            if (PortalRaycast(&sampleEndLoc, &dropLoc, &hitLoc, 0))
               // A cast went all the way down--it's a full-blown
               // cliff.
               return DROP_DISTANCE;
            else {
               float fDrop = sampleSourcePt.z - hitLoc.vec.z;
               if (fDrop > fGreatestDrop)
                  fGreatestDrop = fDrop;
            }
         }
      }
   }

   return fGreatestDrop;
}


///////////////////////////////////////

// This constant must match the number of bits allocated for stair size
static double g_aStairSize[kAIPathCellNumStairSizes]
= {
   .5, .75, 1.0, 1.5
};



// This returns kAdjunctNoStair if we appear to have a flat edge.
// We assume that the points we're getting are clockwise.
static int EdgeStairLevel(mxs_vector *p1, mxs_vector *p2)
{
   const float fSampleInterval = .75;      // in world space
   const float fDistanceUpForSample = 1.0; // so we don't hit our source poly

   mxs_vector Edge, EdgeNorm, Perp;
   Location Source, Dest, Hit;
   mx_sub_vec(&Edge, p2, p1);
   float fEdgeLength = mx_norm_vec(&EdgeNorm, &Edge);
   mx_mk_vec(&Perp, -EdgeNorm.y, EdgeNorm.x, 0.0);
   mx_scaleeq_vec(&Perp, STAIR_SAMPLE_DISTANCE);

   int iNumSamples = (int) ceil(fEdgeLength / fSampleInterval);
   float fInc = 1.0 / (iNumSamples - 1.0);
   float fFraction = 0.0;
   float fSourceZ, fDiffZ;

   for (int i = 0; i < iNumSamples; ++i)
   {
      mx_interpolate_vec(&Source.vec, p1, &Edge, fFraction);
      Dest = Source;
      mx_addeq_vec(&Dest.vec, &Perp);
      Dest.vec.z += fDistanceUpForSample;

      fSourceZ = Source.vec.z;

      // First we make sure we can raycast from a point on our edge to
      // the point over the one we're interested in.
      Source.hint = CELL_INVALID;
      if ((ComputeCellForLocation(&Source) != CELL_INVALID)
       && (PortalRaycast(&Source, &Dest, &Hit, 0)))
      {
         Source = Dest;
         Dest.vec.z -= fDistanceUpForSample
                     + g_aStairSize[kAIPathCellNumStairSizes - 1];
         Source.hint = PortalRaycastCell;

         if ((ComputeCellForLocation(&Source) != CELL_INVALID)
          && PortalRaycast(&Source, &Dest, &Hit, 0) == FALSE)
         {
            // Looks like we hit something...
            fDiffZ = fSourceZ - Hit.vec.z;
            if (fDiffZ >= MIN_STAIR_HEIGHT)
            {
               for (int iLevel = 0;
                    iLevel < kAIPathCellNumStairSizes;
                    ++iLevel)
                  {
                  if (fDiffZ > g_aStairSize[iLevel])
                     break;
                  return iLevel;
               }
            }
         }
      }

      fFraction += fInc;
   }

   return kAdjunctNoStair;
}

//////////////////////

// We call this when we already happen to know that the cell is not a
// ramp.  It returns an index into g_aStairSize, above, or
// kAdjunctNoStair if this cell is really more of a floor.
static int CellStairLevel(int iCellIndex)
{
   sAIPathCell *pCell = &g_AIPathDB.m_Cells[iCellIndex];

   cMxsVector *p1;
   cMxsVector *p2;

   const int iFirstVertex = pCell->firstVertex;
   const int iVertexLimit = iFirstVertex + pCell->vertexCount;

   p2 = &(g_AIPathDB.m_Vertices[g_AIPathDB.m_CellVertices[iFirstVertex].id].pt);
   for (int i = iFirstVertex; i < iVertexLimit; i++)
   {
      p1 = p2;
      if (i == iVertexLimit-1)
         p2 = &(g_AIPathDB.m_Vertices[g_AIPathDB.m_CellVertices[iFirstVertex].id].pt);
      else
         p2 = &(g_AIPathDB.m_Vertices[g_AIPathDB.m_CellVertices[i + 1].id].pt);

      int iStairLevel = EdgeStairLevel(p1, p2);
      if (iStairLevel != kAdjunctNoStair)
      {
         ConfigSpew("PathFindGenSpew", ("Cell %d has a stair.\n", iCellIndex));
         return iStairLevel;
      }
   }

   return kAdjunctNoStair;
}


static BOOL CellIsRamp(int iCellIndex)
{
   BOOL rv = FALSE;

   sAIPathCellPlane *pPlane = &g_AIPathDB.m_Planes[g_AIPathDB.m_Cells[iCellIndex].plane];
   if (pPlane->normal.z <= MIN_RAMP_SLANT)
      rv = TRUE;

   return rv;
}


float g_MinimumValidEdgeWidth  = 0.5;
int   g_EdgePropertySampleSize = 12;
float g_StepClearance          = 1.25;

// forward recl.
int CheckPathClear(mxs_vector *pV1, mxs_vector *pV2,
                   float fHeight, float radius,
                   BOOL bShortCircuitClear, BOOL bShortCircuitHit);

static int CellBlockedDirections(int iCellIndex)
{
   mxs_vector Center = g_AIPathDB.GetCenter(iCellIndex);

   float radius = g_pAICreatureSizes->sizes[0].width * .5;
   float fRise = g_pAICreatureSizes->sizes[0].height * .5 + g_StepClearance * .5;
   float fEffectiveHeight = g_pAICreatureSizes->sizes[0].height - g_StepClearance;

   Center.z += fRise;
   cMxsVector SampleEnd;
   floatang SampleDirection;
   int rv = 0;

   for (int i = 0; i < 16; i++)
   {
      rv += rv;

      SampleDirection.value = TWO_PI * (1.0 / 16.0) * (float) i;

      ProjectFromLocationOnZPlane(Center, BLOCK_CHECK_DISTANCE,
                                  SampleDirection, &SampleEnd);

      // A set bit indicates a blocked direction.
      if (CheckPathClear(&Center, &SampleEnd, fEffectiveHeight, radius,
                         FALSE, TRUE)
        < g_EdgePropertySampleSize)
         rv |= 1;
   }

#ifndef SHIP
   if (config_is_defined("PathFindGenSpew"))
   {
      mprintf("     Direction mask: ");
      for (int iBit = 0; iBit < 16; ++iBit)
         if (rv & (1 << iBit))
            mprintf("B");       // blocked direction
         else
            mprintf("_");       // clear direction
      mprintf("\n");
   }
#endif // ~SHIP

   return rv;
}


/////////////////////////////////////////////////


static int g_aBitMask[16]
= { 1, 3, 7, 15, 31,
    63, 127, 255,
    511, 1023, 2047, 4095,
    8191, 16383, 32767, 65535
};

#define kNumDoorMasks 3
// In bits, these look like: 1001 10001 100001
static int g_aDoorOnMask[kNumDoorMasks]
= { 9, 17, 33 };

// These are inverses of the above: 0110, 01110, 011110
static int g_aDoorOffMask[kNumDoorMasks]
= { 6, 14, 30 };

// This gives us a rough guess about whether the given cell has a
// corner or doorway.  It uses the bits set just above which tell us
// whether the cell is open in each of 16 directions.
static void ProcessOpenDirections(int iCellIndex)
{
   int iMask, iMaskIndex, iBitPos;
   int iBlockedDirectionFlags
      = g_AIPathCellAdjunct[iCellIndex].m_iBlockedDirectionFlags;

   iBlockedDirectionFlags |= (iBlockedDirectionFlags << 16);

   for (iMaskIndex = 15; iMaskIndex >= 0; --iMaskIndex)
   {
      iMask = g_aBitMask[iMaskIndex];
      for (iBitPos = 15; iBitPos >= 0; --iBitPos)
      {
         if ((iBlockedDirectionFlags & (iMask << iBitPos)) == 0)
            goto found_open_range;
      }
   }

found_open_range:
   // length of longest stretch of unblocked directions
   int iOpenRange = iMaskIndex + 1;

   for (iMaskIndex = 15; iMaskIndex >= 0; --iMaskIndex)
   {
      iMask = g_aBitMask[iMaskIndex];
      for (iBitPos = 15; iBitPos >= 0; --iBitPos)
      {
         if ((iBlockedDirectionFlags & (iMask << iBitPos))
          == (iMask << iBitPos))
            goto found_closed_range;
      }
   }

found_closed_range:
   // length of longest stretch of blocked directions
   int iClosedRange = iMaskIndex + 1;

   for (iMaskIndex = kNumDoorMasks - 1; iMaskIndex >= 0; --iMaskIndex)
   {
      for (iBitPos = 15; iBitPos >= 0; --iBitPos)
      {
         if ((iBlockedDirectionFlags & (g_aDoorOnMask[iMaskIndex] << iBitPos))
          == (g_aDoorOnMask[iMaskIndex] << iBitPos)
          && (iBlockedDirectionFlags & (g_aDoorOffMask[iMaskIndex] << iBitPos))
          == 0)
            goto found_door;
      }
   }

found_door:
   // If we didn't find a door bitmask this will be 0.
   int iDoorIndex = iMaskIndex + 1;

   if (iOpenRange >= 5 && iClosedRange >= 8)
      g_AIPathDB.m_Cells[iCellIndex].m_Corner = 1;

   if (iDoorIndex)
      g_AIPathDB.m_Cells[iCellIndex].m_Doorway = 1;

   ConfigSpew("PathFindGenSpew",
              ("     max open: %d  max closed: %d  corner: %s  doorway: %s\n",
               iOpenRange, iClosedRange,
               (g_AIPathDB.m_Cells[iCellIndex].m_Corner)? "Y" : "n",
               (g_AIPathDB.m_Cells[iCellIndex].m_Doorway)? "Y" : "n"));
}


/////////////////////////////////////////////////
//
// Function to see if there is at least one spot on an edge where an
// AI can pass
//

// wsf: this order must be preserved for edge calculation!
enum eEdgeProperties
{
   kEdgeOpen,
   kEdgeTouchWall,
   kEdgeIsWall,
   kEdgeIsHighStrike
};

// This checks whether a vertical cigar is clear.
static inline BOOL CheckCigarClear(mxs_vector *pCenter,
                                   float fHeight, float radius)
{
   Location Bottom;
   Location Top;
   Location hitIgnored;
   ObjID objIgnored;

   Top.vec = *pCenter;
   Top.vec.z += (fHeight * .5 - radius);

   Bottom.vec = *pCenter;
   Bottom.vec.z -= (fHeight * .5 - radius);
   Bottom.cell = Bottom.hint = -1;
   ComputeCellForLocation(&Bottom);

   if (SphrSphereInWorld(&Bottom, radius, 0) &&
       SphrSpherecastStatic(&Bottom, &Top, radius, 0) == 0)
   {
      if (AIPhysRaycast(Bottom, Top, &hitIgnored, &objIgnored, radius, kCollideOBB|kInsideOBB) == kCollideNone)
         return 1;
   }

   return 0;
}


// This returns a value from 0 to g_EdgePropertySampleSize, telling us
// how many vertical cigars along a path are clear.  We allow short-
// circuiting for callers which are looking for any space at all.
static int CheckPathClear(mxs_vector *pV1, mxs_vector *pV2,
                          float fHeight, float radius,
                          BOOL bShortCircuitClear, BOOL bShortCircuitHit)
{
   int rv = 0;

   mxs_vector v;
   float fInc = 1.0 / (float(g_EdgePropertySampleSize) - 1.0);
   float fPos = 0.0;    // position along edge, [0..1]

   for (int i = 0; i < g_EdgePropertySampleSize; ++i)
   {
      mx_interpolate_vec(&v, pV1, pV2, fPos);
      if (CheckCigarClear(&v, fHeight, radius))
      {
         rv++;
         if (bShortCircuitClear)
            break;
      } else
         if (bShortCircuitHit)
            break;

      fPos += fInc;
   }

   return rv;
}


///////////////////////////////////////


// Do raycasts from center of source cell to points along dest cell's edge. If at least n-2 succeed, then
// we return TRUE.

#define SOME_SMALL_Z 0.5000000
#define OMEGA_Z 0.00010000
static BOOL TestEdgeRaycast(int iFromCell, sAIPathCellLink *pLink)
{
   int i;
   int nFailNum = 0;
   mxs_vector EdgePt;
   mxs_vector EdgePt1, EdgePt2;

   mxs_vector EdgePt1a, EdgePt1b;
   mxs_vector EdgePt2a, EdgePt2b;

   mxs_vector CenterPt;
   Location EdgeLoc, CenterLoc, hitLoc;

   // The edge endpoints are the link's edge points, but on the plane of the destination cell, shifted up by
   // some reasonably small value so as to no allow the raycast to collide against the very edge we're connected to.

   EdgePt1 = g_AIPathDB.GetVertex(pLink->vertex_1);
   EdgePt2 = g_AIPathDB.GetVertex(pLink->vertex_2);

   // If source is center is lower than either vertex, then source's center is used, and dest's edges
   EdgePt1a.z = g_AIPathDB.GetZAtXY(iFromCell, EdgePt1);
   EdgePt1b.z = g_AIPathDB.GetZAtXY(iFromCell, EdgePt2);

   EdgePt2a.z = g_AIPathDB.GetZAtXY(pLink->dest, EdgePt1);
   EdgePt2b.z = g_AIPathDB.GetZAtXY(pLink->dest, EdgePt2);

   // If we have degenerate 'x' case, then we'll probably not always do the right thing here.

   if ((EdgePt1a.z > (EdgePt2a.z+OMEGA_Z)) && (EdgePt1b.z > (EdgePt2b.z+OMEGA_Z)))
   {
      CenterPt = g_AIPathDB.m_Cells[pLink->dest].center;
      EdgePt1.z = g_AIPathDB.GetZAtXY(iFromCell, EdgePt1);
      EdgePt2.z = g_AIPathDB.GetZAtXY(iFromCell, EdgePt2);
   }
   else
   {
      CenterPt = g_AIPathDB.m_Cells[iFromCell].center;
      EdgePt1.z = g_AIPathDB.GetZAtXY(pLink->dest, EdgePt1);
      EdgePt2.z = g_AIPathDB.GetZAtXY(pLink->dest, EdgePt2);
   }

   EdgePt1.z += SOME_SMALL_Z;
   EdgePt2.z += SOME_SMALL_Z;

   CenterPt.z += SOME_SMALL_Z;
   MakeLocation(&CenterLoc, CenterPt.x, CenterPt.y, CenterPt.z);

   for (i = 0; (i < g_EdgePropertySampleSize) && (nFailNum < 3); i++)
   {
      mx_interpolate_vec(&EdgePt, &EdgePt1, &EdgePt2, i/(float)(g_EdgePropertySampleSize-1));
      MakeLocation(&EdgeLoc, EdgePt.x, EdgePt.y, EdgePt.z);
      if (!PortalRaycast(&CenterLoc, &EdgeLoc, &hitLoc, 0))
         nFailNum++;
   }

   return (nFailNum < 3);
}


///////////////////////////////////////

// @TBD: do ObjRaycasts here, too?
static int ComputeEdgeProperty(tAIPathVertexID v1, tAIPathVertexID v2, int iFromCell, sAIPathCellLink *pLink, float &fZDiff)
{
   int nPassages;
   int nClearPoints;
   cMxsVector endPt1, endPt2;
   eEdgeProperties EdgeVal;
   mxs_vector v1a, v1b, v2a, v2b;

   float radius = g_pAICreatureSizes->sizes[0].width * .5;
   float fRise = g_pAICreatureSizes->sizes[0].height * .5 + g_StepClearance * .5;
   float fEffectiveHeight = g_pAICreatureSizes->sizes[0].height - g_StepClearance;

   v1a = g_AIPathDB.m_Vertices[v1].pt;
   v1a.z = g_AIPathDB.GetZAtXY(iFromCell, v1a);
   v1b = g_AIPathDB.m_Vertices[v2].pt;
   v1b.z = g_AIPathDB.GetZAtXY(iFromCell, v1b);

   v2a = g_AIPathDB.m_Vertices[v1].pt;
   v2a.z = g_AIPathDB.GetZAtXY(pLink->dest, v2a);
   v2b = g_AIPathDB.m_Vertices[v2].pt;
   v2b.z = g_AIPathDB.GetZAtXY(pLink->dest, v2b);

   fZDiff = __max(__max(fabs(v1a.z-v2a.z),
                        fabs(v1a.z-v2b.z)),
                  __max(fabs(v1b.z-v2a.z),
                        fabs(v1b.z-v2b.z)));

   // First, do old-style test. If that test think this is a wall, do further tests to see if this is
   // a possible LVL.


   ////////////////////////////////////////////////////////////////
   // Test dest cell first. If that's a wall, then we're done.

   // If dest is higher than src, then use dest's z. Else if
   // src is higher, then use src's z.
   endPt1 = v2a;
   endPt1.z += __max((v1a.z-v2a.z), 0) + g_pAICreatureSizes->sizes[0].height * .5 + SOME_SMALL_Z;

   endPt2 = v2b;
   endPt2.z += __max((v1a.z-v2a.z), 0) + g_pAICreatureSizes->sizes[0].height * .5 + SOME_SMALL_Z;

   nPassages = CheckPathClear(&endPt1, &endPt2,
                               fEffectiveHeight, radius, TRUE, FALSE);
   nClearPoints = CheckPathClear(&endPt1, &endPt2,
                                  fEffectiveHeight,
                                  g_MinimumValidEdgeWidth, FALSE, FALSE);

   // dest is a wall, so we're done.
   if (!nPassages && nClearPoints < (g_EdgePropertySampleSize - 2))
      return kEdgeIsWall;


   //////////////////////////////////////
   // Test source's plane.
   endPt1 = v1a;
   endPt1.z += fRise;

   endPt2 = v1b;
   endPt2.z += fRise;

   nPassages = CheckPathClear(&endPt1, &endPt2,
                               fEffectiveHeight, radius, TRUE, FALSE);
   nClearPoints = CheckPathClear(&endPt1, &endPt2,
                                  fEffectiveHeight,
                                  g_MinimumValidEdgeWidth, FALSE, FALSE);

   if (!nPassages && nClearPoints < (g_EdgePropertySampleSize - 2))
      EdgeVal = kEdgeIsWall;
   else if (nPassages && nClearPoints == g_EdgePropertySampleSize)
      EdgeVal = kEdgeOpen;
   else
      EdgeVal = kEdgeTouchWall;

   // In the past, when edge is not a wall, we'd be done by now, but now that we have LVLs and
   // HighStrikes, it's possible that this test passed, yet we can't actually link these cells
   // because there is intervening terrain. For example, steps on a floating staircase don't
   // really want to link to cells under them.

   // The problem here is that even co-planer floor cells fall into this category, and an additional
   // test would be way overkill, and possibly error-prone. Besides, if we want to add ObjRaycastC
   // to the raycast test, then we don't want objects on the floor getting in the way.
   // To this end, we apply additional tests only if they are clearly called for:
   // if minimum zdiff between edge points is less than stair height, then don't both raycasting.

   if (EdgeVal != kEdgeIsWall)
   {
      BOOL bSucceeded = TRUE;
      // endpoints are sufficiently far apart to warrant a raycast.
      if (fZDiff > SOME_SMALL_Z) // g_StepClearance)
         bSucceeded = TestEdgeRaycast(iFromCell, pLink);

      if (bSucceeded) // Raycasts worked. We can link.
         return EdgeVal;
      else // raycasts failed. We're considered an impassible wall.
         return kEdgeIsWall;
   }
   // Alright. This is a wall so far. Now let's Do another cigar tests up to max LVL height to
   // see if this is a possible LVL:
   else
   {
      const int nNumIncs = 5;
      eEdgeProperties BestEdgeVal = kEdgeIsWall;
      int i;
      float fRise1 = g_pAICreatureSizes->sizes[0].height * .5 + g_StepClearance * .5;
      float fEffectiveHeight1 = g_pAICreatureSizes->sizes[0].height - g_StepClearance;
      // Not ideal. Due to LVL, it requires *more* than enough head room in average case.
      float fRise2 = TERR_LVL_SIZE+g_pAICreatureSizes->sizes[0].height * .5;
      float fEffectiveHeight2 = g_pAICreatureSizes->sizes[0].height;

      for (i = 0; (i < nNumIncs) && (BestEdgeVal != kEdgeOpen); i++)
      {
         fRise = fRise1 + (fRise2-fRise1)*i/(float)(nNumIncs-1);
         fEffectiveHeight = fEffectiveHeight1 + (fEffectiveHeight2-fEffectiveHeight1)*i/(float)(nNumIncs-1);

         endPt1 = v1a;
         endPt1.z += fRise;

         endPt2 = v1b;
         endPt2.z += fRise;

         nPassages = CheckPathClear(&endPt1, &endPt2,
                                     fEffectiveHeight, radius, TRUE, FALSE);
         nClearPoints = CheckPathClear(&endPt1, &endPt2,
                                        fEffectiveHeight,
                                        g_MinimumValidEdgeWidth, FALSE, FALSE);

         if (!nPassages && nClearPoints < (g_EdgePropertySampleSize - 2))
            EdgeVal = kEdgeIsWall;
         else if (nPassages && nClearPoints == g_EdgePropertySampleSize)
            EdgeVal = kEdgeOpen;
         else
            EdgeVal = kEdgeTouchWall;

         if (EdgeVal < BestEdgeVal)
            BestEdgeVal = EdgeVal;
      }

      // OK, we think this might be a passable LVL, but now we need to raycast to make sure it isn't
      // blocked by terrain. For example, if there is a cell on a set of floating stairs that might
      // want to link to a cell under the stairs, we don't want to let it do that, so we raycast
      // a few times from center of source cell, to points along edge of dest cell. If at least N-2
      // raycasts succeed (subtract 2 for when cell is flanked by walls), then we can link these cells.

      // We also need to cast down from where our best cigar test succeeded, and make sure we're still hitting the
      // cell we think we should. We do this by getting location of most relevant cigar height that succeeded, and
      // position along edge, then shift that position a little closer to the dest cell's center, then cast down, and
      // see which cell we hit. If it's not the dest cell, then we're not succeeding against the cell we think we are,
      // and should revert to Wall status.

      if (BestEdgeVal != kEdgeIsWall)
      {
         BOOL bSucceeded;

         bSucceeded = TestEdgeRaycast(iFromCell, pLink);

         if (bSucceeded) // Raycasts worked. We can link.
            return BestEdgeVal;
         else // raycasts failed. We're considered an impassible wall.
            return kEdgeIsWall;
      }
#ifndef SHIP
      else if (!g_fAIPDB_EnableHighStrike)
         return kEdgeIsWall;
#endif
      else // This is at least a fairly tall wall. It may be HighStrike-able. Cell height could be
           // anywhere from TERR_LVL to HUGE_Z (a current difference of (5.1-2.41)==2.69 ft.).
           // Test cigars in this range. If any pass, then do raycast test.
      {
         const int nNumIncs = 5;
         int i;
         float fRise1 = TERR_LVL_SIZE+g_pAICreatureSizes->sizes[0].height * .5;
         float fEffectiveHeight1 = g_pAICreatureSizes->sizes[0].height;
         float fRise2 = HUGE_Z+g_pAICreatureSizes->sizes[0].height * .5;
         float fEffectiveHeight2 = g_pAICreatureSizes->sizes[0].height;
         eEdgeProperties BestEdgeVal = kEdgeIsWall;

         for (i = 0; (i < nNumIncs) && (BestEdgeVal != kEdgeOpen); i++)
         {
            fRise = fRise1 + (fRise2-fRise1)*i/(float)(nNumIncs-1);
            fEffectiveHeight = fEffectiveHeight1 + (fEffectiveHeight2-fEffectiveHeight1)*i/(float)(nNumIncs-1);

            endPt1 = v1a;
            endPt1.z += fRise;

            endPt2 = v1b;
            endPt2.z += fRise;

            nPassages = CheckPathClear(&endPt1, &endPt2,
                                        fEffectiveHeight, radius, TRUE, FALSE);
            nClearPoints = CheckPathClear(&endPt1, &endPt2,
                                           fEffectiveHeight,
                                           g_MinimumValidEdgeWidth, FALSE, FALSE);

            if (!nPassages && nClearPoints < (g_EdgePropertySampleSize - 2))
               EdgeVal = kEdgeIsWall;
            else if (nPassages && nClearPoints == g_EdgePropertySampleSize)
               EdgeVal = kEdgeOpen;
            else
               EdgeVal = kEdgeTouchWall;

            if (EdgeVal < BestEdgeVal)
               BestEdgeVal = EdgeVal;
         }

         // This may be a good HighStrike. Do raycast to make sure.
         if (BestEdgeVal != kEdgeIsWall)
         {
            BOOL bSucceeded;
            bSucceeded = TestEdgeRaycast(iFromCell, pLink);
            if (bSucceeded) // Raycasts worked. We can link.
               return kEdgeIsHighStrike;
            else // raycasts failed. We're considered an impassible wall.
               return kEdgeIsWall;
         }
         else // Now THIS is a wall.
            return kEdgeIsWall;
      }
   }
}



static BOOL CellIsPathable(int iCell)
{
   if (g_AIPathDB.m_Cells[iCell].pathFlags & (kAIPF_Unpathable | kAIPF_BlockingOBB))
   {
      // If this cell is part of pathable object, then let it be pathable, and not blocked.
      for (int i = 0; i < g_AIPathDB.m_nCellObjMaps; i++)
         if (iCell == g_AIPathDB.m_CellObjMap[i].cellID)
            return TRUE;
      return FALSE;
   }

   return TRUE;
}


static tAIPathOkBits PathFindComputeOkBits(int iFromCell, int iToCell,
                                           tAIPathVertexID v1,
                                           tAIPathVertexID v2,
                                           sAIPathCellLink *pLink,
                                           tAIPathOkBits *okCondBits)
{
   float fFloorDifference;

   if (okCondBits)
      *okCondBits = 0;

   if (!g_fAIPDB_CalcOkBits)
      return kAIOK_All;

   if (!CellIsPathable(iFromCell) || !CellIsPathable(iToCell))
   {
      return 0;
   }

   int iEdgeProperty = ComputeEdgeProperty(v1, v2, iFromCell, pLink, fFloorDifference);

   // Walls are not passable.
   if (iEdgeProperty == kEdgeIsWall)
      return 0;

   tAIPathOkBits rv = 0;
   sAIPathCellAdjunct *pToAdjunct = &g_AIPathCellAdjunct[iToCell];

   if (pToAdjunct->m_chWaistMedium == MEDIA_AIR)
      rv |= kAIOK_Walk;

   if (g_AIPathOpts.bWaterIsPathable && (pToAdjunct->m_chSwimMedium == MEDIA_WATER))
      rv |= kAIOK_Swim;

   // Is a really good candidate for being a high strike link.
   if (iEdgeProperty == kEdgeIsHighStrike)
   {
      // We want to test this later, for possible HighStrike. If it turns out to fail,
      // then the bits will be removed, and the link invalidated.
      if (okCondBits)
         *okCondBits |= kAIOKCOND_HighStrike;
      return rv;
   }

  if (okCondBits && (rv & (kAIOK_Walk|kAIOK_Swim)) && (fabs(fFloorDifference) < AIPATHDB_SmallCreatureHeight))
     *okCondBits |= kAIOK_SmallCreature;

   return rv;
}



/////////////////////////////////////////////////

#define kNumVolumes 7
static float g_afVolumeHeight[kNumVolumes]
= { 0.0,    3.0,     4.0,    6.0833, 8.0,    12.0,   20.0 };
static float g_afVolumeRadius[kNumVolumes]
= { 0.0,    .25,     .5,     .875,   1.5,    3.0,    4.0 };

static void VolumeCigarAtCenter(int iCellIndex)
{
   mxs_vector Center = g_AIPathDB.GetCenter(iCellIndex);
   mxs_vector RaisedCenter;
   float fHeight, radius, fRise, fEffectiveHeight;
   int iSize = 0;

   for (int i = kNumVolumes - 1; i > 0; --i)
   {

      // This part here should really be precalculated.
      fHeight = g_afVolumeHeight[i];
      radius = g_afVolumeRadius[i];
      fRise = fHeight * .5 + g_StepClearance * .5;
      fEffectiveHeight = fHeight - g_StepClearance;

      RaisedCenter = Center;
      RaisedCenter.z += fRise;

      if (CheckCigarClear(&RaisedCenter, fEffectiveHeight, radius))
      {
         iSize = i;
         goto found_volume;
      }
   }

found_volume:
   ConfigSpew("PathFindGenSpew",
              ("     volume: %d (height %g, radius %g)\n",
               iSize, g_afVolumeHeight[iSize], g_afVolumeRadius[iSize]));

   g_AIPathDB.m_Cells[iCellIndex].m_Volume = iSize;
}


/////////////////////////////////////////////////

// helpers for FindAdjunctData, below
static float AdjunctCeiling(Location *pSamplePoint, int iCellIndex,
                            float fMinCeilingZ)
{
   pSamplePoint->hint = CELL_INVALID;

   if (ComputeCellForLocation(pSamplePoint) != CELL_INVALID)
   {
      Location Hit;
      Location CastEnd = *pSamplePoint;
      CastEnd.vec.z += 1000.0;      // functional gazillion

      if (PortalRaycast(pSamplePoint, &CastEnd, &Hit, 1))
      {
         Warning(("AdjunctCeiling (aipathdb): cast failed from (%g %g %g)\n",
                  pSamplePoint->vec.x, pSamplePoint->vec.y,
                  pSamplePoint->vec.z));
      } else {
         if (Hit.vec.z < fMinCeilingZ)
            fMinCeilingZ = Hit.vec.z;
      }
   }

   return fMinCeilingZ;
}


//////////////////////////


// Here we figure out a few things about each pathfinding cell and
// stuff them into a corresponding sAIPathCellAdjunct structure.

// Most of the work is for floor and ceiling heights, which are both
// worst-case (so lowest ceiling, highest floor).  We also determine
// whether there's little enough water in the cell that a guard could
// ford it, and whether there's enough that a swimming creature could
// swim in it.

static void FindAdjunctData(int iCellIndex,
                            sAIPathCellAdjunct *pAdjunct)
{
   Location SamplePoint, WadePoint, SwimPoint, FlyPoint, DummyPoint;
   mxs_vector *pVertex;

   uchar chSwimMedium = MEDIA_AIR;
   uchar chWaistMedium = MEDIA_AIR;
   uchar chFlyMedium = MEDIA_SOLID;

   sAIPathCell *pCell = &g_AIPathDB.m_Cells[iCellIndex];
   const int iFirstVertex = pCell->firstVertex;
   const int iVertexLimit = iFirstVertex + pCell->vertexCount;

   float fMaxFloorZ = -4500000000000.0; // -4.5 gazillion
   float fMinCeilingZ = 4500000000000.0;// 4.5 gazillion

   mxs_vector CellCenter = g_AIPathDB.GetCenter(iCellIndex);

   // This loop takes care of floor height, wading and swimming media,
   // and the cell's 2d bounding box.

   if (g_fAIPDB_CellInfo)
   {
      for (int i = iFirstVertex; i < iVertexLimit; ++i)
      {
         pVertex = &(g_AIPathDB.m_Vertices[g_AIPathDB.m_CellVertices[i].id].pt);

         // The floor is easy.
         if (pVertex->z > fMaxFloorZ)
            fMaxFloorZ = pVertex->z;

         // Now we find the media at swimming and waist heights.
         mx_interpolate_vec(&SamplePoint.vec, &CellCenter, pVertex, .9);
         SamplePoint.vec.z += .01;   // scoodge to make sure we're in world
         SamplePoint.hint = CELL_INVALID;

         // This is our first pass at ceiling height--will we want
         // something more thorough?
         fMinCeilingZ = AdjunctCeiling(&SamplePoint, iCellIndex, fMinCeilingZ);

         // Can a guard wade into this cell?  If any point we test is
         // underwater, the guard's not allowed here.
         WadePoint = SamplePoint;
         WadePoint.vec.z += WADING_HEIGHT;
         if (ComputeCellForLocation(&WadePoint) != CELL_INVALID)
            if (WR_CELL(WadePoint.cell)->medium == MEDIA_WATER)
               chWaistMedium = MEDIA_WATER;

         // Can a swimming creature negotiate this cell?
         SwimPoint = SamplePoint;
         SwimPoint.vec.z += SWIM_DEPTH;
         if (ComputeCellForLocation(&SwimPoint) != CELL_INVALID)
            if (WR_CELL(SwimPoint.cell)->medium == MEDIA_WATER)
               chSwimMedium = MEDIA_WATER;

         // Does something which flies have enough room to do it here?
         FlyPoint = SamplePoint;
         FlyPoint.vec.z = fMinCeilingZ - .01;
         if (PortalRaycast(&FlyPoint, &SamplePoint, &DummyPoint, 1))
            if (ComputeCellForLocation(&FlyPoint) != CELL_INVALID)
               if (WR_CELL(FlyPoint.cell)->medium == MEDIA_AIR)
                  chFlyMedium = MEDIA_AIR;
      }
   }

   pAdjunct->m_fMaxFloorZ = fMaxFloorZ;
   pAdjunct->m_fMinCeilingZ = fMinCeilingZ;
   pAdjunct->m_chSwimMedium = chSwimMedium;
   pAdjunct->m_chWaistMedium = chWaistMedium;

   pAdjunct->m_bRamp = CellIsRamp(iCellIndex);
   if (!pAdjunct->m_bRamp)
      pAdjunct->m_iStairSize = CellStairLevel(iCellIndex);

   pAdjunct->m_iBlockedDirectionFlags = CellBlockedDirections(iCellIndex);

   ConfigSpew("PathFindGenSpew",
              ("     floor z = %g, ceiling z = %g, media %d %d\n",
               pAdjunct->m_fMaxFloorZ, pAdjunct->m_fMinCeilingZ,
               (uint) pAdjunct->m_chSwimMedium,
               (uint) pAdjunct->m_chWaistMedium));
}


#define kNumCeilingHeights 7
static double g_aCeilingHeight[kNumCeilingHeights]
= {
   2.0, 4.0, 6.0, 8.0, 10.0, 15.0, 20.0
};

static void SetCellInfoFromAdjunct(int iCellIndex,
                                   sAIPathCellAdjunct *pAdjunct,
                                   sAIPathCell *pCell)
{
   // clear all fields
   ProcessOpenDirections(iCellIndex);
   VolumeCigarAtCenter(iCellIndex);

   if (pAdjunct->m_chSwimMedium == MEDIA_WATER
    || pAdjunct->m_chWaistMedium == MEDIA_WATER)
      pCell->m_Water = 1;

   float fRoomHeight = pAdjunct->m_fMinCeilingZ - pAdjunct->m_fMaxFloorZ;
   int i;
   for (i = kNumCeilingHeights - 1; i >= 0; --i)
      if (fRoomHeight > g_aCeilingHeight[i])
         break;
   if (i == -1)
      i = 0;
   pCell->m_CeilingHeight = i;
}


// There's some temporary info for the cells which we need before we
// can figger out our OkBits and other accessory info.
static void SetAdjunctData()
{
   int i;
   g_AIPathCellAdjunct
      = (sAIPathCellAdjunct *) malloc((g_AIPathDB.m_nCells + 1)
                                    * sizeof(sAIPathCellAdjunct));

   memset(g_AIPathCellAdjunct, 0, (g_AIPathDB.m_nCells + 1)
                                  * sizeof(sAIPathCellAdjunct));

   for (i = 1; i <= g_AIPathDB.m_nCells; ++i)
   {
      memset(AIAddrCellInfo(&g_AIPathDB.m_Cells[i]), 0, kSizeofCellInfo);
   }

   sAIPathCellAdjunct *pAdjunct = g_AIPathCellAdjunct;
   sAIPathCell *pCell = g_AIPathDB.m_Cells;

   for (i = 1; i <= g_AIPathDB.m_nCells; ++i)
   {
      ++pAdjunct;
      ++pCell;

#ifndef SHIP
      if (config_is_defined("PathFindGenSpew"))
      {
         mxs_vector Center = g_AIPathDB.GetCenter(i);
         mprintf("Cell %i: Center at (%g %g %g)\n",
                  i, Center.x, Center.y, Center.z);
      }
#endif // ~SHIP

      PumpWindowsEvents();
      if (i % 32 == 0)
      {
         mprintf(".");
      }
      FindAdjunctData(i, pAdjunct);
      SetCellInfoFromAdjunct(i, pAdjunct, pCell);
   }
}

////////////////////////

static void ClearAdjunctData()
{
   free(g_AIPathCellAdjunct);
   g_AIPathCellAdjunct = 0;
}

/////////////////////////////////////////////////

// Two kinds of cells are marked as being stairish: those which we can
// step down from and those adjacent to them.  Notice that this is
// mapping from the adjunct data to the final database.
static void PropagateFloorTypes()
{
   sAIPathCellAdjunct *pAdjunct = g_AIPathCellAdjunct;
   sAIPathCell *pCell = g_AIPathDB.m_Cells;

   for (int iCell = 1; iCell <= g_AIPathDB.m_nCells; ++iCell)
   {
      ++pAdjunct;
      ++pCell;

      if (pAdjunct->m_bRamp)
      {
         pCell->m_IsRamp = 1;
         continue;
      }

      int iStairSize = pAdjunct->m_iStairSize;

      if (iStairSize == kAdjunctNoStair)
         continue;

      pCell->m_IsStair = 1;
      pCell->m_StairSize = iStairSize;

      int iFirstCell = g_AIPathDB.m_Cells[iCell].firstCell;
      int iCellLimit = iFirstCell + g_AIPathDB.m_Cells[iCell].cellCount;

      for (int i = iFirstCell; i < iCellLimit; ++i) {
         sAIPathCellLink *pLink = &g_AIPathDB.m_Links[i];
         int iNeighbor = pLink->dest;
         sAIPathCell *pNeighborCell = &g_AIPathDB.m_Cells[iNeighbor];

         // If our neighbor is already more of a stair than we are, we
         // don't want to water that down.
         if (pNeighborCell->m_IsStair == 1
          && pNeighborCell->m_StairSize > pCell->m_StairSize)
            continue;

         pNeighborCell->m_IsStair = 1;
         pNeighborCell->m_StairSize = iStairSize;
      }
   }

#ifndef SHIP
   if (config_is_defined("PathFindGenSpew")) {
      pCell = g_AIPathDB.m_Cells;

      for (int iCell = 1; iCell <= g_AIPathDB.m_nCells; ++iCell) {
         ++pCell;

         mprintf("Cell %d: Stair %s Ramp %s\n",
                 iCell,
                 (g_AIPathDB.m_Cells[iCell].m_IsStair)? "Y" : "n",
                 (g_AIPathDB.m_Cells[iCell].m_IsRamp)? "Y" : "n");
      }
   }
#endif // ~SHIP
}

/////////////////////////////////////////////////

// No lighting has value 0.  While most cells will be from 0 to 1, the
// range is open-ended.
static double g_LightThresholds[kAIPathCellNumLightLevels]
= {
   0, .1, .2, .3, .4, .5, .6, .7, .8, .9, 1.0, 1.1, 1.2, 1.3, 1.4, 1.5
};

////////////////////////

inline void ComputeCellLightLevels()
{
   mxs_vector Center;

   // so our dummy object can be facing some direction...
   mxs_angvec DummyAngvec;
   mx_mk_angvec(&DummyAngvec, 0, 0, 0);

   // We need a thing to light.
   ObjID DummyObject = BeginObjectCreate(ROOT_ARCHETYPE, kObjectConcrete);
   Assert_(DummyObject != OBJ_NULL);
   EndObjectCreate(DummyObject);

   for (int iCellIndex = 1; iCellIndex <= g_AIPathDB.m_nCells; ++iCellIndex) {
      Center = g_AIPathDB.GetCenter(iCellIndex);
      Center.z += LIGHT_HEIGHT;

      ObjPosUpdate(DummyObject, &Center, &DummyAngvec);
      float fLighting = compute_object_lighting(DummyObject);
      int iLevel = 0;

      for (int i = 0; i < kAIPathCellNumLightLevels; ++i)
         if (fLighting > g_LightThresholds[i])
            iLevel = i;

      g_AIPathDB.m_Cells[iCellIndex].m_LightLevel = iLevel;

      ConfigSpew("PathFindGenSpew", ("Lighting for cell %d: %g => %d\n",
                                     iCellIndex, fLighting, iLevel));
   }

   // We no longer need our thing to light.
   DestroyObject(DummyObject);
}

/////////////////////////////////////////////////

#define kSteepSlopeMultiplier 5.0

inline void ComputeCell2CellCost(const tAIPathCellID cell_1,
                                 const tAIPathCellID cell_2, uchar *cost)
{
   mxs_vector center_1, center_2;
   float      dist;

   g_AIPathDB.GetCenter(cell_1, &center_1);
   g_AIPathDB.GetCenter(cell_2, &center_2);

   dist = AIDistance(center_1, center_2);

   if (g_AIPathDB.GetCellPlane(cell_2)->normal.z < g_AIPlaneZNormPath)
   {
      dist *= kSteepSlopeMultiplier;
   }

   if (dist > 254.0)
      dist = 254.0;

   *cost = (uchar)((int)dist);
}

///////////////////////////////////////////////////////////////////////////////
//
// DATABASE CREATION
//



/////////////////////////////////////////////////

#define MAX_LINKS_INTERESTED_PER_CELL 48
// wsf: bumped this up 'cause some test scenarios were bumping up against this.
// Still, it may substantially bump up memory requirements for db building, so let's
// keep it the way it was.
// #define MAX_LINKS_INTERESTED_PER_CELL 64

struct sAIPathCellConnections
{
   sAIPathCellConnections()
   {
      memset(this, 0, sizeof(sAIPathCellConnections));
   }

   BOOL Wants(sAIEdgeConnectData * p)
   {
      if (totalFound < MAX_LINKS_INTERESTED_PER_CELL)
      {
         totalFound++;
         return (count < MAX_LINKS_FROM_CELL) || pflt(p->distSq, ppConnectionData[0]->distSq);
      }
      return FALSE;
   }

   void Add(sAIEdgeConnectData * pNewData)
   {
      // Coming in, it is assumed Wants() was called...
      int i;
      sAIEdgeConnectData * pTemp;

      if (count < MAX_LINKS_FROM_CELL)
      {
         ppConnectionData[count] = pNewData;
         for (i = count - 1; i > 0; i--)
         {
            if (pflt(ppConnectionData[i]->distSq, ppConnectionData[i+1]->distSq))
            {
               pTemp = ppConnectionData[i];
               ppConnectionData[i] = ppConnectionData[i+1];
               ppConnectionData[i+1] = pTemp;
            }
         }
         count++;
      }
      else
      {
         ppConnectionData[0] = pNewData;
         for (i = 0; i < MAX_LINKS_FROM_CELL - 1; i++)
         {
            if (pflt(ppConnectionData[i]->distSq, ppConnectionData[i+1]->distSq))
            {
               pTemp = ppConnectionData[i];
               ppConnectionData[i] = ppConnectionData[i+1];
               ppConnectionData[i+1] = pTemp;
            }
         }
      }
   }

   int                  totalFound;
   int                  count;
   sAIEdgeConnectData * ppConnectionData[MAX_LINKS_FROM_CELL];
};

/////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////
// Set LVL and HighStrike qualifiers
/////////////////////////////////////////////////////////////////////
// Sadly, when we go to compute LVL and High Strike, not
// all the info we need is present.
// We don't want to preclude shallow ramps, etc, so the
// here are the conditions under which we apply
// LVL and HighStrike:
//    1) Test vertices of cell1 against cell2. If both are
//       completely above or below the other two, then we
//       continue with remaining tests. This isn't ideal, since
//       cell vertices need not be shared, and won't cover all cases.
//       However, it should catch most common and useful cases.
//
//    2) Use midpoint z of each edge for testing.
//
//

// We still need to consider ramp cases. Consider this side view:
//

/*
   flat surface A
      |
      v

   --------     ramp B
           \   /
            \ L
             \
   ----------- ---------   <--- flat surface C
      ^
      |
  flat surface D
*/

// If the Z difference between A and D is greater than TERR_LVL_SIZE, then the link from
// D to B needs to be removed, otherwise AI's will try to path to B from D, but they'll just
// bounce into terrain.
//
// We can treat B like we treat A, with respect to D, and consider B to be more like "above"
// D, as opposed to intersecting D (which is a degenerate case).
// To this end, use an omega. The alternative is to do another SharedEdge calculation based on
// TERR_LVL_SIZE rather than HUGE_Z.
//

#define SURFACE_OMEGA 0.1000

#define PointIsLess(p1, p2a, p2b) \
   ((p1.z < (p2a.z+SURFACE_OMEGA)) && (p1.z < (p2b.z+SURFACE_OMEGA)))



// If one point is clearly lower, then apply OMEGA to the other to treat minor "X" cases (e.g. ramps) more like
// one edge is completely above another. We hope there aren't precision errors here.
static inline BOOL EdgeIsLower(mxs_vector &v1a, mxs_vector &v1b, mxs_vector &v2a, mxs_vector &v2b)
{
   if (v2b.z < v2a.z)
      return (((v1a.z < v2a.z) && (v1a.z < (v2b.z+SURFACE_OMEGA))) &&
          ((v1b.z < v2a.z) && (v1b.z < (v2b.z+SURFACE_OMEGA))));
   else
      return (((v1a.z < v2b.z) && (v1a.z < (v2a.z+SURFACE_OMEGA))) &&
          ((v1b.z < v2b.z) && (v1b.z < (v2a.z+SURFACE_OMEGA))));
}


// This version not used anymore:

/*
// Careful here. Make sure co-planer points return FALSE!
static inline BOOL EdgesDoNotIntersect(mxs_vector &v1a, mxs_vector &v1b, mxs_vector &v2a, mxs_vector &v2b)
{
// old way: ramps would fail:
//  if (((v1a.z < v2a.z) && (v1a.z < v2b.z) && (v1b.z < v2a.z) && (v1b.z < v2b.z)) ||
//      ((v2a.z < v1a.z) && (v2a.z < v1b.z) && (v2b.z < v1a.z) && (v2b.z < v1b.z)))
//       return TRUE;
//  else
//       return FALSE;

   return EdgeIsLower(v1a,v1b,v2a,v2b) || EdgeIsLower(v2a,v2b,v1a,v1b);
}
*/


///////////////////////

// return TRUE if edges do not make 'x' pattern, and fill fZDiff with max mag diff of dest.z-source.z
static inline BOOL EdgesDoNotIntersect(int iFromCell, sAIPathCellLink * pLink, float &fZDiff)
{
   mxs_vector v1a, v1b;
   mxs_vector v2a, v2b;
   float fZ1, fZ2;

   // Get link points on plane of each cell:

   v1a = g_AIPathDB.m_Vertices[pLink->vertex_1].pt;
   v1a.z = g_AIPathDB.GetZAtXY(iFromCell, v1a);

   v1b = g_AIPathDB.m_Vertices[pLink->vertex_2].pt;
   v1b.z = g_AIPathDB.GetZAtXY(iFromCell, v1b);

   v2a = g_AIPathDB.m_Vertices[pLink->vertex_1].pt;
   v2a.z = g_AIPathDB.GetZAtXY(pLink->dest, v2a);

   v2b = g_AIPathDB.m_Vertices[pLink->vertex_2].pt;
   v2b.z = g_AIPathDB.GetZAtXY(pLink->dest, v2b);

   // ack. ConnectionData is bi-directional.

   // There are degenerate cases here, when, say, slope of one cell is
   // perp to another, and their edges bisect one another. But we can't
   // do much for those cases beyond further splitting of cells.
   // Just do best we can here:
   if ((v1a.z <= v2a.z) && (v1a.z <= v2b.z) && (v1b.z <= v2a.z) && (v1b.z <= v2b.z))
      fZDiff = __max(v2a.z, v2b.z)-__min(v1a.z,v1b.z);
   else if ((v2a.z <= v1a.z) && (v2a.z <= v1b.z) && (v2b.z <= v1a.z) && (v2b.z <= v1b.z))
      fZDiff = __min(v2a.z, v2b.z)-__max(v1a.z,v1b.z);
   else // degenerate case
   {
      fZ1 = (v1a.z+v1b.z)/2;
      fZ2 = (v2a.z+v2b.z)/2;
      fZDiff = fZ2-fZ1;
   }

   return EdgeIsLower(v1a,v1b,v2a,v2b) || EdgeIsLower(v2a,v2b,v1a,v1b);
}

///////////////////////

static void PostProcessModifiedOkBits()
{
   tAIPathCellID cell1;  // path cell index
   sAIPathCellLink * pCell1To2Link;
   int iFirstCell1;
   int iCellLimit1;
   int i,j;
   float fZDiff;
   float fLVLSize;
   BOOL bHighStrikeOnly;
   BOOL bEdgesDoNotIntersect;


   for (cell1 = 1; cell1 <= g_AIPathDB.m_nCells; cell1++)
   {
      iFirstCell1 = g_AIPathDB.m_Cells[cell1].firstCell;
      if (g_AIPathDB.m_Cells[cell1].wrapFlags & IDWRAP_Cell2Cell)
         iFirstCell1 += MAX_CELL2CELL_LINKS;

      iCellLimit1 = iFirstCell1 + g_AIPathDB.m_Cells[cell1].cellCount;
      pCell1To2Link = 0;
      for (i = iFirstCell1; i < iCellLimit1; ++i)
      {
         pCell1To2Link = &g_AIPathDB.m_Links[i];
         if (!pCell1To2Link->okBits)
            continue;

         bEdgesDoNotIntersect = EdgesDoNotIntersect(cell1, pCell1To2Link, fZDiff);

         if (fZDiff >= 0) // going up. Test destination cell
         {
            // See if this cell was donated from an object:
            for (j=0; j<g_AIPathDB.m_nCellObjMaps; j++)
               if (g_AIPathDB.m_CellObjMap[j].cellID == pCell1To2Link->dest)
                  break;
         }
         else // going down. Test source cell
         {
            // See if this cell was donated from an object:
            for (j=0; j<g_AIPathDB.m_nCellObjMaps; j++)
               if (g_AIPathDB.m_CellObjMap[j].cellID == cell1)
                  break;
         }

         if (j < g_AIPathDB.m_nCellObjMaps)
            fLVLSize = OBJ_LVL_SIZE;
         else
            fLVLSize = TERR_LVL_SIZE;

         if (bEdgesDoNotIntersect)
         {
            bHighStrikeOnly = !!(pCell1To2Link->okBits & kAIOKCOND_HighStrike);

            // HighStrike added so that AI's can path near you when you are pretty high above them, but not too high.
            // If Edge test consideres this to be a high-strikable wall, or if the z test determines this to be high enough, then
            // do high strike.
            #ifndef SHIP
            if (g_fAIPDB_EnableHighStrike)
            {
            #endif
               if (bHighStrikeOnly || ((fZDiff > fLVLSize) && (fZDiff <= HUGE_Z)))
                  pCell1To2Link->okBits |= kAIOKCOND_HighStrike;
               // Else, this link was setup only because its edge was a wall that might have qualified
               // for highstrike, but it failed, so reset bits.
               else if (bHighStrikeOnly)
                  pCell1To2Link->okBits = 0;
            #ifndef SHIP
            }
            #endif

            // If ZDiff is too high, then set bits to 0.
            #ifndef SHIP
            if (g_fAIPDB_fUseLVLs)
            {
            #endif
               if ((!(pCell1To2Link->okBits & kAIOKCOND_HighStrike)) && (fabs(fZDiff) > VERTICAL_RISE_LIMIT) && (fabs(fZDiff) < fLVLSize))
                  pCell1To2Link->okBits |= kAIOKCOND_Stressed;
               else if ((!(pCell1To2Link->okBits & kAIOKCOND_HighStrike)) && (fabs(fZDiff) >= fLVLSize))
                  pCell1To2Link->okBits = 0;
            #ifndef SHIP
            }
            else
            {
            #endif
               if ((!(pCell1To2Link->okBits & kAIOKCOND_HighStrike)) && (fabs(fZDiff) > VERTICAL_RISE_LIMIT))
                  pCell1To2Link->okBits = 0;
            #ifndef SHIP
            }
            #endif
         }
         // else, edges are shared, or degenerate 'X' shaped intersection.
         else // remove unused "HighStrike" bits, and validate normal pathing.
         {
            if (pCell1To2Link->okBits & kAIOKCOND_HighStrike)
               pCell1To2Link->okBits = 0;

            // If ZDiff is too much, then set bits to 0.

            #ifndef SHIP
            if (g_fAIPDB_fUseLVLs)
            {
            #endif
               if ((fabs(fZDiff) > VERTICAL_RISE_LIMIT) && (fabs(fZDiff) < fLVLSize))
                  pCell1To2Link->okBits |= kAIOKCOND_Stressed;
               else if (fabs(fZDiff) >= fLVLSize)
                  pCell1To2Link->okBits = 0;
            #ifndef SHIP
            }
            else
            {
            #endif
               if (fabs(fZDiff) > VERTICAL_RISE_LIMIT)
                  pCell1To2Link->okBits = 0;
            #ifndef SHIP
            }
            #endif

            // @TBD: we assume degenerate case is pathable, is this ok? Severe 'X' cases (since we're using
            // HUGE_Z for edge test criteria) would fail. Ideally, we'd re-test the edges using TERR_LVL_SIZE
            // as max z val.
         }
      }
   }
}


////////////////////////////////////////






float g_MinimumSkewOverlap = 0.083333;

struct sIsSharedEdgeResult
{
   float distSqA, distSqB;
   int kind;
};

static BOOL __fastcall
IsSharedEdge(const cMxsVector & edgePt_1a, const cMxsVector & edgePt_1b,
             const cMxsVector & edgePt_2a, const cMxsVector & edgePt_2b,
             sIsSharedEdgeResult * pResult)
{

   BOOL test1, test2;

   float distSq2a;
   float distSq2b;
   float distSq1a;
   float distSq1b;
   float sum1, sum2;

   distSq2a = Vec2PointSegmentSquared(edgePt_2a.AsVec2Ptr(), edgePt_1a.AsVec2Ptr(), edgePt_1b.AsVec2Ptr());
   distSq2b = Vec2PointSegmentSquared(edgePt_2b.AsVec2Ptr(), edgePt_1a.AsVec2Ptr(), edgePt_1b.AsVec2Ptr());

   distSq1a = Vec2PointSegmentSquared(edgePt_1a.AsVec2Ptr(), edgePt_2a.AsVec2Ptr(), edgePt_2b.AsVec2Ptr());
   distSq1b = Vec2PointSegmentSquared(edgePt_1b.AsVec2Ptr(), edgePt_2a.AsVec2Ptr(), edgePt_2b.AsVec2Ptr());

   if (g_fAIPDB_FullyContained)
   {
      test1 = (pfle(distSq2a, CLOSE_ENOUGH_HORIZONTALLY_SQ) && pfle(distSq2b, CLOSE_ENOUGH_HORIZONTALLY_SQ));
      test2 = (pfle(distSq1a, CLOSE_ENOUGH_HORIZONTALLY_SQ) && pfle(distSq1b, CLOSE_ENOUGH_HORIZONTALLY_SQ));

      sum1 = distSq2a + distSq2b;
      sum2 = distSq1a + distSq1b;

      if (test1 && !(test2 && sum1 > sum2))
      {
         pResult->distSqA = distSq2a;
         pResult->distSqB = distSq2b;
         pResult->kind = kCell2EdgeContained;
         return TRUE;
      }

      if (test2)
      {
         pResult->distSqA = distSq1a;
         pResult->distSqB = distSq1b;
         pResult->kind = kCell1EdgeContained;
         return TRUE;
      }
   }

   if (g_fAIPDB_Skewed)
   {
      test1 = (pfle(distSq1a, CLOSE_ENOUGH_HORIZONTALLY_SQ) &&
               pfle(distSq2a, CLOSE_ENOUGH_HORIZONTALLY_SQ) &&
               aflt(g_MinimumSkewOverlap, Vec2Dist(edgePt_1a.AsVec2Ptr(), edgePt_2a.AsVec2Ptr())) &&
               aflt(Vec2PointLineDist(edgePt_1b.AsVec2Ptr(), edgePt_2a.AsVec2Ptr(), edgePt_2b.AsVec2Ptr()), CLOSE_ENOUGH_HORIZONTALLY));

      test2 = (pfle(distSq1b, CLOSE_ENOUGH_HORIZONTALLY_SQ) &&
               pfle(distSq2b, CLOSE_ENOUGH_HORIZONTALLY_SQ) &&
               aflt(g_MinimumSkewOverlap, Vec2Dist(edgePt_1b.AsVec2Ptr(), edgePt_2b.AsVec2Ptr())) &&
               aflt(Vec2PointLineDist(edgePt_1a.AsVec2Ptr(), edgePt_2a.AsVec2Ptr(), edgePt_2b.AsVec2Ptr()), CLOSE_ENOUGH_HORIZONTALLY));

      sum1 = distSq1a + distSq2a;
      sum2 = distSq1b + distSq2b;

      if (test1 && !(test2 && sum1 > sum2))
      {
         pResult->distSqA = distSq1a;
         pResult->distSqB = distSq2a;
         pResult->kind = kCell2SkewedLeft;
         return TRUE;
      }

      if (test2)
      {
         pResult->distSqA = distSq1b;
         pResult->distSqB = distSq2b;
         pResult->kind = kCell2SkewedRight;
         return TRUE;
      }
   }

   return FALSE;
}

///////////////////////////////////////


static BOOL __fastcall AIPathCellsShareEdge()
{
   cMxsVector   edgePt_1a, edgePt_1b;
   cMxsVector   edgePt_2a, edgePt_2b;
   int       i, j, iBestForCell1, iBestForCell2;

   const int vertexCount1 = g_AIPathDB.m_Cells[g_CurConnData.cell1].vertexCount;
   const int vertexCount2 = g_AIPathDB.m_Cells[g_CurConnData.cell2].vertexCount;

   // First, we check for the low-cost, simple match...
   g_CurConnData.kind = kPerfect;
   if (g_fAIPDB_QuickShare)
   {
      for (i = 0; i < vertexCount1; i++)
      {
         g_AIPathDB.GetCellVertexFull(g_CurConnData.cell1, i, &edgePt_1a, &g_CurConnData.vertex1a);
         g_AIPathDB.GetCellVertexFull(g_CurConnData.cell1, i + 1, &edgePt_1b, &g_CurConnData.vertex1b);

         for (j = 0; j < vertexCount2; j++)
         {
            g_AIPathDB.GetCellVertexFull(g_CurConnData.cell2, j, &edgePt_2a, &g_CurConnData.vertex2a);
            g_AIPathDB.GetCellVertexFull(g_CurConnData.cell2, j + 1, &edgePt_2b, &g_CurConnData.vertex2b);

            // if they share the exact same 2 vertices, then they are
            // definitely connected.
            if ((g_CurConnData.vertex1a == g_CurConnData.vertex2b && g_CurConnData.vertex1b == g_CurConnData.vertex2a) ||
                (edgePt_1a == edgePt_2b && edgePt_1b == edgePt_2a))
            {
               SetCellEdgeLinked(g_CurConnData.cell1, i, TRUE);
               SetCellEdgeLinked(g_CurConnData.cell2, j, TRUE);
               return TRUE;
            }

#if CHECK_COUNTERCLOCKS
            if ((g_CurConnData.vertex1a == g_CurConnData.vertex2a && g_CurConnData.vertex1b == g_CurConnData.vertex2b) ||
                (edgePt_1a == edgePt_2a && edgePt_1b == edgePt_2b))
            {
               SetCellEdgeLinked(g_CurConnData.cell1, i, TRUE);
               SetCellEdgeLinked(g_CurConnData.cell2, j, TRUE);
               mprintf("!");
               return TRUE;
            }
#endif
         }
      }
   }

   // Otherwise, we must resort to a more exhaustive check...

   if (g_fAIPDB_ExhaustShare)
   {
      #define LIMIT_BEST_DIST_SQ ((float)kFloatMax)
      sAIEdgeConnectData curConnect;
      sIsSharedEdgeResult result;

      int   bestKind;
      float sumDistSq;
      float bestSumDistSq = LIMIT_BEST_DIST_SQ;

      memcpy(&curConnect, &g_CurConnData, sizeof(curConnect));

      for (i = 0; i < vertexCount1; i++)
      {
         g_AIPathDB.GetCellVertexFull(curConnect.cell1, i,     &edgePt_1a, &curConnect.vertex1a);
         g_AIPathDB.GetCellVertexFull(curConnect.cell1, i + 1, &edgePt_1b, &curConnect.vertex1b);

         for (j = 0; j < vertexCount2; j++)
         {
            g_AIPathDB.GetCellVertexFull(curConnect.cell2, j,     &edgePt_2a, &curConnect.vertex2a);
            g_AIPathDB.GetCellVertexFull(curConnect.cell2, j + 1, &edgePt_2b, &curConnect.vertex2b);

            // Check if edges are close enough to connect.
            if (!IsSharedEdge(edgePt_1a, edgePt_1b, edgePt_2a, edgePt_2b, &result))
            {
               continue; // Not close enough
            }

            if (!g_fAIPDB_AllSteps)
            {
               if (DistSqPtLine(edgePt_1a, edgePt_2a, edgePt_2b) > sq(HUGE_Z) || // @TBD (toml 08-23-97): deal with up vs down (see rcs logs version 1.36 for previous code)
                   DistSqPtLine(edgePt_1b, edgePt_2a, edgePt_2b) > sq(HUGE_Z))
                  continue; // Not close enough
            }
            else if (g_fAIPDB_NoSteps)
            {
               if (DistSqPtLine(edgePt_1a, edgePt_2a, edgePt_2b) > sq(0.001) ||
                   DistSqPtLine(edgePt_1b, edgePt_2a, edgePt_2b) > sq(0.001))
                  continue; // Not close enough
            }

            sumDistSq = result.distSqA + result.distSqB;

            // It's close, but is it our best?

            if (pfle(sumDistSq, kAIEpsilon) || pflt(sumDistSq, bestSumDistSq))
            {
               g_CurConnData.vertex1a = curConnect.vertex1a;
               g_CurConnData.vertex1b = curConnect.vertex1b;
               g_CurConnData.vertex2a = curConnect.vertex2a;
               g_CurConnData.vertex2b = curConnect.vertex2b;

               bestSumDistSq = sumDistSq;
               bestKind = result.kind;

               iBestForCell1 = i;
               iBestForCell2 = j;

               // If the edges are essentially colinear, we're done
               if (pfle(sumDistSq, kAIEpsilon))
                  goto done_exhaustive;

            }
         }
      }

done_exhaustive:

      if (bestSumDistSq != LIMIT_BEST_DIST_SQ)
      {
         SetCellEdgeLinked(g_CurConnData.cell1, iBestForCell1, TRUE);
         SetCellEdgeLinked(g_CurConnData.cell2, iBestForCell2, TRUE);
         g_CurConnData.kind = bestKind;
         return TRUE;
      }
   }

   return FALSE;
}

///////////////////////////////////////

#define DumpVertex(cell, index) \
   mprintf("%d [%d (%g, %g, %g]", \
           i, \
           g_AIPathDB.m_CellVertices[g_AIPathDB.m_Cells[cell].firstVertex + index].id, \
           g_AIPathDB.m_Vertices[g_AIPathDB.m_CellVertices[g_AIPathDB.m_Cells[cell].firstVertex + index].id].pt.x, \
           g_AIPathDB.m_Vertices[g_AIPathDB.m_CellVertices[g_AIPathDB.m_Cells[cell].firstVertex + index].id].pt.y, \
           g_AIPathDB.m_Vertices[g_AIPathDB.m_CellVertices[g_AIPathDB.m_Cells[cell].firstVertex + index].id].pt.z)

inline void DumpVertices(tAIPathCellID cell, const char * pszPrefix,
                         const char * pszPostfix = "\n")
{
   for (int i = 0; i < g_AIPathDB.m_Cells[cell].vertexCount; i++)
   {
      mprint(pszPrefix);
      DumpVertex(cell, i);
      mprint(pszPostfix);
   }
}

///////////////////////////////////////

static BOOL ValidateCell(tAIPathCellID cell, BOOL warn = TRUE)
{
   const int vertexCount = g_AIPathDB.m_Cells[cell].vertexCount;
   int i, j;

   cMxsVector edgePt1, edgePt2, edgePt3;

   tAIPathVertexID vertex1;
   tAIPathVertexID vertex2;

   // Check for colinear points
   for (i = 0; i < vertexCount; i++)
   {
      g_AIPathDB.GetCellVertexFull(cell, i, &edgePt1, &vertex1);

      for (j = 0; j < vertexCount; j++)
      {
         if (i == j)
            continue;
         g_AIPathDB.GetCellVertexFull(cell, j, &edgePt2, &vertex2);

         if (!(vertex2 != vertex1 && !edgePt2.IsIdent(edgePt1)))
         {
            if (warn)
            {
               mprintf("ERROR! Cell %d contains duplicate vertices (%d;%d;%d;%d):\n", cell, i, vertex1, j, vertex2);
               DumpVertices(cell, "  ");
               CriticalMsg5("Duplicate vertices in cell %d (%d;%d;%d;%d)", cell, i, vertex1, j, vertex2);
            }
            return FALSE;
         }
      }
   }

   // Test for convexity
   mxs_vector norm;

   for (i = 0; i < vertexCount; i++)
   {
      edgePt1 = g_AIPathDB.GetCellVertex(cell, i);
      edgePt2 = g_AIPathDB.GetCellVertex(cell, i + 1);
      edgePt3 = g_AIPathDB.GetCellVertex(cell, i + 2);

      Vec2Sub(&edgePt2, &edgePt2, &edgePt1);
      Vec2Sub(&edgePt3, &edgePt3, &edgePt1);

      norm.x = edgePt2.y;
      norm.y = -edgePt2.x;

      #define kVC_ConvexEps 0.02
      if (Vec2Dotprod(&norm, &edgePt3) < -kVC_ConvexEps)
      {
         if (warn)
         {
            mprintf("ERROR! Cell %d is not convex (%d;%d):\n", cell, i, j);
            DumpVertices(cell, "  ");
            CriticalMsg3("Cell %d is not convex (%d;%d)", cell, i, j);
         }
         return FALSE;
      }
   }

   return TRUE;
}

static void ValidateAllCells()
{
   for (int i = g_AIPathDB.m_nCells; i >= 1; i--)
   {
      if (!ValidateCell(i))
         ; //g_AIPathDB.DeleteCell(i);
   }
}

///////////////////////////////////////////////////////////////////////////////

float g_MaxEdgeLength         = 14.0;
float g_MaxRatio              = 7.5 / 1.0;
int   g_NumRectSplit          = 2;
float g_MinRatioConsideration = 5.0;

///////////////////////////////////////

static BOOL g_fQualityWatch;

inline void GetEdgePoint(int vertex1, int vertex2, double ratio, mxs_vector * p)
{
   p->x = ((g_AIPathDB.m_Vertices[vertex2].pt.x - g_AIPathDB.m_Vertices[vertex1].pt.x) * ratio) + g_AIPathDB.m_Vertices[vertex1].pt.x;
   p->y = ((g_AIPathDB.m_Vertices[vertex2].pt.y - g_AIPathDB.m_Vertices[vertex1].pt.y) * ratio) + g_AIPathDB.m_Vertices[vertex1].pt.y;
   p->z = ((g_AIPathDB.m_Vertices[vertex2].pt.z - g_AIPathDB.m_Vertices[vertex1].pt.z) * ratio) + g_AIPathDB.m_Vertices[vertex1].pt.z;
}

// Brutal cell splitter...

static BOOL __fastcall AddSplitCell(tAIPathCellID origCell, sAIPathCell2VertexLink * pVertLinks, int nVerts)
{
   if (g_fQualityWatch)
      ValidateCell(origCell);

   if (g_AIPathDB.m_nCells + 1 >= MAX_CELLS)
   {
      mprintf("ERROR: Too many path cells in pathfinding database!\n");
      return FALSE;
   }

   if (g_AIPathDB.m_nCellVertices + nVerts >= MAX_CELL2VERTEX_LINKS)
   {
      mprintf("ERROR: too many cell->vertex links in pathfinding database!\n");
      return FALSE;
   }

   g_AIPathDB.m_nCells++;

   g_AIPathDB.m_Cells[g_AIPathDB.m_nCells].firstVertex   = (tAIPathVertexIDPacked) (g_AIPathDB.m_nCellVertices+1);
   g_AIPathDB.m_Cells[g_AIPathDB.m_nCells].vertexCount   = (uchar) nVerts;
   g_AIPathDB.m_Cells[g_AIPathDB.m_nCells].plane         = g_AIPathDB.m_Cells[origCell].plane;
   g_AIPathDB.m_Cells[g_AIPathDB.m_nCells].pathFlags     = 0;
   g_AIPathDB.m_Cells[g_AIPathDB.m_nCells].wrapFlags     = 0;

   g_AIPathDB.m_nCellVertices += nVerts;

   memcpy(&(g_AIPathDB.m_CellVertices[g_AIPathDB.m_Cells[g_AIPathDB.m_nCells].firstVertex]),
          pVertLinks,
          nVerts * sizeof(sAIPathCell2VertexLink));

   if (g_fQualityWatch)
      ValidateCell(g_AIPathDB.m_nCells);

   return TRUE;
}

typedef enum {
   kQualitySplit,
   kQualityLeave,
   kQualityAbort
} eQualityResult;

///////////////////////////////////////

static int g_Splits;
static int g_Discards;

BOOL cAIPathDB::SplitCell(tAIPathCellID cell, const mxs_vector & pt1, const mxs_vector & pt2)
{
   tAIPathVertexID        idPt1;
   tAIPathVertexID        idPt2;
   sAIPathCell            newCell;
   int                    iAdding = 0;
   sAIPathCell2VertexLink newVertLink;
   int                    i;

   #define AIPDSC_Same(v1, i) Same(v1, GetCellVertex(cell, (i)), 0.001)

   for (i = 0; i < m_Cells[cell].vertexCount; i++)
   {
      if ((AIPDSC_Same(pt1, i) && AIPDSC_Same(pt2, i + 1)) ||
          (AIPDSC_Same(pt2, i) && AIPDSC_Same(pt1, i + 1)))
      {
          g_Discards++;
          return FALSE;
      }
   }

   if (!AICreateOrFindPathVertex(pt1, &idPt1) ||
       !AICreateOrFindPathVertex(pt2, &idPt2))
      return FALSE;

   cAIPathArray<sAIPathCell2VertexLink> old;
   cAIPathArray<sAIPathCell2VertexLink> news[2];
   cDynArray<int> SplitsPerSide(m_Cells[cell].vertexCount);

   memset(SplitsPerSide, 0, sizeof(int) * SplitsPerSide.Size());

   #define kAIPDSC_Eps 0.0005
   BOOL SplitPt1 = FALSE;
   BOOL SplitPt2 = FALSE;

   old.AppendMemCpy(&(m_CellVertices[m_Cells[cell].firstVertex]), m_Cells[cell].vertexCount);

   for (i = 0; i < old.Size(); i++)
   {
      int j = (i + 1) % old.Size();
      if (old[i].id == idPt1 || old[i].id == idPt2)
      {
         if (old[i].id == idPt1)
         {
            if (SplitPt1)
            {
              g_Discards++;
               return FALSE;
            }
            SplitPt1 = TRUE;
         }
         else
         {
            if (SplitPt2)
            {
               g_Discards++;
               return FALSE;
            }
            SplitPt2 = TRUE;
         }

         // Vertex split
         news[iAdding].Append(old[i]);
         iAdding = !iAdding;
         news[iAdding].Append(old[i]);

         SplitsPerSide[i]++;
         SplitsPerSide[(i + (SplitsPerSide.Size() - 1)) % SplitsPerSide.Size()]++;
      }
      else if (DistSqXYPtSeg(pt1, m_Vertices[old[i].id].pt, m_Vertices[old[j].id].pt) < sq(kAIPDSC_Eps))
      {
         if (SplitPt1)
         {
            g_Discards++;
            return FALSE;
         }
         SplitPt1 = TRUE;

         // Edge split
         newVertLink.id = idPt1;
         news[iAdding].Append(old[i]);
         news[iAdding].Append(newVertLink);
         iAdding = !iAdding;
         news[iAdding].Append(newVertLink);

         SplitsPerSide[i]++;
      }
      else if (DistSqXYPtSeg(pt2, m_Vertices[old[i].id].pt, m_Vertices[old[j].id].pt) < sq(kAIPDSC_Eps))
      {
         if (SplitPt2)
         {
            g_Discards++;
            return FALSE;
         }

         SplitPt2 = TRUE;

         // Edge split
         newVertLink.id = idPt2;
         news[iAdding].Append(old[i]);
         news[iAdding].Append(newVertLink);
         iAdding = !iAdding;
         news[iAdding].Append(newVertLink);

         SplitsPerSide[i]++;
      }
      else
      {
         // No split
         news[iAdding].Append(old[i]);
      }
   }

   for (i = 0; i < SplitsPerSide.Size(); i++)
   {
      if (SplitsPerSide[i] > 1)
      {
         g_Discards++;
         return FALSE;
      }
   }

   // Now, insert the new cells
   int nNewVerts = (news[0].Size() + news[1].Size()) - old.Size();
   if (!nNewVerts)
   {
      g_Discards++;
      return FALSE;
   }

   if (m_nCells + 1 >= MAX_CELLS)
   {
      mprintf("ERROR: Too many path cells in pathfinding database!\n");
      return FALSE;
   }

   if (m_nCellVertices + nNewVerts >= MAX_CELL2VERTEX_LINKS)
   {
      mprintf("ERROR: too many cell->vertex links in pathfinding database!\n");
      return FALSE;
   }

   m_CellVertices.Grow(nNewVerts);

   memmove(&(m_CellVertices[m_Cells[cell].firstVertex + m_Cells[cell].vertexCount + nNewVerts]),
           &(m_CellVertices[m_Cells[cell].firstVertex + m_Cells[cell].vertexCount]),
           ((m_nCellVertices - (m_Cells[cell].firstVertex + m_Cells[cell].vertexCount)) + 1) * sizeof(sAIPathCell2VertexLink));

   m_nCellVertices += nNewVerts;

   m_Cells[cell].vertexCount = news[0].Size();
   memcpy(&(m_CellVertices[m_Cells[cell].firstVertex]), news[0], news[0].Size() * sizeof(sAIPathCell2VertexLink));

   newCell.firstVertex = m_Cells[cell].firstVertex + m_Cells[cell].vertexCount;
   newCell.vertexCount = news[1].Size();
   newCell.plane       = m_Cells[cell].plane;
   newCell.pathFlags   = 0;

   memcpy(&(m_CellVertices[newCell.firstVertex]), news[1], news[1].Size() * sizeof(sAIPathCell2VertexLink));

   m_Cells.InsertAtIndex(newCell, cell + 1);
   m_nCells++;

   // Finally, fixup the cell-vertex links for all higher cells
   for (i = cell + 2; i <= m_nCells; i++)
      m_Cells[i].firstVertex += nNewVerts;

   g_Splits++;
   return TRUE;
}

///////////////////////////////////////

void cAIPathDB::DeleteCell(tAIPathCellID cell)
{
   AssertMsg(m_nLinks == 0, "Cannot edit pathfinding database after cell-to-cell links calculated");

   // Delete vertex array
   memmove(&(m_CellVertices[m_Cells[cell].firstVertex]),
           &(m_CellVertices[m_Cells[cell].firstVertex + m_Cells[cell].vertexCount]),
           ((m_nCellVertices - (m_Cells[cell].firstVertex + m_Cells[cell].vertexCount)) + 1) * sizeof(sAIPathCell2VertexLink));

   m_CellVertices.SetSize(m_CellVertices.Size() - m_Cells[cell].vertexCount);
   m_nCellVertices -= m_Cells[cell].vertexCount;

   for (int i = cell + 1; i <= m_nCells; i++)
      m_Cells[i].firstVertex -= m_Cells[cell].vertexCount;

   // Delete cell
   m_Cells.DeleteItem(cell);
   m_nCells--;
}

///////////////////////////////////////

void cAIPathDB::GetCell(tAIPathCellID cell, sAIExternCell ** ppCell)
{
   AssertMsg(m_nLinks == 0, "Cannot edit pathfinding database after cell-to-cell links calculated");

   *ppCell = AINewExternCell(m_Planes[m_Cells[cell].plane].normal,
                             m_Planes[m_Cells[cell].plane].constant,
                             m_Cells[cell].vertexCount);

   for (int i = 0; i < m_Cells[cell].vertexCount; i++)
   {
      (*ppCell)->vertices[i] = GetCellVertex(cell, i);
   }

}

///////////////////////////////////////

BOOL cAIPathDB::AddOBBCell(sAIExternCell * pCell, tAIPathCellID cell, BOOL movable)
{
   AssertMsg(m_nLinks == 0, "Cannot edit pathfinding database after cell-to-cell links calculated");

   if (m_nCells + 1 >= MAX_CELLS)
   {
      mprintf("ERROR: Too many path cells in pathfinding database!\n");
      return FALSE;
   }

   if (m_nCellVertices + pCell->nVertices >= MAX_CELL2VERTEX_LINKS)
   {
      mprintf("ERROR: too many cell->vertex links in pathfinding database!\n");
      return FALSE;
   }

   int             i;
   sAIPathCell     newCell;
   tAIPathVertexID idPt;

   if (cell)
   {
      m_CellVertices.Grow(pCell->nVertices);

      memmove(&(m_CellVertices[m_Cells[cell].firstVertex + pCell->nVertices]),
              &(m_CellVertices[m_Cells[cell].firstVertex]),
              ((m_nCellVertices - (m_Cells[cell].firstVertex + m_Cells[cell].vertexCount)) + 1) * sizeof(sAIPathCell2VertexLink));

      m_nCellVertices += pCell->nVertices;

      newCell.firstVertex = m_Cells[cell].firstVertex;
      newCell.vertexCount = pCell->nVertices;
      newCell.plane;
      newCell.pathFlags   = 0;

      if (movable)
         newCell.plane = AICreatePlane(pCell->plane.normal, pCell->plane.constant);
      else
         newCell.plane = AICreateOrFindPlane(pCell->plane.normal, pCell->plane.constant);

      for (i = cell; i <= m_nCells; i++)
         m_Cells[i].firstVertex += pCell->nVertices;

      for (i = 0; i < pCell->nVertices; i++)
      {
         if (!AICreateOrFindPathVertex(pCell->vertices[i], &idPt))
            return FALSE;
         m_CellVertices[newCell.firstVertex + i].id = (tAIPathVertexIDPacked) idPt;
      }

      m_Cells.InsertAtIndex(newCell, cell);
      m_nCells++;
   }
   else
   {
      m_nCells++;
      m_Cells[m_nCells].firstVertex = (tAIPathVertexIDPacked) (m_nCellVertices + 1);
      m_Cells[m_nCells].vertexCount = pCell->nVertices;
      m_Cells[m_nCells].plane;
      m_Cells[m_nCells].pathFlags = 0;

      if (movable)
         m_Cells[m_nCells].plane = AICreatePlane(pCell->plane.normal, pCell->plane.constant);
      else
         m_Cells[m_nCells].plane = AICreateOrFindPlane(pCell->plane.normal, pCell->plane.constant);

      for (i = 0; i < pCell->nVertices; i++)
      {
         if (!AICreateOrFindPathVertex(pCell->vertices[i], &idPt))
            return FALSE;

         m_nCellVertices++;
         m_CellVertices[m_nCellVertices].id = (tAIPathVertexIDPacked) idPt;
      }
   }
   return TRUE;
}

///////////////////////////////////////

int cAIPathDB::GetLineIntersections(tAIPathCellID cell, const mxs_vector & line1, const mxs_vector & line2, sAICellLineIntersect * pIntersect) const
{
   const int          nVertices = m_Cells[cell].vertexCount;
   const mxs_vector * pV1;
   const mxs_vector * pV2;
   mxs_vector         curIntersect;
   int                type;
   int                i;

   pIntersect->n = 0;

   for (i = 0; pIntersect->n < 2 && i < nVertices; i++)
   {
      pV1 = &GetCellVertex(cell, i);
      pV2 = &GetCellVertex(cell, i + 1);

      type = GetXYLineSegIntersect(line1, line2, *pV1, *pV2, &curIntersect);

      if (type == kLineIdent)
      {
         pIntersect->n = 0;
         break;
      }

      if (type == kLineIntersect)
      {
         pIntersect->intersects[pIntersect->n] = curIntersect;
         pIntersect->intersects[pIntersect->n].z = GetZAtXY(cell, curIntersect);
         pIntersect->n++;
         if (AIXYDistanceSq(curIntersect, *pV2) < kAIEpsilon)
            i++;
      }
   }

   // Check for perfect edge match as resulting from winding
   if (pIntersect->n == 2)
   {
      for (i = 0; i < nVertices; i++)
      {
         pV1 = &GetCellVertex(cell, i);
         pV2 = &GetCellVertex(cell, i + 1);
         type = GetXYIntersection(line1, line2, *pV1, *pV2, &curIntersect);

         if (type == kLineIdent)
         {
            pIntersect->n = 0;
            break;
         }
      }
   }

   return pIntersect->n;
}

///////////////////////////////////////

BOOL cAIPathDB::SplitCellByLine(tAIPathCellID cell, const mxs_vector & line1, const mxs_vector & line2)
{
   sAICellLineIntersect intersection;

   if (GetLineIntersections(cell, line1, line2, &intersection) == 2)
   {
      return SplitCell(cell, intersection.intersects[0], intersection.intersects[1]);
   }
   return FALSE;
}

///////////////////////////////////////

static eQualityResult __fastcall QualitySplit(tAIPathCellID cell, int iVert1, float lenSplitEdge)
{
   const int kMaxSplit = 8;
   sAIPathCell2VertexLink cellVertLinks[kMaxSplit];

   int i;
   const int vertexCount = g_AIPathDB.m_Cells[cell].vertexCount;

   if (vertexCount > kMaxSplit)
      return kQualityLeave;

   if (g_fQualityWatch)
   {
      ValidateCell(cell);
      mprintf("  Original vertices:\n");
      DumpVertices(cell, "    ");
   }

   // Make the target edge the first one
   for (i = 0; i < vertexCount; i++)
      cellVertLinks[i] = g_AIPathDB.m_CellVertices[g_AIPathDB.m_Cells[cell].firstVertex + ((iVert1 + i) % vertexCount)];

   memcpy(&(g_AIPathDB.m_CellVertices[g_AIPathDB.m_Cells[cell].firstVertex]),
          cellVertLinks,
          vertexCount * sizeof(sAIPathCell2VertexLink));

   if (g_fQualityWatch)
   {
      ValidateCell(cell);
      mprintf("  Reordered vertices:\n");
      DumpVertices(cell, "    ");
   }

   // Now split
   switch (vertexCount)
   {
      case 3:
      {
         tAIPathVertexID idPt;
         cMxsVector pt;

         GetEdgePoint(g_AIPathDB.m_CellVertices[g_AIPathDB.m_Cells[cell].firstVertex].id,
                      g_AIPathDB.m_CellVertices[g_AIPathDB.m_Cells[cell].firstVertex + 1].id,
                      0.5,
                      &pt);

         if (!AICreateOrFindPathVertex(pt, &idPt))
            return kQualityAbort;

         g_AIPathDB.m_CellVertices[g_AIPathDB.m_Cells[cell].firstVertex + 1].id = idPt;
         cellVertLinks[0].id = (tAIPathVertexIDPacked) idPt;

         if (!AddSplitCell(cell, cellVertLinks, 3))
            return kQualityAbort;

         if (g_fQualityWatch)
         {
            mprintf("    New cell %d:\n", cell);
            DumpVertices(cell, "      ");
            mprintf("    New cell %d:\n", g_AIPathDB.m_nCells);
            DumpVertices(g_AIPathDB.m_nCells, "    ");
         }
         break;
      }

      case 4:
      {
         i = (lenSplitEdge < g_MaxEdgeLength) ? 1 : g_NumRectSplit - 1;
         for (; i > 0; i--)
         {
            tAIPathVertexID idPt1, idPt2;

            double ratio = (double)i / (double)g_NumRectSplit;
            cMxsVector pt1, pt2;

            GetEdgePoint(g_AIPathDB.m_CellVertices[g_AIPathDB.m_Cells[cell].firstVertex + 0].id,
                         g_AIPathDB.m_CellVertices[g_AIPathDB.m_Cells[cell].firstVertex + 1].id,
                         ratio,
                         &pt1);
            GetEdgePoint(g_AIPathDB.m_CellVertices[g_AIPathDB.m_Cells[cell].firstVertex + 2].id,
                         g_AIPathDB.m_CellVertices[g_AIPathDB.m_Cells[cell].firstVertex + 3].id,
                         1.0 - ratio,
                         &pt2);

            if (!AICreateOrFindPathVertex(pt1, &idPt1) ||
                !AICreateOrFindPathVertex(pt2, &idPt2))
               return kQualityAbort;

            g_AIPathDB.m_CellVertices[g_AIPathDB.m_Cells[cell].firstVertex + 1].id = idPt1;
            g_AIPathDB.m_CellVertices[g_AIPathDB.m_Cells[cell].firstVertex + 2].id = idPt2;
            cellVertLinks[0].id  = idPt1;
            cellVertLinks[3].id  = idPt2;

            if (!AddSplitCell(cell, cellVertLinks, 4))
               return kQualityAbort;

            if (g_fQualityWatch)
            {
               mprintf("    New cell %d:\n", g_AIPathDB.m_nCells);
               DumpVertices(g_AIPathDB.m_nCells, "    ");
            }

            memcpy(cellVertLinks,
                   &(g_AIPathDB.m_CellVertices[g_AIPathDB.m_Cells[cell].firstVertex]),
                   vertexCount * sizeof(sAIPathCell2VertexLink));
         }

         if (g_fQualityWatch)
         {
            mprintf("    New cell %d:\n", cell);
            DumpVertices(cell, "      ");
         }

         break;
      }

      case 5:
      {
         tAIPathVertexID idPt;
         cMxsVector pt;

         GetEdgePoint(g_AIPathDB.m_CellVertices[g_AIPathDB.m_Cells[cell].firstVertex].id,
                      g_AIPathDB.m_CellVertices[g_AIPathDB.m_Cells[cell].firstVertex + 1].id,
                      0.5,
                      &pt);

         if (!AICreateOrFindPathVertex(pt, &idPt))
            return kQualityAbort;

         g_AIPathDB.m_CellVertices[g_AIPathDB.m_Cells[cell].firstVertex].id = idPt;
         g_AIPathDB.m_Cells[cell].vertexCount -= 1;

         // @TBD (toml 01-29-98): at some time, should to recover all the discarded cell to vertex links
         // which is more complicated than this because of higher cells...
         if (cell == g_AIPathDB.m_nCells)
            g_AIPathDB.m_nCellVertices -= 1;

         cellVertLinks[1].id = idPt;
         cellVertLinks[2].id = cellVertLinks[3].id;
         cellVertLinks[3].id = cellVertLinks[4].id;

         if (!AddSplitCell(cell, cellVertLinks, 4))
            return kQualityAbort;

         if (g_fQualityWatch)
         {
            mprintf("    New cell %d:\n", cell);
            DumpVertices(cell, "      ");
            mprintf("    New cell %d:\n", g_AIPathDB.m_nCells);
            DumpVertices(g_AIPathDB.m_nCells, "    ");
         }

         break;
      }

      case 6:
      case 7:
      case 8:
      {
         g_AIPathDB.m_Cells[cell].vertexCount -= 2;

         // @TBD (toml 01-29-98): at some time, should to recover all the discarded cell to vertex links
         // which is more complicated than this because of higher cells...
         if (cell == g_AIPathDB.m_nCells)
            g_AIPathDB.m_nCellVertices -= 2;

         cellVertLinks[1].id = cellVertLinks[vertexCount - 3].id;
         cellVertLinks[2].id = cellVertLinks[vertexCount - 2].id;
         cellVertLinks[3].id = cellVertLinks[vertexCount - 1].id;

         if (!AddSplitCell(cell, cellVertLinks, 4))
            return kQualityAbort;

         if (g_fQualityWatch)
         {
            mprintf("    New cell %d:\n", cell);
            DumpVertices(cell, "      ");
            mprintf("    New cell %d:\n", g_AIPathDB.m_nCells);
            DumpVertices(g_AIPathDB.m_nCells, "    ");
         }

         break;
      }

   }

   return kQualitySplit;
}

///////////////////////////////////////

static int IntComp(const void * pLeft, const void * pRight)
{
   return *((int *)pLeft) - *((int *)pRight);
}

#define kMaxColinears 32

static BOOL Eliminated(int i, int colinears[kMaxColinears], int nColinears)
{
   for (int j = 0; j < nColinears; j++)
   {
      if (colinears[j] == i)
         return TRUE;
   }
   return FALSE;
}

static BOOL __fastcall DoRemoveColinearPoints(tAIPathCellID cell)
{
   // Eliminate colinear points
   const int vertexCount = g_AIPathDB.m_Cells[cell].vertexCount;
   int i;
   int anchor;
   int anchor2;
   int colinears[kMaxColinears];
   int nColinears = 0;

   cMxsVector edgePt1, edgePt2, edgePt3;

   tAIPathVertexID vertex1;
   tAIPathVertexID vertex2;
   tAIPathVertexID vertex3;

   if (g_fQualityWatch)
   {
      ValidateCell(cell);
      mprintf("Removing colinear points for %d", cell);
      mprintf("  Original cell vertices:\n");

      DumpVertices(cell, "    ");
   }

   // For each vertex
   for (anchor = 0; anchor < vertexCount; anchor++)
   {
      // If it has not already been eliminated, check subsequent verticies for colinearity
      if (Eliminated(anchor, colinears, nColinears))
         continue;

      for (i = 1; i < vertexCount; i++)
      {
         if (Eliminated((anchor + i) % vertexCount, colinears, nColinears))
            continue;

         anchor2 = anchor;
         for (int j = i + 1; j < vertexCount; j++)
         {
            if (Eliminated((anchor + j) % vertexCount, colinears, nColinears))
               continue;
            anchor2 = anchor + j;
            break;
         }

         if (anchor2 == anchor)
            continue;

         g_AIPathDB.GetCellVertexFull(cell, anchor,         &edgePt1, &vertex1);
         g_AIPathDB.GetCellVertexFull(cell, anchor + i,     &edgePt2, &vertex2);
         g_AIPathDB.GetCellVertexFull(cell, anchor2, &edgePt3, &vertex3);

         if (pfle(Vec2PointSegmentSquared(edgePt2.AsVec2Ptr(),
                                          edgePt1.AsVec2Ptr(),
                                          edgePt3.AsVec2Ptr()), 0.00001))
         {
            colinears[nColinears] = (anchor + i) % vertexCount;
            nColinears++;

            if (g_fQualityWatch)
            {
               mprintf("  %d is on the line between %d and %d (%g)\n",
                       (anchor + i) % vertexCount,
                       anchor,
                       anchor2 % vertexCount,
                       Vec2PointSegmentSquared(edgePt2.AsVec2Ptr(),
                                               edgePt1.AsVec2Ptr(),
                                               edgePt3.AsVec2Ptr()));
            }
         }
         else
            break;

         if (nColinears == kMaxColinears)
            break;
      }

      if (nColinears == kMaxColinears)
         break;
   }

   if (vertexCount - nColinears < 3)
      return FALSE; // We have, given our tolerance, a line

   // The logic below relies on the colinear vertex indices being sorted
   qsort(colinears, nColinears, sizeof(int), IntComp);

   sAIPathCell2VertexLink * pTo;
   sAIPathCell2VertexLink * pFrom;
   unsigned                 num;

   for (i = nColinears - 1; i >= 0 ; i--)
   {
      pTo = &(g_AIPathDB.m_CellVertices[g_AIPathDB.m_Cells[cell].firstVertex + colinears[i]]);
      pFrom = pTo + 1;
      num = g_AIPathDB.m_Cells[cell].vertexCount - (colinears[i] + 1);

      // @Note (toml 01-13-98): although it appears we're losing vertex references here, its the case that
      // usually embedded verticies are shared with another, simpler cell
      memmove(pTo, pFrom, num * sizeof(sAIPathCell2VertexLink));
      g_AIPathDB.m_Cells[cell].vertexCount--;
      g_AIPathDB.m_nCellVertices--;
   }

   if (g_fQualityWatch)
   {
      ValidateCell(cell);
      if (nColinears)
      {
         mprintf("  New cell vertices:\n");

         DumpVertices(cell, "    ");
      }
   }
   return TRUE;
}

///////////////////////////////////////

static eQualityResult __fastcall DoEnforceSizeQuality(tAIPathCellID cell)
{
   const int vertexCount = g_AIPathDB.m_Cells[cell].vertexCount;

   int i;

   float len;
   float lenShortest = kFloatMax;
   float lenLongest  = -1.0;

   cMxsVector   edgePt1, edgePt2;

   int iShortest;
   int iLongest;

   tAIPathVertexID vertex1;
   tAIPathVertexID vertex2;

   for (i = 0; i < vertexCount; i++)
   {
      g_AIPathDB.GetCellVertexFull(cell, i,     &edgePt1, &vertex1);
      g_AIPathDB.GetCellVertexFull(cell, i + 1, &edgePt2, &vertex2);

      len = mx_dist_vec(&edgePt1, &edgePt2);

      if (len < lenShortest)
      {
         iShortest = i;
         lenShortest = len;
      }

      if (len > lenLongest)
      {
         iLongest = i;
         lenLongest = len;
      }
   }

   if (lenLongest > g_MaxEdgeLength ||
       (lenLongest > g_MinRatioConsideration && sq(lenLongest) / g_AIPathDB.ComputeArea(cell) > g_MaxRatio))
   {
      if (g_fQualityWatch)
      {
         mprintf("Splitting cell %d because %s\n", cell,
                 (lenLongest > g_MaxEdgeLength) ?
                     "an edge is too long" :
                     "of the ratio of longest to shortest");
      }
      return QualitySplit(cell, iLongest, lenLongest);
   }
   return kQualityLeave;
}

///////////////////////////////////////

BOOL __fastcall EnforceDatabaseQuality(tAIPathCellID cell) // Return TRUE, or FALSE to ABORT the process
{
   int prevCellCount;
   int i;

   g_fQualityWatch = (cell == g_AIQualityWatch);

   do
   {
      prevCellCount = g_AIPathDB.m_nCells;
      for (i = cell; i <= prevCellCount; i++)
      {
         if (!DoRemoveColinearPoints(cell))
            return TRUE;
      }
   } while (prevCellCount != g_AIPathDB.m_nCells);

   do
   {
      prevCellCount = g_AIPathDB.m_nCells;
      for (i = cell; i <= prevCellCount; i++)
      {
         if (g_fQualityWatch)
            ValidateCell(i);
         if (DoEnforceSizeQuality(i) == kQualityAbort)
            return FALSE;
         if (g_fQualityWatch)
            ValidateCell(i);
      }
   } while (prevCellCount != g_AIPathDB.m_nCells);

   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////

static void SegSpltFindCloseCellsByCenter(const mxs_vector & location, float distSq, cDynArray<tAIPathCellID> * pResult)
{
   for (tAIPathCellID cell = 1; cell <= g_AIPathDB.NumCells(); cell++)
   {
      if (!g_AIPathDB.GetMark(cell))
      {
         cMxsVector center;
         ComputeCellCenter(cell, &center);       // compute not get because database is only partially built
         float cellDistSq = AIDistanceSq(location, center);
         if (aflt(cellDistSq, distSq))
         {
            pResult->Append(cell);
            g_AIPathDB.Mark(cell);
         }
      }
   }
}

///////////////////////////////////////

static void SegSpltFindCloseCellsByVertices(const mxs_vector & location, float distSq, cDynArray<tAIPathCellID> * pResult)
{
   float vertexDistSq;
   for (tAIPathCellID cell = 1; cell <= g_AIPathDB.NumCells(); cell++)
   {
      if (!g_AIPathDB.GetMark(cell))
      {
         const int lastVertex = g_AIPathDB.m_Cells[cell].firstVertex + g_AIPathDB.m_Cells[cell].vertexCount;

         for (int i = g_AIPathDB.m_Cells[cell].firstVertex; i < lastVertex; i++)
         {
            vertexDistSq = AIDistanceSq(location, (g_AIPathDB.m_Vertices[g_AIPathDB.m_CellVertices[i].id].pt));
            if (aflt(vertexDistSq, distSq))
            {
               pResult->Append(cell);
               g_AIPathDB.Mark(cell);
               break;
            }
         }
      }
   }
}

///////////////////////////////////////

static void SegSpltFindCloseCells(const mxs_vector & start, const mxs_vector & end, float distSq, cDynArray<tAIPathCellID> * pResult)
{
   g_AIPathDB.MarkBegin();

   // Find the points close to the ends
   SegSpltFindCloseCellsByCenter(start, distSq, pResult);
   SegSpltFindCloseCellsByVertices(start, distSq, pResult);
   SegSpltFindCloseCellsByCenter(end, distSq, pResult);
   SegSpltFindCloseCellsByVertices(end, distSq, pResult);

   // And close to the center of the line
   mxs_vector center;
   center.x = (start.x + end.x) / 2;
   center.y = (start.y + end.y) / 2;
   center.z = (start.z + end.z) / 2;
   float centerDistSq = AIDistanceSq(center, end);
   centerDistSq = max(distSq, centerDistSq);

   SegSpltFindCloseCellsByCenter(center, centerDistSq, pResult);
   SegSpltFindCloseCellsByVertices(center, centerDistSq, pResult);

   g_AIPathDB.MarkEnd();
}

//////////////////////////////////////

#define kSegSpltDist 7.0

static void AILineSplit(const mxs_vector & lineA, const mxs_vector & lineB, const cDynArray<tAIPathCellID> & cells)
{
   // Input array is assumed to have been sorted
   unsigned nAdded = 0;
   for (int i = 0; i < cells.Size(); i++)
   {
      if (g_AIPathDB.SplitCellByLine(cells[i] + nAdded, lineA, lineB))
      {
         ValidateCell(cells[i] + nAdded);
         ValidateCell(g_AIPathDB.m_nCells);
         nAdded++;
      }
   }
}

//////////////////////////////////////

static void AISegmentSplit(const mxs_vector & start, const mxs_vector & end,
                           float /*distSq*/, const cDynArray<tAIPathCellID> & cells)
{
   // Generate list of candidiates
   cDynArray<tAIPathCellID> candidates;
   const mxs_vector *       pV1;
   const mxs_vector *       pV2;
   mxs_vector               curIntersect;
   int                      i;

   for (i = 0; i < cells.Size(); i++)
   {
      const tAIPathCellID cell      = cells[i];
      const int           nVertices = g_AIPathDB.m_Cells[cell].vertexCount;

      if (IsPtInCellXYPlane(start, &g_AIPathDB.m_Cells[cell]) || IsPtInCellXYPlane(end, &g_AIPathDB.m_Cells[cell]))
      {
         candidates.Append(cell);
         continue;
      }

      for (int j = 0; j < nVertices; j++)
      {
         pV1 = &g_AIPathDB.GetCellVertex(cell, j);
         pV2 = &g_AIPathDB.GetCellVertex(cell, j + 1);

         if (DistSqXYPtSeg(*pV1, start, end) < kAIEpsilon ||
             GetXYSegmentIntersect(start, end, *pV1, *pV2, &curIntersect) == kLineIntersect)
         {
            // Intersect!
            candidates.Append(cell);
            break;
         }
      }
   }

   // Eliminate cells with lowest Z which above segment
   float minZ = min(start.z, end.z) - kSegSpltDist;
   float maxZ = max(start.z, end.z);

   for (i = candidates.Size() - 1; i >= 0; i--)
   {
      if (g_AIPathDB.GetLowestZ(candidates[i]) > maxZ)
         candidates.FastDeleteItem(i);
   }

   for (i = candidates.Size() - 1; i >= 0; i--)
   {
      if (g_AIPathDB.GetHighestZ(candidates[i]) < minZ)
         candidates.FastDeleteItem(i);
   }

   // Now, split the remaining cells
   candidates.Sort(CellCompareFunc);
   AILineSplit(start, end, candidates);
}

//////////////////////////////////////

#define kSegSpltClosecellDist 25
static void AISegmentSplit(const mxs_vector & start, const mxs_vector & end)
{
   cDynArray<tAIPathCellID> cells;
   SegSpltFindCloseCells(start, end, sq(kSegSpltClosecellDist), &cells);
   AISegmentSplit(start, end, sq(kSegSpltDist), cells);
}

///////////////////////////////////////////////////////////////////////////////

static BOOL InXYBox(const mxs_vector & p, const mxs_vector * box)
{
   int i;
   register const mxs_vector * p1;
   const mxs_vector * p2;

   float edge_vec_x, edge_vec_y;   // 2d vectors we dont have...
   float vert_to_point_x, vert_to_point_y;

   p2=&box[0];
   for (i = 0; i < 4; i++)
   {  // really should ping-pong these two - but im lazy
      p1 = p2;
      if (i == 3)
         p2 = &box[0];
      else
         p2 = &box[i+1];

      // get the 2d edge vector
      edge_vec_x = p2->x - p1->x;
      edge_vec_y = p2->y - p1->y;

      // get the 2d edge vertex to point data
      vert_to_point_x = p.x - p1->x;
      vert_to_point_y = p.y - p1->y;

      if (edge_vec_y*vert_to_point_x - edge_vec_x*vert_to_point_y < -kAIEpsilon)
         return FALSE;
   }
   return TRUE;
}

inline BOOL InMark(const mxs_vector & p, const sAIObjectAvoidMark & mark)
{
   return InXYBox(p, mark.vertices);
}

///////////////////////////////////////

static void AddBlockingOBBs()
{
   cDynArray<sAIObjectAvoidMark> avoidMarks;
   int                           i, j;

   AIGetAllObjMarkDB(&avoidMarks);

   unsigned nStartingCells = g_AIPathDB.NumCells();

   g_Splits = 0;
   g_Discards = 0;

   // Split the database
   for (i = 0; i < avoidMarks.Size(); i++)
   {
      PumpWindowsEvents();
      if (i % 4 == 0)
      {
         mprintf(".");
      }

      if (!ObjPosGet(avoidMarks[i].object))
         continue;

      if (g_AIPDB_MarkOnlyObj && avoidMarks[i].object != g_AIPDB_MarkOnlyObj)
         continue;

      for (j = 0; j < 4; j++)
      {
         if (mx_is_identical(&avoidMarks[i].vertices[j], &avoidMarks[i].vertices[(j + 1) % 4], 0.1))
            continue;
         AISegmentSplit(avoidMarks[i].vertices[j], avoidMarks[i].vertices[(j + 1) % 4]);
      }
   }

   mprintf("\nSplit database by %d blocking obbs. %d splits, %d discards", avoidMarks.Size(), g_Splits, g_Discards);
}

///////////////////////////////////////

#define kObbHeightInterest kSegSpltDist

static void MarkBlockingObbsUnpathable()
{
   cDynArray<sAIObjectAvoidMark> avoidMarks;
   int                           i, j;
   float                         height;

   AIGetAllObjMarkDB(&avoidMarks);

   // Mark cells on interior as unpathable
   if (avoidMarks.Size())
   {
      cMxsVector center;
      for (i = 1; i <= g_AIPathDB.m_nCells; i++)
      {
         for (j = 0; j < avoidMarks.Size(); j++)
         {
            if (!ObjPosGet(avoidMarks[j].object))
               continue;

            if (g_AIPDB_MarkOnlyObj && avoidMarks[i].object != g_AIPDB_MarkOnlyObj)
               continue;

            if (mx_is_identical(&avoidMarks[j].vertices[0], &avoidMarks[j].vertices[1], 0.1))
               continue;
            if (mx_is_identical(&avoidMarks[j].vertices[1], &avoidMarks[j].vertices[2], 0.1))
               continue;
            if (mx_is_identical(&avoidMarks[j].vertices[2], &avoidMarks[j].vertices[3], 0.1))
               continue;
            if (mx_is_identical(&avoidMarks[j].vertices[3], &avoidMarks[j].vertices[0], 0.1))
               continue;

            if (InMark(ComputeCellCenter(i, &center), avoidMarks[j]))       // compute not get because database is only partially built
            {
               height = ObjPosGet(avoidMarks[j].object)->loc.vec.z - center.z;
               if (height > -0.01 &&
                   height < kObbHeightInterest)
               {
                  g_AIPathDB.m_Cells[i].pathFlags |= kAIPF_BlockingOBB;
                  break;
               }
            }
         }
      }
   }
}

////////////////////////////////////////

static void AddPathableOBBs()
{
   sAIPathCellObjMap mapping;
   cAIObjCells obbCells;
   int i;

   AIGetAllPathableOBBCells(&obbCells);

   for (i=0; i<obbCells.Size(); i++)
   {
      g_AIPathDB.AddOBBCell(obbCells[i].pAIExternCell, 0, FALSE);
      ValidateCell(g_AIPathDB.m_nCells);

      // Build obj->cell map
      mapping.objID = obbCells[i].objID;
      mapping.cellID = g_AIPathDB.m_nCells;
      mapping.prevPropState = TRUE;

      g_AIPathDB.m_CellObjMap.Append(mapping);
      g_AIPathDB.m_nCellObjMaps++;
   }
}

///////////////////////////////////////

static void AddMovingTerrainOBBs()
{
   sAIPathCellObjMap mapping;
   cAIObjCells obbCells;
   int i;

   AIGetAllMovingTerrainCells(&obbCells);

   for (i=0; i<obbCells.Size(); i++)
   {
      g_AIPathDB.AddOBBCell(obbCells[i].pAIExternCell, 0, TRUE);
      ValidateCell(g_AIPathDB.m_nCells);

      // Build obj->cell map
      mapping.objID = obbCells[i].objID;
      mapping.cellID = g_AIPathDB.m_nCells;
      mapping.prevPropState = TRUE;
      mapping.data = obbCells[i].data;

      g_AIPathDB.m_CellObjMap.Append(mapping);
      g_AIPathDB.m_nCellObjMaps++;
   }

   obbCells.SetSize(0);
}

///////////////////////////////////////

static void FixupMovingTerrainOBBs()
{
   cDynArray<ObjID> movingTerrainObjs;
   ObjID obj;
   int i, j, k, l, m;

   cDynArray<tAIPathCellID> objCells;
   cDynArray<ObjID>         objWaypoints;

   AIGetAllMovingTerrainObjs(&movingTerrainObjs);

   for (i=0; i<movingTerrainObjs.Size(); i++)
   {
      obj = movingTerrainObjs[i];

      // Gather the cells from this object
      objCells.SetSize(0);
      objWaypoints.SetSize(0);

      for (j=0; j<g_AIPathDB.m_nCellObjMaps; j++)
      {
         if (g_AIPathDB.m_CellObjMap[j].objID == obj)
         {
            objCells.Append(g_AIPathDB.m_CellObjMap[j].cellID);
            objWaypoints.Append((ObjID)g_AIPathDB.m_CellObjMap[j].data);
         }
      }

#ifndef SHIP
      if (objCells.Size() == 0)
         mprintf("Moving terrain %s didn't generate any cells!\n", ObjWarnName(obj));
#endif

      // Preach the gospel of the One True Cell
      sAIMovableCell movableCell;

      movableCell.movingObj = obj;
      movableCell.objVec = ObjPosGet(obj)->loc.vec;
      movableCell.cellID = objCells[0];
      movableCell.waypointLinkList.SetSize(objCells.Size());

      sAIPathCell *pOTC = &g_AIPathDB.m_Cells[objCells[0]];

      // The OTC gets brand new, unshared vertices
      int prev_first_vertex = pOTC->firstVertex;
      int prev_first_cell   = pOTC->firstCell;

      pOTC->firstVertex = g_AIPathDB.m_nCellVertices + 1;
      pOTC->firstCell = g_AIPathDB.m_nLinks + 1;

      for (j=0; j<pOTC->vertexCount; j++)
         g_AIPathDB.m_CellVertices[++g_AIPathDB.m_nCellVertices].id = g_AIPathDB.m_nVertices + 1 + j;

      g_AIPathDB.m_nVertices += pOTC->vertexCount;
      g_AIPathDB.m_nLinks += pOTC->cellCount;

      // Blast the new vertices to the old vertices
      for (j=0; j<pOTC->vertexCount; j++)
      {
         g_AIPathDB.m_Vertices[g_AIPathDB.m_CellVertices[pOTC->firstVertex + j].id] =
            g_AIPathDB.m_Vertices[g_AIPathDB.m_CellVertices[prev_first_vertex + j].id];
      }

      // Because the list of links must be contiguous, new links need to be created at
      // the end of the global link list.  The old links are left dangling.
      for (j=0; j<pOTC->cellCount; j++)
         g_AIPathDB.m_Links[pOTC->firstCell + j] = g_AIPathDB.m_Links[prev_first_cell + j];

      for (j=0; j<objCells.Size(); j++)
      {
         movableCell.waypointLinkList[j].waypointObj = objWaypoints[j];

         // For each of the soon-to-be-defunct cells, find all cells that
         // have links into them, and fix up the inward/outward links
         for (k=0; k<g_AIPathDB.m_nCells; k++)
         {
            sAIPathCell *pRemapCell = &g_AIPathDB.m_Cells[objCells[j]];

            for (l=0; l<g_AIPathDB.m_Cells[k].cellCount; l++)
            {
               tAIPathCell2CellLinkID inLink = g_AIPathDB.m_Cells[k].firstCell + l;
               tAIPathCell2CellLinkID outLink = 0;

               sAIPathCellLink *pInLink = &g_AIPathDB.m_Links[inLink];

               // Does this link point at the cell that's being remapped?
               if (pInLink->dest == objCells[j])
               {
                  // Redirect inward link
                  pInLink->dest = objCells[0];

                  // Add inward link to our structure
                  sAILinkData inLinkData;

                  inLinkData.id = inLink;
                  inLinkData.bits = pInLink->okBits&kNoConditionMask; // wsf: don't store condition bits. Don't see how they can be effectively used.

                  if (j != 0)
                     pInLink->okBits = 0;

                  movableCell.waypointLinkList[j].linkList.Append(inLinkData);

                  // Find matching outward link
                  sAIPathCellLink *pOutLink = NULL;
                  for (m=0; m<pRemapCell->cellCount; m++)
                  {
                     if (g_AIPathDB.m_Links[pRemapCell->firstCell + m].dest == k)
                     {
                        outLink = pRemapCell->firstCell + m;
                        pOutLink = &g_AIPathDB.m_Links[outLink];
                        break;
                     }
                  }

                  if (pOutLink == NULL)
                     continue;

                  // Append new outward link to the One True Cell
                  g_AIPathDB.m_nLinks++;
                  pOTC->cellCount++;

                  g_AIPathDB.m_Links[g_AIPathDB.m_nLinks] = *pOutLink;

                  // Blast the ok bits of outward links
                  pOutLink->okBits = 0;

                  // And add to our structure
                  sAILinkData outLinkData;

                  outLinkData.id = g_AIPathDB.m_nLinks;
                  outLinkData.bits = g_AIPathDB.m_Links[g_AIPathDB.m_nLinks].okBits & kNoConditionMask; // no condition bits.

                  if (j != 0)
                     g_AIPathDB.m_Links[g_AIPathDB.m_nLinks].okBits = 0;

                  movableCell.waypointLinkList[j].linkList.Append(outLinkData);
               }
            }
         }

         if (j != 0)
            g_AIPathDB.m_Cells[objCells[j]].pathFlags |= kAIPF_Unpathable;
      }

      for (j=1; j<objCells.Size(); j++)
         g_AIPathDB.m_Cells[objCells[j]].cellCount = 0;

      g_AIPathDB.m_MovableCells.Append(movableCell);
   }

   movingTerrainObjs.SetSize(0);

#ifndef SHIP
   if (config_is_defined("AIMovingTerrainBuild"))
   {
      mprintf("\n");

      for (i=0; i<g_AIPathDB.m_MovableCells.Size(); i++)
      {
         mprintf("Moving Obj %d (cell %d)\n", g_AIPathDB.m_MovableCells[i].movingObj,
                 g_AIPathDB.m_MovableCells[i].cellID);

         for (j=0; j<g_AIPathDB.m_MovableCells[i].waypointLinkList.Size(); j++)
         {
            mprintf("   waypoint %d\n", g_AIPathDB.m_MovableCells[i].waypointLinkList[j].waypointObj);

            for (k=0; k<g_AIPathDB.m_MovableCells[i].waypointLinkList[j].linkList.Size(); k++)
            {
               mprintf("      link %d, to cell %d\n", g_AIPathDB.m_MovableCells[i].waypointLinkList[j].linkList[k].id,
                       g_AIPathDB.m_Links[g_AIPathDB.m_MovableCells[i].waypointLinkList[j].linkList[k].id].dest);
            }
         }
      }
   }
#endif

}

///////////////////////////////////////

static void AddDoorSplits()
{
   ObjPos *               pPos;
   cDynArray<sAIDoorBBox> doorBBoxes;

   AIGetDoorFloorBBoxes(&doorBBoxes);

   unsigned nStartingCells = g_AIPathDB.NumCells();

   g_Splits = 0;
   g_Discards = 0;

   for (int i = 0; i < doorBBoxes.Size(); i++)
   {
      PumpWindowsEvents();
      if (i % 4 == 0)
         mprintf(".");

      if (g_AIPDB_MarkOnlyObj && doorBBoxes[i].object != g_AIPDB_MarkOnlyObj)
         continue;

      if ((pPos = ObjPosGet(doorBBoxes[i].object)) == NULL || pPos->loc.cell == CELL_INVALID)
          continue;

      for (int j = 0; j < 4; j++)
      {
         AISegmentSplit(doorBBoxes[i].bbox[j], doorBBoxes[i].bbox[(j + 1) % 4]);
      }
   }
   mprintf("\nSplit database by %d doors. %d splits, %d discards", doorBBoxes.Size(), g_Splits, g_Discards);
}

///////////////////////////////////////

static void MarkCellsWithDoors()
{
   ObjPos *               pPos;
   cDynArray<sAIDoorBBox> doorBBoxes;
   int                    i, j;
   float                  height;

   AIGetDoorFloorBBoxes(&doorBBoxes);

   // Mark cells on interior as within doors
   if (doorBBoxes.Size())
   {
      cMxsVector center;
      for (i = 1; i <= g_AIPathDB.m_nCells; i++)
      {
         for (j = 0; j < doorBBoxes.Size(); j++)
         {
            if (g_AIPDB_MarkOnlyObj && doorBBoxes[i].object != g_AIPDB_MarkOnlyObj)
               continue;

            if ((pPos = ObjPosGet(doorBBoxes[j].object)) == NULL || pPos->loc.cell == CELL_INVALID)
                continue;

            if (InXYBox(ComputeCellCenter(i, &center), doorBBoxes[j].bbox))       // compute not get because database is only partially built
            {
               height = ObjPosGet(doorBBoxes[j].object)->loc.vec.z - center.z;
               if (height > -0.01 &&
                   height < kObbHeightInterest)
               {
                  g_AIPathDB.m_Cells[i].pathFlags |= kAIPF_BelowDoor;

                  sAICellDoor * pCellDoor = &(g_AIPathDB.m_CellDoors[g_AIPathDB.m_CellDoors.Grow()]);
                  pCellDoor->cell = i;
                  pCellDoor->door = doorBBoxes[j].object;
                  break;
               }
            }
         }
      }
      for (i = 0; i < g_AIPathDB.m_CellDoors.Size(); i++)
         g_AIPathDB.m_CellToDoorsTable.Insert(&g_AIPathDB.m_CellDoors[i]);
   }
}

///////////////////////////////////////////////////////////////////////////////

unsigned AIGetZoneSize(eAIPathZoneType ZoneType)
{
   return sizeof(tAIPathZone) * g_AIPathDB.m_ZoneDatabases[ZoneType].m_CellZones.Size() +
          sizeof(sZoneOkBitsMap) * g_AIPathDB.m_ZoneDatabases[ZoneType].m_ZonePairTable.GetCount();
}


unsigned AIGetPathDBSize()
{
   return (sizeof(sAIPathCell) * (g_AIPathDB.m_nCells + 1) +
           sizeof(sAIPathCellPlane) * (g_AIPathDB.m_nPlanes + 1) +
           sizeof(sAIPathVertex) * (g_AIPathDB.m_nVertices + 1) +
           sizeof(sAIPathCellLink) * (g_AIPathDB.m_nLinks + 1) +
           sizeof(sAIPathCell2VertexLink) * (g_AIPathDB.m_nCellVertices + 1) +
           AIGetZoneSize(kAIZone_Normal)+
           AIGetZoneSize(kAIZone_NormalLVL)+
           AIGetZoneSize(kAIZone_HighStrike)+
           AIGetZoneSize(kAIZone_HighStrikeLVL)
          );
}

///////////////////////////////////////////////////////////////////////////////

//
// PostProcessNarrow is not working. I don't have time to figure out why. See
// R:\prj\thief2\levels\broken\ai_bad_post_process_narrow.mis near -33,-41,3
//
// HOWEVER: Even if what ever is going on was fixed it's STILL broken because
// a) it uses the pathcaster before link space compression and the pathcaster
// doesn't understand the temporarily enlarged pathspace, b) is uses the "zero
// ok bits" feature that used to allow one to use the pathcaster to find
// the edge of the database, but for the purposes of these functions they're 
// now twarted by highstrikes and lvls, c) we can't do it AFTER, because the
// dimensions are screwed by link removal.
//
// STILL: Much of what this code does is look for spaces not covered by cigar
// casts, but most of these are now covered by the AI including OBBs in 
// cigar casts (new as of end 2/00). It won't catch narrow walkways and the
// like, as this code used to.
//
// For now, gotta take it out. (toml 03-08-00)
//

#define kNarrowDist (g_pAICreatureSizes->sizes[0].width * 1.1)
#define kNarrowSamples 16

BOOL TestPathWidthCenter(tAIPathCellID cell, const mxs_vector & testPt, mxs_vector testVec)
{
   mxs_vector testPt2;
   float      scale = kNarrowDist * 0.5;
   int        opens = 0;

   // Test one direction
   mx_scale_vec(&testPt2, &testVec, scale);
   mx_addeq_vec(&testPt2, &testPt);
   if (!AIPathcast(testPt, cell, testPt2, 0, ((IAIPathfindControl *) NULL))) // looking only for edge of database or blocking obbs
      opens++;

   // And the other
   mx_negeq_vec(&testVec);
   mx_scale_vec(&testPt2, &testVec, scale);
   mx_addeq_vec(&testPt2, &testPt);
   if (!AIPathcast(testPt, cell, testPt2, 0, ((IAIPathfindControl *) NULL))) // looking only for edge of database or blocking obbs
      opens++;

   return (opens == 2);
}

BOOL TestPathWidthRight(tAIPathCellID cell, const mxs_vector & testPt, mxs_vector testVec)
{
   mxs_vector testPt2;

   mx_scale_vec(&testPt2, &testVec, kNarrowDist);
   mx_addeq_vec(&testPt2, &testPt);
   return AIPathcast(testPt, cell, testPt2, 0, ((IAIPathfindControl *) NULL));
}

BOOL TestPathWidthLeft(tAIPathCellID cell, const mxs_vector & testPt, mxs_vector testVec)
{
   mxs_vector testPt2;

   mx_negeq_vec(&testVec);
   mx_scale_vec(&testPt2, &testVec, kNarrowDist);
   mx_addeq_vec(&testPt2, &testPt);
   return AIPathcast(testPt, cell, testPt2, 0, ((IAIPathfindControl *) NULL));
}

///////////////////////////////////////

void PostProcessNarrow()
{
   int cell;
   int i;
   cDynArray<int> LinksToKill;

   if (!g_fAIPDB_PostProcessNarrow)
      return;

   // Post-process looking for too narrow edges
   for (cell = 1; cell <= g_AIPathDB.m_nCells; cell++)
   {
      if (cell % 128 == 0)
      {
         PumpWindowsEvents();
         mprintf(".");
      }

      if (!g_AIPathDB.m_Cells[cell].cellCount)
         continue;

      i = g_AIPathDB.m_Cells[cell].firstCell;

      if (g_AIPathDB.m_Cells[cell].wrapFlags & IDWRAP_Cell2Cell)
         i += MAX_CELL2CELL_LINKS;

      int                iLimit = i + g_AIPathDB.m_Cells[cell].cellCount;
      sAIPathCellLink *  pLink;
      mxs_vector         endA, endB;
      mxs_vector         testBasis, testPtFar, testPtNear;
      mxs_vector         testVec;
      float              d;
      float              inc = 1.0 / (float(g_EdgePropertySampleSize) - 1.0);
      float              pos;    // position along edge, [0..1]

      for (; i < iLimit; i++)
      {
         pLink = &g_AIPathDB.m_Links[i];
         if (!pLink->okBits)
            continue;

         endA = g_AIPathDB.GetVertex(pLink->vertex_1);
         endB = g_AIPathDB.GetVertex(pLink->vertex_2);

         mx_sub_vec(&testVec, &endA, &endB);
         mx_normeq_vec(&testVec);

         pos = 0.0;

         BOOL clear = FALSE;

         for (int j = 0; j < g_EdgePropertySampleSize; j++)
         {
            mx_interpolate_vec(&testBasis, &endA, &endB, pos);

            mx_sub_vec(&testPtFar, &testBasis, &g_AIPathDB.GetCenter(pLink->dest));
            d = mx_normeq_vec(&testPtFar);
            mx_scaleeq_vec(&testPtFar, d - 0.005);
            mx_addeq_vec(&testPtFar, &g_AIPathDB.GetCenter(pLink->dest));

            mx_sub_vec(&testPtNear, &testBasis, &g_AIPathDB.GetCenter(cell));
            d = mx_normeq_vec(&testPtNear);
            mx_scaleeq_vec(&testPtNear, d - 0.005);
            mx_addeq_vec(&testPtNear, &g_AIPathDB.GetCenter(cell));

            if (!IsPtInCellXYPlane(testPtFar, &g_AIPathDB.m_Cells[pLink->dest]))
            {
               clear = TRUE;
               break; // really don't know what to do
            }

            if (!IsPtInCellXYPlane(testPtNear, &g_AIPathDB.m_Cells[cell]))
            {
               clear = TRUE;
               break; // really don't know what to do
            }

            if (TestPathWidthCenter(pLink->dest, testPtFar, testVec))
            {
               if (TestPathWidthCenter(cell, testPtNear, testVec))
               {
                  clear = TRUE;
                  break; // clear on both sides
               }
            }

            if (TestPathWidthRight(pLink->dest, testPtFar, testVec))
            {
               if (TestPathWidthRight(cell, testPtNear, testVec))
               {
                  clear = TRUE;
                  break; // clear on both sides
               }
            }

            if (TestPathWidthLeft(pLink->dest, testPtFar, testVec))
            {
               if (TestPathWidthLeft(cell, testPtNear, testVec))
               {
                  clear = TRUE;
                  break; // clear on both sides
               }
            }
         }

         // at some point, could use number of openings to bias the cost to discourage rather than prohibit link use
         if (!clear)
            LinksToKill.Append(i);
      }
   }

   for (i = 0; i < LinksToKill.Size(); i++)
   {
      g_AIPathDB.m_Links[LinksToKill[i]].okBits = 0;
   }
}

struct sLinkSort
{
   sAIPathCellLink link;
   float           dist;
};

static int LinkSortFunc(const void * pLeft, const void * pRight)
{
   float left = ((sLinkSort *)pLeft)->dist;
   float right = ((sLinkSort *)pRight)->dist;

   if (left == right)
      return 0;
   if (left > right)
      return 1;
   return -1;
}

static float DistXYFromCellSq(tAIPathCellID cell, const mxs_vector & pt)
{
   const sAIPathCell * pCell = g_AIPathDB.GetCell(cell);

   if (IsPtInCellXYPlane(pt, pCell))
      return 0.0;

   float closest = 999999.0;
   float current;
   for (int i = 0; i < pCell->vertexCount; i++)
   {
      current = DistSqXYPtSeg(pt, g_AIPathDB.GetCellVertex(cell, i), g_AIPathDB.GetCellVertex(cell, i + 1));
      if (current < closest)
         closest = current;
   }

   return closest;
}

void SortLinksByDistance()
{
   int                     i, j, last;
   sAIPathCell *           pCell;
   const sAIPathCellLink * pLink;
   sLinkSort               links[MAX_LINKS_FROM_CELL];
   mxs_vector              mid;

   for (i = 1; i <= g_AIPathDB.m_nCells; i++)
   {
      pCell = g_AIPathDB.AccessCell(i);
      last  = pCell->firstCell + pCell->cellCount;

      for (j = 0; j < pCell->cellCount; j++)
      {
         pLink = g_AIPathDB.GetCellLink(pCell->firstCell + j);

         const mxs_vector & v1 = g_AIPathDB.GetVertex(pLink->vertex_1);
         const mxs_vector & v2 = g_AIPathDB.GetVertex(pLink->vertex_2);
         mid.x = (v1.x + v2.x) / 2;
         mid.y = (v1.y + v2.y) / 2;

         links[j].link = *pLink;
         links[j].dist = DistXYFromCellSq(i, mid);
      }

      qsort(links, pCell->cellCount, sizeof(sLinkSort), LinkSortFunc);

      for (j = 0; j < pCell->cellCount; j++)
      {
         g_AIPathDB.m_Links[pCell->firstCell + j] = links[j].link;
      }
   }

}

#endif EDITOR

///////////////////////////////////////////////////////////////////////////////

void AIPathHintObjects()
{
   // @TBD (toml 04-03-98): this should probably happen on save, so it's
   // more likely to be true
   cMxsVector vec;
   int i;

   g_AIPathDB.m_nObjHints = gMaxObjID;
   g_AIPathDB.m_ObjHints.SetSize(g_AIPathDB.m_nObjHints);
   memset(g_AIPathDB.m_ObjHints, 0, sizeof(tAIPathCellID) * g_AIPathDB.m_nObjHints);

   for (i=0; i<g_AIPathDB.m_nObjHints; i++)
   {
      if (ObjPosGet(i) != NULL)
      {
         vec.Set(ObjPosGet(i)->loc.vec);
         AIFindClosestCell(vec, 0, i);
      }
   }
}

///////////////////////////////////////////////////////////////////////////////

void AIPathFindUpdate()
{
   cMxsVector movement_delta;
   int i, j;

   // Sync all movable cells with their moving terrain
   for (i=0; i<g_AIPathDB.m_MovableCells.Size(); i++)
   {
      if (!IsMovingTerrain(g_AIPathDB.m_MovableCells[i].movingObj))
         continue;

      sAIPathCell *pCell = &g_AIPathDB.m_Cells[g_AIPathDB.m_MovableCells[i].cellID];

      mx_sub_vec(&movement_delta, &ObjPosGet(g_AIPathDB.m_MovableCells[i].movingObj)->loc.vec,
                 &g_AIPathDB.m_MovableCells[i].objVec);

      // Update plane
      sAIPathCellPlane *pPlane = &g_AIPathDB.m_Planes[pCell->plane];

      pPlane->constant -= mx_dot_vec(&pPlane->normal, &movement_delta);

      // Update vertices
      for (j=0; j<pCell->vertexCount; j++)
      {
         mx_addeq_vec(&g_AIPathDB.m_Vertices[g_AIPathDB.m_CellVertices[pCell->firstVertex + j].id].pt,
                      &movement_delta);
      }

      mx_addeq_vec(&g_AIPathDB.m_MovableCells[i].objVec, &movement_delta);

      // Update center
      mx_addeq_vec(&pCell->center, &movement_delta);
   }
}

///////////////////////////////////////////////////////////////////////////////

void AIPathFindMTWaypointHit(ObjID movingTerrain, ObjID waypoint)
{
   // Restore okbits of links assocated with this moving terrain and waypoint
   sAIMovableCell  *pMovableCell;
   sAIWaypointLinks *pWaypointLinks;
   int i, j, k;

   for (i=0; i<g_AIPathDB.m_MovableCells.Size(); i++)
   {
      pMovableCell = &g_AIPathDB.m_MovableCells[i];

      if (pMovableCell->movingObj == movingTerrain)
      {
         // clear active moving terr flag
         g_AIPathDB.m_Cells[pMovableCell->cellID].pathFlags &= ~kAIPF_ActiveMovingTerrain;

         for (j=0; j<pMovableCell->waypointLinkList.Size(); j++)
         {
            pWaypointLinks = &pMovableCell->waypointLinkList[j];

            if (pWaypointLinks->waypointObj == waypoint)
            {
               for (k=0; k<pWaypointLinks->linkList.Size(); k++)
               {
                  g_AIPathDB.m_Links[pWaypointLinks->linkList[k].id].okBits =
                     pWaypointLinks->linkList[k].bits;

#ifndef SHIP
                  if (config_is_defined("AIMovingTerrainSpew"))
                  {
                     mprintf("Restoring %d okBits to link %d\n",
                             pWaypointLinks->linkList[k].bits,
                             pWaypointLinks->linkList[k].id);
                  }
#endif
               }
            }
         }
      }
   }
}

////////////////////////////////////////

void AIPathFindMTWaypointLeft(ObjID movingTerrain)
{
   // Blast okbits of all links associated with this moving terrain
   sAIMovableCell   *pMovableCell;
   sAIWaypointLinks *pWaypointLinks;
   int i, j, k;

#ifndef SHIP
   if (config_is_defined("AIMovingTerrainSpew"))
      mprintf("%d mobile\n", movingTerrain);
#endif

   for (i=0; i<g_AIPathDB.m_MovableCells.Size(); i++)
   {
      pMovableCell = &g_AIPathDB.m_MovableCells[i];

      if (pMovableCell->movingObj == movingTerrain)
      {
         // set active moving terr flag
         g_AIPathDB.m_Cells[pMovableCell->cellID].pathFlags |= kAIPF_ActiveMovingTerrain;

         for (j=0; j<pMovableCell->waypointLinkList.Size(); j++)
         {
            pWaypointLinks = &pMovableCell->waypointLinkList[j];

            for (k=0; k<pWaypointLinks->linkList.Size(); k++)
               g_AIPathDB.m_Links[pWaypointLinks->linkList[k].id].okBits = 0;
         }
      }
   }
}


int AIGetFleeRegion(ObjID id)
{
   sFleeRegionData *pData = g_AIPathDB.m_FleeRegions.Search(id);
   if (!pData)
      return -1;
   else
      return pData->nRegionId;
}


///////////////////////////////////////////////////////////////////////////////

#ifdef EDITOR



// Iterate over all flee points. See which ones can pathcast to the others. All flee regions that can
// path to each other are part of the same "region"
static void AISetFleeRegions()
{
   sPropertyObjIter iter_start;
   sPropertyObjIter iter_end;
   ObjID            startObj;
   ObjID            endObj;
   tAIPathCellID startCell;
   mxs_vector       StartPoint;
   mxs_vector       EndPoint;

   g_AIPathDB.m_FleeRegions.DestroyAll();
   g_AIPathDB.m_nNumFleeRegions = 0;

   // This could be made to be faster, but we're in the path db building phase, so we don't really care.
   g_pAIFleePointProperty->IterStart(&iter_start);
   while (g_pAIFleePointProperty->IterNext(&iter_start, &startObj))
   {
      if (!OBJ_IS_CONCRETE(startObj))
         continue;

      GetObjLocation(startObj, &StartPoint);
      startCell = AIFindClosestCell(StartPoint, 0, startObj);

      // This one has not been used yet.
      if (!g_AIPathDB.m_FleeRegions.Search(startObj))
      {
         g_pAIFleePointProperty->IterStart(&iter_end);
         while (g_pAIFleePointProperty->IterNext(&iter_end, &endObj))
         {
            if (!OBJ_IS_CONCRETE(endObj))
               continue;

            // This one has not been set yet.
            if (!g_AIPathDB.m_FleeRegions.Search(endObj))
            {
               IAIPath *pPath = 0;

               GetObjLocation(endObj, &EndPoint);

//               if ((endObj == startObj) ||
//                   AIPathcast(StartPoint, startCell, EndPoint))
               if ((endObj == startObj) ||
                   ((pPath = AIPathfind(StartPoint, startCell, EndPoint)) != 0))
               {
                  sFleeRegionData *pData = new sFleeRegionData;
                  pData->id = endObj;
                  pData->nRegionId = g_AIPathDB.m_nNumFleeRegions;
                  g_AIPathDB.m_FleeRegions.Insert(pData);
                  if (pPath)
                     SafeRelease(pPath);
               }
            }
         }
         g_pAIFleePointProperty->IterStop(&iter_end);
         g_AIPathDB.m_nNumFleeRegions++;
      }
   }
   g_pAIFleePointProperty->IterStop(&iter_start);
}



#ifdef EDITOR
BOOL g_bDoLinkOptimize = TRUE;
#endif


// Remove any links with okbits of 0, and fixup all references. After this pass,
// All links should number less than MAX_CELL2CELL_LINKS.
static void ShrinkLinks()
{
   int i,j;
   sAIPathCellLink *pLink;
   tAIPathCellID pci_1;
   int nLinkIx;
   int nNumRemovedLinks = 0;
   BOOL bDidWrappedLink = FALSE;


#ifdef EDITOR
   if (!g_bDoLinkOptimize)
   {
      mprintf("Link Optimization is OFF\n");
      return;
   }
#endif

   // index at m_nLinks is actually used, but we don't need to worry about it.
   for (i = 1; i < g_AIPathDB.m_nLinks; i++)
   {
      pLink = &(g_AIPathDB.m_Links[i]);


      if (i % 16 == 0)
         mprintf(".");

      // Let the fun begin
      if (!pLink->okBits)
      {
         for (j = i+1; j <= g_AIPathDB.m_nLinks; j++)
            g_AIPathDB.m_Links[j-1] = g_AIPathDB.m_Links[j];
         g_AIPathDB.m_nLinks--;

         nNumRemovedLinks++;

         // now fixup all references to the newly deleted link.
         for (pci_1 = 1; pci_1 <= g_AIPathDB.m_nCells; pci_1++)
         {
            if (!g_AIPathDB.m_Cells[pci_1].firstCell || !g_AIPathDB.m_Cells[pci_1].cellCount)
               continue;

            nLinkIx = g_AIPathDB.m_Cells[pci_1].firstCell;
            if (g_AIPathDB.m_Cells[pci_1].wrapFlags & IDWRAP_Cell2Cell)
            {
               bDidWrappedLink = TRUE;
               nLinkIx += MAX_CELL2CELL_LINKS;
            }

            if ((i >= nLinkIx) && (i < (nLinkIx+g_AIPathDB.m_Cells[pci_1].cellCount))) // deleted link is within range.
               g_AIPathDB.m_Cells[pci_1].cellCount--;
            else if (i < nLinkIx) // deleted link is before this one, so just decrement firstCell
            {
               g_AIPathDB.m_Cells[pci_1].firstCell--;
               if (g_AIPathDB.m_Cells[pci_1].firstCell >= MAX_CELL2CELL_LINKS)
               {
                  g_AIPathDB.m_Cells[pci_1].wrapFlags |= IDWRAP_Cell2Cell; // Redundant, yes.
                  mprintf("Resetting wrapped link @%d\n", pci_1);
                  g_AIPathDB.m_Cells[pci_1].firstCell -= MAX_CELL2CELL_LINKS;
               }
            }
         }

         i--; // compensate for loop-end autoincrement.
      }
   }

   if (bDidWrappedLink)
      mprintf("Encountered Wrapped Links!\n");

   mprintf("\n   ... Removed %d links\n", nNumRemovedLinks);

   if (g_AIPathDB.m_nLinks >= MAX_CELL2CELL_LINKS)
      Warning(("Number of Cell to Cell links is too high. You're in TROUBLE!\n"));
}



BOOL AIPathFindDBBuild()
{
   tAIPathCellID     ci;            // portal cell index
   uchar          pi;    // portal polygon index, count
   tAIPathVertexID   newVertexID;   // new vertex's id
   sAIPathVertex     newVertex;     // new vertex from portal polygon vertex
   tAIPathCellID     pci_1, pci_2;  // path cell index
   uchar          current_planeid;
   int            firstPortalVertex;
   int            vi;            // vertex index
   int i;

   int nUsedLinks;

   int nNumUselessLinks = 0;

   if (!wr_num_cells)
   {
      mprintf("Cannot build pathfind database: Level is not portalized.\n");
      return FALSE;
   }

   AIPhysCastCollectObjects();
   
   g_fAIPathFindInited = FALSE;

   config_get_float("ai_db_max_edge", &g_MaxEdgeLength);
   config_get_float("ai_db_max_ratio", &g_MaxRatio);
   config_get_int("ai_quality_watch", &g_AIQualityWatch);

   #define MAX_CONSIDERATION_SQ  ((float)sq(50.0)) // chosen based on empirical data gathered from the ects demo level, still generous, should be periodically tested. (toml 08-22-97)

   mprintf("Calculating path find cells...");
   Status("Path: calculating cells");

   // the zero cells aren't used to make debugging easier.
   g_AIPathDB.m_nCells = 0;
   g_AIPathDB.m_nVertices = 0;
   g_AIPathDB.m_nLinks = 0;
   g_AIPathDB.m_nCellVertices = 0;
   g_AIPathDB.m_nPlanes = 0;

   for (i = 0; i < kAIZone_Num; i++)
      g_AIPathDB.m_ZoneDatabases[i].m_nZones = 0;

   g_AIPathDB.m_nCellObjMaps = 0;

   LGALLOC_PUSH_CREDIT();

   g_AIPathDB.m_Cells.SetSize(0);
   g_AIPathDB.m_Marks.SetSize(0);
   g_AIPathDB.m_Planes.SetSize(0);
   g_AIPathDB.m_Vertices.SetSize(0);
   g_AIPathDB.m_Links.SetSize(0);
   g_AIPathDB.m_CellVertices.SetSize(0);

   g_AIPathDB.m_CellObjMap.SetSize(0);
   g_AIPathDB.m_ObjHints.SetSize(0);

   g_AIPathDB.m_CellToDoorsTable.SetEmpty();
   g_AIPathDB.m_CellDoors.SetSize(0);

   g_AIPathDB.m_MovableCells.SetSize(0);

   ResetPathfindZones();

   _heapmin();

   g_AIPathDB.m_Cells.SetSize(MAX_CELLS);
   g_AIPathDB.m_Marks.SetSize(MAX_CELLS);
   memset(g_AIPathDB.m_Marks, 0, MAX_CELLS * sizeof(sbool));
   g_AIPathDB.m_Planes.SetSize(MAX_CELLS);
   g_AIPathDB.m_Vertices.SetSize(MAX_VERTICES);
   g_AIPathDB.m_Links.SetSize(MAX_PROCESSED_CELL2CELL_LINKS);
   g_AIPathDB.m_CellVertices.SetSize(MAX_CELL2VERTEX_LINKS);

   LGALLOC_POP_CREDIT();

   ///////////////////////////////////////
   // Create Path Cells

   for (ci = 0; ci < wr_num_cells; ci++)
   {
      firstPortalVertex = 0;

      PumpWindowsEvents();
      if (ci % 16 == 0)
      {
         mprintf(".");
      }

      int num_solid_polys = WR_CELL(ci)->num_polys - WR_CELL(ci)->num_portal_polys;

      for (pi = 0; pi < num_solid_polys; pi++)
      {
         current_planeid = wr_cell[ci]->poly_list[pi].planeid;

         if (wr_cell[ci]->plane_list[current_planeid].normal.z > g_AIPlaneZNormAccept)
         {
            g_AIPathDB.m_nCells++;

            if ((g_AIPathDB.m_nCells / 50) * 50 == g_AIPathDB.m_nCells)
            {
               mprintf(".");
            }

            if (g_AIPathDB.m_nCells >= MAX_CELLS-1)
            {
               mprintf("AIPathFindDBBuild: too many path cells!!\n");

               return FALSE;
            }

            // wsf added:
            memset(&g_AIPathDB.m_Cells[g_AIPathDB.m_nCells], 0, sizeof(sAIPathCell));

            g_AIPathDB.m_Cells[g_AIPathDB.m_nCells].firstVertex = (tAIPathVertexIDPacked) (g_AIPathDB.m_nCellVertices+1);
            g_AIPathDB.m_Cells[g_AIPathDB.m_nCells].vertexCount = wr_cell[ci]->poly_list[pi].num_vertices;

            g_AIPathDB.m_Cells[g_AIPathDB.m_nCells].plane = AICreateOrFindPlane(wr_cell[ci]->plane_list[current_planeid].normal, wr_cell[ci]->plane_list[current_planeid].plane_constant);
            g_AIPathDB.m_Cells[g_AIPathDB.m_nCells].pathFlags = 0;

            for (vi = firstPortalVertex; vi < firstPortalVertex + wr_cell[ci]->poly_list[pi].num_vertices; vi++)
            {
               newVertex.pt.x = wr_cell[ci]->vpool[wr_cell[ci]->vertex_list[vi]].x;
               newVertex.pt.y = wr_cell[ci]->vpool[wr_cell[ci]->vertex_list[vi]].y;
               newVertex.pt.z = wr_cell[ci]->vpool[wr_cell[ci]->vertex_list[vi]].z;

               // find/create the newVertex's id

               if (!AICreateOrFindPathVertex(newVertex.pt, &newVertexID))
               {
                  // too many vertices!
//                  Warning (("AIPathFindDBBuild: too many vertices!\n"));
                  mprintf("AIPathFindDBBuild: too many vertices!\n");

                  return FALSE;
               }

               // create the link
               g_AIPathDB.m_nCellVertices++;
               if (g_AIPathDB.m_nCellVertices >= MAX_CELL2VERTEX_LINKS-1)
               {
//                  Warning (("AIPathFindDBBuild: too many cell->vertex links!\n"));
                  mprintf("AIPathFindDBBuild: too many cell->vertex links!\n");

                  return FALSE;
               }
               g_AIPathDB.m_CellVertices[g_AIPathDB.m_nCellVertices].id = (tAIPathVertexIDPacked) newVertexID;
            }

            if (!ValidateCell(g_AIPathDB.m_nCells, FALSE))
            {
               mprintf("ERROR! Portal cell %d contains duplicate vertices or is not convex. Cell discarded: pathfinding may have problems\n", ci);
               g_AIPathDB.m_nCells--;
            }
            else if (g_fAIEnforceDBQuality && !EnforceDatabaseQuality(g_AIPathDB.m_nCells))
            {
//               Warning(("AIPathFindDBBuild: EnforceDBQuality Failed!\n"));
               mprintf("Cannot enforce quality!\n");

               return FALSE;
            }
         }

         firstPortalVertex += wr_cell[ci]->poly_list[pi].num_vertices;
      }
   }
   mprintf("\n");

   ///////////////////////////////////////

   for (i = 1; i <= g_AIPathDB.m_nCells; i++)
      g_AIPathDB.m_Cells[i].pathFlags = 0;

   ///////////////////////////////////////

   Status("Path: adding object splits");
   if (g_fAIPDB_BlockingObjects)
   {
      mprintf("Adding blocking objects splits...");
      AddBlockingOBBs();
      mprintf("\n");
   }

   ///////////////////////////////////////

   if (g_fAIPDB_DoorMarks)
   {
      mprintf("Adding door splits...");
      AddDoorSplits();
      mprintf("\n");
   }

   ///////////////////////////////////////

   for (i = 1; i <= g_AIPathDB.m_nCells; i++)
      g_AIPathDB.m_Cells[i].pathFlags = 0;

   ///////////////////////////////////////

   mprintf("Adding pathable objects...");
   AddPathableOBBs();
   mprintf("\n");

   ///////////////////////////////////////

   mprintf("Adding moving terrain objects...");
   AddMovingTerrainOBBs();
   mprintf("\n");

   ///////////////////////////////////////

   ValidateAllCells();

   mprintf("  Calculated %d path cells.\n", g_AIPathDB.m_nCells);

   ///////////////////////////////////////

   if (g_fAIPDB_BlockingObjects)
   {
      mprintf("Marking cells with blocking objects...");
      MarkBlockingObbsUnpathable();
      mprintf("\n");
   }

   ///////////////////////////////////////

   if (g_fAIPDB_DoorMarks)
   {
      mprintf("Marking cells with doors...");
      MarkCellsWithDoors();
      mprintf("\n");
   }

   ///////////////////////////////////////
   //
   // Precompute cell centers
   //
   mprintf("Precomputing cell centers...", g_AIPathDB.m_nCells);
   for (ci = 1; ci <= g_AIPathDB.m_nCells; ci++)
   {
      ComputeCellCenter(ci, &g_AIPathDB.m_Cells[ci].center);
   }
   mprintf("\n");

   if (g_fAIPDB_CellsOnly)
      goto done;

   ///////////////////////////////////////
   // find rough dimensions of cells & other miscellany
   mprintf("Collecting extra data for cells...");
   SetAdjunctData();
   mprintf("\n");

   ///////////////////////////////////////
   // Find cliffs and walls and mark path cells edges

   Status("Path: finding walls & cliffs");
   mprintf("Preprocessing path cell edges to find walls and cliffs...");

   for (ci = 1; ci <= g_AIPathDB.m_nCells; ci++)
   {
      mxs_vector      pt1, pt2, midPt, center;
      tAIPathVertexID v1, v2;
      int          i;
      float        heightSq, widthSq;
      floatang     angle;
      const float  IS_WALL_DISTANCE = 1;

      PumpWindowsEvents();
      if (ci % 16 == 0)
      {
         mprintf(".");
      }

      for (i = 0; i < g_AIPathDB.m_Cells[ci].vertexCount; i++)
      {
         // normally paired edges
         g_AIPathDB.GetCellVertexFull(ci, i,     &pt1, &v1);
         g_AIPathDB.GetCellVertexFull(ci, i + 1, &pt2, &v2);

         g_AIPathDB.GetCenter(ci, &center);
         SetCellEdgeLinked(ci, i, FALSE);
         SetCellEdgeInfo(ci, i, PATH_EDGE_OPEN);

         // check to see if the AI can stand on the pt
         pt1.z += VERTICAL_RISE_LIMIT + 0.5;
         angle.set(pt1.x, pt1.y, center.x, center.y);
         ProjectFromLocationOnZPlane(pt1, 0.25, angle, &pt1);
         PathFindMinimumRaycastAtLocation(pt1, heightSq, widthSq);
         if (widthSq < IS_WALL_DISTANCE)
            g_AIPathDB.m_Vertices[v1].ptInfo = PATH_PT_WALL;

         // check to see if the AI can stand on the edge
         midPt.x = (pt1.x + pt2.x) / 2;
         midPt.y = (pt1.y + pt2.y) / 2;
         midPt.z = (pt1.z + pt2.z) / 2;
         midPt.z += VERTICAL_RISE_LIMIT + 0.5;
         angle.set(midPt.x, midPt.y, center.x, center.y);
         ProjectFromLocationOnZPlane(midPt, 0.25, angle, &midPt);
         PathFindMinimumRaycastAtLocation(midPt, heightSq, widthSq);
         if (widthSq < IS_WALL_DISTANCE)
            SetCellEdgeInfo(ci, i, PATH_EDGE_WALL);

         // check to see if there's a cliff nearby
         float fCliffHeight = PathFindCliffHeight(midPt);
         if (fCliffHeight == DROP_DISTANCE)
            SetCellEdgeInfo(ci, i, PATH_EDGE_CLIFF);
         g_AIPathDB.m_Cells[ci].m_CliffEdge = 1;
      }
   }

   ///////////////////////////////////////

   Status("Path: finding lighting");
   mprintf("\n");
   mprintf("Finding cell lighting levels...\n");
   ComputeCellLightLevels();

   ///////////////////////////////////////
   // Create Path Cell To Cell Links
   //
   // by finding 2 cells with at least two shared vertices
   // (aka, an edge)


   AIPhysRaycastSetObjFilter(kAIPR_PathblockingOrPathable);
   
   Status("Path: linking cells");
   mprintf("Adding cell to cell links...");

   { // Scope
      #define MAX_CONNECTIONS 0x10000
      DECLARE_TIMER(PathLinks, Average);

      TIMER_Start(PathLinks);
      // what size object can cross this edge?
      const float kMaxConsiderationSq = MAX_CONSIDERATION_SQ;

      const int    MAX_POSSIBLE_LINKS_PER_CELL = 60;
      mxs_vector   center1, center2;
      int          sizePathArray = g_AIPathDB.m_nCells + 1;

      sAIEdgeConnectData *     connectionPool = new sAIEdgeConnectData[MAX_CONNECTIONS];
      unsigned                 nConnections = 0;
      sAIPathCellConnections * pathCellConnections = new sAIPathCellConnections[sizePathArray];

      // For every cell, test against every other cell not already tested
      for (pci_1 = 1; pci_1 <= g_AIPathDB.m_nCells; pci_1++)
      {
                                                 // if (pci_1 != 1 && pci_1 != 4) continue;
         PumpWindowsEvents();
         if (pci_1 % 16 == 0)
         {
            mprintf(".");
         }

         g_CurConnData.cell1 = pci_1;
         g_AIPathDB.GetCenter(pci_1, &center1);

         // find all the cells that pci_1 could link to
         for (pci_2 = pci_1 + 1;
              (pci_2 <= g_AIPathDB.m_nCells)
           && (pathCellConnections[pci_1].totalFound
             < MAX_POSSIBLE_LINKS_PER_CELL);
              pci_2++)
         {
                                                 // if (pci_2 != 1 && pci_2 != 4) continue;
            TIMER_Mark(PathLinks);

            g_CurConnData.cell2  = pci_2;
            g_CurConnData.distSq = AIDistanceSq(center1, g_AIPathDB.GetCenter(pci_2, &center2));

            if (pfle(g_CurConnData.distSq, kMaxConsiderationSq) &&
                AIPathCellsShareEdge())
            {
               const BOOL fCell1Wants
                  = pathCellConnections[pci_1].Wants(&g_CurConnData);
               const BOOL fCell2Wants
                  = pathCellConnections[pci_2].Wants(&g_CurConnData);

               if (fCell1Wants || fCell2Wants)
               {
                  AssertMsg(nConnections < MAX_CONNECTIONS,
                            "Maximum cell connections reached");
                  sAIEdgeConnectData * pNewData = &(connectionPool[nConnections]);
                  memcpy(pNewData, &g_CurConnData, sizeof(sAIEdgeConnectData));

                  nConnections++;

                  if (fCell1Wants)
                     pathCellConnections[pci_1].Add(pNewData);

                  if (fCell2Wants)
                     pathCellConnections[pci_2].Add(pNewData);
               }
            }
         }
      }

      // Okay, now lets generate the links
      int * inwardCounts = new int[MAX_CELLS + 1];
      memset(inwardCounts, 0, sizeof(int) * (MAX_CELLS + 1));
      int i;

      for (pci_1 = 1; pci_1 <= g_AIPathDB.m_nCells; pci_1++)
      {
         // init the path cell
         const int linkCount = pathCellConnections[pci_1].count;

         PumpWindowsEvents();
         if (pci_1 % 64 == 0)
         {
            mprintf(".");
         }

         if (!linkCount)
         {
            g_AIPathDB.m_Cells[pci_1].firstCell = 0;
            g_AIPathDB.m_Cells[pci_1].cellCount = 0;
            continue;
         }

         // if we have links, set firstCell to next available spot in
         // cell2cell link array.
         // wsf: now we can process more than is actually stored, so we temporarily store
         // 'wrap' bit if necessary.
         if ((g_AIPathDB.m_nLinks + 1) >= MAX_PROCESSED_CELL2CELL_LINKS)
         {
            mprintf("ERROR: too many cell->cell links\n");
            mprintf("ERROR: Useless links: %d\n",nNumUselessLinks);
            TIMER_Stop(PathLinks);
            TIMER_Clear(PathLinks);
            goto done_links_error;
         }
         else if ((g_AIPathDB.m_nLinks + 1) >= MAX_CELL2CELL_LINKS)
         {
            g_AIPathDB.m_Cells[pci_1].firstCell
               = (tAIPathCell2CellLinkIDPacked)((g_AIPathDB.m_nLinks + 1)-MAX_CELL2CELL_LINKS);
            g_AIPathDB.m_Cells[pci_1].wrapFlags |= IDWRAP_Cell2Cell;
            mprintf("Setting wrapped link! Cell=%d\n", pci_1);
         }
         else
         {
            g_AIPathDB.m_Cells[pci_1].wrapFlags &= ~IDWRAP_Cell2Cell;
            g_AIPathDB.m_Cells[pci_1].firstCell
               = (tAIPathCell2CellLinkIDPacked)(g_AIPathDB.m_nLinks + 1);
         }

         nUsedLinks = 0;
         
         // and create a link to the closest cells...
         for (i = 0; i < linkCount; i++)
         {
            if ((g_AIPathDB.m_nLinks+1) >= MAX_PROCESSED_CELL2CELL_LINKS)
            {
               mprintf("ERROR: too many cell->cell links\n");
               mprintf("ERROR: Useless links: %d\n",nNumUselessLinks);
               mprintf("ERROR: Processing cell %d of %d\n", pci_1, g_AIPathDB.m_nCells);
               TIMER_Stop(PathLinks);
               TIMER_Clear(PathLinks);
               goto done_links_error;
            }

            g_AIPathDB.m_nLinks++;

            // set cell2cell to point to the other guy
            sAIEdgeConnectData * pConnectionData
               = pathCellConnections[pci_1].ppConnectionData[i];
            sAIPathCellLink *    pLink
               = &(g_AIPathDB.m_Links[g_AIPathDB.m_nLinks]);

            // if ((pConnectionData->cell1 != 1 && pConnectionData->cell1 != 4) || (pConnectionData->cell2 != 1 && pConnectionData->cell2 != 4)) continue;
            // @TBD (toml 02-11-98): Below I retain the clockwiseness of the
            // only because I dont presently know if it matters, and dont want
            // to risk anything. Eventually that should be investigated and
            // if it doesnt matter the below could be make much simpler
            if (pci_1 == pConnectionData->cell1)
            {
               pLink->dest = pConnectionData->cell2;

               switch (pConnectionData->kind)
               {
                  case kCell1EdgeContained:
                     pLink->vertex_1
                        = (tAIPathVertexIDPacked)pConnectionData->vertex1a;
                     pLink->vertex_2
                        = (tAIPathVertexIDPacked)pConnectionData->vertex1b;
                     break;

                  case kPerfect:
                  case kCell2EdgeContained:
                     pLink->vertex_1
                        = (tAIPathVertexIDPacked)pConnectionData->vertex2b;
                     pLink->vertex_2
                        = (tAIPathVertexIDPacked)pConnectionData->vertex2a;
                     break;

                  case kCell2SkewedLeft:
                     pLink->vertex_1
                        = (tAIPathVertexIDPacked)pConnectionData->vertex1a;
                     pLink->vertex_2
                        = (tAIPathVertexIDPacked)pConnectionData->vertex2a;
                     break;

                  case kCell2SkewedRight:
                     pLink->vertex_1
                        = (tAIPathVertexIDPacked)pConnectionData->vertex2b;
                     pLink->vertex_2
                        = (tAIPathVertexIDPacked)pConnectionData->vertex1b;
                     break;
               }
            }
            else
            {
               pLink->dest = pConnectionData->cell1;

               switch (pConnectionData->kind)
               {
                  case kPerfect:
                  case kCell1EdgeContained:
                     pLink->vertex_1
                        = (tAIPathVertexIDPacked)pConnectionData->vertex1b;
                     pLink->vertex_2
                        = (tAIPathVertexIDPacked)pConnectionData->vertex1a;
                     break;

                  case kCell2EdgeContained:
                     pLink->vertex_1
                        = (tAIPathVertexIDPacked)pConnectionData->vertex2a;
                     pLink->vertex_2
                        = (tAIPathVertexIDPacked)pConnectionData->vertex2b;
                     break;

                  case kCell2SkewedLeft:
                     pLink->vertex_1
                        = (tAIPathVertexIDPacked)pConnectionData->vertex2a;
                     pLink->vertex_2
                        = (tAIPathVertexIDPacked)pConnectionData->vertex1a;
                     break;

                  case kCell2SkewedRight:
                     pLink->vertex_1
                        = (tAIPathVertexIDPacked)pConnectionData->vertex1b;
                     pLink->vertex_2
                        = (tAIPathVertexIDPacked)pConnectionData->vertex2b;
                     break;
               }
            }

            inwardCounts[pLink->dest]++;

            AssertMsg1(inwardCounts[pLink->dest] <= 64, "Unusually high inward links to cell %d -- cell could be corrupt", pLink->dest);

            tAIPathOkBits okCondBits = 0;
            // What modes of travel make sense for following this link?
            pLink->okBits = PathFindComputeOkBits(pci_1, pLink->dest,
                                                  pLink->vertex_1,
                                                  pLink->vertex_2, pLink, &okCondBits) | okCondBits;

            if (!pLink->okBits) // no path from one cell to another. Just forget this link.
            {
               nNumUselessLinks++;
               inwardCounts[pLink->dest]--;
               g_AIPathDB.m_nLinks--;
            }
            else
            {
               nUsedLinks++;
               ConfigSpew("PathFindGenSpew", ("OkBits from %d to %d: %d\n",
                                              pci_1, pci_2, pLink->okBits));
               ComputeCell2CellCost(pci_1, pLink->dest, &pLink->cost);
            }
         }

         g_AIPathDB.m_Cells[pci_1].cellCount = (uchar)nUsedLinks;
      }

      PostProcessNarrow();

      PostProcessModifiedOkBits();

      // Remove any links with okbits of 0, and fixup all references. After this pass,
      // All links should number less than MAX_CELL2CELL_LINKS.
      mprintf("\nOptimizing Cell to Cell links... ");
      ShrinkLinks();
      mprintf("Done\n");

      // Links out of a cell are sorted by distance from the edge of the polygon for the benefit of the pathcaster (toml 06-04-99)
      SortLinksByDistance();

done_links_error:
      delete [] inwardCounts;
      delete [] connectionPool;
      delete [] pathCellConnections;

      TIMER_Stop(PathLinks);
      TIMER_OutputResult(PathLinks);
      TIMER_Clear(PathLinks);
   }

   AIPhysRaycastClearObjFilter();

   mprintf("\n");

   _heapmin();

   mprintf("Setting floor types...");
   PropagateFloorTypes();
   mprintf("\n");

   mprintf("Post-processing all moving terrain cells...");
   FixupMovingTerrainOBBs();
   mprintf("\n");

   ///////////////////////////////////////
   // Split plathfinding database into contiguous zones

   Status("Path: constructing zones");
   mprintf("Determining zones...");
   DeterminePathfindZones();
   mprintf("\n");

   mprintf("Linking pathfind zones...");
   LinkPathfindZones();
   mprintf("\n");

   mprintf("Fixing up pathfind zones...");
   FixupPathfindZones();
   mprintf("\n");

   ///////////////////////////////////////
   // Hint all objects
   mprintf("Hinting objects...");
   AIPathHintObjects();
   mprintf("\n");

done:
   ///////////////////////////////////////
   // All done

   sAIPathCell *            pPathCells            = g_AIPathDB.m_Cells.Detach();
   sAIPathCellPlane *       pPathPlanes           = g_AIPathDB.m_Planes.Detach();
   sAIPathVertex *          pPathVertexs          = g_AIPathDB.m_Vertices.Detach();
   sAIPathCellLink *        pPathCell2CellLinks   = g_AIPathDB.m_Links.Detach();
   sAIPathCell2VertexLink * pPathCell2VertexLinks = g_AIPathDB.m_CellVertices.Detach();

   LGALLOC_PUSH_CREDIT();

   g_AIPathDB.m_Cells.SetSize(g_AIPathDB.m_nCells + 1);
   g_AIPathDB.m_Planes.SetSize(g_AIPathDB.m_nPlanes + 1);
   g_AIPathDB.m_Vertices.SetSize(g_AIPathDB.m_nVertices + 1);
   g_AIPathDB.m_Links.SetSize(g_AIPathDB.m_nLinks + 1);
   g_AIPathDB.m_CellVertices.SetSize(g_AIPathDB.m_nCellVertices + 1);

   g_AIPathDB.m_Marks.SetSize(g_AIPathDB.m_nCells + 1);
   memset(g_AIPathDB.m_Marks, 0, (g_AIPathDB.m_nCells + 1) * sizeof(sbool));
   g_AIPathDB.m_BestCostToReach.SetSize(g_AIPathDB.m_nCells + 1);

   LGALLOC_POP_CREDIT();

   g_AIPathDB.m_Cells.MemCpy(pPathCells, g_AIPathDB.m_nCells + 1);
   g_AIPathDB.m_Planes.MemCpy(pPathPlanes, g_AIPathDB.m_nPlanes + 1);
   g_AIPathDB.m_Vertices.MemCpy(pPathVertexs, g_AIPathDB.m_nVertices + 1);
   g_AIPathDB.m_Links.MemCpy(pPathCell2CellLinks, g_AIPathDB.m_nLinks + 1);
   g_AIPathDB.m_CellVertices.MemCpy(pPathCell2VertexLinks, g_AIPathDB.m_nCellVertices + 1);

   delete [] pPathCells;
   delete [] pPathPlanes;
   delete [] pPathVertexs;
   delete [] pPathCell2CellLinks;
   delete [] pPathCell2VertexLinks;

   _heapmin();

   ClearAdjunctData();

   Status("Path: done");
   mprintf("\nDatabase built: %dk, %d cells, %d vertices, %d cell links,\n"
           "                  %d Normal zones, %d zone pairs. %dk\n"
           "                  %d NormalLVL zones, %d zone pairs. %dk\n"
           "                  %d HighStrike zones, %d zone pairs. %dk\n"
           "                  %d HighStrikeLVL zones, %d zone pairs. %dk\n\n",
           AIGetPathDBSize() / 1024,
           g_AIPathDB.m_nCells, g_AIPathDB.m_nVertices, g_AIPathDB.m_nLinks,
           g_AIPathDB.m_ZoneDatabases[kAIZone_Normal].m_nZones - 1,
           g_AIPathDB.m_ZoneDatabases[kAIZone_Normal].m_ZonePairTable.GetCount(),
           AIGetZoneSize(kAIZone_Normal)/1024,
           g_AIPathDB.m_ZoneDatabases[kAIZone_NormalLVL].m_nZones - 1,
           g_AIPathDB.m_ZoneDatabases[kAIZone_NormalLVL].m_ZonePairTable.GetCount(),
           AIGetZoneSize(kAIZone_NormalLVL)/1024,
           g_AIPathDB.m_ZoneDatabases[kAIZone_HighStrike].m_nZones - 1,
           g_AIPathDB.m_ZoneDatabases[kAIZone_HighStrike].m_ZonePairTable.GetCount(),
           AIGetZoneSize(kAIZone_HighStrike)/1024,
           g_AIPathDB.m_ZoneDatabases[kAIZone_HighStrikeLVL].m_nZones - 1,
           g_AIPathDB.m_ZoneDatabases[kAIZone_HighStrikeLVL].m_ZonePairTable.GetCount(),
           AIGetZoneSize(kAIZone_HighStrikeLVL)/1024
          );

   _heapmin();

   g_fAIPathFindInited = TRUE;


   // Build Flee regions
   mprintf("Building Flee Regions...");
   AISetFleeRegions();
   mprintf("\n");


   // Build the room database, because it will use the new pathfinding data
#ifdef EDITOR
   ConvertRoomBrushes();
#endif
   BuildAIRoomDatabase();

   _heapmin();

   return TRUE;
}

#endif // EDITOR

///////////////////////////////////////////////////////////////////////////////

// Note! when you invalidate the pathfinding database, you should
// also bump the version on the ai room database (in roomloop.cpp)

static TagFileTag g_AIPathTag        = { "AIPATH" };
static TagVersion g_AIPathTagVersion = { 3, 3 };

static BOOL OpenPathTagFile(ITagFile* pTagFile)
{
   HRESULT result;
   TagVersion v = g_AIPathTagVersion;

   result = pTagFile->OpenBlock(&g_AIPathTag, &v);

   if (result == S_OK)
   {
      if (v.major != g_AIPathTagVersion.major || v.minor != g_AIPathTagVersion.minor)
      {
         pTagFile->CloseBlock();
         Warning (("File contains old AI path data, data not loaded.\n"));
      }
   }

   return ((result == S_OK)  &&
           (v.major == g_AIPathTagVersion.major) &&
           (v.minor == g_AIPathTagVersion.minor));
}

///////////////////////////////////////

#define ArrToMoveParm(arr) ((char *)((arr).AsPointer()))


void AIPathFindRead(ITagFile * pTagFile)
{
   g_AIPathDB.Term();

   BOOL rehint = FALSE;
   
   if (!OpenPathTagFile(pTagFile))
      return;

   pTagFile->Move((char*)&g_fAIPathFindInited, sizeof(int));

   if (g_fAIPathFindInited)
   {
      int size;
      int i, j;

      LGALLOC_PUSH_CREDIT();

      int obsolete;
      pTagFile->Move((char*)&obsolete, sizeof(int));

      pTagFile->Move((char*)&g_AIPathDB.m_nCells, sizeof(tAIPathCellID));
      g_AIPathDB.m_Cells.SetSize(g_AIPathDB.m_nCells+1);
      pTagFile->Move(ArrToMoveParm(g_AIPathDB.m_Cells), sizeof(sAIPathCell) * (g_AIPathDB.m_nCells+1));

      pTagFile->Move((char*)&g_AIPathDB.m_nPlanes, sizeof(tAIPathCellID));
      g_AIPathDB.m_Planes.SetSize(g_AIPathDB.m_nPlanes+1);
      pTagFile->Move(ArrToMoveParm(g_AIPathDB.m_Planes), sizeof(sAIPathCellPlane) * (g_AIPathDB.m_nPlanes+1));

      pTagFile->Move((char*)&g_AIPathDB.m_nVertices, sizeof(tAIPathVertexID));
      g_AIPathDB.m_Vertices.SetSize(g_AIPathDB.m_nVertices+1);
      pTagFile->Move(ArrToMoveParm(g_AIPathDB.m_Vertices), sizeof(sAIPathVertex) * (g_AIPathDB.m_nVertices+1));

      pTagFile->Move((char*)&g_AIPathDB.m_nLinks, sizeof(tAIPathCell2CellLinkID));
      g_AIPathDB.m_Links.SetSize(g_AIPathDB.m_nLinks+1);
      pTagFile->Move(ArrToMoveParm(g_AIPathDB.m_Links), sizeof(sAIPathCellLink) * (g_AIPathDB.m_nLinks+1));

      pTagFile->Move((char*)&g_AIPathDB.m_nCellVertices, sizeof(tAIPathCell2VertexLinkID));
      g_AIPathDB.m_CellVertices.SetSize(g_AIPathDB.m_nCellVertices+1);
      pTagFile->Move(ArrToMoveParm(g_AIPathDB.m_CellVertices), sizeof(sAIPathCell2VertexLink) * (g_AIPathDB.m_nCellVertices+1));

      pTagFile->Move((char*)&size, sizeof(int));
     
#ifndef SHIP
      if (size > gMaxObjID)
         mprintf("Warning: Pathfinding database was built with an object space larger than the current object space!\n");
#endif
      rehint = (size != gMaxObjID);
      
      g_AIPathDB.m_nObjHints = size;
      g_AIPathDB.m_ObjHints.SetSize(g_AIPathDB.m_nObjHints);
      memset(g_AIPathDB.m_ObjHints, 0, sizeof(tAIPathCellID) * g_AIPathDB.m_nObjHints);

      pTagFile->Move(ArrToMoveParm(g_AIPathDB.m_ObjHints), sizeof(tAIPathCellID) * size);

      pTagFile->Move((char*)&g_AIPathDB.m_nCellObjMaps, sizeof(int));
      g_AIPathDB.m_CellObjMap.SetSize(g_AIPathDB.m_nCellObjMaps);
      pTagFile->Move(ArrToMoveParm(g_AIPathDB.m_CellObjMap), sizeof(sAIPathCellObjMap) * g_AIPathDB.m_nCellObjMaps);

      // New for 3.2, multiple pathfinding zones:
      for (i = 0; i < kAIZone_Num; i++)
      {
         g_AIPathDB.m_ZoneDatabases[i].m_CellZones.SetSize(g_AIPathDB.m_nCells+1);
         pTagFile->Move(ArrToMoveParm(g_AIPathDB.m_ZoneDatabases[i].m_CellZones), sizeof(tAIPathZone) * (g_AIPathDB.m_nCells+1));
         pTagFile->Move((char *)&g_AIPathDB.m_ZoneDatabases[i].m_nZones, sizeof(int));

         sZoneOkBitsMap *pMap;
         sZoneOkBitsMap  map;

         pTagFile->Move((char *)&map.key, sizeof(long));
         pTagFile->Move((char *)&map.okBits, sizeof(tAIPathOkBits));

         while (map.key != 0)
         {
            pMap = new sZoneOkBitsMap;

            pMap->key = map.key;
            pMap->okBits = map.okBits;

            g_AIPathDB.m_ZoneDatabases[i].m_ZonePairTable.Insert(pMap);

            pTagFile->Move((char *)&map.key, sizeof(long));
            pTagFile->Move((char *)&map.okBits, sizeof(tAIPathOkBits));
         }
      }

#ifndef NO_CELLDOOR_SAVELOAD
      size = 0;
      pTagFile->Move((char*)&size, sizeof(int));
      g_AIPathDB.m_CellDoors.SetSize(size);
      pTagFile->Move(ArrToMoveParm(g_AIPathDB.m_CellDoors), sizeof(sAICellDoor) * size);

      for (i = 0; i < size; i++)
         g_AIPathDB.m_CellToDoorsTable.Insert(&g_AIPathDB.m_CellDoors[i]);
#endif

      pTagFile->Move((char *)&size, sizeof(int));
      g_AIPathDB.m_MovableCells.SetSize(size);

      for (i=0; i<size; i++)
      {
         pTagFile->Move((char *)&g_AIPathDB.m_MovableCells[i].movingObj, sizeof(ObjID));
         pTagFile->Move((char *)&g_AIPathDB.m_MovableCells[i].cellID, sizeof(tAIPathCellID));
         pTagFile->Move((char *)&g_AIPathDB.m_MovableCells[i].objVec, sizeof(cMxsVector));

         int waypoint_size;
         pTagFile->Move((char *)&waypoint_size, sizeof(int));
         g_AIPathDB.m_MovableCells[i].waypointLinkList.SetSize(waypoint_size);

         for (j=0; j<waypoint_size; j++)
         {
            sAIWaypointLinks *pWaypointLinks = &g_AIPathDB.m_MovableCells[i].waypointLinkList[j];

            pTagFile->Move((char *)&pWaypointLinks->waypointObj, sizeof(ObjID));

            int link_size;
            pTagFile->Move((char *)&link_size, sizeof(int));
            pWaypointLinks->linkList.SetSize(link_size);
            pTagFile->Move(ArrToMoveParm(pWaypointLinks->linkList), sizeof(sAILinkData) * link_size);
         }
      }

      if (g_AIPathDB.m_nCells >= MAX_CELLS)
      {
         Warning (("g_AIPathDB.m_nCells is really large! Setting to 0\n"));
         g_AIPathDB.m_nCells = 0;
      }

      if (g_AIPathDB.m_nVertices >= MAX_VERTICES)
      {
         Warning (("g_AIPathDB.m_nVertices is really large! Setting to 0\n"));
         g_AIPathDB.m_nVertices = 0;
      }

      if (g_AIPathDB.m_nLinks >= MAX_CELL2CELL_LINKS)
      {
         Warning (("g_AIPathDB.m_nLinks is really large! Setting to 0\n"));
         g_AIPathDB.m_nLinks = 0;
      }

      if (g_AIPathDB.m_nCellVertices >= MAX_CELL2VERTEX_LINKS)
      {
         Warning (("g_AIPathDB.m_nCellVertices is really large! Setting to 0\n"));
         g_AIPathDB.m_nCellVertices = 0;
      }

      g_AIPathDB.m_Marks.SetSize(g_AIPathDB.m_nCells + 1);
      memset(g_AIPathDB.m_Marks, 0, (g_AIPathDB.m_nCells + 1) * sizeof(sbool));
      g_AIPathDB.m_BestCostToReach.SetSize(g_AIPathDB.m_nCells + 1);

      // New for version 3.0: Store Flee Point Region Info
      pTagFile->Move((char *)&g_AIPathDB.m_nNumFleeRegions, sizeof(int));
      int nNum;
      pTagFile->Move((char *)&nNum, sizeof(int));
      for (i = 0; i < nNum; i++)
      {
         sFleeRegionData *pData;
         pData = new sFleeRegionData;
         pTagFile->Move((char *)pData, sizeof(sFleeRegionData));
         g_AIPathDB.m_FleeRegions.Insert(pData);
      }

      LGALLOC_POP_CREDIT();

      // New for 3.1, store Large Door Size.
      float ignored;
      pTagFile->Move((char *)&ignored, sizeof(float));
   }
#ifndef SHIP
   else
      mprintf("No AI pathfinding database.\n");
#endif
   pTagFile->CloseBlock();
   
   if (rehint)
   {
#ifndef SHIP
      mprintf("Warning: rehinting objects on AI pathfinding database load.\n");
#endif
      AIPathHintObjects();
   }
}

///////////////////////////////////////

void AIPathFindWrite(ITagFile * pTagFile)
{
   int i;

#ifdef EDITOR
   if (g_fAIPDB_CellsOnly)
      return;
#endif

   if (!OpenPathTagFile(pTagFile))
      return;

   pTagFile->Move((char*)&g_fAIPathFindInited, sizeof(int));

   if (g_fAIPathFindInited)
   {
      int obsolete;
      pTagFile->Move((char*)&obsolete, sizeof(int));

      if (g_AIPathDB.m_nCells >= MAX_CELLS)
      {
         Warning (("g_AIPathDB.m_nCells is really large! Setting to 0\n"));
         g_AIPathDB.m_nCells = 0;
      }

      if (g_AIPathDB.m_nVertices >= MAX_VERTICES)
      {
         Warning (("g_AIPathDB.m_nVertices is really large! Setting to 0\n"));
         g_AIPathDB.m_nVertices = 0;
      }

      if (g_AIPathDB.m_nLinks >= MAX_CELL2CELL_LINKS)
      {
         Warning (("g_AIPathDB.m_nLinks is really large! Setting to 0\n"));
         g_AIPathDB.m_nLinks = 0;
      }

      if (g_AIPathDB.m_nCellVertices >= MAX_CELL2VERTEX_LINKS)
      {
         Warning (("g_AIPathDB.m_nCellVertices is really large! Setting to 0\n"));
         g_AIPathDB.m_nCellVertices = 0;
      }

      pTagFile->Move((char*)&g_AIPathDB.m_nCells, sizeof(tAIPathCellID));
      pTagFile->Move(ArrToMoveParm(g_AIPathDB.m_Cells), sizeof(sAIPathCell) * (g_AIPathDB.m_nCells+1));

      pTagFile->Move((char*)&g_AIPathDB.m_nPlanes, sizeof(tAIPathCellID));
      pTagFile->Move(ArrToMoveParm(g_AIPathDB.m_Planes), sizeof(sAIPathCellPlane) * (g_AIPathDB.m_nPlanes+1));

      pTagFile->Move((char*)&g_AIPathDB.m_nVertices, sizeof(tAIPathVertexID));
      pTagFile->Move(ArrToMoveParm(g_AIPathDB.m_Vertices), sizeof(sAIPathVertex) * (g_AIPathDB.m_nVertices+1));

      pTagFile->Move((char*)&g_AIPathDB.m_nLinks, sizeof(tAIPathCell2CellLinkID));
      pTagFile->Move(ArrToMoveParm(g_AIPathDB.m_Links), sizeof(sAIPathCellLink) * (g_AIPathDB.m_nLinks+1));

      pTagFile->Move((char*)&g_AIPathDB.m_nCellVertices, sizeof(tAIPathCell2VertexLinkID));
      pTagFile->Move(ArrToMoveParm(g_AIPathDB.m_CellVertices), sizeof(sAIPathCell2VertexLink) * (g_AIPathDB.m_nCellVertices+1));

      pTagFile->Move((char*)&g_AIPathDB.m_nObjHints, sizeof(int));
      pTagFile->Move(ArrToMoveParm(g_AIPathDB.m_ObjHints), sizeof(tAIPathCellID) * g_AIPathDB.m_nObjHints);

      pTagFile->Move((char*)&g_AIPathDB.m_nCellObjMaps, sizeof(int));
      pTagFile->Move(ArrToMoveParm(g_AIPathDB.m_CellObjMap), sizeof(sAIPathCellObjMap) * g_AIPathDB.m_nCellObjMaps);

      long zero = 0;

      // New for 3.2, multiple pathfinding zones:
      for (i = 0; i < kAIZone_Num; i++)
      {
         sZoneOkBitsMap *pMap;
         tHashSetHandle  handle;

         pTagFile->Move(ArrToMoveParm(g_AIPathDB.m_ZoneDatabases[i].m_CellZones), sizeof(tAIPathZone) * (g_AIPathDB.m_nCells+1));
         pTagFile->Move((char*)&g_AIPathDB.m_ZoneDatabases[i].m_nZones, sizeof(int));

         pMap = g_AIPathDB.m_ZoneDatabases[i].m_ZonePairTable.GetFirst(handle);
         while (pMap != NULL)
         {
            pTagFile->Move((char*)&pMap->key, sizeof(long));
            pTagFile->Move((char*)&pMap->okBits, sizeof(tAIPathOkBits));

            pMap = g_AIPathDB.m_ZoneDatabases[i].m_ZonePairTable.GetNext(handle);
         }
         pTagFile->Move((char*)&zero, sizeof(long));
         pTagFile->Move((char*)&zero, sizeof(tAIPathOkBits));
      }

      int size;

#ifndef NO_CELLDOOR_SAVELOAD
      size = g_AIPathDB.m_CellDoors.Size();
      pTagFile->Move((char*)&size, sizeof(int));
      pTagFile->Move(ArrToMoveParm(g_AIPathDB.m_CellDoors), sizeof(sAICellDoor) * size);
#endif

      size = g_AIPathDB.m_MovableCells.Size();
      pTagFile->Move((char *)&size, sizeof(int));

      int i, j;

      for (i=0; i<size; i++)
      {
         pTagFile->Move((char *)&g_AIPathDB.m_MovableCells[i].movingObj, sizeof(ObjID));
         pTagFile->Move((char *)&g_AIPathDB.m_MovableCells[i].cellID, sizeof(tAIPathCellID));
         pTagFile->Move((char *)&g_AIPathDB.m_MovableCells[i].objVec, sizeof(cMxsVector));

         int waypoint_size = g_AIPathDB.m_MovableCells[i].waypointLinkList.Size();
         pTagFile->Move((char *)&waypoint_size, sizeof(int));

         for (j=0; j<waypoint_size; j++)
         {
            sAIWaypointLinks *pWaypointLinks = &g_AIPathDB.m_MovableCells[i].waypointLinkList[j];

            pTagFile->Move((char *)&pWaypointLinks->waypointObj, sizeof(ObjID));

            int link_size = pWaypointLinks->linkList.Size();
            pTagFile->Move((char *)&link_size, sizeof(int));
            pTagFile->Move(ArrToMoveParm(pWaypointLinks->linkList), sizeof(sAILinkData) * link_size);
         }
      }

      //////////////////////////////////////////////////////////////////////////
      //
      // New for version 3.0: Store Flee Point Region Info
      tHashSetHandle hs;

      // write out how many:
      sFleeRegionData *pData = g_AIPathDB.m_FleeRegions.GetFirst(hs);
      size = 0;
      while (pData)
      {
         pData = g_AIPathDB.m_FleeRegions.GetNext(hs);
         size++;
      }

      pTagFile->Move((char *)&g_AIPathDB.m_nNumFleeRegions, sizeof(int));
      pTagFile->Move((char *)&size, sizeof(int));

      // write out data:
      pData = g_AIPathDB.m_FleeRegions.GetFirst(hs);
      while (pData)
      {
         pTagFile->Move((char *)pData, sizeof(sFleeRegionData));
         pData = g_AIPathDB.m_FleeRegions.GetNext(hs);
      }


      // New for 3.1, store Large Door Size.
      float ignored;
      pTagFile->Move((char *)&ignored, sizeof(float));

   }

// @TBD (toml 09-25-98): save cell doors

   pTagFile->CloseBlock();
}

///////////////////////////////////////////////////////////////////////////////
