/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/portal/pt_mem.c,v 1.13 2000/02/19 13:18:47 toml Exp $

// texture map storage management

#include <string.h>

#include <lg.h>
#include <dev2d.h>
#include <stdlib.h>
#include <r3d.h>

#include <port.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

  // damn people who misspell things
#define ASSERT(x) \
     if (!(x)) Error(1, "Assertion failed: %s %d\n", __FILE__, __LINE__); else


static uchar temp[64][256];
static uchar temp2[64][256];
static grs_bitmap dummy[8];

#pragma off(unreferenced)
grs_bitmap *make_texture_map(int i)
{
   static int init = TRUE;
   if (init) {
      int x,y;
      for (x=0; x < 64; ++x)
         for (y=0; y < 64; ++y)
            temp[x][y] = ((x^y) << 3) + ((rand() >> 2) % 3);
      for (x=0; x < 64; ++x)
         temp[x][0] = temp[x][1] = 0,
         temp[0][x] = temp[1][x] = 8;
      for (x=0; x < 64; ++x)
         for (y=0; y < 64; ++y)
            if ((x & 3) && (y & 3))
               temp2[x][y] = 0;

      for (x=0; x <= 6; ++x) {
         dummy[x].bits = &temp[0][0];
         dummy[x].row  = 256;
         dummy[x].h    = 1 << (6-x);
         dummy[x].w    = 1 << (6-x);
         dummy[x].hlog = 6-x;
         dummy[x].wlog = 8;
         dummy[x].type = BMT_FLAT8;
         dummy[x].flags = 0;
      }
      dummy[x].w = 0;

      init = FALSE;
   }

   return dummy;
}
#pragma on(unreferenced)

//////////////////////////////////////////////////////////////////////////////
//
//  texture memory management
//
//    we create a bunch of "strips", which are some height and ROW wide.
//    each strip is maintained as a little separate 1d malloc buffer, with
//    merging on frees (this data must be maintained in a separate buffer).

int   ptmem_row=256;
uchar ptmem_wlog=8;

#define ROW          ptmem_row      // must be a power of two
#define MAX_HOLES    64             // 128 bytes overhead per strip

typedef struct
{
   ushort x;     // x location of hole
   ushort wid;   // width-1 of hole
} Hole;

typedef struct
{
   uchar *block;
   Hole *holes;

   uchar num_alloc;
   uchar num_holes;
   uchar pad0;
   uchar pad1;

   int ht;
} Strip;

static int num_rects=0;               // total number of memrects allocated
int num_strips, max_strips;
static Strip *strip_list;

void portal_set_mem_rect_row(int row)
{
   int i;
   if (row==ptmem_row)
      return;

   clear_surface_cache();
   AssertMsg(num_rects==0,
      "portal_set_mem_rect_row(): all rects must be freed before setting row.");
   AssertMsg(((row-1) & row) == 0, 
      "portal_set_mem_rect_row(): row must be a power of two.");
   ptmem_row = row;
   for (ptmem_wlog = 0, i=1; i<row; i+=i)
      ptmem_wlog++;
}
   
static void free_strip(Strip *s)
{
   Free(s->block);
}

void portal_free_all_mem_rects(void)
{
   int i;
   for (i=0; i < num_strips; ++i)
      free_strip(&strip_list[i]);
   if (strip_list)
   {
      Free(strip_list);
      strip_list=NULL;
   }
   num_strips=0;
   max_strips=0;
}

static void init_strip(Strip *s, int ht)
{
   s->ht = ht;
   s->block = Malloc(ht * ROW + MAX_HOLES * sizeof(Hole));
   s->holes = (Hole *) (s->block + ht*ROW);  // relies on s->block is uchar
   s->num_alloc = 0;
   s->num_holes = 1;

   s->holes[0].x = 0;
   s->holes[0].wid = ROW-1;
}

