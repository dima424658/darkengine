// $Header: x:/prj/tech/libsrc/mm/RCS/mm_.h 1.3 1998/03/26 08:44:01 MAT Exp $
// internal mesh data

#ifndef _MM__H
#define _MM__H

#include <mm.h>
#include <r3ds.h>

// callbacks
extern mmf_joint_pos_cback joint_pos_cback;     // from render.c
extern mmf_stretchy_joint_cback stretchy_joint_cback;

extern mmf_light_setup_cback mmd_light_setup_cback;
extern mmf_light_cback mmd_light_cback;

// globals set from the model
extern mms_model   *mmd_model;       // pointer to the model
extern mms_smatr   *mmd_smatrs;      // single material regions list
extern mms_segment *mmd_segs;        // segment list
extern mms_mapping *mmd_mappings;
extern mms_smatseg *mmd_smatsegs;
extern mms_uvn     *mmd_vert_uvns;   // uv and normal list
extern mxs_vector  *mmd_vert_vecs;   // vertex list
extern mms_pgon    *mmd_pgons;       // pgon list
extern mxs_vector  *mmd_norms;       // pgon normal list
extern mms_weight  *mmd_weights;     // weight list

// globals for object buffer
extern void        *mmd_buff;
extern r3s_point   *mmd_buff_points;
extern float       *mmd_buff_norms;   // a pointer to the normal dot product results
extern mms_pgon    *mmd_buff_sorted_pgons;  // a pointer to sorted polygons
extern mxs_trans   *mmd_buff_attach_trans; // transforms for attachments

#endif
