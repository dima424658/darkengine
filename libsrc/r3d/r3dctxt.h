// $Header: x:/prj/tech/libsrc/r3d/RCS/r3dctxt.h 1.7 1998/06/18 11:27:11 JAEMZ Exp $
// public functions for the context system

#ifndef __R3DCTXT_H
#define __R3DCTXT_H

#include <ctxts.h>

// super basic start and end frame, implicitly create, set and destroy a context
EXTERN r3s_context *r3_start_frame(void);
EXTERN bool r3_end_frame(void);
// for using a context you have built to start a frame
EXTERN bool r3_start_frame_context(r3s_context *ctxt);

// basic special context support, for building and playing with persistant contexts
EXTERN r3s_context *r3_alloc_context(r3s_context *ctxt,ulong flags);
EXTERN bool r3_use_context(r3s_context *ctxt);
EXTERN bool r3_done_with_context(void);
EXTERN bool r3_free_context(r3s_context *ctxt);

// fancy pants context stuff for cool people who use multicontext support
EXTERN bool r3_switch_context(r3s_context *dst);
EXTERN bool r3_push_context(r3s_context *dst);
EXTERN bool r3_pop_context(void);
EXTERN bool r3_abort_context(r3s_context *ctxt);

// driver switching calls.  Currently this state is not context related.
EXTERN void r3_use_g2(void);      // use g2, the default driver.
EXTERN void r3_use_lgd3d(void);   // use lgd3d, the looking glass direct 3d interface.
EXTERN void r3_use_null(void);    // use null driver; i.e. no rasteriztion.

#endif // __R3DCTXT_H