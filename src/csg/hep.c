/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/csg/hep.c,v 1.5 2000/02/19 12:55:04 toml Exp $
//  "hheap" data structure

#include <lg.h>
#include <hep.h>
#include <mprintf.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

#define MIN_ALLOC   30000

void MoreHep(Hep *h)
{
   h->left  = MIN_ALLOC-4;
   h->block = Malloc(MIN_ALLOC);
   *((char **) h->block) = h->last;
   h->last = h->block;
   h->block += 4;
}

void InitHep(Hep *h)
{
   h->next = 0;
   h->last = 0;
   h->left = 0;
   h->alloc = 0;
   h->total = 0;
}

void MakeHep(Hep *h, int size)
{
   h->size = size;
   InitHep(h);
}

void ResetHep(Hep *h)
{
   char *p,*q;
#ifdef DBG_ON
   if (h->alloc) Warning(("Freeing non-empty hep of size %d (%d still alloced)\n", h->size, h->alloc));
#endif
   p = h->last;
   while (p) {
      q = * (char **) p;
      Free(p);
      p = q;
   }
   InitHep(h);
}

void *HepAlloc(Hep *h)
{
   void *q;
   if (h->next) {
      q = h->next;
      h->next = * (char **) h->next;
   } else {
      if (!h->size) {
         Error(1, "Tried to allocate from an uninitialized hep.\n");
      }

      if (h->left < h->size)
         MoreHep(h);   
      q = h->block;
      h->block += h->size;
      h->left -= h->size;
      ++h->total;
   }
   ++h->alloc;
   return q;
}

void HepFree(Hep *h, void *p)
{
   if (h->size >= 8) {
      if (((int *) p)[1] == 0xabadacae)
         mprintf("Probable double-free in hep size %d.\n", h->size);
      ((int *) p)[1] = 0xabadacae;
   }

   * (char **) p = h->next;
   h->next = p;
   if (--h->alloc < 0)
      mprintf("Too many frees in hep size %d.\n", h->size);
}

int HepSize(Hep *h)
{
   return h->total * h->size;
}
