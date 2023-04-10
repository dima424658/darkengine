/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//  $Header: r:/t2repos/thief2/libsrc/portold/portal.c,v 1.99 1998/04/16 20:00:07 buzzard Exp $
//
//  PORTAL
//
//  dynamic portal/cell-based renderer

#ifndef SHIP
#include <stdio.h>
#endif

#include <string.h>
#include <math.h>

#include <lg.h>
#include <dev2d.h>
#include <r3d.h>
#include <g2pt.h>
#include <mprintf.h>
#include <timer.h>

#include <portal_.h>
#include <portdraw.h>
#include <portclip.h>
#include <portsky.h>
#include <pt_clut.h>
#include <animlit.h>
#include <portwatr.h>
#include <bspsphr.h>

#include <refsys.h>

#include <profile.h>

#ifndef SHIP
// write out a log of our portal traversal
static FILE *trav_file;
#endif

  // enable this to collect per-scene statistics
#define STATS_ON

  // do some internal checking
//#define VALIDATE

//#define VALIDATE_UNIQUENESS_OF_PORTALS

#if defined(DBG_ON) || defined(PROFILE_ON)
  #define STATIC
#else
  #define STATIC static
#endif

  // max object fragments visible onscreen at once
#define MAX_VISIBLE_OBJECTS 4096

  // does lighting compute light maps or at vertices?
  // We probably don't really support vertex lighting anymore
#define LIGHT_MAP

#ifdef DBG_ON
  bool debug_cell_traversal;
  #define CELL_DEBUG(x)  if (debug_cell_traversal) x; else
#else
  #define CELL_DEBUG(x)
#endif

/////////////////////    globals    /////////////////////////

  // default callback for turning texture id into texture
extern grs_bitmap *make_texture_map(int d);
r3s_texture (*portal_get_texture)(int d) = make_texture_map;

  // default callbacks
#pragma off(unreferenced)
static void render_object(ObjID obj, uchar *clut, ulong fragment)
{
}

static void dummy_sfx(int cell)
{
}

static BOOL object_visible(ObjID o1)
{
   return FALSE;
}

static BOOL object_blocks(ObjID o1, ObjID o2)
{
   return FALSE;
}
#pragma on(unreferenced)

void (*portal_render_object)(ObjID, uchar *, ulong fragment) = render_object;
BOOL (*portal_object_visible)(ObjID) = object_visible;
BOOL (*portal_object_blocks)(ObjID, ObjID) = object_blocks;

void (*portal_sfx_callback)(int cell) = dummy_sfx;

bool portal_allow_object_splitting = TRUE;

unsigned char pt_medium_entry_clut[256];
unsigned char pt_medium_exit_clut[256];
unsigned char pt_medium_haze_clut[256];

mxs_vector portal_camera_loc;

/////////////////////////////////////////////////////////////////////
//
//  statistics

#ifdef DBG_ON
  #ifndef STATS_ON
  #define STATS_ON
  #endif
#endif

#ifdef STATS_ON

 #define STAT(x)  (x)

 static int stat_num_port;
 static int stat_num_port_visit;
 static int stat_num_port_traverse;
 static int stat_num_cell;
 static int stat_num_cell_visit;
 static int stat_num_cell_explored;
 static int stat_num_facing;
 static int stat_num_objects;
 static int stat_num_visible_objects;
 static int stat_num_terrsplit_objects;
 static int stat_num_resplit_objects;
 static int stat_num_hidden_objects;
 static int stat_num_cell_tests;
 int stat_num_polys_clipped_away;
 int stat_num_poly_raw;
 int stat_num_poly_considered;
 int stat_num_poly_drawn;
 int stat_num_lit_pixels;
 int stat_num_source_pixels;
 int stat_num_spans_drawn;
 int stat_num_spans_clamped;
 int stat_num_dest_pixels;
 int stat_num_backface_tests;
 int stat_num_traverse_ms;
 int stat_num_render_ms;
 int stat_num_sort_ms;
 int stat_num_object_ms;
 int stat_num_drawn_pixels;
 int stat_num_clipped_pixels;
 int stat_num_transp_pixels;
 int stat_max_undrawn_pixels;
 int stat_max_undrawn_polys;
 int stat_max_total_polys;

#else

 #define STAT(x)

#endif

#define STAT_INC(x)  STAT(++stat_num_##x)

///// TODO: move externs into h file

extern ClipData *PortalGetClipInfo(PortalCell *, PortalPolygonCore *, int voff, ClipData *);
extern uchar compute_water_clut(mxs_real, mxs_real);
extern mxs_real compute_portal_z(void);
extern void duv_set_size(int x, int y);
extern void draw_region(int);
extern int  pick_region(PortalCell *, int x, int y);
extern bool portal_raycast_light(PortalCell *, Location *loc, uchar perm);
extern bool portal_nonraycast_light(PortalCell *, Location *loc, uchar perm);
extern void portal_dynamic_light(PortalCell *, Location *loc);
extern void portal_dynamic_dark(PortalCell *, Location *loc);
extern void render_background_hack();
extern void background_hack_cleanup(void);
extern void PortalSetLightInfo(Location *l, float br, uchar start);
extern bool dynamic_light;
extern void light_region(int);
//extern void spotlight_region(int);
extern void init_portal_light(void);
extern void init_portal_shading(int dark, int light);
extern void init_background_hack(void);

///// examine all portals in region /////

void add_region(int, ClipData *c, PortalCell *src, PortalPolygonCore *port);

  // explore through the portal
void explore_portals(PortalCell *r)
{  PROF
   int n = r->num_portal_polys;
   PortalPolygonCore *p = r->portal_poly_list;
   int v = r->portal_vertex_list;
   void *clip = CLIP_DATA(r);

   // If this cell is in a door, or something silly like that, we
   // won't explore from it.
   if (r->flags & CELL_BLOCKS_VISION) {
      END_PROF;
      return;
   }

   cur_ph = POINTS(r);

   r3_start_block();
   r3_set_clipmode(NEED_CLIP(r) ? R3_CLIP : R3_NO_CLIP);
   while (n--) {
      STAT_INC(port_visit);

#ifndef SHIP
      if (trav_file)
         fprintf(trav_file, "  Consider portal %d:", r->num_portal_polys-n+1);
#endif
      if (check_surface_visible(r, p, v)) {   // if we can see through portal
         ClipData *portal_extent;             // get 2d extents of portal
         portal_extent = PortalGetClipInfo(r, p, v, clip);
         if (portal_extent) {                 // clip against cell's visibility
            STAT_INC(port_traverse);          // if visible, go through it
#ifndef SHIP
            if (trav_file)
               fprintf(trav_file, "  cell %d visible\n", p->destination);
#endif
            add_region(p->destination, portal_extent, r, p);
              // add_region may change our clipmode, so reset it
            r3_set_clipmode(NEED_CLIP(r) ? R3_CLIP : R3_NO_CLIP);
         } else {
#ifndef SHIP
            if (trav_file)
               fprintf(trav_file, "  obscured/offscreen\n");
#endif
         }
      } else {
#ifndef SHIP
         if (trav_file)
            fprintf(trav_file, "  backfaced\n");
#endif
      }
      v += p->num_vertices;
      ++p;
   }
   r3_end_block();
   END_PROF;
}

bool hack_fragment[MAX_VISIBLE_OBJECTS]; // temporary hack
#define FRAGMENT(x)   Assert_(hack_fragment[x])
#define CELL(x)       Assert_(!hack_fragment[x])

///// do object processing for region /////

#define VISOBJ_NULL      (-1)

#define HACK_MAX_OBJ  1024

bool obj_dealt[HACK_MAX_OBJ];  // HACK: need real object dealt flags
bool obj_split[HACK_MAX_OBJ];  // HACK: need real object split flags
bool obj_hide[HACK_MAX_OBJ];   // HACK: need real object hide flags

// only need this for objects which have dealt != 0
PortalCell *obj_first_cell[HACK_MAX_OBJ]; // HACK: argh what a waste

ObjVisibleID obj_fragment_list[HACK_MAX_OBJ];

ObjVisible vis_objs[MAX_VISIBLE_OBJECTS];  // 8K
ObjVisibleID num_visible_objects;
ObjID obj_need_testing[512]; // max visible objects, not fragments
int need_testing_count;

STATIC
int get_visobj(ObjID o, int fragment)
{
   ObjVisibleID his = num_visible_objects++;
   if (his >= MAX_VISIBLE_OBJECTS) return VISOBJ_NULL;
   vis_objs[his].obj = o;
hack_fragment[his] = TRUE;
   vis_objs[his].fragment = fragment;
   return his;
}

