/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aipthdbg.cpp,v 1.26 2000/03/01 13:03:01 bfarquha Exp $
//
//
//


#include <lg.h>
extern "C"
{
#include <r3d.h>
}

#include <playrobj.h>

#include <guistyle.h>

#include <wrtype.h>
#include <objpos.h>
#include <aipthtyp.h>
#include <aipathdb.h>
#include <aipthloc.h>
#include <aipthdbg.h>
#include <aipthedg.h>
#include <rpaths.h>
#include <aiapipth.h>
#include <aiwr.h>
#include <aiman.h>
#include <ai.h>
#include <aipthloc.h>

#include <editbr_.h>
#include <vbrush.h>
#include <brinfo.h>
#include <uiedit.h>
#include <status.h>

#ifdef PLAYTEST
#define NAME_SUPPORT
#endif
#include <2d.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////

BOOL ValidateCellSpace(tAIPathCellID cell, float height)
{
   #define kVCS_EpsIn 0.005
   #define kVCS_EpsUp 0.001

   // Collect the vertices and center
   int                   i, j;
   mxs_vector            center = g_AIPathDB.GetCenter(cell);
   cDynArray<mxs_vector> vertices;
   mxs_vector            temp;
   const int             lastVertex = g_AIPathDB.m_Cells[cell].firstVertex + g_AIPathDB.m_Cells[cell].vertexCount;

   for (i = g_AIPathDB.m_Cells[cell].firstVertex; i < lastVertex; i++)
      vertices.Append(g_AIPathDB.m_Vertices[g_AIPathDB.m_CellVertices[i].id].pt);

   // Move the vertices in a bit toward the center
   for (i = 0; i < vertices.Size(); i++)
   {
      mx_sub_vec(&temp, &center, &vertices[i]);
      mx_normeq_vec(&temp);
      mx_scaleeq_vec(&temp, kVCS_EpsIn);
      mx_addeq_vec(&temp, &vertices[i]);
      temp.z += kVCS_EpsUp;
      vertices[i] = temp;
   }

   Location start, end, hit;

   unsigned nSucceeded;

   for (i = 0; i < vertices.Size(); i++)
   {
      nSucceeded = 0;
      MakeLocationFromVector(&start, &vertices[i]);

      for (j = 0; j < vertices.Size(); j++)
      {
         if (j == i)
            continue;
         MakeLocationFromVector(&end, &vertices[j]);
         end.vec.z += height;

         if (AIRaycast(&start, &end, &hit, kAIR_NoHintWarn))
            nSucceeded++;
      }
      if (!nSucceeded)
         return FALSE;
   }
   return TRUE;
}

///////////////////////////////////////

void ValidateAllCellSpaces(float height, BOOL linkedOnly, cDynArray<tAIPathCellID> * pFailures)
{
   cDynArray<BOOL> test(g_AIPathDB.m_nCells + 1);
   int cell;
   int i;

   memset(test, 0, test.Size() * sizeof(BOOL));

   if (linkedOnly)
   {
      for (cell = 1; cell <= g_AIPathDB.m_nCells; cell++)
      {
         // and gather arcs to all the cells it's connected to with centers within range
         tAIPathCellID i;
         const int lastCell = g_AIPathDB.m_Cells[cell].firstCell + g_AIPathDB.m_Cells[cell].cellCount;

         for (i = g_AIPathDB.m_Cells[cell].firstCell; i < lastCell; i++)
         {
            if (g_AIPathDB.m_Links[i].okBits)
               test[g_AIPathDB.m_Links[i].dest] = TRUE;
         }
      }
   }

   for (cell = 1; cell <= g_AIPathDB.m_nCells; cell++)
   {
      if ((!linkedOnly || test[cell]) && !ValidateCellSpace(cell, height))
         pFailures->Append(cell);
   }

   if (linkedOnly)
   {
      const cAIArray & AIs = g_pAIManager->AccessAIs();
      cDynArray<mxs_vector> Locs;
      cDynArray<tAIPathCellID> Cells;

      for (i = 0; i < AIs.Size(); i++)
      {
         tAIPathCellID curCell = AIFindClosestCell(*GetObjLocation(AIs[i]->GetObjID()), 0);
         if (curCell)
         {
            Locs.Append(*GetObjLocation(AIs[i]->GetObjID()));
            Cells.Append(curCell);
         }
      }

      BOOL reach;

      for (i = pFailures->Size() - 1; i >= 0; i--)
      {
         reach = FALSE;
         for (int j = 0; j < Locs.Size(); j++)
         {
            IAIPath * pPath = AIPathfind(Locs[j], Cells[j], g_AIPathDB.GetCenter((*pFailures)[i]), (*pFailures)[i]);
            if (pPath)
            {
               reach = TRUE;
               pPath->Release();
               break;
            }
         }
         if (!reach)
            pFailures->FastDeleteItem(i);
      }
   }

}

