/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <stdlib.h>
#include <stdio.h>
#include <mem.h>

#include <lg.h>
#include <pt_mem.h>

#define MAX_RECT     1024

struct info
{
   uchar *pt;
   int wid, ht;
   uchar data;
} rect[MAX_RECT];

int num_added, num_deleted;

int sizes[] = { 1,2,4,8, 16,32,64,128, 256 };

void delete(int x)
{
   int i,j;
   ++num_deleted;

   for(j=0; j < rect[x].ht; ++j)
      for (i=0; i < rect[x].wid; ++i)
         if (rect[x].pt[j*256+i] != rect[x].data) goto ouch;
   portal_free_mem_rect(rect[x].pt, rect[x].wid, rect[x].ht);
   rect[x].pt = 0;
   return;

ouch:
   Error(1, "Bad data!\n");
}

void add(int x)
{
   int i ,sz, y;

   ++num_added;
   sz = rand() % 8;
   y = rand() >> 3;

   rect[x].wid = sizes[sz + !!(y & 1)];
   rect[x].ht = sizes[sz + !!(y & 2)];
   rect[x].data = rand();
   rect[x].pt = portal_allocate_mem_rect(rect[x].wid, rect[x].ht);
   for (i=0; i < rect[x].ht; ++i)
      memset(rect[x].pt + i*256, rect[x].data, rect[x].wid);
}

void process_rect(int x)
{
   if (rect[x].pt) delete(x); else add(x);
}

extern int max_strips;

int main(int argc, char **argv)
{
   int n, i;
   MemCheckOn(TRUE);

   n = argc > 1 ? atoi(argv[1]) : 4096;

   while(n--)
      process_rect((unsigned int) rand() % MAX_RECT);

   for (i=0; i < MAX_RECT; ++i)
      if (rect[i].pt)
         delete(i);

   printf("Successfully inserted %d and deleted %d\n", num_added, num_deleted);
   printf("%d strips\n", max_strips);
   return 0;
}
