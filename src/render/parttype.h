/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/parttype.h,v 1.16 2000/02/20 11:57:18 adurant Exp $
#pragma once

#ifndef __PARTTYPE_H
#define __PARTTYPE_H

#include <matrixs.h>
#include <fix.h>
#include <r3ds.h>

/*
 *  Yo!  This header file is entirely private
 *  to the particle system... it's only published
 *  here so that it can be propertized
 */

typedef struct st_ParticleList ParticleList;
typedef struct st_ParticleLaunchInfo ParticleLaunchInfo;
typedef struct st_ParticleGroup ParticleGroup;
typedef struct st_ParticleClass ParticleClass;

// arrays of per-particle information; this is basically an
// array of structs rearranged into a struct of arrays, to
// allow unused fields to use no storage
struct st_ParticleList {
   int n;      // number of particles currently valid in lists
   mxs_vector *location;
   mxs_vector *velocity;
   // orientation? generally particles shouldn't have orientation,
   // but it would only cost 4 or 8 bytes to have two more NULL orientation
   // pointers, and would allow the system to be leveraged by oriented objects
   fix *time_info;
   int *cell;
   uchar *render_data;   // special per-renderer data
};

enum ParticleLaunchType
{
   PLT_BBOX,
   PLT_SPHERE,
   PLT_CYLINDER,
};

// information for creating new particles, used by "emitters"
struct st_ParticleLaunchInfo
{
   int type;

   mxs_vector loc_min;
   mxs_vector loc_max;

   mxs_vector vel_min;
   mxs_vector vel_max;

   mxs_real min_radius;
   mxs_real max_radius;

   mxs_real min_time;
   mxs_real max_time;

   BOOL vel_unrotated;  // if true, don't orient launch with object
   BOOL loc_unrotated;  // if true, don't orient location with object

   // these are fixes to allow faster random number generation
   fix start_loc_bbox[2][3];   // where to create new particles
   fix start_vel_bbox[2][3];   // how fast they should move initially
   fix radius[2];
   fix time_range[2];          // linear time range for how long before decay
};

struct st_ParticleClass
{
   // per "tick" simulation--return TRUE to kill the object
   void (*group_sim)(ParticleGroup *pg, float time);
   BOOL (*particle_sim)(ParticleGroup *pg, float time);
   void (*particle_sim_precompute)(ParticleGroup *pg);

   // stuff locs / points with particle locations
   void (*particle_worldspace)(ParticleGroup *pg);  // optional
   void (*particle_objectspace)(ParticleGroup *pg); // optional
   void (*particle_transform)(ParticleGroup *pg);    // defaultable

   // actually render--if list is non-null, it is a "sparse list"
   void (*particle_render)(ParticleGroup *pg, int *list);

   //   miscellaneous
   // computes particle for each cell, puts it in "cells"
   void (*particle_compute_cell)(ParticleGroup *pg);    // defaultable
   // allocates a list of particles per cell into sort_lists
   void (*particle_cell_partition)(ParticleGroup *pg);  // defaultable
};

enum ParticleRenderTypes
{
   PRT_SINGLE_COLOR_PIXEL,
   PRT_VARIABLE_COLOR_PIXEL,
   PRT_SINGLE_COLOR_SQUARE,
   PRT_VARIABLE_COLOR_SQUARE,
   PRT_SINGLE_COLOR_DISK,
   PRT_SCALED_BITMAP,

   PRT_NUM_USER_EDITTABLE,

   // only useable by code:

   PRT_CODE_ONLY=255,   // keep numbering from changing if we add new ones
};

enum ParticleGroupMotionType
{
   PGMT_IMMOBILE,
   PGMT_VELOCITY,
   PGMT_VELOCITY_GRAVITY,
   PGMT_OBJECT_TRAILING, 
   PGMT_OBJECT_ATTACHED,
   PGMT_FIREANDFORGET,

   PGMT_NUM_USER_EDITTABLE,

   // only useable by code:

   PGMT_CODE_ONLY=255,
};

enum ParticleAnimationType
{
   PAT_LAUNCH_ONE_SHOT,
   PAT_LAUNCH_CONTINUOUS,

   // only uses particle group animations
   PAT_FIXED_SPHERE_SHELL,
   PAT_FIXED_SPHERE,
   PAT_FIXED_CUBE_SHELL,
   PAT_FIXED_CUBE,
   PAT_FIXED_DISK,
   PAT_FIXED_CIRCLE,
   PAT_FIXED_PYRAMID,