static Strip *alloc_strip(int ht)
{
   int mem;
   ++num_strips;
   if (num_strips > max_strips) {
      max_strips = num_strips;
      mem = num_strips * sizeof(Strip);
      if (strip_list)
         strip_list = Realloc(strip_list, mem);
      else
         strip_list = Malloc(mem);
   }
   init_strip(&strip_list[num_strips-1], ht);
   return &strip_list[num_strips-1];
}

static void delete_hole(Strip *s, int hole)
{
   memmove(&s->holes[hole], &s->holes[hole+1], sizeof(Hole) * (s->num_holes-1 - hole));
   --s->num_holes;
}

static void insert_hole(Strip *s, int hole)
{
   ASSERT(s->num_holes < MAX_HOLES);
   memmove(&s->holes[hole+1], &s->holes[hole], sizeof(Hole) * (s->num_holes - hole));
   ++s->num_holes;
}

static uchar *allocate_rectangle(Strip *s, int hole, int w)
{
   uchar *p = s->block + s->holes[hole].x;

   if (s->holes[hole].wid == w-1) {
      delete_hole(s, hole);
   } else {
      s->holes[hole].wid -= w;
      s->holes[hole].x += w;
   }

   s->num_alloc += 1;

   return p;
}

static uchar *alloc_rectangle_from_strip(Strip *s, int w)
{
   int i;

   // make sure there's room to allocate more... due to merging,
   //   at most every other alloced block can be turned into a hole
   if (s->num_alloc/2 + s->num_holes >= MAX_HOLES) 
      return 0;

   for (i=0; i < s->num_holes; ++i)
      if (s->holes[i].wid >= w-1)
         return allocate_rectangle(s, i, w);

   return 0;
}

static void free_rectangle_from_strip(Strip *s, int x, int w)
{
   bool before,after;
   int i;

   // search for the first free node after this
   for (i=0; i < s->num_holes; ++i)
      if (s->holes[i].x > x)
         break;

   s->num_alloc -= 1;

   // hole # s->num_holes should come immediately after this hole
   // first check if we attach to either adjacent hole

   before = (i-1 >= 0) && (s->holes[i-1].x + s->holes[i-1].wid+1 == x);
   after = (i < s->num_holes) && (x+w == s->holes[i].x);

   if (before && after) {
      s->holes[i-1].wid += w + s->holes[i].wid+1;
      delete_hole(s, i);
   } else if (before) {
      s->holes[i-1].wid += w;
   } else if (after) {
      s->holes[i].x -= w;
   } else {
      insert_hole(s, i);
      s->holes[i].x = x;
      s->holes[i].wid = w-1;
   }
}

uchar *portal_allocate_mem_rect(int x, int y)
{
   Strip *s;
   uchar *p;
   int i;

   ASSERT(x <= ROW && y <= ROW+4);

   num_rects++;

   if (x == ROW)
      return Malloc(x * y);

   s = strip_list;

   for (i=0; i < num_strips; ++i, ++s) {
      if (s->ht == y) {
         p = alloc_rectangle_from_strip(s, x);
         if (p) return p;
      }
   }

   p = alloc_rectangle_from_strip(alloc_strip(y), x);
   ASSERT(p);

   return p;
}

#ifndef DBG_ON
#pragma off(unreferenced)
#endif
void portal_free_mem_rect(uchar *p, int x, int y)
{
   Strip *s;
   int i;

   num_rects--;

   if (x == ROW) {
      Free(p);
      return;
   }

   s = strip_list;
   for (i=0; i < num_strips; ++i, ++s) {
      if (s->block <= p && p < s->block+ROW) {
         ASSERT(s->ht == y);
         free_rectangle_from_strip(s, p - s->block, x);

         // free this strip if it's empty
         if (s->num_alloc == 0) {
            free_strip(s);
            *s = strip_list[--num_strips];
         }

         return;
      }
   }

#ifdef DBG_ON
   Error(1, "portal_free_rectangle: rectangle not found\n");
#endif
}
#ifndef DBG_ON
#pragma on(unreferenced)
#endif