///////////////////////////////////////////////////////////////////////////////

ObjID GetCurrentBrushObjID()
{
#ifdef EDITOR
   editBrush* cur = vBrush_GetSel();
   if (brushGetType(cur) == brType_OBJECT)
      return brObj_ID(cur);
#endif

   return OBJ_NULL;
}

///////////////////////////////////////////////////////////////////////////////

inline void ComputeCenterOfVertices(const tAIPathVertexID v1, const tAIPathVertexID v2, cMxsVector &p)
{
   p.x = (g_AIPathDB.GetVertex(v1).x + g_AIPathDB.GetVertex(v2).x) / 2;
   p.y = (g_AIPathDB.GetVertex(v1).y + g_AIPathDB.GetVertex(v2).y) / 2;
   p.z = (g_AIPathDB.GetVertex(v1).z + g_AIPathDB.GetVertex(v2).z) / 2;
}

///////////////////////////////////////////////////////////////////////////////

// How long can a path be?
#define MAX_PATH_LENGTH 256

void AIPathFindDrawPath(IAIPath * pPath)
{
   int        pathColor = guiScreenColor(guiRGB(0,0,255));
   static r3s_point  pt[MAX_PATH_LENGTH];
   cAIPathEdge   edge;
   cMxsVector    edgeCenter, p;
   mxs_vector translate;
   mxs_vector temp;

   translate.x = 0;
   translate.y = 0;
   translate.z = 0.2;

   if (pPath->Active())
   {
      int    i, count;

      r3_start_object(&translate);
      r3_start_block();

      r3_set_color(pathColor);

      count = pPath->GetPathEdgeCount();
      for (i = 0; i < count; i++)
      {
         pPath->GetPathEdge(i, &edge);

         edge.GetCenter(&edgeCenter);
         temp.x = edgeCenter.x;
         temp.y = edgeCenter.y;
         temp.z = edgeCenter.z;

         r3_transform_point(&pt[i], &temp);
      }

      pPath->GetFinalDestination(&p);
      temp.x = p.x;
      temp.y = p.y;
      temp.z = p.z;
      r3_transform_point(&pt[0], &temp);

      if (count > 1)
      {
         for (i = 0; i < count; i++)
         {
            r3_draw_line(&pt[i], &pt[i+1]);
         }
      }

      r3_end_block();
      r3_end_object();
   }
}

inline void MakeMidPt(const cMxsVector &a, const cMxsVector &b, cMxsVector &c)
{
   c.x = (a.x + b.x) / 2;
   c.y = (a.y + b.y) / 2;
   c.z = (a.z + b.z) / 2;
}

tAIPathCellID AIDebugMagicLocatorCell=0;
BOOL AIDebugCenters;

void AIDrawOnePathCell(tAIPathCellID cell, BOOL proxCull = TRUE);
void AIDrawOneCellsLinks(tAIPathCellID cell, BOOL proxCull = TRUE);

