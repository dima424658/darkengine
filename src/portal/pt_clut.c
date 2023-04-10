/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/portal/pt_clut.c,v 1.4 2000/02/19 13:18:44 toml Exp $

#include <lg.h>
#include <port.h>
#include <pt_clut.h>
#include <string.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

  // pointer to user cluts
uchar *pt_clut_list[256];

  // number of cluts we keep cached
#define CLUT_CACHE_SIZE    4

  // length of clut chain we cache
#define CLUT_CACHE_IDSIZE  8

  // storage for the clut cache
static uchar pt_clut_cache[CLUT_CACHE_SIZE][256];

static uchar pt_clut_cache_idlist[CLUT_CACHE_SIZE][CLUT_CACHE_IDSIZE];
static uchar pt_clut_cache_idlist_len[CLUT_CACHE_SIZE];

  // compare a clut chain with an idlist, assuming first one already matched
static bool compare_clut_chain(ClutChain *cc, int n)
{
   int i=0, len = pt_clut_cache_idlist_len[n];
   while (i < len) {
      if (cc->clut_id != pt_clut_cache_idlist[n][i])
         return FALSE;
      ++i;
      if (cc->clut_id2) {
         if (i >= len) return FALSE;

         if (cc->clut_id2 != pt_clut_cache_idlist[n][i])
            return FALSE;
         ++i;
      }
      cc = cc->next;
      if (!cc)
         return i == n;
   }
   return FALSE;
}

uchar *pt_get_clut(ClutChain *cc)
{
   static int ref;
   int i, c;

     // check if it's just a single clut
   if (!cc->next && !cc->clut_id2)
      return pt_clut_list[cc->clut_id];

     // check if we already have this one cached
   c = cc->clut_id;
   for (i=0; i < CLUT_CACHE_SIZE; ++i)
      if (pt_clut_cache_idlist[i][0] == c && pt_clut_cache_idlist_len[i])
         if (compare_clut_chain(cc, i))
            return pt_clut_cache[i];

     // cycle through to the next cache entry
     //   note lack of LRUness... who cares,
     //   this should be super rare if it every happens at all
   if (++ref == CLUT_CACHE_SIZE) ref = 0;

   {
      uchar *dest = pt_clut_cache[ref];
      uchar *s1 = pt_clut_list[cc->clut_id];
      uchar *s2;
      int j, n;

      pt_clut_cache_idlist[ref][0] = cc->clut_id;

      if (cc->clut_id2) {
         s2 = pt_clut_list[cc->clut_id2];
         pt_clut_cache_idlist[ref][1] = cc->clut_id2;
         for (j=0; j < 256; ++j)
            dest[j] = s2[s1[j]];
         n = 2;
      } else {
         memcpy(dest, s1, 256);
         n = 1;
      }

      while (cc->next) {
         cc = cc->next;
         if (n < CLUT_CACHE_IDSIZE)
            pt_clut_cache_idlist[ref][n] = cc->clut_id;
         ++n;

         s2 = pt_clut_list[cc->clut_id];
         for (j=0; j < 256; ++j)
            dest[j] = s2[dest[j]];

         if (cc->clut_id2) {
            if (n < CLUT_CACHE_IDSIZE)
               pt_clut_cache_idlist[ref][n] = cc->clut_id2;
            ++n;

            s2 = pt_clut_list[cc->clut_id2];
            for (j=0; j < 256; ++j)
               dest[j] = s2[dest[j]];
         }
      }

      pt_clut_cache_idlist_len[ref] = (n <= CLUT_CACHE_IDSIZE ? n : 0);
   }

   return pt_clut_cache[ref];
}
