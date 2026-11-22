/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/csg/hep.c,v 1.5 2000/02/19 12:55:04 toml Exp $
//  "hheap" data structure

#include <stdint.h>
#include <string.h>

#include <lg.h>
#include <hep.h>
#include <mprintf.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

#define MIN_ALLOC   30000

union HepBlock {
   union HepBlock *previous;
   long double alignment;  // keep the payload after this header aligned
};

void MoreHep(Hep *h)
{
   size_t block_size = MIN_ALLOC;
   if (block_size < sizeof(union HepBlock) + h->stride)
      block_size = sizeof(union HepBlock) + h->stride;

   union HepBlock *block = Malloc(block_size);
   if (!block)
      Error(1, "Out of memory allocating a hep block.\n");

   block->previous = h->last;
   h->last = block;
   h->block = (char *) (block + 1);
   h->left = block_size - sizeof(*block);
}

void InitHep(Hep *h)
{
   h->next = 0;
   h->block = 0;
   h->last = 0;
   h->left = 0;
   h->alloc = 0;
   h->total = 0;
}

void MakeHep(Hep *h, int size)
{
   const size_t alignment = _Alignof(union HepBlock);
   h->size = size;
   h->stride = ((size_t) size + alignment - 1) / alignment * alignment;
   InitHep(h);
}

void ResetHep(Hep *h)
{
#ifdef DBG_ON
   if (h->alloc) Warning(("Freeing non-empty hep of size %d (%d still alloced)\n", h->size, h->alloc));
#endif
   while (h->last) {
      union HepBlock *block = h->last;
      h->last = block->previous;
      Free(block);
   }
   InitHep(h);
}

void *HepAlloc(Hep *h)
{
   void *q;
   if (h->next) {
      q = h->next;
      h->next = *(void **) h->next;
   } else {
      if (!h->size) {
         Error(1, "Tried to allocate from an uninitialized hep.\n");
      }

      if (h->left < h->stride)
         MoreHep(h);
      q = h->block;
      h->block += h->stride;
      h->left -= h->stride;
      ++h->total;
   }
   ++h->alloc;
   return q;
}

void HepFree(Hep *h, void *p)
{
   const uint32_t free_marker = 0xabadacae;
   if ((size_t) h->size >= sizeof(h->next) + sizeof(free_marker)) {
      // The free-list link occupies the first pointer-sized bytes.
      char *marker_address = (char *) p + sizeof(h->next);
      uint32_t marker;
      memcpy(&marker, marker_address, sizeof(marker));
      if (marker == free_marker)
         mprintf("Probable double-free in hep size %d.\n", h->size);
      memcpy(marker_address, &free_marker, sizeof(free_marker));
   }

   *(void **) p = h->next;
   h->next = p;
   if (--h->alloc < 0)
      mprintf("Too many frees in hep size %d.\n", h->size);
}

int HepSize(Hep *h)
{
   return h->total * h->size;
}
