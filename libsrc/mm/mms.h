// $Header: x:/prj/tech/libsrc/mm/RCS/mms.h 1.4 1998/03/26 08:43:08 MAT Exp $

#ifndef __MMS_H
#define __MMS_H

#include <matrixs.h>

#define MM_CUR_VER 1

enum _mme_layout
{
   MM_LAYOUT_MAT,
   MM_LAYOUT_SEG,
};
typedef uchar mme_layout;

// Model header, where it all starts
typedef struct _mms_model
{
   char        id[4];   // name of model "LGMM"
   ulong version;
   float radius;     // bounding radius
   ulong flags;
   ulong app_data;   // to be set by app, so it can check it during callbacks. 
   uchar layout;
   uchar segs;   // number of segments, including stretchy
   uchar smatrs;     // number of single material regions
   uchar smatsegs;  // number of single material segments
   ushort pgons;     // number of pgons;
   ushort verts;     // number of verts;
   ushort weights;   // number of weights;
   ushort pad;
   ulong map_off;    // offset to mappings from seg/smatr to smatsegs
   ulong seg_off;	   // relative to start of the model, used to generate pointers
   ulong smatr_off;	
   ulong smatseg_off;
   ulong pgon_off;   
   ulong norm_off;      // offset to array of pgon normal vectors
   ulong vert_vec_off;   // offset to array of mxs_vectors of vertex positions
   ulong vert_uvn_off;  // offset to array of other vertex data - (u,v)'s, normals etc
   ulong weight_off;
} mms_model;


typedef struct _mms_attachment
{
   int segment_index;
   mxs_trans relative_trans;
   ulong user_data;
} mms_attachment;


// data passed in with model which controls special cases in working
// with it (attachments, for instance, are transformed and rendered)
typedef struct _mms_custom_data
{
   int num_attachments;
   mms_attachment *attachment_list;
} mms_custom_data;


typedef float mms_weight;

typedef uchar mms_mapping;

typedef struct _mms_uvn
{
   float u,v;
   ulong norm;    // compacted normal
} mms_uvn;

typedef struct _mms_pgon
{
   ushort v[3];
   ushort smatr_id; // necessary if doing global sort
   float d;         // plane equation coeff to go with normal
   ushort norm;     // index of pgon normal
   ushort pad;
} mms_pgon;

typedef struct _mms_data_chunk
{
   ushort pgons;
   ushort pgon_start;
   ushort verts;
   ushort vert_start;
   ushort weight_start;    // offset into array of vertex weights.
                           // number of weights = num vertices in segment
   ushort pad;
} mms_data_chunk;

enum _mme_mat {
   MM_MAT_TMAP,
   MM_MAT_COLOR
};
typedef unsigned char mme_mat;

typedef struct _mms_smatr
{
   char name[16]; // for sanity
   ulong handle;  // texture handle or 0bgr
   union {
      float uv;   // uv coords per 3d unit for mipmapping or ipal index
      ulong ipal; // inverse pal lookup
   };
   mme_mat type;    // type 0 = texture, 1 = virtual color
   uchar smatsegs; // number of single material segments
   uchar map_start;
   uchar flags;
   mms_data_chunk data;    // this field only relevent if smatsegs are laid
     // out in material order, so pgons and verts will be consecutive per material.
} mms_smatr;


#define MMSEG_FLAG_STRETCHY 0x1  // segment composed of stretchy polygons

typedef ulong mms_bbox;

typedef struct _mms_segment
{
   mms_bbox bbox;
   uchar joint_id;
   uchar smatsegs; // number of smatsegs in segment
   uchar map_start;
   uchar flags;
   mms_data_chunk data;    // this field only relevent if smatsegs are laid
      // out in segment order, so pgons and verts will be consecutive per segment.
} mms_segment;

typedef struct _mms_smatseg
{
   mms_data_chunk data;
   ushort smatr_id;
   ushort seg_id;
} mms_smatseg;

// for mm_ray_hit_detect
typedef struct mms_ray_hit_detect_info {
   // fill these in before calling mm_ray_hit_detect()
   mxs_vector *start, *end;
   bool early_out;
   mxs_vector *bound_center;
   float bound_radius;

   // this is set by mm_ray_hit_detect()
   bool result;

   // these are set by mm_ray_hit_detect() if result is TRUE
   mxs_vector hit_point;
   mxs_vector hit_normal;
   float hit_time;
   int segment_index;
   int polygon_index;
} mms_ray_hit_detect_info;


#endif
