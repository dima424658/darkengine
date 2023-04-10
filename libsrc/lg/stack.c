/*
** stack.c
**
** Routines for controlling user-defined stacks of large memory
**
** $Header: r:/prj/lib/src/lg/rcs/stack.c 1.4 1995/01/10 19:07:29 rex Exp $
** $Log: stack.c $
 * Revision 1.4  1995/01/10  19:07:29  rex
 * Fixed Free warning to use hex format for ptr
 * 
 * Revision 1.3  1993/12/20  13:40:17  ept
 * Added MemStackRealloc.
 * 
 * Revision 1.2  1993/09/13  12:40:25  dfan
 * ptr and size were reserved words in assembler
 * Warning if MemStackAlloc returns NULL
 * 
 * Revision 1.1  1993/09/13  11:10:56  dfan
 * Initial revision
 * 
*/

#include <memall.h>
#include <dbg.h>

//////////////////////////////
//
// Initializes a MemStack.  The user must already have allocated the memory
// himself, and have set baseptr and size in the MemStack structure.
//
void MemStackInit (MemStack *ms)
{
   ms->topptr = ms->baseptr;
}

//////////////////////////////
//
// Allocates size bytes of memory from the MemStack.
//
void *MemStackAlloc (MemStack *ms, long size)
{
   char *newptr = (char *) ms->topptr + size;
   char *oldptr = (char *) ms->topptr;

   if (newptr > (char *) ms->baseptr + ms->sz)
   {
      Warning (("MemStackAlloc: requested %ld, only %ld left\n", size,
         (long) ((char *) ms->baseptr + ms->sz) - (long) (ms->topptr)));
      return NULL;
   }

   ms->topptr = (void *) newptr;
   return oldptr;
}

//////////////////////////////
//
// Change the size of the ptr.  Note that if this is not the last on
// the stack bad things will occur!
//
void *MemStackRealloc (MemStack *ms, void *ptr, long newsize)
{
   char *newptr = (char *)ptr + newsize;

   if (newptr > (char *)ms->baseptr + ms->sz)
   {
      Warning(("MemStackRealloc: %ld too large, only %ld left\n", newsize,
         (long) ((char *) ms->baseptr + ms->sz) - (long) (ms->topptr)));
      return NULL;
   }
   ms->topptr = (char *)ptr + newsize;
   return ptr;
}

//////////////////////////////
//
// Frees memory allocated with MemStackAlloc().  You must free memory in
// reverse order from allocating it - this is a stack.  Violations may
// not be caught right away.
//
bool MemStackFree (MemStack *ms, void *ptr)
{
   if (ms->topptr < ptr)
   {
      Warning (("MemStackFree: $%x freed before $%x\n", ms->topptr, ptr));
      return FALSE;
   }

   // Return ms->topptr to where it was when we allocated ptr: namely, ptr
   ms->topptr = ptr;
   return TRUE;
}
