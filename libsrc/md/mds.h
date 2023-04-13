/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

/*
 * $Source: x:/prj/tech/libsrc/md/RCS/mds.h $
 * $Revision: 1.22 $
 * $Author: alique $
 * $Date: 1970/01/01 00:00:00 $
 *
 * Structure definitions for the Model library
 *
 */


#ifndef __MDS_H
#define __MDS_H
#pragma once

#include <matrixs.h>
#include <r3ds.h>

// Current MD version number
#define MD_CUR_VER 4
// Current backwards compatible number
#define MD_COMPATIBLE_VER 3

// Enable backface intersection checking
#define BACKFACE_INTERSECTION

// New thing we use for parms
typedef struct mds_parm {
   union {
      mxs_ang ang;
      float slide;
   };
} mds_parm;

typedef struct mds_sphere {
   mxs_vector cen;
   float rad;
} mds_sphere;

// Model header, the big guy
typedef struct mds_model {
   char        id[4];   // name of model "LGMD"
   uint        ver;     // version of bsp that produced it
   char        name[8]; // name of model
   float       radius;  // bounding sphere radius, zero centered
   float       max_pgon_radius;  // maximum polygon radius
   mxs_vector  bmax; // bbox maximum
   mxs_vector  bmin; // bbox minimum
   mxs_vector  pcen;  // centroid pos relative to parent
   ushort pgons;  // number of pgons
   ushort verts;  // number of verts
   ushort parms;  // max id of parms
   ubyte mats;    // number of materials
   ubyte vcalls;  // number of vcalls
   ubyte vhots;   // number of hot spots
   ubyte subobjs;	// number of subobjs

   ulong subobj_off;	// relative to start of the model, used to generate pointers
   ulong mat_off;    // offset of materials
   ulong uv_off;
   ulong vhot_off;
   ulong point_off;
   ulong light_off;
   ulong norm_off;
   ulong pgon_off;
   ulong node_off;
   ulong mod_size;   // size of the model
   // Next time, make materials field expandable...
   ulong mat_flags;     // which features are used, trans, illum, etc.
   ulong amat_off;      // offset of auxilliary material info
   ulong amat_size;     // size of new material, never assume size of struct

   ulong  mesh_off;
   ulong  submeshlist_off;
   ushort meshes;
} mds_model;

// Subobjects
// Each subobject header contains the starting index and number of
// subobject specific thing

enum _mde_sub {
   MD_SUB_NONE,   // no strangeness in subobject, no rotation or sliding
   MD_SUB_ROT,	   // rotating subobject, use parameter
   MD_SUB_SLIDE   // sliding subobject, use parameter
};
typedef unsigned char mde_sub;

typedef struct mds_subobj {
   char name[8];	// name of subobject
   mde_sub type;	// type of subobj rot, or slide
   int parm;		// which parameter it uses (can share)
   float min_range;	// range of motion
   float max_range;	// range of motion
   mxs_trans trans;  // transform to put through after rotation
   short child;      // First child object, if any, -1 if none
   short next;       // next child object in list, -1 if none
   ushort vhot_start;
   ushort vhot_num;
   ushort point_start;	// index of starting point
   ushort point_num;	// number of points in that subobject
   ushort light_start;  // relative to start of list
   ushort light_num;
   ushort norm_start;   // relative to start of list
   ushort norm_num;
   ushort node_start;	// relative to start of list
   ushort node_num;     // just so we know
} mds_subobj;


// This is the structure for a material.  Maybe someday it should include
// more information like  shininess and things like that.  That's more a
// feature for lighting.


enum _mde_mat {
   MD_MAT_TMAP,
   MD_MAT_COLOR
};
typedef unsigned char mde_mat;

typedef struct mds_mat {
   char  name[16];	// name of the material, for viewer craziness
   mde_mat type;    // type 0 = texture, 1 = virtual color
   uchar num;     // slot to put it in
   ulong handle;  // texture handle or argb
   union {
      float uv;   // uv coords per 3d unit for mipmapping or ipal index
      ulong ipal; // inverse pal lookup
   };
} mds_mat;        // material info

// Which features are used by the model as a whole, these get or'd
// in mat_flags
// Do we have translucency?
#define MD_MAT_TRANS 0x1
// Do we have self illumination?
#define MD_MAT_ILLUM 0x2

// Auxilliary information for material
typedef struct mds_amat {
   float trans; // translucency 0-1
   float illum; // self illumination, 0-1
   float MaxTU,MaxTV; //mipmap max texel size
} mds_amat;


// UVs Wow, is this structure uninteresting
typedef struct mds_uv {
   float u;
   float v;
} mds_uv;

// Structure for a vhot, not too exciting
typedef struct mds_vhot {
   ulong       id;  // id of vhot
   mxs_vector  v;   // location rel to obj
} mds_vhot;       // vhot info

// Each element, x,y,z has 1 bit of sign, 1 bit of integer, and 8 bits
// of fraction, for 10 bytes total.  So the x, y, and z values of each
// lighting normal can be extracted in the following manner:

#define X_NORM(norm) ((short)((norm>>16)&0xFFC0))/16384.0
#define Y_NORM(norm) ((short)((norm>>6)&0xFFC0))/16384.0
#define Z_NORM(norm)  ((short)((norm<<4)&0xFFC0))/16384.0

typedef struct mds_light {
   ushort mat;    // material index
   ushort point;	// index of of vertex
   ulong norm;	   // compacted normal
} mds_light;


// lighting callback for per object setup
typedef void (*mdf_light_setup_cback)(mxs_vector *bmin,mxs_vector *bmax);

// lighting callback for setting the lighting
typedef void (*mdf_light_cback)(int num,float *i,mds_light *lts,mxs_vector *pts,r3s_point *tpts);
typedef void (*mdf_mesh_light_cback)(int num,float *ivals,mds_light *lights);

