/*
 * $Source: x:/prj/tech/libsrc/md/RCS/md_.h $
 * $Revision: 1.5 $
 * $Author: jaemz $
 * $Date: 1998/09/28 16:41:55 $
 *
 * Model Library Internals
 *
 */

#ifndef __MD__H
#define __MD__H
#pragma once

#include <mds.h>
#include <r3ds.h>

// Internally, the library must keep several pointers around after getting 
// a pointer to the model.  We expose these for adventurous users
// Carpe diem, er.. Cave Canem, uh.. something like that.
// Caveat Emptor!  
// These are:

typedef struct _mds_con {
   mds_model   *model;    // a pointer to the model
   mds_subobj  *subs;     // subobject header list
   mds_mat     *mats;     // material list
   mds_uv      *uvs;      // the uv list
   mds_vhot    *vhots;    // vhot list
   mxs_vector  *points;   // point list
   mds_light   *lights;   // lighting normal and point list
   mxs_vector  *norms;    // the polygon normal list
   uchar       *pgons;    // the top of the polygon list 
   uchar       *nodes;    // the top of the node list

   // And once the user buffer is allocated and set, it keeps:
   void        *buff;    // a pointer to the buffer
   r3s_point   *buff_points; // a pointer to the transformed points
   float       *buff_lights; // a pointer to the lighting values
   float       *buff_norms;  // a pointer to the normal do product results

   // These are some per model goodies for material contents
   bool use_illum;      // does the model use self illumination in materials
   bool use_alpha;      // does the model use alpha in materials
   float alpha_scale;    // what to scale the alpha by
} mds_con;

EXTERN mds_con mdd;

EXTERN ulong       *mdd_index_table;   // index into an array of points

#define md_buff_point(i) \
   ((r3s_point *)(((uchar *)mdd.buff_points) + mdd_index_table[(i)]))
                                       // pointer to the ith buff_point.

EXTERN mds_parm* mdd_parms;   // currently set parms
EXTERN ubyte    mdd_type_and;  // mask for pgon type at render time
EXTERN ubyte    mdd_type_or;   // mask for pgon type at render time

// Texture
EXTERN ulong    mdd_tmap_mode;  // default to perspective
EXTERN bool     mdd_rgb_lighting; // if true, use RGB lighting

// this gets called back by the subobj traverser
// used internally mostly
EXTERN void md_transform_subobj(int i);

// Set all the globals based on the model
// Can be called from outside if you are sneaky
EXTERN void md_set_globals(mds_model *m);

// Call if you want to reset the tables
// use only if you are sneaky
EXTERN void md_reset_tables();

// Vertex list.  Pretty darn big.
// Temporary state
// This is used by the pgon rendering callbacks
EXTERN r3s_phandle mdd_vlist[64];



#endif // MD__H