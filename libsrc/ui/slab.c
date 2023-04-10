#include <lg.h>
#include <slab.h> 

/*
 * $Source: x:/prj/tech/libsrc/ui/RCS/slab.c $
 * $Revision: 1.6 $
 * $Author: mahk $
 * $Date: 1997/06/27 15:14:07 $
 *
 * $Log: slab.c $
 * Revision 1.6  1997/06/27  15:14:07  mahk
 * Fixed == bug.
 * 
 * Revision 1.5  1996/09/27  21:14:17  mahk
 * Added some "call_deferred" cases.
 * 
 * Revision 1.4  1994/08/24  08:55:51  mahk
 * Cursor stacks and invisible regions.
 * 
 * Revision 1.3  1993/10/11  20:26:47  dc
 * Angle is fun, fun fun fun
 * 
 * Revision 1.2  1993/04/28  14:40:01  mahk
 * Preparing for second exodus
 * 
 * Revision 1.1  1993/04/05  23:40:58  mahk
 * Initial revision
 * 
 *
 */



// ------------------- 
// Defines and Globals
// -------------------

uiSlab* uiCurrentSlab = NULL;

// ---------
// INTERNALS
// ---------

errtype ui_init_slabs(void)
{
   uiCurrentSlab = NULL;
   return OK;
}


// -------------
// API FUNCTIONS
// -------------

errtype uiMakeSlab(uiSlab* slab, Region* cursor_reg, Cursor* default_cursor)
{
   errtype err;
   extern errtype ui_init_focus_chain(uiSlab* slab);
   extern errtype ui_init_cursor_stack(uiSlab* slab, Cursor* default_cursor);

   slab->creg = cursor_reg;
   err = ui_init_focus_chain(slab);
   if (err != OK) return err;
   err = ui_init_cursor_stack(slab,default_cursor);
   if (err != OK) return err;
   return OK;
}

extern void call_deferred(void);

errtype uiDestroySlab(uiSlab* slab)
{
   if (slab == uiCurrentSlab)
      call_deferred();
   slab->creg = NULL;
   uiDestroyCursorStack(&slab->cstack);
   array_destroy(&slab->fchain.chain);
   return OK;
}

errtype uiSetCurrentSlab(uiSlab* slab)
{
   call_deferred();
   uiCurrentSlab = slab;
   return OK;
}

errtype uiGetCurrentSlab(uiSlab** slab)
{
   *slab = uiCurrentSlab;
   return OK;
}



