// $Header: x:/prj/tech/libsrc/r3d/RCS/ctxtmult.c 1.6 1997/10/01 13:32:46 KEVIN Exp $
// support for multiple simul contexts

#include <dev2d.h>

#include <ctxts.h>
#include <r3dctxt.h>

#include <_ctxt.h>

// % requires: a valid r3s_context pointer, which is not current or on stack
// % modifies: the pointer to the current active context
// % effects : activates the dst context, deactivates current context
// %           ignores the stack (ie. does not replace last stack entry)
// % implement : calls end context on old, then set on the new
bool r3_switch_context(r3s_context *dst)
{
   if (dst == NULL)
      return FALSE;
   if (dst->flags&(R3F_CTXT_ONSTACK|R3F_CTXT_CURRENT))
      return FALSE;
   _r3_end_context(NULL,TRUE);
   _r3_set_context(dst);
   return TRUE;
}

// % requires: a valid r3s_context pointer, which is not currently active
// % modifies: the pointer to the current active context + context stack
// % effects : pushes dst context, deactivates current context
// %           adds current to stack prior to switching to dst
// % implement: calls set context with push
bool r3_push_context(r3s_context *dst)
{
   gr_push_canvas(grd_canvas);   // a simple hack to do two things:
                                 // 1) make sure grd_canvas is the 
                                 // current canvas when dst is set
                                 // in case dst is new (swaping out the
                                 // old context will pop the canvas)
                                 // 2) restore this canvas when 
                                 // r3_pop_context() is called.

   if (!_r3_push_context(r3d_state.cur_con))
      return FALSE;              // make this smarter, how do we deal with overflow
   if (dst == NULL)
      dst = r3_alloc_context(NULL,0);
   return r3_switch_context(dst);
}

// % modifies: removes top element of context stack
// % effects : makes current context former top of context stack
// % returns : false if stack is empty
bool r3_pop_context(void)
{
   r3s_context *c;
   _r3_end_context(NULL,FALSE);     // really end the current context
   c = _r3_pop_context();
   if (c==NULL)
      return FALSE;
   _r3_set_context(c);
   gr_pop_canvas();                 // see r3_push_context() for explanation.
   return TRUE;
}

// % requires: an in progress context which is not current
// % effect  : clears any data related to the passed in context
bool r3_abort_context(r3s_context *ctxt)
{
   if (ctxt->flags&R3F_CTXT_CURRENT)      // if we are current
      _r3_end_context(NULL,FALSE);        // then go end the context (no swap)
   if (ctxt->flags&R3F_CTXT_ACTIVE)       // if we are active
      if (ctxt->flags&R3F_CTXT_ONSTACK)   // need to do special things if on
         ctxt->flags&=~R3F_CTXT_ACTIVE;   //  the stack, like deactivate it
      else                                // it will be ended later
	      _r3_end_context(ctxt,FALSE);     // else end it here and now
   else
      return FALSE;
   return TRUE;
}

