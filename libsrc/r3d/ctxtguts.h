// $Header: x:/prj/tech/libsrc/r3d/RCS/ctxtguts.h 1.1 1996/05/05 01:36:20 dc Exp $

// this is the bottom level of the context system
// here is where the actual impact of context switching is propagated
//  out to the rest of the r3d system
// in particular, transform and clipping stuff is dealt with here

#ifndef __CTXTGUTS_H
#define __CTXTGUTS_H

void _r3_guts_ctxt_alloc(r3s_context *ctxt);
void _r3_guts_ctxt_free(r3s_context *ctxt);
void _r3_guts_ctxt_start(void);
void _r3_guts_ctxt_end(void);
void _r3_guts_ctxt_swapin(void);
void _r3_guts_ctxt_swapout(void);

#endif // __CTXTGUTS_H
