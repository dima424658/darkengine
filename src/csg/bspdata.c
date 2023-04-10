/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/csg/bspdata.c,v 1.6 2000/02/19 12:26:34 toml Exp $

#include <lg.h>
#include <csg.h>
#include <bspdata.h>
#include <hep.h>
#include <mprintf.h>
#include <media.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

Hep Nodes;

void dump_plane(BspPlane *p)
{
   mprintf("Plane: %g %g %g %g\n", p->a, p->b, p->c, p->d);
}

void dump_node(BspNode *b, int n, int inf)
{
   int i;
   for (i=0; i < n; ++i)
      mprintf(" ");
   if (b->leaf) {
      mprintf("Leaf: solid %d\n", (int) b->medium);
   } else {
      mprintf("Inside: %g %g %g %g\n", b->split_plane.a, b->split_plane.b, b->split_plane.c, b->split_plane.d);
      dump_node(b->inside, n+1, inf);
      for (i=0; i < n; ++i)
         printf(" ");
      mprintf("Outside: %g %g %g %g\n", b->split_plane.a, b->split_plane.b, b->split_plane.c, b->split_plane.d);
      dump_node(b->outside, n+1, inf);
   }
}

void dump_tree(void *tree)
{
   dump_node((BspNode *) tree, 0, 0);
}

/////
//
// storage and type management
//

void BspChangeLeafToNode(BspNode *b, BspPlane *p)
{
   b->leaf = 0;
   b->split_plane = *p;
}

BspNode *BspAllocateLeaf()
{
   BspNode *b;
   b = HepAlloc(&Nodes);

   b->leaf = 1;
   b->mark = 0;
   b->user_data = 0;
   b->ph = 0;
   b->cell_id = 0;

   return b;
}

void (*CSGfree)(void *);

void BspFreeLeaf(BspNode *b)
{
   if (b->user_data) {
      if (CSGfree) CSGfree(b->user_data);
      b->user_data = 0;
   }
   HepFree(&Nodes, b);
}

void BspRecursiveFreeNode(BspNode *b)
{
   if (IS_LEAF(b))
      BspFreeLeaf(b);
   else {
      BspRecursiveFreeNode(b->inside);
      BspRecursiveFreeNode(b->outside);
      BspFreeLeaf(b);  // not really a leaf, but hey
   }
}

int base_medium = MEDIA_SOLID;
extern BspPlane all_planes[];
extern int find_plane(BspPlane *p);

static char bsp_buffer[256];

int BspReadNode(BspNode **n, FILE *f)
{
   BspPlane p;
   *n = BspAllocateLeaf();
   (*n)->medium = base_medium;
   (*n)->parent = 0;

   fgets(bsp_buffer, 256, f);

   if (sscanf(bsp_buffer, "-1 %lg%lg%lg%lg", &p.a, &p.b, &p.c, &p.d) == 4) {
      int z = find_plane(&p);
      if (z >= 0) {
         BspPlane *q = &all_planes[z];
         if (q->a*p.a + q->b*p.b + q->c*p.c > 0)
            p = *q;
         else {
            p.a = -q->a;      
            p.b = -q->b;      
            p.c = -q->c;      
            p.d = -q->d;
         }
      } else {
         extern void show_match_plane(BspPlane *);
         Warning(("BspReadNode: optimized split plane not from any brush.\n"));
         mprintf("%lg %lg %lg %lg (input)\n", p.a,p.b,p.c,p.d);
         show_match_plane(&p);
      }
      BspChangeLeafToNode(*n, &p);
      if (BspReadNode(&(*n)->inside, f)) return 1;
      if (BspReadNode(&(*n)->outside, f)) return 1;
      (*n)->inside->parent = *n;
      (*n)->outside->parent = *n;
   } else if (bsp_buffer[0] != '0') {
      mprintf("Error in bsp file: %s\n", bsp_buffer);
      return 1;
   }
   return 0;
}

void *BspReadTree(FILE *f)
{
   BspNode *n;

   if (BspReadNode(&n, f))
      return 0;    // memory leak, but keep going

   return n;
}

void *BspMakeTree(void)
{
   BspNode *n = BspAllocateLeaf();
   n->medium = base_medium;
   n->parent = 0;

   return n;
}

int BspCompareNode(BspNode *n, FILE *f)
{
   BspPlane p;
   // ok, now we read whether this guy has a split plane or not

   fgets(bsp_buffer, 256, f);

   if (sscanf(bsp_buffer, "-1 %lg%lg%lg%lg", &p.a, &p.b, &p.c, &p.d) == 4) {
      if (n && n->medium != NO_MEDIUM) {
         Warning(("BspCompareNode: Node that was split in optimizer "
                  "not split in portalization.\n"));
         n = 0;  // ignore further recursive warnings
      }
      if (BspCompareNode(n ? n->inside : 0, f)) return 1;
      if (BspCompareNode(n ? n->outside : 0, f)) return 1;
   } else if (bsp_buffer[0] == '0') {
      if (n && n->medium == NO_MEDIUM) {
         Warning(("BspCompareNode: Node that was unsplit in optimizer "
                  "split in portalization.\n"));
      }
   } else {
      return 1;
   }
   return 0;
}

void BspFreeTree(void *tree)
{
   BspRecursiveFreeNode((BspNode *) tree);
}

