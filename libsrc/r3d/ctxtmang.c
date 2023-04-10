// $Header: x:/prj/tech/libsrc/r3d/RCS/ctxtmang.c 1.9 1997/10/15 12:19:51 TOML Exp $
// basic support for contexts, alloc and dealloc, low level set and end

#include <lg.h>

#include <ctxts.h>
#include <r3dctxt.h>

#include <_ctxt.h>
#include <ctxtguts.h>

// this is responsible for maintaining the global current context pointer
// as well as the correct updating of the flags and controls for the
//   context as the user sets and clears it
// the major work is handled by the two _r3 calls, which are really the
//   "concentrators" which all of the major user visible context calls use
// those two call 6 functions in ctxtguts.c, all as _r3_guts_ctxt_??????
//  they are alloc, free, start, end, swapin, and swapout
//  most of these simply take the current context as implicit arg
//  though alloc and free take the context involved as a parameter

// static variables
static r3s_context   _r3d_1st_ctxt;       // context to use for 1st alloc
static bool          _r3d_1st_ctxt_inuse; // is my secret context in use

// % modifies: static local variables _r3d_ctxt stack and depth
// % effects : allocates the context stack, if size 0 doesnt
bool r3_init_context(int stack_depth)
{
   if (stack_depth)
   {
      r3d_state.ctxt_stack=(r3s_context **)
                            Malloc(sizeof(r3s_context *)*stack_depth);
      if (r3d_state.ctxt_stack==NULL)       // out of memory
         return FALSE;
   }
   else
      r3d_state.ctxt_stack=NULL;
   r3d_state.stack_depth=stack_depth;
   r3d_state.stack_pos=0;
   _r3d_1st_ctxt_inuse=FALSE;
   return TRUE;
}

// % modifies: frees the context stack, if there is one
// % effects : allows resource freeing by the context system
void r3_close_context(void)
{
   if (r3d_state.stack_depth)
      Free(r3d_state.ctxt_stack);
}

// % modifies: the context stack
// % effects : puts the current context on the top of the context stack
bool _r3_push_context(r3s_context *newtop)
{
   if (r3d_state.stack_pos>=r3d_state.stack_depth)
   {
      Warning(("_r3_push_context: 3d context stack overflow\n"));
      return FALSE;
   }                          // THIS CODE IS WRONG (the pointer part)
   *(r3d_state.ctxt_stack+r3d_state.stack_pos)=newtop;
   newtop->flags|=R3F_CTXT_ONSTACK;
   r3d_state.stack_pos++;
   return TRUE;
}

// % modifies: the context stack
// % effects : returns the top of the context stack
r3s_context *_r3_pop_context(void )
{
   r3s_context *c;

   if (r3d_state.stack_pos==0)
   {
      Warning(("_r3_push_context: 3d context stack underflow\n"));
      return NULL;
   }                       // ok, the stack pos is pointing at the next
	r3d_state.stack_pos--;  // thing, so move it back to take this off
   c=r3d_state.ctxt_stack[r3d_state.stack_pos];
   c->flags&=~R3F_CTXT_ONSTACK;
   return c;
}

// % requires: a valid r3s_context pointer, which is not currently active
// % modifies: the pointer to the current active context
// % effects : activates the dst context
bool _r3_set_context(r3s_context *dst)
{
   if (r3d_state.cur_con!=NULL)     // assuming we have a current context
      _r3_guts_ctxt_swapout();       // swap it out for now
   r3d_state.cur_con=dst;
   if (dst->flags&R3F_CTXT_ACTIVE)
	   _r3_guts_ctxt_swapin();        // if we were already active
   else
   {
      _r3_guts_ctxt_start();         // else this is a new "frame"
		dst->flags|=R3F_CTXT_ACTIVE;
   }
	dst->flags|=R3F_CTXT_CURRENT;
   return TRUE;
}