STATIC
void queue_object_fragment_in_cell(PortalCell *r, int visobj)
{
   if (visobj < 0) return;

#ifdef STATS_ON
   ++stat_num_visible_objects;
#endif

   vis_objs[visobj].next_visobj = OBJECTS(r);
   OBJECTS(r) = visobj;
}

STATIC
void queue_object_fragment_for_obj(PortalCell *r, int visobj)
{
   int o = vis_objs[visobj].obj;
hack_fragment[visobj] = FALSE;
   vis_objs[visobj].cell = r;
   vis_objs[visobj].next_visobj = obj_fragment_list[o];
   obj_fragment_list[o] = visobj;
}

// first time we encounter an object not split
void deal_with_unsplit_object(PortalCell *r, ObjID o)
{
   int n = get_visobj(o, OBJ_NO_SPLIT);
   queue_object_fragment_in_cell(r, n);
   obj_fragment_list[o] = n;
   obj_first_cell[o] = r;

   // add to the list of unsplit objects which might need splitting
   obj_need_testing[need_testing_count++] = o;
}

// other times we encounter an object not split
void deal_with_object_leftover(PortalCell *r, ObjID o)
{
   queue_object_fragment_for_obj(r, get_visobj(o, OBJ_NO_SPLIT));
}

// first time we encounter an object to be split
void deal_with_splitting_object(PortalCell *r, ObjID o)
{
   queue_object_fragment_in_cell(r, get_visobj(o, OBJ_SPLIT_FIRST));
   obj_fragment_list[o] = VISOBJ_NULL;
}

// other times we encounter an object to be split
void deal_with_split_object(PortalCell *r, ObjID o)
{
   queue_object_fragment_in_cell(r, get_visobj(o, OBJ_SPLIT_OTHER));
}

BOOL poly_greater(PortalCell *r, PortalPolygonCore *p, int v, int axis,
   float value)
{
   int i,n = p->num_vertices;
   uchar *vl = r->vertex_list + v;
   
   for (i=0; i < n; ++i)
      if (r->vpool[*vl++].el[axis] < value)
         return FALSE;
   return TRUE;
}

BOOL poly_less(PortalCell *r, PortalPolygonCore *p, int v, int axis,
   float value)
{
   int i,n = p->num_vertices;
   uchar *vl = r->vertex_list + v;
   
   for (i=0; i < n; ++i)
      if (r->vpool[*vl++].el[axis] > value)
         return FALSE;
   return TRUE;
}

BOOL poly_overlap_2d(PortalCell *r, PortalPolygonCore *p, int v, fix *min2d, fix *max2d)
{
   int ccode = 0, n = p->num_vertices, i, code_2d;
   uchar *vl = r->vertex_list + v;
   r3s_point *plist = POINTS(r);

   // generate the polygon clip code
   for (i=0; i < n; ++i)
      ccode |= plist[vl[i]].ccodes;

   code_2d = 255;

   if (!ccode) {
      // if we don't need to clip, test it directly by generating new ccode,
      // this time we want a ccode_and

      for (i=0; i < n; ++i) {
         r3s_point *pt = plist + vl[i];
         fix xy,yx;
         xy = pt->grp.sx + pt->grp.sy;
         yx = pt->grp.sx - pt->grp.sy;
         if (pt->grp.sx > min2d[0]) code_2d &= ~1;
         if (pt->grp.sx < max2d[0]) code_2d &= ~2;
         if (pt->grp.sy > min2d[1]) code_2d &= ~4;
         if (pt->grp.sy < max2d[1]) code_2d &= ~8;
         if (xy > min2d[2]) code_2d &= ~16;
         if (xy < max2d[2]) code_2d &= ~32;
         if (yx > min2d[3]) code_2d &= ~64;
         if (yx < max2d[3]) code_2d &= ~128;
      }
   } else {
      // clip the polygon
      r3s_phandle vlist[32], *nvl;
      for (i=0; i < n; ++i)
         vlist[i] = plist + vl[i];
      n = r3_clip_polygon(n, vlist, &nvl);
      for (i=0; i < n; ++i) {
         r3s_point *pt = nvl[i];
         fix xy,yx;
         xy = pt->grp.sx + pt->grp.sy;
         yx = pt->grp.sx - pt->grp.sy;
         if (pt->grp.sx > min2d[0]) code_2d &= ~1;
         if (pt->grp.sx < max2d[0]) code_2d &= ~2;
         if (pt->grp.sy > min2d[1]) code_2d &= ~4;
         if (pt->grp.sy < max2d[1]) code_2d &= ~8;
         if (xy > min2d[2]) code_2d &= ~16;
         if (xy < max2d[2]) code_2d &= ~32;
         if (yx > min2d[3]) code_2d &= ~64;
         if (yx < max2d[3]) code_2d &= ~128;
      }
   }
   if (code_2d != 0)
      return FALSE;  // we trivially do not overlap the bounding octagon

   // we don't trivially miss the rectangle
   // in this case, we should probably do a non-trivial test (clip
   // the polygon by the bounding octagon, see if result is non-empty)
   // better yet, we should use the convex bounding volume of
   // the object, by computing the silhouette edges of the
   // bounding box... oh well

   // we could at least do a more thorough test of whether
   // the polygon is in front/behind the object, say by using
   // the object's bounding planes

   return TRUE; 
}

extern BOOL bbox_intersects_plane(mxs_vector *bbox_min, mxs_vector *bbox_max,
          PortalPlane *p);

// return TRUE if the object can be drawn in front of this cell
BOOL object_in_front(PortalCell *r, mxs_vector *mn, mxs_vector *mx,
             fix *mn2d, fix *mx2d)
{
   int i,n,v=0;
   ulong plane_check;     // have we tested this plane
   ulong plane_result;    // the result for this plane
   PortalPolygonCore *p = r->poly_list;

   // any rendered portal polygon will force an explicit split, so ignore those
   n = r->num_polys - r->num_portal_polys;

   plane_check = 0;
   plane_result = 0;

   r3_start_block();  // for clipping I guess

   for (i=0; i < n; ++i) {
      int plane = p->planeid;
      BOOL result;
      if (plane_check & (1 << plane))
         result = plane_result & (1 << plane);
      else {
         PortalPlane *p = &r->plane_list[plane];
         float dist = mx_dot_vec(&portal_camera_loc, &p->norm->raw)
                      + p->plane_constant;
         if (dist >= 0 && bbox_intersects_plane(mn, mx, p)) {
            result = TRUE;
            plane_result |= 1 << plane;
         } else {
            result = FALSE;
         }

         plane_check |= 1 << plane;
      }

      if (result) {
         // check for an axially aligned split plane which reveals
         // that the polygon is on the opposite side of the object
         // from the viewer
         // maybe we should explicitly build the bbox of the poly?
         // this way we get to early out...

#define MIN(a,b)  ((a) < (b) ? (a) : (b))
#define MAX(a,b)  ((a) > (b) ? (a) : (b))
#define pcl       portal_camera_loc
         if (poly_less(r, p, v, 0, MIN(pcl.x,mn->x))) goto behind;
         if (poly_greater(r, p, v, 0, MAX(pcl.x,mx->x))) goto behind;
         if (poly_less(r, p, v, 1, MIN(pcl.y,mn->y))) goto behind;
         if (poly_greater(r, p, v, 1, MAX(pcl.y,mx->y))) goto behind;
         if (poly_less(r, p, v, 2, MIN(pcl.z,mn->z))) goto behind;
         if (poly_greater(r, p, v, 2, MAX(pcl.z,mx->z))) goto behind;
#undef pcl
#undef MAX
#undef MIN

         if (poly_overlap_2d(r, p, v, mn2d, mx2d))
         {
            r3_end_block();
            return FALSE;
         }
      }
     behind:
      v += p->num_vertices;
      ++p;
   }

   r3_end_block();
   return TRUE;
}

extern void portal_obj_bounds(ObjID obj, mxs_vector **mn, mxs_vector **mx,
           mxs_vector **center, float *radius, fix *mn2d, fix *mx2d);

bool visible_in_cell(PortalCell *r, fix *mn2d, fix *mx2d)
{
   return PortalClipOverlap(CLIP_DATA(r), mn2d, mx2d);
}

// the list of cells to draw
int active_regions[MAX_ACTIVE_REGIONS];

bool portal_object_complete_test=TRUE;