void AIDrawOnePathCell(tAIPathCellID cell, BOOL proxCull)
{
   const             MAX_CELL_LINKS_DRAWABLE = 100;
   static r3s_point  pt[MAX_CELL_LINKS_DRAWABLE];
   static int        ptColor[MAX_CELL_LINKS_DRAWABLE];

   cMxsVector           pt1, pt2, midPt, mid1Pt, mid2Pt;

   int          i, j;
   int          vCount = g_AIPathDB.GetCell(cell)->vertexCount; // vertex count
   const int    pathCellColor = 254; // pink
   mxs_vector   translate;
   mxs_vector   temp;
   int          count;
   int          lineColor[5];

   // is the cell close enough to the player?
   cMxsVector cellCenter;
   g_AIPathDB.GetCenter(cell, &cellCenter);

   if (proxCull)
   {
      ObjID obj = GetCurrentBrushObjID();
      if (obj == OBJ_NULL)
         return;

      Position *pPos =  ObjPosGet(obj);

      if (mx_dist_vec(&cellCenter, &pPos->loc.vec) > 30)
      {
         return;
      }
   }

   lineColor[0] = 254;
   lineColor[1] = guiScreenColor(guiRGB(255,255,255));
   lineColor[2] = guiScreenColor(guiRGB(0,0,255));
   lineColor[3] = guiScreenColor(guiRGB(255,255,255));

   translate.x = 0;
   translate.y = 0;
   translate.z = 0.05;

   r3_start_object(&translate);
   r3_start_block();

   count = 0;
   for (i = 0; i < vCount; i++)
   {
      pt1 = g_AIPathDB.GetCellVertex(cell, i);
      pt2 = g_AIPathDB.GetCellVertex(cell, i + 1);

      MakeMidPt(pt1, pt2, midPt);
      MakeMidPt(pt1, midPt, mid1Pt);
      MakeMidPt(pt2, midPt, mid2Pt);

      temp.x = pt1.x;
      temp.y = pt1.y;
      temp.z = pt1.z;
      r3_transform_point(&pt[count], &temp);
      ptColor[count] = g_AIPathDB.GetVertexInfo(g_AIPathDB.GetCellVertexID(cell, i));
      count++;

      temp.x = mid1Pt.x;
      temp.y = mid1Pt.y;
      temp.z = mid1Pt.z;
      r3_transform_point(&pt[count], &temp);
      //ptColor[count] = g_AIPathDB.GetCellEdgeInfo(cell, i);
      //ptColor[count] = g_AIPathDB.m_Vertices[v1].edgeInfo;
      count++;

      temp.x = mid2Pt.x;
      temp.y = mid2Pt.y;
      temp.z = mid2Pt.z;
      r3_transform_point(&pt[count], &temp);
      ptColor[count] = g_AIPathDB.GetVertexInfo(g_AIPathDB.GetCellVertexID(cell, i + 1));
      count++;
   }

   // draw lines to all vertices...
   /*
   r3_set_color(255);
   for (i = 0; i < vCount; i++)
      for (j = 0; j < vCount; j++)
         r3_draw_line(&pt[i], &pt[j]);
   */

   j = count - 1;
   for (i = 0; i < count; i++)
   {
      r3_set_color(lineColor[ptColor[j]]);

      r3_draw_line(&pt[i], &pt[j]);
      j = i;
   }

   if (cell==AIDebugMagicLocatorCell || AIDebugCenters)
   {
      const sAIPathCell *pCell=g_AIPathDB.GetCell(cell);
      mxs_vector temp, norm;
      temp.x = cellCenter.x; temp.y = cellCenter.y; temp.z = cellCenter.z;
      r3_transform_point(&pt[0], &temp);
      mx_scale_vec(&norm,&g_AIPathDB.GetCellPlane(pCell)->normal,8.0);
      mx_addeq_vec(&temp,&norm);
      r3_transform_point(&pt[1], &temp);
      r3_set_color(guiScreenColor(guiRGB(0x88,0x10,0xc4)));
      r3_draw_line(&pt[0], &pt[1]);
   }

   r3_end_block();
   r3_end_object();
}

enum eAILinkDrawType
{
   kAll,
   kLocomotive,
   kNormalLocomotive
};

static int g_LinkDrawType;