// subobject callback
typedef void (*mdf_subobj_cback)(mds_subobj *s);

#define MD_OBJ_PUSH 0
#define MD_OBJ_POP 1

typedef void (*mdf_light_obj_cback)(int p);

// Polygons.  Variable length structure

#define MD_PGON_PRIM_MASK  0x07
#define MD_PGON_LIGHT_ON   0x18
#define MD_PGON_COLOR_MASK 0x60

#define MD_PGON_PRIM_NONE    0  // no primitive drawn
#define MD_PGON_PRIM_SOLID   1  // vcolor lookup
#define MD_PGON_PRIM_WIRE    2  // wire
#define MD_PGON_PRIM_TMAP    3  // texture map

#define MD_PGON_COLOR_PAL  0x20  // palette color
#define MD_PGON_COLOR_VCOL 0x40  // vcolor lookup

typedef struct mds_pgon {
   ushort   index;   // absolute index of pgon
   ushort   data;    // color or tmap indice
   ubyte    type;    // bit field
   ubyte    num;     // number of verts
   ushort   norm;    // index of normal of pgon (in object space)
   float    d;       // plane equation coefficient to go with normal
   ushort   verts[]; // vertex indices, then light indices, then uv
                     // indices, optionally
} mds_pgon;

// Auxilliary info for the pgon.. like material
// occurs after verts, has extra info
typedef struct mds_pgon_aux {
   ubyte   mat; // which material
} mds_pgon_aux;


// The node list is still relatively similar to an interpreter, in that there are many types of nodes, and
// basically they are interpreted based on type.  Oh well.  I weep.

enum _mde_node {
   MD_NODE_RAW,
   MD_NODE_SPLIT,
   MD_NODE_CALL,
   MD_NODE_VCALL,
   MD_NODE_SUBOBJ
};
typedef unsigned char mde_node;

typedef struct mds_node_raw {
   mde_node type;       // type of node
   mds_sphere  sphere;  // bounding sphere of node
   ushort   num;		   // number of polys
   ushort   polys[];    // polygon list
} mds_node_raw;

typedef struct mds_node_split {
   mde_node type;             // type of node
   mds_sphere sphere;      // bounding sphere
   ushort   pgons_before;	// number of polygons to render before split
   ushort   norm;	   	   // index of polygon normal
   float    d;		         // plane equation d for normal
   ushort   node_behind;	// offset of node in back of the plane
   ushort   node_front;	   // offset of node in front of the plane
   ushort   pgons_after;	// number of polygons to render after split
   ushort   polys[];
} mds_node_split;

typedef struct mds_node_call {
   mde_node type;
   mds_sphere  sphere;
   ushort   pgons_before;	// number of polygons to render before call
   ushort   node_call;	// offset of node to jump to
   ushort   pgons_after;		// number of polygons to render after call
   ushort   polys[];
} mds_node_call;

typedef struct mds_node_vcall {
   mde_node type;
   mds_sphere  sphere;
   ushort   index;	// index of vcall to call
} mds_node_vcall;

typedef struct mds_node_subobj {
   mde_node type;
   ushort   index;	// which subobj we are starting, this is followed by the next node
} mds_node_subobj;

typedef struct mds_segment_hit_detect_info {
   mxs_vector *a, *b;         // fill in before calling md_segment_hit_detect()
   bool early_out;            // fill in before calling md_sphere_hit_detect()
   mxs_vector *hit_normal;    // set by md_segment_hit_detect()
   mxs_real r;                // set by md_segment_hit_detect()
   bool hit;                  // set by md_segment_hit_detect()
   mxs_vector *verts, *norms; // used internally by md_segment_hit_detect()
   uchar *pgons;              // used internally by md_sphere_hit_detect()
   uchar *nodes;              // used internally by md_sphere_hit_detect()
} mds_segment_hit_detect_info;

typedef struct mds_sphere_hit_detect_info {
   mds_sphere sphere;         // fill in before calling md_sphere_hit_detect()
   bool early_out;            // fill in before calling md_sphere_hit_detect()
   ushort *polys;             // allocate before calling md_sphere_hit_detect()
   bool hit;                  // set by md_sphere_hit_detect()
   ushort num;                // set by md_sphere_hit_detect()
   mxs_vector *verts, *norms; // used internally by md_sphere_hit_detect()
   uchar *pgons;              // used internally by md_sphere_hit_detect()
   uchar *nodes;              // used internally by md_sphere_hit_detect()
} mds_sphere_hit_detect_info;

// function type that gets recurred through
typedef void (* mdf_subrecur)(int i);


typedef enum {
MD_PGON_SOLID, // vcolor lookup
MD_PGON_WIRE,  // wire
MD_PGON_TMAP
} mde_pgon;

typedef struct mds_plyinfo {
   mde_pgon   type;
   ushort     data;
   ushort     normi;
   float      d;
} mds_plyinfo; //mesh polygon info


typedef struct mds_mesh {
   ushort       pgons;   // number of pgons
   ushort       verts;   // number of verts
   ushort       mat;     // index to material 
   mxs_vector  *pnts;    // vertices
   float       *uvs;     // uvs
   mds_light   *lights;  // vertex normals
   mxs_vector  *normals; // poly normals
   short       *trilist; // triangle indice
   mds_plyinfo *ply_info;// polygon info  
   float       *ivals;   // light values
} mds_mesh; // Mesh struct

typedef struct SubObMeshList {
   ushort      meshnum;
   ushort      startpnt,numpnts;
   ushort      startply,pgons;
} SubObMeshList;
typedef struct SubOMeshList {
  SubObMeshList *subobj;
  ushort        numMeshes;
} SubOMeshList;



#endif // __MDS_H