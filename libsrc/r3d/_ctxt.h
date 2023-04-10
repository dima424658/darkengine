// $Header: x:/prj/tech/libsrc/r3d/RCS/_ctxt.h 1.5 1997/01/15 13:37:11 KEVIN Exp $
// internals for the context system

bool r3_init_context(int stack_depth);
void r3_close_context(void);

bool _r3_push_context(r3s_context *newtop);
r3s_context *_r3_pop_context(void );
bool _r3_set_context(r3s_context *dst);
bool _r3_end_context(r3s_context *dst, bool swap);

#define R3_DEF_CTXT_STACK 8
