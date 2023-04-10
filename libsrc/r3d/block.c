// $Header: x:/prj/tech/libsrc/r3d/RCS/block.c 1.5 1996/06/13 11:53:26 JAEMZ Exp $
// This has start and end block

#include <_xfrm.h>
#include <r3ds.h>
#include <matrix.h>

// Requires:  In a context, not in a block 
// Modifies:  r3d_glob and r3d_state
// Effects: sets the block flag, and rebuilds xform
//    matrix o2c and w2c if necessary
//    This also recalculates vino

void r3_start_block()
{

   TEST_STATE("r3_start_block");

   // if either is true, recalculate vino
   // vino = inv(o2w) * viewer_pos
   if (cx.w2c_invalid || cx.o2c_invalid) {
      mx_trans_tmul_vec(&cx.vino,X2TRANS(&cx.o2w),X2VEC(&cx.view_trans));

      // world to camera built from world,
      // inv(v2w), and space_vec
      // w2c = csp^t * world * inv(v2w)
      // world is truly world space, not view space
      // manipulation

      if (cx.w2c_invalid) {
         if (r3d_glob.cur_space == R3_LINEAR_SPACE) {
            // now we get sneaky, linear space has a full transform it needs
            // to pre-push, not just a wimpy csp

            _r3_recalc_linear();
            mx_mul_trans(X2TRANS(&cx.w2c),X2TRANS(&cx.lin_trans),X2TRANS(&cx.w2v));
         } else {
            // Apply space_vec across the whole transform to scale dest space
            // copy transform over
            cx.w2c = cx.w2v;
            mx_mat_eltmuleq_vec(X2MAT(&cx.w2c),&cx.cspace_vec);
            mx_elmuleq_vec(X2VEC(&cx.w2c),&cx.cspace_vec);
         }   

         cx.w2c_invalid = FALSE;
      }

      // this is just o2c = w2c * o2w
      if (cx.o2c_invalid) {
         mx_mul_trans(X2TRANS(&cx.o2c),X2TRANS(&cx.w2c),X2TRANS(&cx.o2w));

         cx.o2c_invalid = FALSE;
      }
  }


   r3d_state.in_block = TRUE;
}


// Requires:  In a context, in a block
// Modifies:  r3d_glob and r3d_state
// Effects:  unsets the block flag

void r3_end_block()
{
   #ifdef DBG_ON 
   if (!r3d_state.in_block) { 
      Warning(("r3d: r3_end_block: called outside of a block!\n")); 
      return; 
   } 
   if (r3d_state.cur_con==NULL) { 
      Warning(("r3d: r3_end_block: called outside of a context!\n")); 
      return; 
   }
   #endif

   r3d_state.in_block = FALSE;

}