void split_object(ObjID o)
{
   int v,next;
   v = obj_fragment_list[o];

   // for all fragments except the last in the chain, move them
   // to the appropriate cell and set their fragment # to the split
   // value.  The last item in the chain (the nearest to the viewer)
   // is already in the right cell, and just needs fragment #

   next = vis_objs[v].next_visobj;
   do {
CELL(v);
      queue_object_fragment_in_cell(vis_objs[v].cell, v);
hack_fragment[v] = TRUE;
      vis_objs[v].fragment = OBJ_SPLIT_OTHER;

      v = next;
      next = vis_objs[v].next_visobj;
   }
   while (next != VISOBJ_NULL && vis_objs[next].obj == o);

hack_fragment[v] = TRUE;
   vis_objs[v].fragment = OBJ_SPLIT_FIRST;
   obj_fragment_list[o] = VISOBJ_NULL;
}

void check_object_split(ObjID o)
{
   int v,next;
   mxs_vector *mn,*mx,*center;
   fix mn2d[4], mx2d[4];
   float radius;

   if (!portal_object_visible(o)) {
      obj_hide[o] = TRUE;
#ifdef STATS_ON
         ++stat_num_hidden_objects;
#endif
      return;
   }

   portal_obj_bounds(o, &mn, &mx, &center, &radius, mn2d, mx2d);

   v = obj_fragment_list[o];
   next = vis_objs[v].next_visobj;

   if (next == VISOBJ_NULL || vis_objs[next].obj != o) {
      if (!visible_in_cell(obj_first_cell[o], mn2d, mx2d)) {
         obj_hide[o] = TRUE;
#ifdef STATS_ON
         ++stat_num_hidden_objects;
#endif
      }
      return;    // only in one cell
   }

   // iterate over all the cells the object is in,
   // and see if it's visible in any
   if (!visible_in_cell(obj_first_cell[o], mn2d, mx2d)) {

      obj_hide[o] = TRUE;

      // check all but the last entry
      while (next != VISOBJ_NULL && vis_objs[next].obj == o) {
CELL(v);
         if (visible_in_cell(vis_objs[v].cell, mn2d, mx2d)) {
            obj_hide[o] = FALSE;
            break;
         }
         v = next;
         next = vis_objs[v].next_visobj;
      }

      if (obj_hide[o] == TRUE) {
#ifdef STATS_ON
         ++stat_num_hidden_objects;
#endif
         return;
      }

      // reset current search location to how it was before we looped

      v = obj_fragment_list[o];
      next = vis_objs[v].next_visobj;
   }

   {
      int far_index, near_index, i;

CELL(v);
      far_index = ((PortalCell *) vis_objs[v].cell)->render_data->sorted_index;
      near_index = obj_first_cell[o]->render_data->sorted_index;

      // now test all cells except furthest
      for (i=near_index; i < far_index; ++i) {
#ifdef STATS_ON
         ++stat_num_cell_tests;
#endif
         if (!object_in_front(wr_cell[active_regions[i]], mn, mx, mn2d, mx2d))
            break;
      }

      if (i == far_index)
      {
         return;
      }
   }

   // we get here if the object needs splitting

   STAT(++stat_num_terrsplit_objects);
   split_object(o);
}

// check if objects need to be split due to terrain
void check_for_object_splitting(void)
{
   int n=need_testing_count,i;

   for (i=0; i < n; ++i)
      check_object_split(obj_need_testing[i]);
}

// check all objects (even non-visible refs) in this cell;
// if they are visible and not already split, and their
// frontmost cell is > front, check if object obj blocks view
// of them; if it does, force it to split

// cell is a cell#; front is a sorted index

BOOL force_object_splits(int cell, int front, int obj)
{
   PortalCell *r = wr_cell[cell];
   ObjRefID id = * (int *) (&r->refs);

   while (id) {
      ObjRef *p = OBJREFID_TO_PTR(id);
      ObjID o = p->obj;

      // is object o being drawn?
      if (obj_dealt[o] && !obj_hide[o]) {
         // does it matter if o blocked obj?
         // if o blocks obj, but obj gets drawn "in front", then bad;
         // that happens if FRONT(o) is < front,
         // or if it's split

         if (obj_fragment_list[o] == VISOBJ_NULL ||
              front < obj_first_cell[o]->render_data->sorted_index) {
            if (portal_object_blocks(o, obj)) {
               STAT(++stat_num_resplit_objects);
               split_object(obj);
               return TRUE;
            }
         }
      }
      id = p->next_in_bin;
   }
   return FALSE;
}

int r_sorted_count, r_total_count;

// check if objects need to be split due to other objects
void check_for_extra_object_splitting(void)
{
   int i;

   for(i=0; i < r_sorted_count; ++i) {
      // consider all objects rendered in this cell
      
      int cell = active_regions[i];
      PortalCell *r = wr_cell[cell];
      int v,next,o;

      v = OBJECTS(r);
      while (v != VISOBJ_NULL) {
        // grab the next field before we mess with it
        // this actually SHOULD be totally safe without
        // it, because I'm so zany with this goofy data
        // structure, but better safe than sorry
        next = vis_objs[v].next_visobj;

        o = vis_objs[v].obj;
        if (!obj_hide[o] && vis_objs[v].fragment == OBJ_NO_SPLIT) {
           // determine frontmost & backmost cells
           int far_index, near_index, j;

           // if more than one cell
           if (vis_objs[obj_fragment_list[o]].next_visobj != VISOBJ_NULL &&
               vis_objs[vis_objs[obj_fragment_list[o]].next_visobj].obj == o) {
CELL(obj_fragment_list[o]);
              far_index = ((PortalCell *) vis_objs[obj_fragment_list[o]].cell)->
                                                   render_data->sorted_index;
              near_index = i;
              for (j=near_index; j <= far_index; ++j)
                 if (force_object_splits(active_regions[j], near_index, o))
                    break;
           }
        }
        v = next;
      }
   }
}

void process_objects(PortalCell *r)
{
   ObjRefID id = * (int *) (&r->refs);

   while (id) {
      ObjRef *p = OBJREFID_TO_PTR(id);
      ObjID o = p->obj;

      if (!obj_dealt[o]) {
         if (portal_allow_object_splitting && obj_split[o])
            deal_with_splitting_object(r, o);
         else
            deal_with_unsplit_object(r,o);

         obj_dealt[o] = TRUE;
         obj_hide[o] = FALSE;
#ifdef STATS_ON
         ++stat_num_objects;
#endif
      } else
         if (portal_allow_object_splitting && obj_split[o])
            deal_with_split_object(r, o);
         else
            deal_with_object_leftover(r, o);

      id = p->next_in_bin;
   }
}

void init_process_objects(void)
{
   num_visible_objects=0;
   need_testing_count = 0;
}

///////////////////// the rendering pipeline ///////////////////////

#define IS_VISITED(n)     (VISIT(WR_CELL(n)))
#define CLEAR_VISITED(n)  (VISIT(WR_CELL(n))=FALSE)
#define SET_VISITED(n)    (VISIT(WR_CELL(n))=TRUE)

#define AVERAGE_PORTALS_PER_CELL    8

PortalCellRenderData rdata[MAX_ACTIVE_REGIONS];

// new sorting algorithm is in sort.txt in the renderer docs

static ushort outgoing_portals[MAX_ACTIVE_REGIONS * AVERAGE_PORTALS_PER_CELL];
static int outgoing;

bool has_portal_to(int from, PortalCell *dest)
{  PROF
   PortalCell *src = WR_CELL(from);
   int i, n, k;
   bool result;

   if (!src->render_data) {
      END_PROF;
      return FALSE;
   }

   result = FALSE;

   n = NUM_OUTGOING(src), 
   k = FIRST_OUTGOING(src);
   for (i=0; i < n; ++i)
      if (WR_CELL(outgoing_portals[k+i]) == dest) {
#ifdef VALIDATE_UNIQUENESS_OF_PORTALS
         for(++i; i < n; ++i)
            if (WR_CELL(outgoing_portals[k+i]) == dest) {
               Warning(("has_portal_to: more than one portal between two cells!\n"));
            }
#endif // ~VALIDATE_UNIQUENESS_OF_PORTALS
         result = TRUE;
         break;
      }

   END_PROF;
   return result;
}

  // add the destinations to the outgoing portal list