   PAT_NUM_USER_EDITTABLE,

   // only useable by code:

   PAT_CODE_ONLY=255,
};

// location and orientation are stored as a separate property
struct st_ParticleGroup
{
   ParticleClass pc;     // the vlist for this object
   ObjID obj;            // allows passing around of ParticleGroup only
   enum ParticleRenderTypes        render_type;
   enum ParticleGroupMotionType   motion_type;
   enum ParticleAnimationType       anim_type;
   int d,e;              // placeholder integers for future enums
   int n;                // maximum number of particles
   ParticleList pl;      // the actual particles
   mxs_vector velocity;  // velocity of group as a whole
   mxs_vector gravity;   // applied to group or to particles

   // 4 bytes:
   uchar cr,cg,cb,ca;    // global color value

   // 4 bytes:
   bool always_simulate; // simulate even if not visible (slow)
   bool always_simulate_group;
                         // if not visible, simulate group but not particles
   bool cell_sort;       // force particles to render in the right cell (slow)
                         // unimplemented

   bool zsort;           // sort points back to front (slow)

   // 4 bytes:
   bool terrain_collide; // detect particle vs. terrain collision (slow?)
   bool accelerate_cell; // tradeoff storage for speed to accelerate terrain
                         // collision detect/cell sorting (unimplemented)

   bool ignore_attach_refs;  // whether our reffed-ness depends on our attachment
   bool pad2;

   ParticleLaunchInfo *launch;  // info for creating new particles

   mxs_vector spin;         // spin amounts
   int pulse_period;        // time in milliseconds between pulses
   float pulse_percentage;  // how small we get when pulsing
   float fixed_scale;       // radius for fixed arrangements

   // 4 bytes:
   bool pre_launch;      // group should start already launched
   bool spin_group;      // do we animate the group in various ways as above

   bool tiny_alpha;      // draw tiny particles as alpha
   bool tiny_dropout;    // omit particles when they get tiny

//// derived data

   // 4 bytes:
   bool shared_list;     // malleable or not?  free the lists or not?
   bool worldspace;      // points are in worldspace not objectspace
                         // also bounding box
   bool launching;
   bool active;

//// regular data

   int ms_offset;        // delay time in the animation state
   float size;           // size in worldspace

   int reserved;
   int reserved2;        

   mxs_vector prev_loc;  // for trailing, location in previous frame

   float scale_vel;      // fixed-group radius grows at this speed

//// internal data

   int render_datum;     // cached as appropriate for the current mode

   mxs_vector bmin, bmax;  // bounding box
   float radius;           // bounding sphere

   float cur_scale;      // fixed_scale scaled by pulse animation

   r3s_point *points;    // current points
   int **sort_lists;     // list of items when sorted

   // 4 bytes:
   bool locs_worldspace; // are locs in objectspace or worldspace?
   bool free_locs;       // do we need to free locs?
   bool seen;            // did we render last frame?
   unsigned char need_flags;  // derived

//// data I had to tack on the end of the structure

   int list_length;      // n again, now the actual length of the lists
   int delete_count;     // number of leading particles to skip
   fix next_launch;      // when to launch next particle if not pre_launch
   fix launch_period;    // time between particle launches

   char modelname[16];   // for bitmaps
   int modelnum;         // for bitmaps
   fix fade_time;        // for fading alpha particles

   mxs_matrix obj_rot_mat; // the rotation of the particle group this frame

   int last_sim_time;    // last time at which we simmed

   ObjID attach_obj;     // the object we're attached to (OBJ_NULL if we're not)

   BOOL force_match_unrefs; //Really really unref if our attached obj
                            //is unreffed.  No particles at origin!
                            //Um, maybe this should have been lower case
                            //bool, but it would have forced other bools,
                            //where now we can turn it into flags! (oh joy)
                            

};

// any custom transform should provide screenspace sx,sy
// values at a minimum.  the code that sets the rendering type should
// also set the need_flags, and custom transforms can consult this to
// determine exactly what they must compute, or else they should compute
// everything
#define NEED_Z     1  //    p.z
#define NEED_W     2  //  grp.w
#define NEED_PXY   4  //    p.x, p.y

#endif
