/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <lg.h>
#include <csg.h>
#include <bspdata.h>
#include <csgbrush.h>
#include <media.h>
#include <mediaop.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

extern int base_medium;

///////////////////////////////////////////////////////////////////
//
//   CSG math
//
//

// dangling off of every node we keep a linked list of the brushes
// which fully contain this node.  We put the brushes in buckets to
// reduce overhead.

// this is the data structure pointed to by user_data in BspNode

#define BRUSH_BUCKET_SIZE    6
typedef struct csg_data{
   short brushes[BRUSH_BUCKET_SIZE];     // a list of those brushes which contain this thing
   struct csg_data *next;
} CsgData;

Hep csg_hep;

void free_csg_data(void *head)
{
   CsgData *cur = (CsgData *) head, *next;
   while (cur) {
      next = cur->next;
      HepFree(&csg_hep, cur);
      cur = next;
   }
}

void csgdata_add_brush(void **head, int brush)
{
   CsgData *cur;
   int i;

   cur = (CsgData *) *head;
   
   if (cur) {
      for (i=0; i < BRUSH_BUCKET_SIZE; ++i) {
         if (cur->brushes[i] == -1) {
            cur->brushes[i] = brush;
            return;
         }
      }
//printf("> 1 bucket when adding brush %d\n");
   }

   cur = HepAlloc(&csg_hep);
   for (i=1; i < BRUSH_BUCKET_SIZE; ++i)
      cur->brushes[i] = -1;

   cur->brushes[0] = brush;

   cur->next = *head;
   *head = cur;
}

void csgdata_delete_brush(void **head, int brush)
{
   CsgData *cur, *first;
   int i;

   first = cur = (CsgData *) *head;
  
   while (cur) {
      for (i=0; i < BRUSH_BUCKET_SIZE; ++i)
         if (cur->brushes[i] == brush)
            break;

      if (i == BRUSH_BUCKET_SIZE)
         cur = cur->next;
      else {
           // delete this, and move the hole
           // to the 0'th slot
//if (cur != first) printf("delete %d from not-first-bucket.\n", brush);
//else if (cur->next) printf("delete %d with > 1 bucket.\n", brush);
         cur->brushes[i] = cur->brushes[0];
           // now search in the first bucket
           // for a non -1 brush
         for (i=0; i < BRUSH_BUCKET_SIZE; ++i)
            if (first->brushes[i] != -1)
               break;
         if (i == BRUSH_BUCKET_SIZE)
            Error(1, "oops, invariant failed in csgdata_delete_brush\n");
         cur->brushes[0] = first->brushes[i];
         first->brushes[i] = -1;
         if (i == BRUSH_BUCKET_SIZE-1) {
            // we've emptied out the last one of these
            *head = first->next;
            HepFree(&csg_hep, first);
         }
         return;
      }
   }
   Error(1, "csgdata_delete_brush: deletion failed, brush not present.\n");
}

// Now, as we iterate down the tree, we maintain
// a linked list of all of the brushes that this node
// is within

typedef struct active_brush ActiveBrush;

struct active_brush
{
   ActiveBrush *next;
   short brush;
   short timestamp;
};

Hep ab_hep;

ActiveBrush *active;

int cur_brush;

static void add_active_brush(int b, int t)
{
   ActiveBrush **cur, *this;
//printf("add %d\n",b);

   cur = &active;

   while ((*cur) && (*cur)->timestamp < t)
      cur = &(*cur)->next;

   this = HepAlloc(&ab_hep);
   this->brush = b;
   this->timestamp = t;

   this->next = *cur;
   *cur = this;
}

static void delete_active_brush(int b)
{
   ActiveBrush **cur, *this;
//printf("delete %d\n",b);

#ifdef DBG_ON
   if (!active) Error(1, "delete_active_brush: brush list is empty deleting %d during %d.\n", b, cur_brush);
#endif

   cur = &active;

   while ((*cur) && (*cur)->brush != b)
      cur = &(*cur)->next;

   this = *cur;

#ifdef DBG_ON
   if (!this)
      Error(1, "delete_active_brush: tried to delete non-active brush %d during %d.\n", b, cur_brush);
#endif

   *cur = this->next;
   HepFree(&ab_hep, this);
}

void preorder_update_active_brushes(BspNode *b)
{
   CsgData *list = (CsgData *) b->user_data;
   int i, x;

   while (list) {
      for (i=0; i < BRUSH_BUCKET_SIZE; ++i) {
         x = list->brushes[i];
         if (x != -1)
            add_active_brush(x, CB_TIMESTAMP(x));
      }
      list = list->next;
   }
}

void postorder_update_active_brushes(BspNode *b)
{
   CsgData *list = (CsgData *) b->user_data;
   int i, x;

   while (list) {
      for (i=0; i < BRUSH_BUCKET_SIZE; ++i) {
         x = list->brushes[i];
         if (x != -1)
            delete_active_brush(x);
      }
      list = list->next;
   }
}

// compute the medium of a given area based on the active list

extern int csg_clip_count;
static void compute_medium(BspNode *b)
{
   // iterate over active list computing medium;
   int medium = base_medium, op;
   int clipped=0;
   ActiveBrush *a = active;

   while (a) {
      op = CB_MEDIUM(a->brush);
      if (op == CB_MEDIUM_HOTREGION)
         ++clipped;
      else
         medium = media_op[op][medium];
      a = a->next;
   }

   // if there are any hot regions enabled,
   // and we are not inside any of them,
   // reset to default
   if (csg_clip_count && !clipped)
      medium = base_medium;

   b->medium = medium;
}

void recursive_recompute_node(BspNode *b)
{
   preorder_update_active_brushes(b);
   if (IS_LEAF(b)) {
      compute_medium(b);
   } else {
      recursive_recompute_node(b->inside);
      recursive_recompute_node(b->outside);
   }
   postorder_update_active_brushes(b);
}

void recompute_node(BspNode *b)
{
   if (IS_LEAF(b)) {
      compute_medium(b);
   } else {
      recursive_recompute_node(b->inside);
      recursive_recompute_node(b->outside);
   }
}

  // this routine is invoked on the nodes that contain a given brush
void add_area(BspNode *b)
{
   csgdata_add_brush(&b->user_data, cur_brush);   
   add_active_brush(cur_brush, CB_TIMESTAMP(cur_brush));
   recompute_node(b);
}

void init_csgmedia(void)
{
   MakeHep(&csg_hep, sizeof(CsgData));
   MakeHep(&ab_hep, sizeof(ActiveBrush));
}