void examine_portals(PortalCell *r)
{  PROF
   int n = r->num_portal_polys;
   PortalPolygonCore *p = r->portal_poly_list;
   int v = r->portal_vertex_list;
   r3s_point *old_ph = cur_ph;
   PortalCellRenderData *z = r->render_data;

   // don't bother putting us on the 0count list

   z->num_outgoing_portals = 0;
   z->num_unexplored_entries = 0;
   z->outgoing_portal_offset = outgoing;

   cur_ph = POINTS(r);

   while (n--) {
      // check if the destination already points to us; we have
      // to avoid inconsistency in the backface check

      if (has_portal_to(p->destination, r)) {
         if (!IS_VISITED(p->destination)) {
            ++z->num_unexplored_entries;
         }
      } else if (check_surface_visible(r, p, v)) {
         if (outgoing < MAX_ACTIVE_REGIONS * AVERAGE_PORTALS_PER_CELL) {
            ++z->num_outgoing_portals;
            outgoing_portals[outgoing++] = p->destination;
            CELL_DEBUG(mprintf("  outgoing to %d\n", p->destination));
         } else
            mprintf("examine_portals: Overflowed outgoing_portals table.\n");
      }

      // it might be that the destination didn't point to us, and
      // we didn't point to it, if we're right on the plane.  But
      // that should be ok.  Rounding error could also make that
      // happen, but we've guaranteed that we won't point to each
      // other and cause a little local cycle

      v += p->num_vertices;
      ++p;
   }

   cur_ph = old_ph;
   END_PROF;
}

// now, during the front-to-back pass, the
//   above list is topologically sorted
//   with a selection sort.  This means that
//   at any given point in time, the first
//   n elements are sorted and the rest aren't,
//   and then some more are appended on the
//   end as we go, woo woo

// for every active region, we need
// some clipping data

int r_rdata;

void validate_incoming_count(int n, char *where)
{
   int i,k=0;
   for (i=0; i < r_rdata; ++i) {
      if (has_portal_to(active_regions[i], WR_CELL(n)))
         if (!IS_VISITED(active_regions[i]))
            ++k;
   }
   if (k != NUM_INCOMING(WR_CELL(n)))
      Error(1, "%s: Invalid NUM_INCOMING: is %d, should be %d\n", where,
         NUM_INCOMING(WR_CELL(n)), k);
}

void portal_validate_lists(char *where)
{
   int i;

   for (i=0; i < r_sorted_count; ++i)
      if (!IS_VISITED(active_regions[i]))
         Error(1, "!IS_VISITED entry out of place in %s\n", where);
   for (; i < r_total_count; ++i)
      if (IS_VISITED(active_regions[i]))
         Error(1, "IS_VISITED entry out of place in %s\n", where);

   for (i=0; i < r_total_count; ++i)
      validate_incoming_count(active_regions[i], where);
}

bool skip_clip;

bool setup_cell(PortalCell *r)
{  PROF
   int i;
   float m;
   if (r_rdata == MAX_ACTIVE_REGIONS) {
#ifdef DBG_ON
      mprintf("Scene complexity high, increase MAX_ACTIVE_REGIONS\n");
#endif // ~DBG_ON
      END_PROF;
      return TRUE;
   }

   r3_set_clipmode(R3_CLIP);

   STAT_INC(cell);
   STAT(stat_num_port += r->num_portal_polys);

   r->render_data = &rdata[r_rdata++];

   CLIP_DATA(r) = 0;
   VISIT(r) = 0;
   ZWATER(r) = 0;
   OBJECTS(r) = VISOBJ_NULL;
   CLUT(r).clut_id = 0;
   CLUT(r).clut_id2 = 0;
   CLUT(r).next = 0;

   r3d_ccodes_or = 0;

   POINTS(r) = Malloc(r->num_vertices * sizeof(r3s_point));
   r3_transform_block(r->num_vertices, POINTS(r), r->vpool); 
   NEED_CLIP(r) = skip_clip ? r3d_ccodes_or : 1;

   m = 0;

   for (i=0; i < r->num_vertices; ++i)
      m += POINTS(r)[i].p.z;
   DIST(r) = m / r->num_vertices;

   examine_portals(r);

   END_PROF;
   return FALSE;
}

void free_cell(PortalCell *r)
{
   if (CLIP_DATA(r))
      PortalClipFree(CLIP_DATA(r));
   if (POINTS(r))
      Free(POINTS(r));

   //free_render_data(r->render_data);
   r->render_data = 0;
}

void mark_outgoing_portals(PortalCell *r)
{  PROF
   int i,n = NUM_OUTGOING(r), k = FIRST_OUTGOING(r);
#ifdef DBG_ON
   if (VISIT(r)) Error(1, "Tried to mark outgoing from a sorted cell.\n");
#endif

   for (i=0; i < n; ++i) {
      PortalCell *out = WR_CELL(outgoing_portals[k+i]);
      if (out->render_data)
         ++NUM_INCOMING(out);
   }
   END_PROF;
}

void unmark_outgoing_portals(PortalCell *r)
{  PROF
   int i, n = NUM_OUTGOING(r), k = FIRST_OUTGOING(r);
#ifdef DBG_ON
   if (!VISIT(r)) Error(1, "Tried to unmark outgoing from an unsorted cell.\n");
#endif
   for (i=0; i < n; ++i) {
      PortalCell *out = WR_CELL(outgoing_portals[k+i]);
      if (out->render_data)
         --NUM_INCOMING(out);
   }
   END_PROF;
}

// we've uncovered a region, add it to the list
//   or expand its clipping info if already there
void add_region(int n, ClipData *c, PortalCell *src, PortalPolygonCore *p)
{  PROF
   PortalCell *dest = WR_CELL(n);
   STAT_INC(cell_visit);

   if (!dest->render_data) {
#ifndef SHIP
      if (trav_file)
         fprintf(trav_file, "    First time in %d.\n", n);
#endif
      CELL_DEBUG(mprintf("Expand new cell %d\n", n));
      if (setup_cell(dest)) {
         END_PROF;
         return;
      }
      CLEAR_VISITED(n);
      CLIP_DATA(dest) = c;
      CLUT(dest) = CLUT(src);
      if (p->clut_id) {
         uchar clut, m1, m2, clut2;

         // first determine what medium we're emerging from and going into
         m1 = src->medium;
         m2 = dest->medium;

         // check if the source medium has a clut
         clut = pt_medium_exit_clut[m1];
         if (!clut) {
            clut = pt_medium_haze_clut[m1];
            if (clut)
               clut += compute_water_clut(ZWATER(src), compute_portal_z());
         }

         // check if the dest medium has an entry clut
         clut2 = pt_medium_entry_clut[m2];

         // if only one clut, make clut1 = it
         // if two cluts, swap them, because order is reversed
         if (clut2) {
            if (!clut) {
               clut = clut2;
               clut2 = 0;
            } else {
               uchar temp = clut;
               clut = clut2;
               clut2 = temp;
            }
         }

         if (clut) {
            CLUT(wr_cell[n]).next = CLUT(src).clut_id ? &CLUT(src) : 0;
            CLUT(wr_cell[n]).clut_id = clut;
            CLUT(wr_cell[n]).clut_id2 = clut2;
         }

         // check if the dest medium hazes
         if (pt_medium_haze_clut[m2])
            ZWATER(wr_cell[n]) = compute_portal_z();
      }
      active_regions[r_total_count++] = n;
   } else {
      PortalClipUnion(CLIP_DATA(dest), c);
      PortalClipFree(c);
      CELL_DEBUG(mprintf("Revisit cell %d\n", n));

      // if we portal from air into water, merge z value
      // so we keep nearest, which matches well when we move camera under water
      if (p->clut_id) {
         mxs_real z = compute_portal_z();
         // BUG: if you can get here one way through water and one way not,
         // this comparison is totally bogus
         if (z < ZWATER(dest))
            ZWATER(dest) = z;
      }

      // TODO: we should merge the clut info!

      if (IS_VISITED(n)) {
         // move this place off the visited list back onto the
         // unvisited list, because we might explore it differently,
         // and to force correct sorting.
         int i=0;
         while (active_regions[i] != n)
            ++i;

         memmove(&active_regions[i], &active_regions[i+1],
            sizeof(active_regions[0])*(r_sorted_count-i-1));
         active_regions[--r_sorted_count] = n;
         CLEAR_VISITED(n);
      } else {
         END_PROF;
         return; // don't mark outgoing portals, they already are!
      }
   }

   if (!IS_VISITED(n))
      mark_outgoing_portals(wr_cell[n]);
   END_PROF;
}

/////////////////////// select region /////////////////////////


// Take all the cells on the list, and pick one which
// has nothing unselected in front of it