// % requires: an active context, on null to use the current context
// % modifies: does an end operation on current context
// %           as it has to decide whether it is swapping or ending
// %            it uses swap to indicate a switch, !swap to mean end
// %           if you end a non persistant context, it frees it
// %           both swap and !swap do swapout, however, effectively ending
// %           your context
bool _r3_end_context(r3s_context *dst, bool swap)
{
   r3s_context *store=NULL;      // hack for now to allow ending non cur ctxt
   if (dst==NULL)                // if dst is null, use default
      dst=r3d_state.cur_con;
   if (dst!=r3d_state.cur_con)   // if dst is not current
   {
      store=r3d_state.cur_con;   // save off the real context
      r3d_state.cur_con=dst;     // swap cur to context to end for now
   }
#ifdef DBG_ON
   if (dst==NULL)
   {
      Warning(("_r3_end_context: trying to end the null context\n"));
      return FALSE;
   }
   if ((dst->flags&R3F_CTXT_ACTIVE)==0)
   {
      Warning(("_r3_end_context: trying to end an inactive context\n"));
      return FALSE;
   }
#endif

	dst->flags&=~R3F_CTXT_CURRENT;

   _r3_guts_ctxt_swapout();

   if (!swap)
   {
      dst->flags&=~R3F_CTXT_ACTIVE;    // deactive us
      _r3_guts_ctxt_end();
	   if ((dst->flags&R3F_CTXT_PERSIST)==0)
         r3_free_context(dst);
   }
   r3d_state.cur_con=store;      // normally would be NULL, this will work
   return TRUE;                  // for both that and the end other case...
}

// % requires: pass memory or null to have lib allocate, and flags you wish,
// %  e.g. R3F_CTXT_PERSIST and R3F_RELOAD2D
// % modifies: sets up the r3s_context, might malloc
// % returns : a new context buffer, initialized correctly, not active
r3s_context *r3_alloc_context(r3s_context *ctxt,ulong flags)
{
   if (ctxt==NULL)      // user wants us to alloc the context
   {
      if (_r3d_1st_ctxt_inuse)
      {
	      ctxt=(r3s_context *)Malloc(sizeof(r3s_context));
	      if (ctxt==NULL)
	      {
	         Warning(("r3_alloc_context: no memory available to alloc context\n"));
	         return NULL;
	      }
	      ctxt->flags=R3F_CTXT_ALLOCED;
      }
      else
      {
         ctxt=&_r3d_1st_ctxt;
         ctxt->flags=R3F_CTXT_IAM1ST;
         _r3d_1st_ctxt_inuse=TRUE;
      }
   } else {
      // context created in stack frame could be uninitialized
      ctxt->flags = 0;
   }
   _r3_guts_ctxt_alloc(ctxt);

   ctxt->flags |= flags;

   return ctxt;
}

// % requires: a valid context, and _NO CURRENT CONTEXT_
// % effects : set cur_con, set use/done bit, DOESNT CALL _R3_SET_CONTEXT
bool r3_use_context(r3s_context *ctxt)
{
   if (r3d_state.cur_con!=NULL)
   {
      Warning(("r3_use_context: tried to use while there was a current context\n"));
      return FALSE;
   }
   r3d_state.cur_con=ctxt;
   ctxt->flags|=R3F_CTXT_USEDONE;
   return TRUE;
}

// % requires: a current context which has a use/done set
// % effects : clears bit and clears current context, NO CALL TO _R3_END
bool r3_done_with_context(void)
{
   if (r3d_state.cur_con)
      if (r3d_state.cur_con->flags&R3F_CTXT_USEDONE)
      {
         r3d_state.cur_con->flags&=~R3F_CTXT_USEDONE;
         r3d_state.cur_con=NULL;
         return TRUE;
      }
   Warning(("r3_done_with_context: tried to done a context which wasnt in use\n"));
   return FALSE;
}

// % requires: a valid context which is not in use or in progress
// %           (ie. a persistent context between "frames")
// % modifies: frees all memory associated with context, removes it
bool r3_free_context(r3s_context *ctxt)
{
   if (ctxt->flags&(R3F_CTXT_ACTIVE|R3F_CTXT_USEDONE))
   {
      Warning(("r3_free_context: trying to free an active or used context\n"));
      return FALSE;
	}
   _r3_guts_ctxt_free(ctxt);
   if (ctxt->flags&R3F_CTXT_IAM1ST)
      _r3d_1st_ctxt_inuse=FALSE;
   if (ctxt->flags&R3F_CTXT_ALLOCED)
      Free(ctxt);
   return TRUE;
}

#ifdef SO_WHAT
im a timebomb inside no one listens to reason its too late and im ready to fight
#endif
