// $Header: x:/prj/tech/libsrc/r3d/RCS/frame.c 1.3 1997/01/06 18:27:22 JAEMZ Exp $
// simple start and end frame for single context work

#include <lg.h>

#include <ctxts.h>
#include <r3dctxt.h>

#include <_ctxt.h>

// note that really this is a lie
// as "frame" is really defined to be "whenever a context is used/not"

// % requires: a valid and allocated context (for now, ie. non-null)
// % modifies: the master r3 context 
// % effects : sets context active/in prog bit to true
// % returns : whether or not we are successfully in the new context
bool r3_start_frame_context(r3s_context *ctxt)
{
   DoIfDBG({if (ctxt==NULL) return FALSE;});    // punt if this is null
   return _r3_set_context(ctxt);             // then try setting it
}

// % requires: the 2d to be initialized, and a current canvas set
// % modifies: allocates and creates a base context, sets it to current
// % returns : pointer to the set up context, null if no can do
r3s_context *r3_start_frame(void)
{
   r3s_context *ctxt;
   if (ctxt=r3_alloc_context(NULL,0))
	   if (_r3_set_context(ctxt))
	      return ctxt;
      else
         r3_free_context(ctxt);
   return NULL;
}

// % modifies: ends and frees the current context
// % effects : sets global current context
// % returns : false if no active context
bool r3_end_frame(void)
{
   return _r3_end_context(NULL,FALSE);
}