void AIDrawOneCellsLinks(tAIPathCellID cell, BOOL proxCull)
{
   const      MAX_CELL_LINKS_DRAWABLE = 100;
   static r3s_point  pt[MAX_CELL_LINKS_DRAWABLE];
   int        i;
   int        cCount = g_AIPathDB.GetCell(cell)->cellCount;   // cell connection count
   int        connectCellColor = guiScreenColor(guiRGB(0,192,0));
   int        toSmallCellColor = guiScreenColor(guiRGB(255,0,0));
   int        maxExpenseColor = guiScreenColor(guiRGB(255,0,255));
   int        LVLColor = guiScreenColor(guiRGB(255,255,0));
   int        HighStrikeColor = guiScreenColor(guiRGB(252,118,0));
   mxs_vector translate;
   cMxsVector connectCenter;
   mxs_vector temp;

   translate.x = 0;
   translate.y = 0;
   translate.z = 0.05;

   // is the cell close enough to the current brush?
   cMxsVector cellCenter;
   g_AIPathDB.GetCenter(cell, &cellCenter);

   if (proxCull)
   {
      ObjID obj = GetCurrentBrushObjID();
      if (obj == OBJ_NULL)
         return;

      Position *pPos = ObjPosGet(obj);

      if (mx_dist_vec(&cellCenter, &pPos->loc.vec) > 30)
      {
         return;
      }
   }

   if (cCount >= MAX_CELL_LINKS_DRAWABLE)
   {
      Warning (("DrawOneCell: too many cell links cCount=%d!\n", cCount));
      return;
   }

   r3_start_object(&translate);
   r3_start_block();

   temp.x = cellCenter.x;
   temp.y = cellCenter.y;
   temp.z = cellCenter.z;
   r3_transform_point(&pt[0], &temp);

   for (i = 1; i <= cCount; i++)
   {
      ComputeCenterOfVertices(g_AIPathDB.GetCellLink(g_AIPathDB.GetCell(cell)->firstCell + i - 1)->vertex_1,
                              g_AIPathDB.GetCellLink(g_AIPathDB.GetCell(cell)->firstCell + i - 1)->vertex_2,
                              connectCenter);
      temp.x = connectCenter.x;
      temp.y = connectCenter.y;
      temp.z = connectCenter.z;

      r3_transform_point(&pt[i], &temp);
   }


   for (i = 1; i <= cCount; i++)
   {
      if (g_AIPathDB.GetCellLink(g_AIPathDB.GetCell(cell)->firstCell + i - 1)->okBits > 0)
      {
         if (g_LinkDrawType == kLocomotive && g_AIPathDB.GetCellLink(g_AIPathDB.GetCell(cell)->firstCell + i - 1)->okBits & kAIOKCOND_HighStrike)
            continue;
         if (g_LinkDrawType == kNormalLocomotive && g_AIPathDB.GetCellLink(g_AIPathDB.GetCell(cell)->firstCell + i - 1)->okBits & (kAIOKCOND_HighStrike | kAIOKCOND_Stressed))
            continue;

         if (g_AIPathDB.GetCellLink(g_AIPathDB.GetCell(cell)->firstCell + i - 1)->cost < 100)
            {
            if (g_AIPathDB.GetCellLink(g_AIPathDB.GetCell(cell)->firstCell + i - 1)->okBits & kAIOKCOND_Stressed)
               r3_set_color(LVLColor);
            else if (g_AIPathDB.GetCellLink(g_AIPathDB.GetCell(cell)->firstCell + i - 1)->okBits & kAIOKCOND_HighStrike)
               r3_set_color(HighStrikeColor);
            else
               r3_set_color(connectCellColor);
            }
         else
            r3_set_color(maxExpenseColor);

      }
      else
         r3_set_color(toSmallCellColor);

      r3_draw_line(&pt[0], &pt[i]);
   }

   r3_end_block();
   r3_end_object();
}


// make extern so accessible from rendobj.c
extern "C" BOOL g_AIPathDBDrawCellIds = FALSE;
extern "C" BOOL g_AIPathDBDrawCells = FALSE;

static BOOL g_AIPathDBDrawLinks;
int  g_AIPathDBDrawZone = 0;
int  g_AIPathDBDrawZoneType = 0;
int  g_AIPathDBDrawRoom = -1;

void AIPathFindDrawCellLinks()
{
   tAIPathCellID      i;

   if (g_AIPathDBDrawLinks)
      for (i = 1; i <= g_AIPathDB.NumCells(); i++)
         AIDrawOneCellsLinks(i);
}

