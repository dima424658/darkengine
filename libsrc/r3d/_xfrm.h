// $Header: x:/prj/tech/libsrc/r3d/RCS/_xfrm.h 1.14 1996/06/13 11:51:32 JAEMZ Exp $

#ifndef ___XFRM_H
#define ___XFRM_H

#include <lg.h>

#include <ctxts.h>
#include <_test.h>

// get the vector address from a 3d transform
#define X2VEC(xformptr) (&((xformptr)->t.vec))
#define X2MAT(xformptr) (&((xformptr)->t.mat))
#define X2TRANS(xformptr) (&((xformptr)->t))

// current transform context
#define cx (r3d_state.cur_con->trans)
#define cur_canv (r3d_state.cur_con->cnv)

void _r3_concat_matrix(mxs_matrix *m, mxs_angvec *a,r3e_order o);
void _r3_recalc_csp();
void _r3_recalc_linear();
void _r3_recalc_w2v();
void _r3_recalc_world_pyr();
void _r3_trans_alloc(r3s_xform_context *trans);
void _r3_trans_free(r3s_xform_context *trans);

#endif
