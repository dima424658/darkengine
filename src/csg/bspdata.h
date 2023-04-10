/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/csg/bspdata.h,v 1.7 2000/01/29 12:58:23 adurant Exp $
#pragma once

#ifndef BSPDATA_H
#define BSPDATA_H

#include <csg.h>
#include <hep.h>
#include <string.h>
#include <csgbrush.h>

#define MAX_FACE_BRUSHES  MAX_CSG_BRUSHES
#define MAX_FACES         16

#define VEC_DOT_ONE .99999
#define PLANE_CONST_EPSILON .0005

#define VERTEX_COMPARE(x,y)   memcmp(&(x), &(y), 3*sizeof(Real))

// result values for a plane/volume crossing a plane
enum {
   COPLANAR = 0,
   IN_FRONT = 1,
   BEHIND = 2,
   CROSS = 3,
   COPLANAR_EDGE = 4
};
//  CROSS == IN_FRONT | BEHIND

extern Hep Nodes;

typedef struct BspPlane
{
   Real a,b,c;  // normal to the plane
   Real d;      //   plane equation:   ax + by + cz + d = 0
} BspPlane;  // 16 or 32 bytes

typedef struct bsp_node
{
 // 4 bytes
   char leaf;
   char mark;
   signed char medium;
   char dummy;

 // 4 bytes
   int cell_id;

 // 8 bytes
   void *user_data;
   void *ph;   // polyhedron storage during portalization

// 16 bytes
   BspPlane split_plane;

// 8 bytes
   struct bsp_node *inside;
   struct bsp_node *outside;

   struct bsp_node *parent;
} BspNode;


/////
//
//  Accessors

#define IS_MARKED(b)   ((b)->mark)
#define IS_LEAF(b)     ((b)->leaf)

#define MARK(b)        ((b)->mark = TRUE)
#define UNMARK(b)      ((b)->mark = FALSE)

typedef struct PortalEdge PortalEdge;
typedef struct PortalPolyEdge PortalPolyEdge;
typedef struct PortalPolygon PortalPolygon;
typedef struct
{
   PortalPolygon *poly;
   BspNode *leaf;
} PortalPolyhedron;

struct PortalEdge
{
   BspVertex start, end;
};

struct PortalPolyEdge
{
   PortalPolygon *poly;
   PortalPolyEdge *poly_next;
   PortalPolyEdge *edge_next;
   PortalEdge *data;
};

typedef struct poly_info pinfo;

struct PortalPolygon
{
   PortalPolyhedron *ph[2];
   PortalPolygon *ph_next[2];
   PortalPolyEdge *edge;
   BspPlane *plane;
   pinfo *misc;
   int brface;
};

extern void dump_plane(BspPlane *p);
extern void dump_node(BspNode *b, int n, int inf);
extern void dump_tree(void *tree);
extern void BspChangeLeafToNode(BspNode *b, BspPlane *p);
extern BspNode *BspAllocateLeaf();
extern void (*CSGfree)(void *);
extern void BspFreeLeaf(BspNode *b);
extern void BspRecursiveFreeNode(BspNode *b);

#endif