tAIPathCellID gDrawOneCell;
void AIPathFindDrawPathCells()
{
   tAIPathCellID      i;

   if (!g_AIPathDBDrawCells)
      return;

   if (gDrawOneCell == 0)
   {
      for (i = 1; i <= g_AIPathDB.NumCells(); i++)
         AIDrawOnePathCell(i);
   }
   else
   {
      AIDrawOnePathCell(gDrawOneCell);
   }
}

// Assumes canvas is locked when in HW.
extern "C" void AIPathFindDrawPathCellIds()
{
   int color = 2000; // hard-coded for now. Dunno what this really means - palette or literal, but is a nice color

   tAIPathCellID      i;
   cMxsVector cellCenter;
   char buf[64];
   r3s_point pt;

   if (!g_AIPathDBDrawCellIds)
      return;

   guiStyleSetupFont(NULL,StyleFontNormal);
   gr_set_fcolor(color);
   r3_start_block();

   ObjID obj = GetCurrentBrushObjID();
   if (obj == OBJ_NULL)
      return;

   Position *pPos =  ObjPosGet(obj);


   for (i = 1; i <= g_AIPathDB.NumCells(); i++)
   {
      memset(&pt, 0, sizeof(r3s_point));
      g_AIPathDB.GetCenter(i, &cellCenter);

      if (mx_dist_vec(&cellCenter, &pPos->loc.vec) > 30)
         continue;

      r3_transform_point(&pt,&cellCenter);
      if (pt.ccodes)
         continue;
      sprintf(buf,"%d",i);
      gr_string(buf,fix_int(pt.grp.sx),fix_int(pt.grp.sy));
   }

   r3_end_block();
   guiStyleCleanupFont(NULL,StyleFontNormal);
}


void AIPathFindDrawZone()
{
   if (g_AIPathDBDrawZone == 0)
      return;

   if (g_AIPathDBDrawZoneType < 0)
      g_AIPathDBDrawZoneType = 0;
   else if (g_AIPathDBDrawZoneType > kAIZone_Num)
      g_AIPathDBDrawZoneType = kAIZone_Num-1;

   if (g_AIPathDBDrawZone == -1)
   {
      // Find cell for selected brush

      ObjID obj = GetCurrentBrushObjID();
      if (obj == OBJ_NULL)
         return;

      Position *pPos = ObjPosGet(obj);
      tAIPathCellID cellID = AIFindClosestCell(pPos->loc.vec, 0, obj);

      if (cellID == 0)
         return;

      tAIPathZone   zone = g_AIPathDB.GetCellZone((eAIPathZoneType)g_AIPathDBDrawZoneType, cellID);
      tAIPathCellID i;

      for (i=1; i <= g_AIPathDB.NumCells(); i++)
      {
         if (g_AIPathDB.GetCellZone((eAIPathZoneType)g_AIPathDBDrawZoneType, i) == zone)
         {
            AIDrawOnePathCell(i, FALSE);
            AIDrawOneCellsLinks(i, FALSE);
         }
      }
   }
   else
   {
      tAIPathCellID i;

      for (i = 1; i <= g_AIPathDB.NumCells(); i++)
      {
         if (g_AIPathDB.GetCellZone((eAIPathZoneType)g_AIPathDBDrawZoneType, i) == g_AIPathDBDrawZone)
         {
            AIDrawOnePathCell(i, FALSE);
            AIDrawOneCellsLinks(i, FALSE);
         }
      }
   }
}

void AIPathFindDrawRoom()
{
   if (g_AIPathDBDrawRoom < 0)
      return;

   g_pAIRoomDB->UseValidChecks(TRUE);
   g_pAIRoomDB->MarkRoomCells(g_AIPathDBDrawRoom);

   for (tAIPathCellID i = 1; i <= g_AIPathDB.NumCells(); i++)
   {
      if (g_pAIRoomDB->IsCellValid(i))
      {
         AIDrawOnePathCell(i, FALSE);
         AIDrawOneCellsLinks(i, FALSE);
      }
   }

   g_pAIRoomDB->UnMarkRoomCells(g_AIPathDBDrawRoom);
   g_pAIRoomDB->UseValidChecks(FALSE);
}

