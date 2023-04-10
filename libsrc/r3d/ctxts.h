// $Header: x:/prj/tech/libsrc/r3d/RCS/ctxts.h 1.14 1998/06/18 11:26:17 JAEMZ Exp $

#ifndef __CTXTS_H
#define __CTXTS_H

#include <r3ds.h>
#include <xfrmtab.h>
#include <cliptab.h>
#include <primtab.h>
#include <clipcon.h>
#include <primcon.h>
typedef struct grs_canvas grs_canvas;

// these really belong with the h files that take these

// typedef enum for camera spaces
// clipping is optimized for clipping alot
// project is optimizd for fast projection
// unscaled is slow at projection and clipping
// linear is super fast but only good for far objects
typedef enum {
   R3_CLIPPING_SPACE=0,
   R3_PROJECT_SPACE=1,
   R3_UNSCALED_SPACE=2,
   R3_LINEAR_SPACE=3,
   R3_NUM_SPACE=4
} r3e_space;

typedef enum {
   R3_XYZ=6,    //00 01 10
   R3_XZY=9,    //00 10 01
   R3_YXZ=18,   //01 00 10
   R3_YZX=24,   //01 10 00
   R3_ZXY=33,   //10 00 01
   R3_ZYX=36,   //10 01 00
   R3_DEFANG=6  //00 01 10
} r3e_order;

// Special transform that contains explicit handedness info
typedef struct _r3s_trans {
   mxs_trans t;   // transform part of it
   int left;      // is it left handed or not                        
      // if an mxs_real is a double, we need 4 bytes more padding
} r3s_trans;

// this is the transform context
// it has all the goodies for the stack and the like
typedef struct _r3s_xform_context {
  // all the mxs_real-sized stuff
   mxs_vector cspace_vec;  // camera space vector
   r3s_trans world;  // custom world to 3d world space transform
   r3s_trans o2w;    // object to world
   r3s_trans w2v;    // world to viewer, no cspace, built from world and in(view trans);
   r3s_trans w2c;    // world to camera with cspace added, built from w2v, and space_vec
   r3s_trans o2c;    // object to camera space
   r3s_trans view_trans;   // raw view pos and matrix, for getting
   r3s_trans lin_trans; // linear trans
   mxs_vector vino;  // viewer coords in object space
   r3s_trans *stack_cur;   // pointer to next entry on stack
   r3s_trans *stack_base;  // pointer to base of stack
   int   stack_size; // current size of stack, ie, number of elements
   int   stack_max;  // maximum num elements on stack

   mxs_real zoom;     // zoom factor 90 degrees horizontal = 1
   mxs_real aspect;   // aspect ratio height/width
   mxs_vector lin_cen; // center of linear space
   mxs_real  lin_width;   // linear screen width or zero if using lin_cen
   mxs_real  lin_w;      // linear w value
   float bias_x;     // x bias to the right of center of the viewer
   float bias_y;     // y bias down of center of the viewer
   int stride;       // point size for block operations

   mxs_angvec view_ang; // raw view angles
   r3e_space cspace; // camera space
   bool w2c_invalid; // w2v invalid, need to reconcatenate before cur
   bool o2c_invalid; // set when cur is invalid
   bool world_set;   // set if the user has set world
} r3s_xform_context;

typedef struct _r3s_context {
   // canvas and related processed information
   grs_canvas *cnv;     // 2d canvas to render to
   r3s_xform_context trans;   // transform context structure har har
   r3s_clip_context clip;  // clip context structure har
   r3s_prim_context prim;  // primitive context
   // context management info
   int flags;        // yea, ok, here they are.  neat, huh?
} r3s_context;

// global context that doesn't get copied
typedef struct {
   r3s_context *cur_con; // current context, null when none
   int trans_depth;  // default nesting of transform stack
   int clip_planes;  // default number of clipping planes above the basic 5
   r3s_trans world;  // default world coordinates
   int stride;       // default size of points for block operations
   // context stack info
   r3s_context **ctxt_stack;
   int stack_depth, stack_pos;
   // move to bottom for alignment
   r3e_space space;  // default camera space
   bool in_block;    // am I in a block
} r3s_global_state;

// global context that gets changed by current context
typedef struct {
   // move to top to align if they're doubles
   mxs_real x_prj;            // x projection constant
   mxs_real y_prj;            // y projection constant
   mxs_real x_clip;           // x clip constant
   mxs_real y_clip;           // y clip constant

   fix x_off;                 // screen x offset constant
   fix y_off;                 // screen y offset constant
   int cur_stride;            // point size for block operations

// note that fields above this line should be easy to access from assembly
   r3s_xform_tab  xform_tab;  // transform function tables
   r3s_prim_tab   prim_tab;   // primitive dispatch tables
   // move to bottom for alignment
   r3e_space cur_space;       // current space
} r3s_global_context;

EXTERN r3s_global_state   r3d_state;      // global state
EXTERN r3s_global_context r3d_glob;       // global context

// everyone will hate this, as the debugger wont know what it is
// cause life is so darn sad
// so i guess we wont do it
// #define r3d_context (r3d_state.cur_con)

//------- flags for r3s_context
// flags for dealing with having a stack/multicontext model
#define R3F_CTXT_CURRENT   (1<<0)      // current context?
#define R3F_CTXT_ACTIVE    (1<<1)      // currently started/in progress?
#define R3F_CTXT_ONSTACK   (1<<2)      // on the stack?
#define R3F_CTXT_USEDONE   (1<<3)      // in a use/done block?
// flag for bonus internal/external memory fun
#define R3F_CTXT_ALLOCED   (1<<4)      // memory that r3d allocated?
#define R3F_CTXT_IAM1ST    (1<<5)      // secret interal 1st context memory
// flags for allowing persistant canvii
#define R3F_CTXT_PERSIST   (1<<6)      // a persistant context?
#define R3F_CTXT_RELOAD2D  (1<<7)      // reload canvas from 2d each frame?
// flag we need if context drives start and end frame
#define R3F_CTXT_PRIMARY   (1<<8)      // "primary, frame rate" context?
                                       // not set means a "background" context

#endif // __CTXTS_H
