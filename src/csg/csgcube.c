/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/csg/csgcube.c,v 1.2 2000/02/19 12:54:57 toml Exp $

#include <lg.h>
#include <csg.h>
#include <bspdata.h>
#include <csgalloc.h>
#include <csgutil.h>
#include <csgcheck.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

static BspPlane cube_plane[6] =
{
   {  0,  0, -1,  1 },
   {  0, -1,  0,  1 },
   { -1,  0,  0,  1 },
   {  0,  0,  1,  1 },
   {  0,  1,  0,  1 },
   {  1,  0,  0,  1 }
};

static PortalEdge cube_edge[12] =
{
   { {  1, 1, 1 }, {  1,-1, 1 } },
   { {  1,-1, 1 }, { -1,-1, 1 } },
   { { -1,-1, 1 }, { -1, 1, 1 } },

   { { -1, 1, 1 }, {  1, 1, 1 } },
   { {  1, 1, 1 }, {  1, 1,-1 } },
   { {  1, 1,-1 }, { -1, 1,-1 } },

   { { -1, 1,-1 }, { -1, 1, 1 } },
   { {  1, 1,-1 }, {  1,-1,-1 } },
   { {  1,-1,-1 }, {  1,-1, 1 } },

   { { -1,-1,-1 }, {  1,-1,-1 } },
   { { -1,-1,-1 }, { -1, 1,-1 } },
   { { -1,-1,-1 }, { -1,-1, 1 } }
};

static cube_poly_edgelist[6][4] =
{
   { 0,1,2,3 },         //  0 1 2 3
   { 4,5,6,3 },         //  3 4 5 6      3
   { 4,7,8,0 },         //  0 4 7 8      0 4
   { 7,9,10,5 },        //  5 7 9 10     5 7
   { 8,9,11,1 },        //  1 8 9 11     1 8 9
   { 6,10,11,2 }        //  2 6 10 11    2 6 10 11
};

PortalPolyhedron *PortalMakeCube(Real size)
{
   // construct a cube.  Note that this is wacky
   // since there will be one-sided polygons

   // there are 6 polygons, 12 edges, and each edge
   // belongs to two polygons so 24 polyedges.

   PortalPolyhedron *ph;
   PortalPolygon *poly[6];
   PortalEdge *edge_data[12];
   PortalPolyEdge *edge_head[12], *edge;

   int i,j,k;

   ph = PortalPolyhedronAlloc();
   for (i=0; i < 6; ++i) {
      poly[i] = PortalPolygonAlloc();
      poly[i]->ph[0] = ph;
      poly[i]->ph[1] = 0;
      poly[i]->edge = 0;
      poly[i]->misc = 0;
      poly[i]->plane = &cube_plane[i];
      poly[i]->plane->d = size;
      if (i) poly[i]->ph_next[0] = poly[i-1];
   }
   poly[0]->ph_next[0] = poly[5];
   ph->poly = poly[0];
   ph->leaf = 0;

   for (i=0; i < 12; ++i) {
      edge_data[i] = PortalEdgeAlloc();
      edge_data[i]->start.x = cube_edge[i].start.x * size;
      edge_data[i]->start.y = cube_edge[i].start.y * size;
      edge_data[i]->start.z = cube_edge[i].start.z * size;
      edge_data[i]->end.x   = cube_edge[i].end.x * size;
      edge_data[i]->end.y   = cube_edge[i].end.y * size;
      edge_data[i]->end.z   = cube_edge[i].end.z * size;
      edge_head[i] = 0;
   }

   for (i=0; i < 6; ++i) {
      for (j=0; j < 4; ++j) {
          k = cube_poly_edgelist[i][j];
          edge = PortalPolyEdgeAlloc();
          edge->data = edge_data[k];
          edge->poly = poly[i];

          // if there's already one, make them point to each other
          // (this isn't quite circular-link general since I know there are two
          if (edge_head[k]) {
             edge_head[k]->edge_next = edge;
             edge->edge_next = edge_head[k];
          } else
             edge_head[k] = edge;

          if (j == 0) {
             edge->poly_next = edge;
             poly[i]->edge = edge;
          } else {
             edge->poly_next = poly[i]->edge->poly_next;
             poly[i]->edge->poly_next = edge;
          }
      }
   }

#ifdef DBG_ON
   CheckPolyhedron(ph, "(starting cube)");
#endif

   return ph;
}

