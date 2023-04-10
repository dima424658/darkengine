// $Header: x:/prj/tech/libsrc/r3d/RCS/ctxtguts.c 1.26 1997/10/15 12:19:44 TOML Exp $
// this is the bottom level of the context system
// here is where the actual impact of context switching is propagated
//  out to the rest of the r3d system
// in particular, transform and clipping stuff is dealt with here

#include <lg.h>
#include <dev2d.h>

#include <_xfrm.h>
#include <clip.h>
#include <prim.h>
#include <space.h>
#include <view.h>
#include <ctxtguts.h>

// % requires: a context which has just been allocated
// % modifies: the data in the passed in context structure
// % effects : sets up transform and clip and other subfields of ctxt
// %         : note ctxt passed in has flags and cur_con as _ONLY_ valid fields
// % note that you still need to do a start frame with it to make it usable
void _r3_guts_ctxt_alloc(r3s_context *ctxt)
{
   // Alloc and initialize the transform context
   _r3_trans_alloc(&(ctxt->trans));

   // setup "user" clip planes
   ctxt->clip.max_user_planes = r3d_state.clip_planes;
   ctxt->clip.num_user_planes = 0;

   ctxt->clip.clip_data = Malloc(sizeof(mxs_plane) * 2 * ctxt->clip.max_user_planes);

   // setup initial clipping mode
   ctxt->clip.clip_mode = R3_CLIP;
   ctxt->clip.clip_flags = 0;

   // setup default near clip plane
   ctxt->clip.near_plane = 0.0001;

   // set canvas to NULL
   ctxt->cnv = NULL;

   // clear primitive stuff
   ctxt->prim.bm = NULL;
   ctxt->prim.color = 0;
   ctxt->prim.clip_2d = 0;
   ctxt->prim.poly_flags = 0;
   ctxt->prim.line_flags = 0;
   ctxt->prim.point_flags = 0;
}

// % requires: an already extant context which is ready to be freed
// % modifies: the data in the passed in context structure
// % effects : frees any resources associated with the context
// %         : doesnt free ctxt itself
void _r3_guts_ctxt_free(r3s_context *ctxt)
{
   // free the context stack
   _r3_trans_free(&(ctxt->trans));

   Free(ctxt->clip.clip_data);
}

// % requires: r3d_state.cur_con != NULL
// % modifies: the current context
// % effects : takes any actions necessary for a frame start of this context
void _r3_guts_ctxt_start(void)
{
   r3s_context *c = r3d_state.cur_con;

   // if not persistant, or 2d reload set, grab
   // the current canvas AND aspect ratio, curse curse curse
   if ( (!(c->flags&R3F_CTXT_PERSIST)) || (c->flags&R3F_CTXT_RELOAD2D)) {
      r3_set_view_canvas(grd_canvas);
   }

   // Look ma, ctxt_start is a superset
   // of swapin
   _r3_guts_ctxt_swapin();
}

// % requires: r3d_state.cur_con != NULL
// % modifies: the current context
// % effects : takes any actions necessary for a frame end of this context
// %         : called as soon as
void _r3_guts_ctxt_end(void)
{

   // check to make sure object stack is empty, if not, whine
   #ifdef DBG_ON
   if (cx.stack_size != 0) {
      Warning(("r3_frame_end: object stack is not empty yet\n"));
   }
   #endif

}

// % requires: r3d_state.cur_con != NULL
// % modifies: the current context
// % effects : takes any actions necessary to swap in this context
// %         : note this is called right after cur_con is set
void _r3_guts_ctxt_swapin(void)
{
   // always push the current canvas.
   gr_push_canvas(cur_canv);

   // set the new space, which stuffs everything, etc
   r3_set_space(cx.cspace);

   // set the point stride for block operations
   r3d_glob.cur_stride = cx.stride;

   // need to reset the globals for new view and stuff
   _r3_recalc_csp();
   r3_set_prim();
   r3_set_clip();
}

// % requires: r3d_state.cur_con != NULL
// % modifies: the current context
// % effects : takes any actions necessary to swap out this context
// %         : note this is called right after cur_con is set
void _r3_guts_ctxt_swapout(void)
{
   // grab the current stride, in case it's changed
   cx.stride = r3d_glob.cur_stride;

   // grab the current space,
   cx.cspace = r3d_glob.cur_space;

   // pop the canvas to clean up
   gr_pop_canvas();
}
