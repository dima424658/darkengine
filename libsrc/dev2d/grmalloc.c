/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/grmalloc.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 15:54:37 $
 *
 * Internal memory allocation routines.
 *
 * This file is part of the dev2d library.
 */

#include <malloc.h>

/* dynamic memory allocation/deallocation is done through the indirected
   functions gr_malloc() and gr_free(). they default to malloc() and
   free(). library clients can change the default with gr_set_malloc()
   and gr_set_free(). */
void *(*gr_malloc)(int n) = malloc;
void (*gr_free)(void *m) = free;

/* set 2d's internal function pointer to a malloc routine. */
void gr_set_malloc (void *(*malloc_func)(int bytes))
{
   gr_malloc = malloc_func;
}

/* set 2d's internal function pointer to a free routine. */
void gr_set_free (void (*free_func)(void *mem))
{
   gr_free = free_func;
}