int select_next_region(void)
{  PROF
   int i, n;
   float mn;

   n = -1;
   mn = 1e20;

   for (i = r_sorted_count; i < r_total_count; ++i) {
      if (NUM_INCOMING(wr_cell[active_regions[i]]) > 0)
         continue;
      if (DIST(wr_cell[active_regions[i]]) < mn) {
         mn = DIST(wr_cell[active_regions[i]]);
         n = i;
      }
   }

   if (n == -1) {
      for (i = r_sorted_count; i < r_total_count; ++i) {
         if (DIST(wr_cell[active_regions[i]]) < mn) {
            mn = DIST(wr_cell[active_regions[i]]);
            n = i;
         }
      }
      if (n == -1) n = r_sorted_count;
      Warning(("Didn't find any 0-incoming cells, used %d!\n", NUM_INCOMING(wr_cell[active_regions[n]])));
   }

   END_PROF;
   return n;
}

///////////////// main pipeline loop //////////////////////////

void initialize_first_region(int n)
{
   // this needs to be the location of the camera

   r3_start_block();
   if (setup_cell(WR_CELL(n)))
      Error(1, "initialize_first_region: no free cells?!!.\n");

   mark_outgoing_portals(wr_cell[n]);

   r3_end_block();

   active_regions[0] = n;
   r_total_count = 1;
   r_sorted_count = 0;

   CLIP_DATA(wr_cell[n]) = PortalClipRectangle(0,0, grd_bm.w, grd_bm.h);
   CLUT(wr_cell[n]).clut_id = 0;
   CLUT(wr_cell[n]).next = 0;
}

bool render_backward;

long pt_get_time(void) { return 0; }
long (*portal_get_time)(void) = pt_get_time;
long (*portal_get_frame_time)(void) = pt_get_time;

void (*portal_anim_light_callback)(long time_change_millisec);
void (*portal_anim_medium_callback)(long time_change_millisec);

void portal_traverse_scene(int cell)
{  PROF
   int off,n, count = 0;

#ifdef STATS_ON
   stat_num_traverse_ms = portal_get_time();
#endif

   r_rdata = 0;  // clear count of used cells
   outgoing = 0; // clear count of used outgoing portal list

     // initialize the list of unexplored regions
     // pass in the region containing the camera
   initialize_first_region(cell);

     // now, as long as there are unexplored regions
   while (r_sorted_count < r_total_count) {

#ifdef VALIDATE
      portal_validate_lists("before select");
#endif

        // find the next region to explore (roughly front-to-back)
      off = select_next_region();
      n = active_regions[off];

#ifndef SHIP
      if (trav_file) 
         fprintf(trav_file, "Expand cell %d:\n", n);
#endif
      STAT_INC(cell_explored);
      CELL_DEBUG(mprintf("%d onto sorted list\n", n));

        // swap it to the end of the sorted list
      active_regions[off] = active_regions[r_sorted_count];
      active_regions[r_sorted_count++] = n;

        // mark it as visited
      SET_VISITED(n);

        // decrement NUM_INCOMING counts of reachable cells
      unmark_outgoing_portals(wr_cell[n]);

        // and now explore the portals out of it, this adds new regions to
        // the active region list (and may even move some back from the sorted
        // area to the unsorted area)
      explore_portals(wr_cell[n]);

        // avoid us getting in an infinite loop;
        // although this may result in major sorting bugs!
      if (++count == 1024) {
#ifndef SHIP
         if (trav_file)
            fprintf(trav_file, "Apparent cell cycle!\n");
#endif
         Warning(("Apparent cell cycle in portal_traverse_scene\n"));
         r_sorted_count = r_total_count;
         break;
      }
   }

#ifdef STATS_ON
   stat_num_traverse_ms = portal_get_time() - stat_num_traverse_ms;
#endif
   
   CELL_DEBUG(debug_cell_traversal = FALSE);
   END_PROF;
}

int last_r_count;
bool project_space=TRUE;

#ifdef STATS_ON
static void zero_stats(void);
#endif

extern void PortalMovePointInsideCell(Location *loc);
static Position adjusted;

static void portal_start_3d(Position *pos)
{
   adjusted = *pos;
   pos = &adjusted;
   if (pos->loc.cell != CELL_INVALID)
       PortalMovePointInsideCell(&pos->loc);

   r3_start_frame();
   r3_set_view_angles(&pos->loc.vec, &pos->fac, R3_DEFANG);
   if (project_space)
      r3_set_space(R3_PROJECT_SPACE);
   else
      r3_set_space(R3_CLIPPING_SPACE);
   portal_camera_loc = pos->loc.vec;
}