void AIPathFindSpewZone()
{
   if (g_AIPathDBDrawZoneType < 0)
      g_AIPathDBDrawZoneType = 0;
   else if (g_AIPathDBDrawZoneType > kAIZone_Num)
      g_AIPathDBDrawZoneType = kAIZone_Num-1;

   ObjID obj = GetCurrentBrushObjID();
   if (obj == OBJ_NULL)
   {
      Warning(("AIPathFindSpewZone: no object brush selected!\n"));
      return;
   }

   Position *pPos = ObjPosGet(obj);
   tAIPathCellID cellID = AIFindClosestCell(pPos->loc.vec, 0, obj);

   if (cellID == 0)
   {
      Warning(("AIPathFindSpewZone: couldn't find closest cell to object!\n"));
      return;
   }

   tAIPathZone zone = g_AIPathDB.GetCellZone((eAIPathZoneType)g_AIPathDBDrawZoneType, cellID);

   mprintf("Object %d in zone %d\n", obj, zone);
}

void AIPathFindSpewZones()
{
   int i, j;

   if (g_AIPathDBDrawZoneType < 0)
      g_AIPathDBDrawZoneType = 0;
   else if (g_AIPathDBDrawZoneType > kAIZone_Num)
      g_AIPathDBDrawZoneType = kAIZone_Num-1;

   mprintf("Zone Connectivity:\n");
   for (i=0; i<g_AIPathDB.m_ZoneDatabases[g_AIPathDBDrawZoneType].m_nZones; i++)
   {
      for (j=0; j<g_AIPathDB.m_ZoneDatabases[g_AIPathDBDrawZoneType].m_nZones; j++)
      {
         if (i == j)
            continue;

         tAIPathOkBits okBits = g_AIPathDB.GetZoneOkBits((eAIPathZoneType)g_AIPathDBDrawZoneType, i, j);
         if (okBits > 0)
            mprintf("[%d - %d] %d\n", i, j, okBits);
      }
   }

   mprintf("Cell Zones:");
   for (i=0; i<g_AIPathDB.m_nCells; i++)
   {
      mprintf("[%d] %d\n", i, g_AIPathDB.GetCellZone((eAIPathZoneType)g_AIPathDBDrawZoneType, i));
   }
}

// actual call from editor
extern "C" void AIPathFindDrawDB()
{
   if (g_fAIPathFindInited)
   {
      AIPathFindDrawPathCells();
      AIPathFindDrawCellLinks();
      AIPathFindDrawPathCellIds();
      AIPathFindDrawZone();
      AIPathFindDrawRoom();

      if (AIDebugMagicLocatorCell)
         AIDrawOnePathCell(AIDebugMagicLocatorCell);
   }
}


// toggle rendering of
void AIPathDrawCellsToggle()
{
   g_AIPathDBDrawCells ^= TRUE;
   if (g_AIPathDBDrawCells)
      Status("Now drawing AI path cells.");
   else
      Status("No longer drawing AI path cells.");
#ifdef EDITOR
   redraw_all_cmd();
#endif
}

void AIPathDrawCellIdsToggle()
{
   g_AIPathDBDrawCellIds ^= TRUE;
   if (g_AIPathDBDrawCellIds)
      Status("Now drawing AI path cell ids.");
   else
      Status("No longer drawing AI path cell ids.");
#ifdef EDITOR
   redraw_all_cmd();
#endif
}

void AIPathDrawLinksToggle(int i)
{
   if (!i)
      g_AIPathDBDrawLinks ^= TRUE;
   else
      g_AIPathDBDrawLinks = TRUE;

   if (i)
      g_LinkDrawType = i-1;

   if (g_AIPathDBDrawLinks)
      Status("Now drawing AI path cell links.");
   else
      Status("No longer drawing AI path cell links.");


#ifdef EDITOR
   redraw_all_cmd();
#endif
}

