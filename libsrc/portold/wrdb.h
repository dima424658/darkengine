/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/libsrc/portold/wrdb.h,v 1.20 1997/10/27 18:10:28 MAT Exp $
//
// World Representation Database

#ifndef __WRDB_H
#define __WRDB_H

#include <lg.h>
#include <matrix.h>
#include <r3ds.h>

#ifdef __cplusplus
extern "C"
{
#endif


typedef mxs_vector Vertex;
typedef mxs_vector Vector;

//   A cached relative vector
typedef struct st_CachedVector
{
   // 16 bytes
   Vector raw;
   struct st_CachedData *cached;
} CachedVector;


typedef struct st_CachedData
{
   // 32 bytes
   Vector viewspace;
   float dot1, dot2;
   struct st_CachedVector *next;  // list of cached ones
   CachedVector *back;
   int dummy1;
} CachedData;


///////////////////////////////////////////
//
//   WorldRep BSP tree structure
//   
//     This BSP tree is generated from the CSG BSP tree just before 
//     it emits cells.  It keeps track of cells split during emission 
//     (caused by too many polys).  It is the BSP tree used at runtime.

typedef struct wrBspNode
{
   bool        leaf;             // Is it a leaf?

   char        medium;           // Medium of leaf
   bool        mark;             // Markable for fast rendering traversal
   char        padding;          // Padding out to 4 byte boundary
   int         cell_id;          // Index to reference cell (-1 if !leaf)

   mxs_plane         plane;      // Split plane
   struct wrBspNode *inside;     // Children (NULL if !leaf) 
   struct wrBspNode *outside;

   // General
   struct wrBspNode *parent;     // Parent (NULL if root)
} wrBspNode;
// 36 bytes


///////////////////////////////////////////
//
//   Polygon structures
//
//     Polygons can be either walls or portals.
//     The core polygon structure holds data common
//     to both, and an additional structure holds
//     the rest of the wall information.
//
//     Portals also have some specialized information.
//     So we use a union to overlap storage required
//     for portals.
//     Well, they don't yet, but maybe they will.
//
//     Also, since there are sometimes coplanar planes,
//     we separate out those planes into a separate list.
//     This saves storage; even if all planes are unique,
//     we only use one byte per polygon to index the plane,
//     and that would have been a byte of padding anyway.

typedef struct
{
   CachedVector *norm;
   float plane_constant;
} PortalPlane;


//  Polygon data that all polygons (including portals) have
typedef struct
{
   uchar flags;
   uchar num_vertices;
   uchar planeid;
   uchar clut_id;       // for portals, the clut to draw with, 0 means none
   ushort destination;  // for portals, the destination cell
   uchar motion_index;  // index into position array for moving polygons
   uchar padding;
} PortalPolygonCore;         // 8 bytes


//  Polygon data necessary for rendering
typedef struct
{
   CachedVector *u, *v;
   ushort u_base,v_base;    // SFIX u&v values at the anchor vertex
   uchar texture_id;        // which texture to paint
   uchar texture_anchor;    // which vertex to anchor the texture to
   short cached_surface;

   mxs_real texture_mag;    // used in finding MIP level
   mxs_vector center;       // also used in finding MIP level
} PortalPolygonRenderInfo;  // 32 bytes


typedef struct
{
   r3s_texture decal;
   uint offset;
} PortalDecal;


// basic light map storage
typedef struct 
{
   short base_u, base_v; // these can be negative, they indicate top
                         // left of rectangle
   short row;            // bitmapesque data
   uchar h, w;

   uchar *bits;          // the actual data--the static lightmap comes first,
                         // and then the bits for all the animated lightmaps,
                         // in the order in which they appear in the cell's
                         // palette
   uchar *dynamic;       // if it's dynamically lit, this has the new data

   PortalDecal *decal;   // any decal overlays
   uint anim_light_bitmask; // Which animated lights reaching this cell
                         // affect this polygon?  The bits match the
                         // anim light palette of the cell.
} PortalLightMap; // 24 bytes


////
//
//  POLYGON FLAGS
//
//    PORTAL_  is only paid attention to for portals
//    RENDER_  is only paid attention to by the renderer
//    POLYGON_ applies to all polygons

#define PORTAL_BLOCKS_VISION     1
#define PORTAL_BLOCKS_PHYSICS    2
#define RENDER_DOESNT_LIGHT      4   // generally self-lit
#define RENDER_NO_PHYSICS        8   // ignore for physics, typically a decal
#define PORTAL_SPLITS_OBJECT    16
#define RENDER_REBUILD_SURFACE  32   // when lighting or texture has changed
                                     // (not used for one-frame dynamic light)


////
//
//  CELL FLAGS
//

#define CELL_RENDER_WIREFRAME       1
#define CELL_RENDER_WIREFRAME_ONCE  2

// This is for any function traversing the cell database--and it
// should clear it when it's done!
#define CELL_TRAVERSED              4

// Cells which are part of doorways should be in small, contiguous
// clusters, and wear funny little hats, like fezzes with earflaps.
// The flags for such groups of cells are altered using
// PortalBlockVision() and PortalUnblockVision() and such in wrfunc.
#define CELL_BLOCKS_VISION          8
#define CELL_CAN_BLOCK_VISION       16

///////////////////////////////////////////
//
//   Cell structures
//
// For each cell, the polygon data is arranged in the following order:
//
//   polygons which are opaque (not portals) and solid (to physics)
//   polygons which are either transparent or not solid (rendered portals)
//   polygons which are non-rendered portals
//
// The first class of polygons can never be treated differently, because
// they have no portal information.  The exact rendering behavior of a
// given polygon in the second class is determined by its texture id;
// if it is opaque, then the wall is opaque.  The flags field indicates
// the exact physics behavior, which allows portals to be invisible walls.

typedef struct port_render_data PortalCellRenderData;

typedef struct
{
   // 4
   uchar num_vertices;              // size of vertex pool for this cell
   uchar num_polys;                 // total number of all polygons here
   uchar num_render_polys;          // number of renderable polygons
   uchar num_portal_polys;          // number of portals
             // since some portals are rendered, it's allowable
             // for   num_render + num_portal != num_polys

   // 4
   uchar num_planes;
   uchar medium;
   uchar flags;
   uchar spare_byte_1;

   // 16
   Vertex *vpool;
   PortalPolygonCore *poly_list;          // beginning of all polygons
   PortalPolygonCore *portal_poly_list;   // beginning of second type of polygons
   PortalPolygonRenderInfo *render_list;  // beginning of renderable polygon data

   // 8
   uchar *vertex_list;              // beginning of all polygons per-poly vertex data
   int portal_vertex_list;          // offset of second type of polygons per-poly vertex list

   // 20
   wrBspNode *leaf;                 // ptr to leaf of worldrep BSP tree
   PortalPlane *plane_list;         // plane equations for all unique planes
   PortalCellRenderData *render_data;// data used by renderer while rendering
   void *refs;
   ushort num_vlist;                // number of vertices/polygon
   uchar num_anim_lights;
   uchar motion_index;

   // 16
   uint changed_anim_light_bitmask; // bitfield--which lights have changed
   ushort *anim_light_index_list;   // palette of anim lights
   PortalLightMap *light_list;      // num_render_polys worth of light maps
   ushort *light_indices;           // which lights shine on this cell

   // 16
   Vertex sphere_center;
   mxs_real sphere_radius;

} PortalCell;   // 84 bytes


// for that axis field, below--gives the axis not considered
#define MEDIUM_AXIS_X 0
#define MEDIUM_AXIS_Y 1
#define MEDIUM_AXIS_Z 2 // for water

// This "motion" structure really only tells us how to draw the
// texture on the medium boundary in the current frame.  Moving with
// time is handled through a callback.
typedef struct {
   // This is the point around which the water rotates, in world space.
   // The z is ignored since we're projecting the texture in (x, y).
   mxs_vector center;

   // The water is turning.  Or it can be.
   mxs_ang angle;

   // If this is 0, a medium boundary with this index is rendered
   // normally, with none of this motion shit.
   BOOL in_motion;
   uchar major_axis;
   uchar pad_1;
   uchar pad_2;
} PortalCellMotion;


// A given cell is stored as a contiguous block of memory.  The pointers
// are swizzled at load time to point to the right places.  So the memory
// is laid out like this:
//      1 PortalCell (52 bytes)
//     ~m PortalPlane (8 * ~m, ~m == number of unique planes)
//      m PortalPolygonCore (16 * m, m == num_total_polys)
//      k PortalPolygonRenderInfo (8 * k, k == num_render_polys)
//      n Vertex (12 * n, n == num_vertices)
//      p uchars vertex_pool (p bytes, p = sum of number of vertices per poly)
//      q uchars vertex_pool_lighting (q bytes, q = sum of number of vertices per render poly)
// (dynamic lighting is dynamically allocated q more bytes)

// This probably puts a level somewhere between 200K and 1M, depending on complexity
// A pretty big portion of that is the vertices.  Maybe we should share vertices--
// we make one big vertex pool with all vertices, and change the vpool to being
// indices into that pool, saving 10*n bytes.  We can still skip caching points
// across cells--just immediately build them all directly.  Hmm.

#ifdef __cplusplus
};
#endif

#endif
