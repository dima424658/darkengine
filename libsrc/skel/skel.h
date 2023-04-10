/*
 * $Source: x:/prj/tech/libsrc/skel/RCS/skel.h $
 * $Revision: 1.32 $
 * $Author: kate $
 * $Date: 1997/12/10 11:10:38 $
 *
 * Skeleton libary includes
 *
 */

#ifndef __SKEL_H
#define __SKEL_H

#include <limits.h>
#include <r3d.h>
#include <g2.h>
#include <md.h>
#include <res.h>
#include <matrixs.h>

// Data types for a cylindrical bitmap object
// eventually we'll want one for a full 3d one

typedef struct {
   int nviews;    // number of views
   fix ppu;       // pixels per unit
   fix len;       // length from anchor to anchor
   bool bisym;    // bilateral symmetry or not (means its a mirror)
   int off[1];    // offsets, in reality there should be off[nview] of them
} CylBMObj;       // cylindrical 3d bitmap object

typedef struct {
   grs_bitmap bm;
   short  u1,v1;   // anchor point 1
   short  u2,v2;   // anchor point 2
   fix   vper1;   //  v1 / (v2-v1)
   fix   vper2;   // (h-v2) / (v2-v1)
   fix   uper;    // (w - u) / u
} CylBMFrame;     // one frame of the cylindrical bm object.  Always put the bits after this.

typedef void (*CylBMDecalFunc)(void *data, grs_vertex *v, uchar frame);

// Segment flag, mirror image
#define SKF_MIRROR 1
// Are joints 1 and 2 exposed to light or not
#define SKF_EXPOSE1 2
#define SKF_EXPOSE2 4
#define SKF_DONTRENDER 16 // take a big guess

#define LINE 0
#define CYL  1
#define BMAP 2
#define POLY 3

typedef enum {
   NONE,
   FAST,
   MEDIUM,
   SLOW
} ske_sorttype;

// Structure for each segment
typedef struct {
   ubyte p1;  // offsets of points 1 and 2
   ubyte p2;
   ubyte or;  // offset of orientation vector
   ubyte flag; // flags, for instance, is it mirrored,exposed?,locked?
   union {
      struct { // for cylinder dude
         float rad;
         long col;
      };
      struct {
         ulong handle;  // for locking
         union {
            struct { // for bitmap dude
               CylBMObj *co;
               ubyte *clut;
               CylBMDecalFunc decfunc; // decal rendering function
               void *decdata;          // data for decal rendering func
            };
            struct { // for 3d model dude
               mds_model *model;
               float *parm;
            };
         };
      };
   };
} sks_seginfo;

//typedef struct sks_skel;

// The skeleton structure, followed by seginfo
// then orientation vectors, then joints
typedef struct _sks {
   uchar type;       // type of skeleton (used to be an enum)
   ubyte segn;       // number of segments
   ubyte pointn;     // number of points
   ubyte orn;        // number of orientation vectors
   void (*qsort) (struct _sks *s,ubyte *rl,r3s_point *tmp);  // pointer to sorter
   sks_seginfo *sg;   // pointer to start of seg info
   mxs_vector  *pn;   // pointer to start of list of vectors
   mxs_vector  *or;   // first in array of local orientation vectors
} sks_skel;

typedef mds_model *(*sk_poly_prep_func)(sks_seginfo *cseg,mxs_vector *pos);

void sk_set_poly_prep(sk_poly_prep_func f);

void sk_init(void);

// Allocate space for a skeleton
// given a model
sks_skel *sk_alloc_skel(ubyte segn,ubyte pointn,ubyte orn);

// initialize a skeleton given the model
sks_skel *sk_init_skel(sks_skel *sk,ubyte segn,ubyte pointn,ubyte orn);

// returns target size of the skeleton, given the model
int   sk_size_skel(ubyte segn,ubyte pointn,ubyte orn);

// Free a skeleton, does not unlock it
void sk_free_skel(sks_skel *s);

// Set this skeleton to use the given color lookup table when rendering
// in bitmap limb mode.  NULL (default) means no clut
void sk_set_bmap_clut(sks_skel *s, ubyte *clut);

// Set the quick sorter for this model
void sk_set_skel_sort(sks_skel *sk,void (*qsort)(sks_skel *s,ubyte *rl,r3s_point *tmp));

// Lock down the cylinder bmap objects for a bmap object, ignore otherwise
void sk_res_lock(sks_skel *sk,Id resid);
void sk_res_unlock(sks_skel *sk,Id resid);

// lock down bmap objects, assuming handle IS refid
void sk_lock(sks_skel *sk);
void sk_unlock(sks_skel *sk);

// Render a skeleton given
// skeleton and model
// assuming all other things have been set,
// points transformed, etc.
void sk_render_skel(sks_skel *s,ske_sorttype);

// phandle of transformed point, pointer to mxs_vector
typedef  void (*skt_vcall_func)(r3s_phandle p,mxs_vector *v);

// Do a callback at the joint in question, passes in
// r3s_phandle of point.
void sk_set_joint_call(int seg_id,skt_vcall_func v_func,int j);
void sk_set_seg_call(int set_id,skt_vcall_func v_func,fix xoff,fix yoff,fix zoff);

// Do a vhot set that puts the world coords into sk_vhot_vecs[vhot_num];
void sk_set_joint_vhot(int seg_id,int vhot_num,int j);
void sk_set_seg_vhot(int set_id,int vhot_num,fix xoff,fix yoff,fix zoff);
void sk_vhot_skel(sks_skel *s);
EXTERN mxs_vector skd_vhot_vecs[];
EXTERN uchar   skd_light_detail;
EXTERN bool    skd_debug;
EXTERN bool    skd_scale_polys;

// 0 = maximum
// 1 = front only
#define sk_set_light_detail(x) (skd_light_detail = (x))
#define sk_vhot_vec(x) (&skd_vhot_vecs[x])
#define sk_set_debug(val) (skd_debug=(val))

// Provide it the segment info and the endpoints, orientation vector, transformed endpoints, and clut or not
void sk_render_bmap_piece(sks_seginfo *cseg,mxs_vector *j1,mxs_vector *j2,mxs_vector *or,r3s_phandle p1,r3s_phandle p2);
void sk_render_poly_piece(sks_seginfo *cseg,mxs_vector *j1,mxs_vector *j2,mxs_vector *or);



#endif /* __SKEL_H */