// debug info
void AIDebugDumpCellVertexInfo(int cell_id)
{
   const sAIPathCell *pCell=g_AIPathDB.GetCell(cell_id);
   const mxs_vector * p;
   char buf[120];
   int i;

   mprintf("Cell %d has %d vertices\n",cell_id,pCell->vertexCount);
   for (i = 0; i < pCell->vertexCount; i++)
   {  // really should ping-pong these two - but im lazy
      p=&g_AIPathDB.GetCellVertex(cell_id, i);
      sprintf(buf,"%f %f %f\n",p->x,p->y,p->z);
      mprintf(buf);
   }
}

// debug info
void AIDebugDumpCellConnectionInfo(int cell_id)
{
   const sAIPathCell *pCell=g_AIPathDB.GetCell(cell_id);
   int          i, has_invalid=0;

   const tAIPathCellIDPacked iFirstCellID = pCell->firstCell;
   const tAIPathCellIDPacked iFinalCellID = (tAIPathCellIDPacked)(iFirstCellID+pCell->cellCount);

   mprintf("Cell %d: %d connections: ",cell_id,pCell->cellCount);
   for (i=iFirstCellID; i<iFinalCellID; i++)
      if (g_AIPathDB.GetCellLink(i)->okBits)
         mprintf("%d ",g_AIPathDB.GetCellLink(i)->dest,g_AIPathDB.GetCellLink(i)->okBits);
      else has_invalid++;
   if (has_invalid)
   {
      mprintf("invalid ");
      for (i=iFirstCellID; i<iFinalCellID; i++)
         if (g_AIPathDB.GetCellLink(i)->okBits==0)
            mprintf("%d ",g_AIPathDB.GetCellLink(i)->dest);
   }
   mprintf("\n");
}

///////////////////////////////////////

void ValidateAllCellRooms(BOOL linkedOnly, cDynArray<tAIPathCellID> * pFailures)
{
   cDynArray<BOOL> test(g_AIPathDB.m_nCells + 1);
   int cell;
   int i;

   memset(test, 0, test.Size() * sizeof(BOOL));

   if (linkedOnly)
   {
      for (cell = 1; cell <= g_AIPathDB.m_nCells; cell++)
      {
         // and gather arcs to all the cells it's connected to with centers within range
         tAIPathCellID i;
         const int lastCell = g_AIPathDB.m_Cells[cell].firstCell + g_AIPathDB.m_Cells[cell].cellCount;

         for (i = g_AIPathDB.m_Cells[cell].firstCell; i < lastCell; i++)
         {
            if (g_AIPathDB.m_Links[i].okBits)
               test[g_AIPathDB.m_Links[i].dest] = TRUE;
         }
      }
   }

   g_pAIRoomDB->MarkAllRoomCells();
   g_pAIRoomDB->UseValidChecks(TRUE);

   for (cell = 1; cell <= g_AIPathDB.m_nCells; cell++)
   {
      if ((!linkedOnly || test[cell]) && !g_pAIRoomDB->IsCellValid(cell))
         pFailures->Append(cell);
   }

   g_pAIRoomDB->UseValidChecks(FALSE);
   g_pAIRoomDB->UnMarkAllRoomCells();

   if (linkedOnly)
   {
      const cAIArray & AIs = g_pAIManager->AccessAIs();
      cDynArray<mxs_vector> Locs;
      cDynArray<tAIPathCellID> Cells;

      for (i = 0; i < AIs.Size(); i++)
      {
         tAIPathCellID curCell = AIFindClosestCell(*GetObjLocation(AIs[i]->GetObjID()), 0);
         if (curCell)
         {
            Locs.Append(*GetObjLocation(AIs[i]->GetObjID()));
            Cells.Append(curCell);
         }
      }

      BOOL reach;

      for (i = pFailures->Size() - 1; i >= 0; i--)
      {
         reach = FALSE;
         for (int j = 0; j < Locs.Size(); j++)
         {
            IAIPath * pPath = AIPathfind(Locs[j], Cells[j], g_AIPathDB.GetCenter((*pFailures)[i]), (*pFailures)[i]);
            if (pPath)
            {
               reach = TRUE;
               pPath->Release();
               break;
            }
         }
         if (!reach)
            pFailures->FastDeleteItem(i);
      }
   }

}

///////////////////////////////////////////////////////////////////////////////