static void portal_end_3d(void)
{
   r3_end_frame();
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   It's possible this wants to be in the host program, since all we're
   doing is calling two external functions using the time we get from
   another external function.  On the other hand, if there's every
   any reason to call these before or after anything else in Portal,
   we'll be able to.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static void update_terrain_animation(void)
{
   long delta_time_ms = portal_get_frame_time();

   if (portal_anim_medium_callback)
      (*portal_anim_medium_callback)(delta_time_ms);

   if (portal_anim_light_callback)
      (*portal_anim_light_callback)(delta_time_ms);
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   The visualization tools need to render back-to-front.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
bool portal_render_back_to_front()
{
   bool front_to_back;

#ifndef SHIP
   if (g2pt_span_clip && !(draw_solid_by_MIP_level
                    | draw_solid_by_cell
                    | draw_solid_wireframe))
      front_to_back = TRUE;
   else
      front_to_back = FALSE;
#else  // ~SHIP
   front_to_back = g2pt_span_clip;
#endif // ~SHIP
   return !(front_to_back ^ render_backward);
}


void draw_lightmap_points(int);
bool show_lightmap;
extern int cache_mem_this_frame;


// When a leaf is going to be rendered, we mark it and its
//  ancestors so that when we traverse the BSP tree, we can
//  cull out any subtrees that have no leaves to be rendered.
void markup_bsp(wrBspNode *n)
{
   n->mark = TRUE;

   while (!wrIS_ROOT(n) && !n->parent->mark) {
      n = n->parent;
      n->mark = TRUE;
   }
}

// Traverse the tree, unmarking all nodes and leaves.
void unmark_bsp(wrBspNode *n)
{
   n->mark = FALSE;

   if (wrIS_NODE(n)) {
     if (n->inside->mark)
         unmark_bsp(n->inside);
     if (n->outside->mark)
         unmark_bsp(n->outside);
   }
}

// First unmark any leaves and nodes that were marked, then
//  mark all the to-be-rendered leaves and nodes.
void setup_bsp(wrBspNode *tree)
{
   int i;

   unmark_bsp(tree);

   for (i=0; i<r_sorted_count; ++i) 
      markup_bsp(WR_CELL(active_regions[i])->leaf);
}

// rebuild active_regions so it's in front-to-back BSP sorted order
int sort_via_bsp(wrBspNode *n, mxs_vector *pos, int index)
{
   // If it's a node, traverse in the proper order and unmark it
   if (wrIS_NODE(n)) {
      if ((mx_dot_vec(&(n->plane.v), pos) + n->plane.d) < 0) {
         if (n->outside->mark) 
            index = sort_via_bsp(n->outside, pos, index);
         if (n->inside->mark) 
            index = sort_via_bsp(n->inside, pos, index);
      } else {
         if (n->inside->mark) 
            index = sort_via_bsp(n->inside, pos, index);
         if (n->outside->mark)
            index = sort_via_bsp(n->outside, pos, index);
      }
   // Otherwise it's a leaf, so make it has been marked
   } else if (n->mark) {
      active_regions[index] = n->cell_id;
      return index+1;
   }
   return index;
}

bool portal_write_traversal;
bool portal_obj_fixup = TRUE;

void (*portal_post_render_cback)(void);
void (*portal_render_overlays_cback)(void);
extern int num_water_polys;
extern void portal_render_water_polys(void);
extern bool background_setup;

void portal_render_scene(Position *pos, float zoom)
{  PROF
   Location *loc = &pos->loc;
   int cell = CellFromLoc(loc), i;
#ifdef STATS_ON
   zero_stats();
#endif

   cache_mem_this_frame = 0;
   background_setup = FALSE;

#ifndef SHIP
   if (portal_write_traversal) {
      trav_file = fopen("traverse.log", "a");
      portal_write_traversal = FALSE;
   }
#endif
      
   portal_start_3d(pos);
   r3_set_zoom (zoom);

     // tell the scene clipper to clear its clipinfo

   g2pt_reset_clip(0, grd_bm.w, grd_bm.h);

   g2pt_duv_set_size(grd_bm.w, grd_bm.h);

     // if we have the special "just draw it all flag",
     // just go through it all and draw it in any order
     // but we don't draw any objects!

   if (cell == CELL_INVALID) {
      Warning(("Viewer not in any cell.\n"));
      for (i=0; i < wr_num_cells; ++i) {
         r_rdata=0; // clear the active cell after every cell.  duh
         outgoing=0; // similarly
         r3_start_block();
         r3_set_clipmode(R3_CLIP);
         setup_cell(WR_CELL(i));
         r3_end_block();
         CLIP_DATA(wr_cell[i]) = PortalClipRectangle(0,0, grd_bm.w, grd_bm.h);
         draw_region(i);
         free_cell(wr_cell[i]);
      }
      goto cleanup;
   }

     // do the traversal of all visible cells
   portal_traverse_scene(cell);

   update_terrain_animation();

     // now traverse them in front-to-back order so we can determine
     // object rendering information for now we just draw each object
     // in the first cell we encounter

   setup_bsp(wrBspTree);
   sort_via_bsp(wrBspTree, &(loc->vec), 0);

   for (i=0; i < r_sorted_count; ++i)
      wr_cell[active_regions[i]]->render_data->sorted_index = i;

#ifdef STATS_ON
   stat_num_sort_ms = portal_get_time();
#endif

   init_process_objects();

   for(i=0; i < r_sorted_count; ++i)
      process_objects(wr_cell[active_regions[i]]);

   if (portal_allow_object_splitting) {
      check_for_object_splitting();

      if (portal_obj_fixup)
         check_for_extra_object_splitting();
   }

#ifdef STATS_ON
   stat_num_sort_ms = portal_get_time() - stat_num_sort_ms;
#endif

     // Now that we've explored all of the areas, we're going to draw
     // them all.  If the variable "span_clip" is set, then we're
     // using a scene clipper that requires we render front to back.
     // Otherwise, we render back to front.  But to keep things
     // interesting, if the flag "render_backward" is on, we do the
     // opposite--this is a useful debugging tool to show you how much
     // stuff is being processed.

   // We set the threshold distances for each MIP level at the start
   // of each frame in case the window's been resized, or we have more
   // than one camera.
   PortalSetMIPTable();

#ifdef STATS_ON
   stat_num_render_ms = portal_get_time();
#endif
   if (portal_render_back_to_front()) {
      for(i=r_sorted_count-1; i >= 0; --i) {
         draw_region(active_regions[i]);
         free_cell(WR_CELL(active_regions[i]));
         if (show_lightmap)
            draw_lightmap_points(active_regions[i]);
      }
   } else {
      for(i=0; i < r_sorted_count; ++i) {
         draw_region(active_regions[i]);
         free_cell(wr_cell[active_regions[i]]);
         if (show_lightmap)
            draw_lightmap_points(active_regions[i]);
      }
      if (!render_backward) {
#ifdef STATS_ON
         // don't count background hack pixels as clipped away
         int old_drawn = stat_num_drawn_pixels;
         int old_clipped = stat_num_clipped_pixels;
         render_background_hack();
         stat_num_clipped_pixels =
            old_clipped + (stat_num_drawn_pixels - old_drawn);
#else
         render_background_hack();
#endif
      }
   }
#ifdef STATS_ON
   stat_num_render_ms = portal_get_time() - stat_num_render_ms;
#endif

cleanup:
   if (ptsky_type == PTSKY_SPAN)
      ptsky_render_stars();

     // now that we've finished traversing the scene, we tell the
     // renderer--it may have deferred rendering some polygons, e.g.
     // ones with transparency/translucency if we were drawing
     // front-to-back

   background_hack_cleanup();

   g2pt_post_render_polys();

   if (portal_post_render_cback)
      portal_post_render_cback();

   if (num_water_polys > 0)
      portal_render_water_polys();

   if (portal_render_overlays_cback)
      portal_render_overlays_cback();

   portal_end_3d();

#ifndef SHIP
   if (trav_file) {
      fprintf(trav_file, "End of scene.\n");
      fclose(trav_file);
      trav_file = NULL;
   }
#endif

   last_r_count = r_sorted_count;
   END_PROF;
}

int PortalRenderPick(Position *pos, int x, int y, float zoom)
{
   Location *loc = &pos->loc;
   int cell = CellFromLoc(loc), i, val;

   if (cell == CELL_INVALID)
      return -1;

   portal_start_3d(pos);
   r3_set_zoom (zoom);

     // do the traversal of all visible cells
   portal_traverse_scene(cell);

     // pick_region until val != -1, and free them all
   val = -1;
   for(i=0; i < r_sorted_count; ++i) {
      if (val == -1) {
         val = pick_region(wr_cell[active_regions[i]], x, y);
         if (val != -1)
            val = active_regions[i]*256 + val;  // encode cell+polygon
      }
      free_cell(wr_cell[active_regions[i]]);
   }

   portal_end_3d();

   return val;
}

void portal_render_light(Position *pos, float zoom, float br, void (*light_func)(int))
{
   Location *loc = &pos->loc;
   grs_bitmap dum_bm;
   grs_canvas dum_cnv;

   int cell = CellFromLoc(loc), i;

   if (cell == CELL_INVALID) {
      //mprintf("Light not in region\n");
      return;
   }

   gr_init_bitmap(&dum_bm, 0, BMT_FLAT8, 0, 240, 240);
   gr_make_canvas(&dum_bm, &dum_cnv);
   gr_push_canvas(&dum_cnv);

   portal_start_3d(pos);

   r3_set_zoom(zoom);

   portal_traverse_scene(cell);

     // setup some parameters which are true forever
   PortalSetLightInfo(loc, br, wr_cell[cell]->medium);

   for (i=0; i < r_sorted_count; ++i) {
      light_func(active_regions[i]);
      free_cell(wr_cell[active_regions[i]]);
   }
//mprintf("lit cells: %d  viewed cells %d\n", r_sorted_count, last_r_count);

   portal_end_3d();
   gr_pop_canvas();
}

#define MAX_LIT_CELLS 512
int lit_cell[MAX_LIT_CELLS], num_lit, num_culled;

// store indexes of all cells reached in a given direction in lit_cell[]
bool portal_visit_light(Position *pos, float zoom)
{
   Location *loc = &pos->loc;
   grs_bitmap dum_bm;
   grs_canvas dum_cnv;
   bool enough_active_regions = TRUE;

   int cell = CellFromLoc(loc), i, j;

   if (cell == CELL_INVALID) {
      //mprintf("Light not in region\n");
      return TRUE;
   }

   gr_init_bitmap(&dum_bm, 0, BMT_FLAT8, 0, 240, 240);
   gr_make_canvas(&dum_bm, &dum_cnv);
   gr_push_canvas(&dum_cnv);

   portal_start_3d(pos);

   r3_set_zoom(zoom);

   portal_traverse_scene(cell);
   if (r_sorted_count == MAX_ACTIVE_REGIONS)
      enough_active_regions = FALSE;

   for (i=0; i < r_sorted_count; ++i) {
      int r = active_regions[i];
      for (j=0; j < num_lit; ++j)
        if (r == lit_cell[j])
           break;
      if (j == num_lit && num_lit < MAX_LIT_CELLS)
        lit_cell[num_lit++] = r;
      free_cell(wr_cell[r]);
   }

   portal_end_3d();
   gr_pop_canvas();
   return enough_active_regions;
}

#define BRIGHT_SCALE   4


int portal_add_spotlight(float br, Position *pos, float zoom,
                         uchar dynamic)
{
#if 0
   dynamic_light = dynamic & LIGHT_DYNAMIC;

//   if (dynamic == LIGHT_STATIC)
//      LightDefineStart();

   br *= BRIGHT_SCALE;

   portal_render_light(pos, zoom, br, spotlight_region);

//   if (dynamic == LIGHT_STATIC) {
//      int lt = LightDefineEnd();
//      LightTurnOnSet(lt, 255);
//      return lt;
//   } else
#endif
      return -1;
}

int cur_raycast_cell;

#ifdef LIGHT_MAP

#define MAX_RAYCAST_CELL 256
#define MAX_RAYCAST_PT   256

mxs_vector lt_pt[MAX_RAYCAST_CELL][MAX_RAYCAST_PT];
int lt_count[MAX_RAYCAST_CELL];

void save_lightmap_point(mxs_vector *pt, bool dummy)
{
   if (cur_raycast_cell >= MAX_RAYCAST_CELL) return;
   if (lt_count[cur_raycast_cell] < MAX_RAYCAST_PT)
      lt_pt[cur_raycast_cell][lt_count[cur_raycast_cell]++] = *pt;
}

void draw_lightmap_points(int cell)
{
   int i;
   r3s_point p;
   if (cell >= MAX_RAYCAST_CELL) return;
   for (i=0; i < lt_count[cell]; ++i) {
      r3_transform_point(&p, &lt_pt[cell][i]);
      if (!p.ccodes) {
         gr_set_pixel(1, fix_int(p.grp.sx), fix_int(p.grp.sy));
      }
   }
}

extern void (*lightmap_point_callback)(mxs_vector *loc, bool lit);

Location *light_loc;
void draw_light_region_number(int n)
{
   portal_dynamic_light(wr_cell[n], light_loc);
}


extern float ambient_weight;
extern float max_dist_plain, max_dist_2;

int portal_add_omni_light(float br, float ambient, Location *loc,
                          uchar style, float radius)
{
   Position pos;
   int cell, i, t;
   bool region_check = TRUE;

   num_lit = 0;
   num_culled = 0;

   cell = CellFromLoc(loc);
   if (cell == CELL_INVALID)
      return -1;

   br *= BRIGHT_SCALE / 2;
   max_dist_plain = radius;
   max_dist_2 = radius * radius;

   lightmap_point_callback = save_lightmap_point;

   ambient_weight = br;

   PortalSetLightInfo(loc, br * BRIGHT_SCALE, (WR_CELL(cell))->medium);
   light_loc = loc;

   // record all the cells reached by our light in lit_cell[] and num_lit
   pos.loc = *loc;
   pos.fac.tx = 0;
   pos.fac.ty = 0;
   pos.fac.tz = 0x4000; region_check &= portal_visit_light(&pos, 1.0);
   pos.fac.tz = 0x8000; region_check &= portal_visit_light(&pos, 1.0);
   pos.fac.tz = 0xc000; region_check &= portal_visit_light(&pos, 1.0);
   pos.fac.tz = 0;      region_check &= portal_visit_light(&pos, 1.0);
   pos.fac.ty = 0x4000; region_check &= portal_visit_light(&pos, 1.0);
   pos.fac.ty = 0xc000; region_check &= portal_visit_light(&pos, 1.0);

   if (!region_check)
      mprintf("\nToo many cells to light at (%g, %g, %g)",
              loc->vec.x, loc->vec.y, loc->vec.z);

   for (i = 0; i < (num_lit - num_culled); ++i) {
      int n = lit_cell[i];
      bool cell_lit = TRUE;

      cur_raycast_cell = n;
      if (n < MAX_RAYCAST_CELL)
         lt_count[n] = 0;

      // Dynamic lights are never animated, since they change every
      // frame anyhow.  And quickness overrides raycast lighting.
      // (Currently, they're actually opposites.)
      if (style & LIGHT_DYNAMIC)
         portal_dynamic_light(WR_CELL(n), loc);
      else if (style & LIGHT_QUICK)
         cell_lit = portal_nonraycast_light(WR_CELL(n), loc, style);
      else
         cell_lit = portal_raycast_light(WR_CELL(n), loc, style);

      // If we didn't really reach a cell, we move it to a group of them
      // at the end of the list.  This lets us discard those cells if we
      // like (this is used to cull the cell lists of animated lights).
      if (!cell_lit) {
         ++num_culled;
         t = lit_cell[i];
         lit_cell[i] = lit_cell[num_lit - num_culled];
         lit_cell[num_lit - num_culled] = t;
         --i;
      }
   }

   return num_lit;
}
#endif // LIGHT_MAP


// This is meant to be called right after portal_add_omni_light.
// It sets up the arrays which tell each cell which lights affect it.
void portal_shine_omni_light(int light_index, Location *loc,
                             uchar dynamic)
{
   int i;

   for (i=0; i < num_lit; ++i) {
      int n = lit_cell[i];
      PortalCell *r = WR_CELL(n);
      ++r->light_indices[0];  // count of lights
      r->light_indices = Realloc(r->light_indices,
           (r->light_indices[0]+1) * sizeof(r->light_indices[0]));
      r->light_indices[r->light_indices[0]] = light_index;
   }
   return;
}


// helper for portal_add_simple_dynamic_light
static void limit_to_contiguous_cells(int root_cell_index)
{
   int i, j, t, portal_index;
   PortalPolygonCore *portal;
   PortalCell *cell, *adjacent_cell;
   int adjacent_cell_index;
   int num_contiguous;

   if (num_lit == 1)
      return;

   // find the seed cell in the list
   i = 0;
   while ((i < num_lit) && (lit_cell[i] != root_cell_index))
      ++i;

#ifdef DBG_ON
   // Is the seed cell in the list?
   AssertMsg1(i != num_lit, 
              "Dynamic light cell list does not contain root cell %d\n",
              root_cell_index);
#endif // ~DBG_ON

   // force the seed to element 0
   lit_cell[i] = lit_cell[0];
   lit_cell[0] = root_cell_index;

   // We'll mark all cells but the first, then clear each we visit as
   // we recurse through the database from the seed cell.
   for (i = 1; i < num_lit; ++i)
      WR_CELL(lit_cell[i])->flags |= CELL_TRAVERSED;

   num_contiguous = 1;

   // Now we iterate in the brute_force, ugly way, collecting all the
   // cells we can reach in the beginning of the array.  Notice that
   // num_contiguous may rise.
   for (i = 0; i < num_contiguous; ++i) {
      cell = WR_CELL(lit_cell[i]);

      portal_index = cell->num_portal_polys;
      portal = cell->portal_poly_list;

      while (portal_index--) {
         adjacent_cell_index = portal->destination;
         adjacent_cell = WR_CELL(adjacent_cell_index);

         // If a cell is flagged, we know that it's within the sphere
         // and has not yet been visited.  We swap it into the lower
         // part of lit_cell, where we're collecting the contiguous
         // cells, and clear its flag.
         if (adjacent_cell->flags & CELL_TRAVERSED) {
            adjacent_cell->flags &= ~CELL_TRAVERSED;

            // find our cell in the array
            for (j = num_contiguous; j < num_lit; ++j)
               if (lit_cell[j] == adjacent_cell_index)
                  break;

#ifdef DBG_ON
            AssertMsg1(j != num_lit, 
                       "light cell list does not contain marked cell %d\n",
                       adjacent_cell_index);
#endif // ~DBG_ON

            t = lit_cell[num_contiguous];
            lit_cell[num_contiguous] = lit_cell[j];
            lit_cell[j] = t;
            ++num_contiguous;
         }

         portal++;
      }
   }

   // Now in theory, we have num_contiguous cells for which we've
   // cleared the CELL_TRAVERSED flags, and some others at the end of
   // the array which are still marked.
#ifdef DBG_ON
   for (i = num_contiguous; i < num_lit; ++i)
      AssertMsg1((WR_CELL(lit_cell[i])->flags & CELL_TRAVERSED) != 0,
                 "cell we did not visit is not marked %d\n",
                 adjacent_cell_index);
#endif // ~DBG_ON

   for (i = num_contiguous; i < num_lit; ++i)
      WR_CELL(lit_cell[i])->flags &= ~CELL_TRAVERSED;

   num_lit = num_contiguous;
}


// This is for the following function.  It's on a scale of 0 to 255.
#define SIMPLE_LIGHT_MIN_BRIGHTNESS 16.0

// This is a sloppier way to generate a dynamic light.  Rather than
// using the rendering system's visibility code, it just finds all the
// cells which 1) are within the radius affected by the light and 2)
// can be reached in the cell database from the cell containing the
// light.
void portal_add_simple_dynamic_light(float br, float ambient, Location *loc,
                                     float radius)
{
   int i;
   int cell;

   br *= BRIGHT_SCALE / 2;

   ambient_weight = br;

   cell = CellFromLoc(loc);
   if (cell == CELL_INVALID)
      return;

   PortalSetLightInfo(loc, br * BRIGHT_SCALE, (WR_CELL(cell))->medium);
   light_loc = loc;

   max_dist_plain = radius;
   max_dist_2 = radius * radius;

   num_lit = portal_cells_intersecting_sphere(loc, radius, &lit_cell[0]);
   limit_to_contiguous_cells(loc->cell);

   for (i = 0; i < num_lit; ++i) {
      int n = lit_cell[i];
      cur_raycast_cell = n;

      // Does this do anything?
      if (n < MAX_RAYCAST_CELL)
         lt_count[n] = 0;

      portal_dynamic_light(WR_CELL(n), loc);
   }
}


void portal_add_simple_dynamic_dark(float br, float ambient, Location *loc,
                                    float radius)
{
   int i;
   int cell;

   br *= BRIGHT_SCALE / 2;

   ambient_weight = br;

   cell = CellFromLoc(loc);
   if (cell == CELL_INVALID)
      return;

   PortalSetLightInfo(loc, br * BRIGHT_SCALE, (WR_CELL(cell))->medium);
   light_loc = loc;

   max_dist_plain = radius;
   max_dist_2 = radius * radius;

   num_lit = portal_cells_intersecting_sphere(loc, radius, &lit_cell[0]);
   limit_to_contiguous_cells(loc->cell);

   for (i = 0; i < num_lit; ++i) {
      int n = lit_cell[i];
      cur_raycast_cell = n;

      // Does this do anything?
      if (n < MAX_RAYCAST_CELL)
         lt_count[n] = 0;

      portal_dynamic_dark(WR_CELL(n), loc);
   }
}


#define RECIP_TABLE_SIZE 2048
fix reciprocal_table_24[RECIP_TABLE_SIZE+1];
float int_table[32];

void init_portal_renderer(int dark, int light)
{
   int i;

   init_background_hack();

   reciprocal_table_24[0] = 0x7fffffff;

   for (i=1; i <= RECIP_TABLE_SIZE; ++i)
      reciprocal_table_24[i] = fix_make(256,0) / i;

   for (i=0; i < 32; ++i)
      int_table[i] = i;

   init_portal_shading(dark, light);
   init_portal_light();
}


// callback needed by lighting system (once upon a time)
#if 0
unsigned char *portal_get_lighting_data(int cell_id)
{
   return WR_CELL(cell_id)->vertex_list_lighting;
}
#endif

bool show_span_lengths, show_render_times;

 int pixel_count[32];
#ifdef STATS_ON

 static void zero_stats(void)
 {
   int undrawn = stat_num_clipped_pixels - stat_num_drawn_pixels;

   if (undrawn > stat_max_undrawn_pixels)
      stat_max_undrawn_pixels = undrawn;

   if (stat_num_polys_clipped_away > stat_max_undrawn_polys) {
      stat_max_undrawn_polys = stat_num_polys_clipped_away;
      stat_max_total_polys = stat_num_poly_drawn;
   }

   stat_num_port = 0;
   stat_num_port_visit = 0;
   stat_num_port_traverse = 0;
   stat_num_cell = 0;
   stat_num_cell_visit = 0;
   stat_num_cell_explored = 0;
   stat_num_facing = 0;
   stat_num_poly_raw = 0;
   stat_num_poly_considered = 0;
   stat_num_poly_drawn = 0;
   stat_num_polys_clipped_away = 0;
   stat_num_lit_pixels = 0;
   stat_num_source_pixels = 0;
   stat_num_dest_pixels = 0;
   stat_num_spans_drawn = 0;
   stat_num_spans_clamped = 0;
   stat_num_backface_tests = 0;
   stat_num_render_ms = 0;
   stat_num_traverse_ms = 0;
   stat_num_object_ms = 0;
   stat_num_sort_ms = 0;
   stat_num_drawn_pixels = 0;
   stat_num_clipped_pixels = 0;
   stat_num_transp_pixels = 0;
   stat_num_objects = 0;
   stat_num_visible_objects = 0;
   stat_num_terrsplit_objects = 0;
   stat_num_resplit_objects = 0;
   stat_num_hidden_objects = 0;
   stat_num_cell_tests = 0;
   memset(pixel_count, 0, sizeof(pixel_count));
 }

   // The following macro is used internally in the output function.
   // "index" is the variable # of the last message we printed; the
   // current one is found in 'i', so if i <= index, we've already
   // been printed.  Ideally we would just switch() on the value of index,
   // but then we'd have to explicitly assign numbers to each case,
   // instead of making them implicit using i as seen here.

 #define INFO(var, str, my_vol)              \
    if (++i > index && vol >= my_vol) {      \
       index = i;                            \
       sprintf(buffer, str, stat_num_##var); \
       return buffer;                        \
    }

 #define MAXINFO(var, str, my_vol)           \
    if (++i > index && vol >= my_vol) {      \
       index = i;                            \
       sprintf(buffer, str, stat_max_##var); \
       return buffer;                        \
    }

 #define BLANK(my_vol)  if (++i > index && vol >= my_vol) { index = i; return "----"; }

 static char buffer[256];
 char *portal_scene_info(int vol)
 {
    static int index=0;
    int i=0;

    if (++i > index && show_render_times) {
       index = i;
       // note that stat_num_object_ms is sampling based!
       sprintf(buffer,
              "traverse %d ms; objsort %d ms; render %d ms (%d ms on objects)",
                stat_num_traverse_ms, stat_num_sort_ms,
                stat_num_render_ms, stat_num_object_ms);
       return buffer;
    }

    if (g2pt_span_clip) {
       int stat_num_poly_notclipped =
             stat_num_poly_drawn - stat_num_polys_clipped_away;
       INFO(poly_notclipped, "Polygons past span-clip: %d", 5);
    }

    INFO(poly_drawn, "Polygons rendered: %d", 1);
    INFO(poly_considered, "Polygons before portal clip: %d", 20);
    INFO(poly_raw, "Polygons before backfacing: %d", 20);

    BLANK(25);

    if (g2pt_span_clip) {
       int stat_num_undrawn_pixels =
              stat_num_clipped_pixels - stat_num_drawn_pixels;
       INFO(drawn_pixels, "Opaque pixels output: %d", 10);
       INFO(undrawn_pixels, "Opaque Pixels not drawn: %d", 10);
       INFO(transp_pixels, "Transparent pixels output: %d", 10);
    } else {
       INFO(dest_pixels, "Pixels output: %d", 10);
    }
    INFO(lit_pixels, "Surface pixels computed: %d", 3);
    INFO(source_pixels, "Surface pixels used: %d", 5);

    BLANK(40);

    INFO(spans_drawn, "Spans: %d", 15);
    INFO(spans_clamped, "Spans clamped: %d", 30);

    BLANK(20);

    INFO(cell, "Unique cells: %d", 10);
    INFO(cell_explored, "Cells explored: %d", 20);
    INFO(cell_visit, "Cells visited: %d", 30);
    
    BLANK(30);

    INFO(port, "Portals: %d", 5);
    INFO(port_visit, "Portals visited: %d", 30);
    INFO(port_traverse, "Portals traversed: %d", 30);
    INFO(backface_tests, "Backface checks: %d", 10);

    BLANK(30);

    if (g2pt_span_clip) {
       MAXINFO(undrawn_pixels, "Max span-clipped-away pixels: %d", 10);
       if (++i > index && vol >= 10) {
          index = i;
          sprintf(buffer, "Max span-clipped-away polys: %d of %d (%d percent)",
             stat_max_undrawn_polys, stat_max_total_polys,
             (int)(((float)stat_max_undrawn_polys / (float)stat_max_total_polys) * 100));
          return buffer;
       }
    }

    INFO(objects, "Visible objects: %d", 8);
    INFO(hidden_objects, "Re-hidden objects: %d", 10);
    INFO(terrsplit_objects, "Terrain-split objects: %d", 10);
    INFO(resplit_objects, "Object-split objects: %d", 10);
    INFO(visible_objects, "Visible object fragments: %d", 10);
    INFO(cell_tests, "object cell tests: %d", 15);
    
    if (++i > index && show_span_lengths) {
       index = i;
       sprintf(buffer, "%d %d %d %d %d %d %d %d %d %d %d %d",
          pixel_count[0], pixel_count[1], pixel_count[2], pixel_count[3],
          pixel_count[4], pixel_count[5], pixel_count[6], pixel_count[7],
          pixel_count[8], pixel_count[9], pixel_count[10], pixel_count[11]);
       return buffer;
    }

    if (++i > index && show_span_lengths) {
       index = i;
       sprintf(buffer, "%d %d %d %d %d %d %d %d %d %d %d %d",
          pixel_count[12], pixel_count[13], pixel_count[14], pixel_count[15],
          pixel_count[16], pixel_count[17], pixel_count[18], pixel_count[19],
          pixel_count[20], pixel_count[21], pixel_count[22], pixel_count[23]);
       return buffer;
    }

    if (++i > index && show_span_lengths) {
       index = i;
       sprintf(buffer, "%d %d %d %d %d %d %d %d", 
          pixel_count[24], pixel_count[25], pixel_count[26], pixel_count[27],
          pixel_count[28], pixel_count[29], pixel_count[30], pixel_count[31]);
       return buffer;
    }

    index=0; // we got to end without printing, so reset to beginning of list
    return 0;
 }

 #undef INFO
 #undef BLANK

#else

 #pragma off(unreferenced)
 char *portal_scene_info(int vol)
 {
    return 0;
 }
 #pragma on(unreferenced)

#endif


/*
Local Variables:
typedefs:("ClipData" "LightPermanence" "Location" "ObjID" "ObjRef" "ObjRefID" "ObjVisibleID" "PortalCell" "PortalPolygonCore" "Position" "grs_bitmap" "grs_canvas" "mxs_real" "uchar")
End:
*/
