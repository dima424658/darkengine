/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/csg/csg.c,v 1.95 2000/02/19 12:26:35 toml Exp $
//
// csg.c
//
/////

#include <stdlib.h>        // rand
#include <string.h>        // memcpy
#include <stdio.h>         // tree dumping
#include <math.h>          // fabs, fmod

#include <allocapi.h>

#include <mprintf.h>

#include <lg.h>

#include <csg.h>
#include <csgbrush.h>
#include <r3d.h>
#include <wr.h>
#include <wrbsp.h>
#include <wrdbrend.h>
#include <portal.h>
#include <media.h>
#include <mediaop.h>
#include <wrbsp.h>
#include <wrlimit.h>

#include <hep.h>
#include <bspdata.h>
#include <csgcheck.h>
#include <bsppinfo.h>
#include <csgmerge.h>
#include <csgalloc.h>
#include <csgutil.h>
#include <csgbbox.h>

#include <prof.h>
#include <texmem.h>
#include <status.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


float REAL_EPSILON = 0.0001;

// storage for the world rep?  What's this doing here?
PortalCell *wr_cell[MAX_REGIONS];
int wr_num_cells;

// for each cell, we have a list of which
// csg brush&face it came from.  The length
// of array #x is wr_cell[x]->num_render_polys
// this is edit-time only
int *wr_brfaces[MAX_REGIONS];

bool merge_polys, debug_merge;

extern PortalPolyhedron *SplitPortalPolyhedronByPlane(PortalPolyhedron *s, BspPlane *clip, bool merge, bool set_plane);
extern void BspFreeTree(void *tree);
extern void FreeWR(void);

extern void *active; // not really right type, but hey it's modular

////
//
// geometry operations
//

int VertexCode(BspVertex *v, BspPlane *p)
{
   Real d;
   d = v->x*p->a + v->y*p->b + v->z*p->c + p->d;
   if ( d > REAL_EPSILON) return IN_FRONT;
   if (-d > REAL_EPSILON) return BEHIND;
   return COPLANAR;
}

extern void *BspMakeTree(void);
extern void BspCompareNode(BspNode *n, FILE *f);

#define MAX_PLANES   16384
BspPlane all_planes[MAX_PLANES] = { { 1,0,0 }, {0,1,0}, {0,0,1} };   // 96K
int plane_brushid[MAX_PLANES];  // first brush with this id

bool coplanar=TRUE, split_polys;
bool optimize_bsp;

// This function is called with a pointer to a bspnode and
// a polyhedron which represents the space for this node.
// If we're a leaf, we emit it.  If we're an internal node,
// we split the polyhedron and go.  Splitting involves extra
// work due to shared polygons, so we use the weird split
// functions above.

static void PortalizeSubTree(BspNode *b, PortalPolyhedron *s, bool merge)
{
   bool virtual_leaf;

      // if we're a "supposed" leaf (we treat some subtrees as a leaf)   
   if (b->medium != NO_MEDIUM && !(coplanar || split_polys)) {
      // stuff the "inside leaf" pointer with the polyhedron,
      // and when we're done with the tree we'll traverse over
      // all of the leaves
      b->ph = (void *) s;
      s->leaf = b;
   } else {
      PortalPolyhedron *out;

      // if split_polys is true, we're splitting polys excessively,
      // so even if this was a virtual leaf we want to recurse and
      // split...
      if (b->medium != NO_MEDIUM && !merge) {
         merge = TRUE;
         virtual_leaf = TRUE;
      } else
         virtual_leaf = FALSE;

      if (!IS_LEAF(b)) {
         // Split this polyhedron by the clipping plane.  The function
         // leaves the inside polyhedron in s, and returns the outside.

         out = SplitPortalPolyhedronByPlane(s, &b->split_plane, merge, TRUE);

#ifdef DBG_ON
         if (!merge) {
#if 0
            if (s->poly || !optimize_bsp)
               CheckPolyhedron(s, " (inner clipped polyhedron)");
            if (out->poly || !optimize_bsp)
               CheckPolyhedron(out, " (outer clipped polyhedron)");
#else
            CheckPolyhedron(s, " (inner clipped polyhedron)");
            CheckPolyhedron(out, " (outer clipped polyhedron)");
#endif
         } else {
            CheckPolyhedronQuick(s, " (inner clipped polyhedron)");
            CheckPolyhedronQuick(out, " (outer clipped polyhedron)");
         }
#endif

         if (s->poly)
            PortalizeSubTree(b->inside, s, merge);
         if (out->poly)
            PortalizeSubTree(b->outside, out, merge);

         if (merge) {
            PortalMergeCells(s, out);
         }
      }

      if (virtual_leaf) {
#ifdef DBG_ON
         CheckPolyhedron(s, " (remerged polyhedron)");
#endif
         b->ph = (void *) s;
         s->leaf = b;
      } else
         b->ph = 0;
   }
}

// split a polyhedron just to try to simplify the db

#define MAX_DUMMY_NODES    4096
BspNode *dummy_node[MAX_DUMMY_NODES];
int num_dummy_nodes, cur_dummy_nodes;

static void clear_dummy_nodes(void)
{
   while (num_dummy_nodes) {
      --num_dummy_nodes;
      dummy_node[num_dummy_nodes]->user_data = 0;
      BspFreeLeaf(dummy_node[num_dummy_nodes]);
   }
   cur_dummy_nodes = 0;
}

BspNode *BspAllocateDummyNode(void)
{
   return dummy_node[num_dummy_nodes++] = BspAllocateLeaf();
}

//////////////////////////////////////
//
//  Extra data so we can determine which brush face a polygon came from

// the same, but this time we're using it to recompute
// the texture on a brush that's been updated

int csg_num_brushes;

typedef struct {
   int cell;
   uchar surface;
   uchar brush_face;
   short vertex;
} SurfaceRef;

SurfaceRef *ref_locs[MAX_FACE_BRUSHES];
int ref_count[MAX_FACE_BRUSHES];

void ClearRegisteredTextures(void)
{
   int i;
   for (i=0; i < csg_num_brushes; ++i) {
      if (ref_locs[i])
         Free(ref_locs[i]);
      ref_locs[i] = 0;
      ref_count[i] = 0;
   }
}

  // we need to register more info now so we can update
  // the alignment and scale of the textures

//            RegisterFace(poly_brface[i], cell, surface, vc+k);

void RegisterFace(ulong brface, int cell, int surface, int vertex)
{
   int br = brface >> 8;
   int face = brface & 255;
   int c;

   if (br >= MAX_FACE_BRUSHES)
      Error(1, "RegisterFace: MAX_FACE_BRUSHES exceeded.\n");

   if (ref_count[br]) {
      c = ref_count[br]++;
      ref_locs[br] = Realloc(ref_locs[br], sizeof(SurfaceRef) * ref_count[br]);
   } else {
      ref_locs[br] = Malloc(sizeof(SurfaceRef));
      ref_count[br] = 1;
      c = 0;
   }
   ref_locs[br][c].cell = cell;
   ref_locs[br][c].surface = surface;
   ref_locs[br][c].brush_face = face;
   ref_locs[br][c].vertex = vertex;
}

void compute_poly_texture(PortalPolygonRenderInfo *render, Vertex *base,
  PortalPlane *plane, int brush, int face, PortalCell *, int s, int vc);

static BOOL is_unlit_texture_id(int t_id)
{
   return (t_id==BACKHACK_IDX || t_id==WATERIN_IDX || t_id==WATEROUT_IDX);
}

void update_surface(int cell, int surface, int brush, int face, int vertex, BOOL texture_only)
{
   PortalCell *p = WR_CELL(cell);
   
   // WARNING: this is somewhat wrong
   //   in particular, using is_unlit_texture_id to know it is water due to 
   // texture_id is a lie, it is really water for other reasons, but Sean
   // wasnt sure what the right thing to test was
   //   we had to do something since CB_FACE_TEXTURE is constant throughout the
   // call, since it looks at the real brush database, so the turn off/turn on
   // isnt right.  ideally we would figure out it was a media transition with a
   // cooler thing and we would be ok, but im not sure how to do that
   //   so for now, i always clear it, then use sky/water texture id to tell if
   // i should redoesntlight it, which works in all known current cases, but it
   // somewhat dodgy
   p->poly_list[surface].flags &= ~RENDER_DOESNT_LIGHT;   
   p->render_list[surface].texture_id = CB_FACE_TEXTURE(brush, face);
   if (is_unlit_texture_id(p->render_list[surface].texture_id) || CB_FACE_IS_SELF_LUMINOUS(brush, face))
      p->poly_list[surface].flags |= RENDER_DOESNT_LIGHT;

   if (texture_only)
      clear_surface_cache();
   else
      compute_poly_texture(
          &p->render_list[surface],
          &p->vpool[p->vertex_list[vertex]],
          &p->plane_list[p->poly_list[surface].planeid],
          brush, face,
          p, surface, vertex );
}

void ReassignTexture(int br, BOOL texture_only)
{
   int j;
   SurfaceRef *loc = ref_locs[br];
   for (j=0; j < ref_count[br]; ++j, ++loc) 
      update_surface(loc->cell,loc->surface, br, loc->brush_face, loc->vertex, texture_only);
}

extern void emit_portal_polyhedron(PortalPolyhedron *ph);
extern int num_cells;
extern int num_points;
extern int num_poly;
extern int num_portal;
extern int subdivide;

extern int find_brface_from_poly(PortalPolygon *poly);

void compute_brface(PortalPolyhedron *ph)
{
   PortalPolygon *poly, *first;
   first = poly = ph->poly;
   do {
      int n = (poly->ph[0] == ph);
      if (poly->ph[n] && IS_MARKED(poly->ph[n]->leaf)) {
         poly->brface = -1;
      } else
         poly->brface = find_brface_from_poly(poly);

      if (poly->ph[0] == ph)
         poly = poly->ph_next[0];
      else
         poly = poly->ph_next[1];
   } while (poly != first);
}

bool resplit_cell;
void assign_pinfo(PortalPolyhedron *ph)
{
   PortalPolygon *poly, *first;

   // visit all polygons and compute their info

   first = poly = ph->poly;
   do {
      pinfo *x;
      int n = (poly->ph[0] == ph);
      if (!poly->misc) {
         x = Malloc(sizeof(*x));
         poly->misc = x;
         if (poly->ph[n] && poly->ph[n]->leaf->cell_id > 0) {
            x->portal = poly->ph[!n]->leaf->cell_id;
            x->portal_2 = poly->ph[n]->leaf->cell_id;
            x->brface = -1;
         } else {
            x->portal = 0;
            x->brface = poly->brface;
         }
      }

      if (poly->ph[0] == ph)
         poly = poly->ph_next[0];
      else
         poly = poly->ph_next[1];
   } while (poly != first);
}

void free_pinfo(PortalPolyhedron *ph)
{
   PortalPolygon *poly, *first;
   // free the pinfo
   first = poly = ph->poly;
   do {
      if (poly->misc) {
         Free(poly->misc);
         poly->misc = NULL;
      }

      if (poly->ph[0] == ph)
         poly = poly->ph_next[0];
      else
         poly = poly->ph_next[1];
   } while (poly != first);
}

// used by level optimizer
#if 1
// ick, we have to write it out the old way
void write_cell(FILE *f, PortalCell *p)
{
   uchar buffer[256];
   int i, vl;

   vl = 0;
   for (i=0; i < p->num_polys; ++i)
      vl += p->poly_list[i].num_vertices;

   fwrite(&vl, 1, sizeof(vl), f);
   fwrite(p, 1, sizeof(*p), f);   
   fwrite(p->vpool, sizeof(Vertex), p->num_vertices, f);
   fwrite(p->poly_list, sizeof(PortalPolygonCore), p->num_polys, f);
   for (i=0; i < p->num_render_polys; ++i) {
      // write the pointers to the u,v vectors
      fwrite(buffer, 8, 1, f);
      // write out the render list sans u,v vectors
      fwrite(&p->render_list[i].u_base, sizeof(PortalPolygonRenderInfo)-sizeof(Vector)*2, 1, f);
   }

   for (i=0; i < p->num_planes; ++i) {
      fwrite(buffer, 4, 1, f);   // write out faux pointer
      fwrite(&p->plane_list[i].plane_constant, sizeof(PortalPlane)-sizeof(Vector), 1, f);
   }
   fwrite(p->vertex_list, 1, vl, f);

   for (i=0; i < p->num_render_polys; ++i) {
      fwrite(&p->render_list[i].tex_u, sizeof(Vector), 1, f);
      fwrite(&p->render_list[i].tex_v, sizeof(Vector), 1, f);
   }

   for (i=0; i < p->num_planes; ++i) {
      fwrite(&p->plane_list[i].normal, sizeof(Vector), 1, f);
   }
   for (i=0; i < p->num_render_polys; ++i) {
      fwrite(&p->light_list[i], sizeof(PortalLightMap), 1, f);
      fwrite(buffer, 4, 1, f);  // decal is gone
      // actually, this doesn't line up exactly right, but the anim
      // light bitmask isn't used by the optimizer either
   }
   for (i=0; i < p->num_render_polys; ++i) {
      fwrite(p->light_list[i].data, 1, // the data is bogus, but the same size
            p->light_list[i].w*p->light_list[i].h, f);
   }
}
#else
void write_cell(FILE *f, PortalCell *p)
{
   int i, vl;

   vl = 0;
   for (i=0; i < p->num_polys; ++i)
      vl += p->poly_list[i].num_vertices;

   fwrite(&vl, 1, sizeof(vl), f);
   fwrite(p, 1, sizeof(*p), f);   
   fwrite(p->vpool, sizeof(Vertex), p->num_vertices, f);
   fwrite(p->poly_list, sizeof(PortalPolygonCore), p->num_polys, f);
   fwrite(p->render_list, sizeof(PortalPolygonRenderInfo), p->num_render_polys, f);
   fwrite(p->plane_list, sizeof(PortalPlane), p->num_planes, f);
   fwrite(p->vertex_list, 1, vl, f);
}
#endif

#ifdef DBG_ON
static void CheckPolyhedronFinal(PortalPolyhedron *ph)
{
   CheckPolyhedron(ph, " (after splitting)");
}
#endif

//  Mark all the nodes which are open
static void MarkNode(BspNode *b)
{
   if (b->ph) {
      if (b->medium == NO_MEDIUM) Error(1, "Bad medium in MarkNode.\n");
      if (b->medium == MEDIA_SOLID)
         UNMARK(b);
      else {
#if 0
         if (((PortalPolyhedron *) b->ph)->poly)
            MARK(b);
         else
            UNMARK(b);
#else
         MARK(b);
#endif
      }
   } else {
      if (!IS_LEAF(b)) {
         MarkNode(b->inside);
         MarkNode(b->outside);
      }
      UNMARK(b);
   }
}      

static void portal_id_point_list(PortalPolyhedron *ph)
{
   ph->leaf->cell_id = ++num_cells;  // store cell id + 1
}

void (*traverse_func)(PortalPolyhedron *);

static void PortalTraverseVirtualLeavesRaw(BspNode *b)
{
   if (b->ph) {
      if (IS_MARKED(b))
         traverse_func((PortalPolyhedron *) b->ph);
   } else {
      PortalTraverseVirtualLeavesRaw(b->inside);
      PortalTraverseVirtualLeavesRaw(b->outside);
   }
}

static void PortalTraverseVirtualLeaves(BspNode *b, void (*tfunc)(PortalPolyhedron *))
{
   int i;
   traverse_func = tfunc;
   PortalTraverseVirtualLeavesRaw(b);
   for (i=0; i < cur_dummy_nodes; ++i)
      tfunc((PortalPolyhedron *) dummy_node[i]->ph);
}

int stat_edge_merge, stat_poly_merge, stat_max_vertices;
extern PortalPolyhedron *PortalMakeCube(Real size);

static void port_status(char *s, int pass)
{
   char buf[128];
   if (pass == 1)
      sprintf(buf, "Portalize: %s", s);
   else if (pass > 1)
      sprintf(buf, "Port pass %d: %s", pass, s);
   else if (pass == -1)
      sprintf(buf, "Port edge-merge pass: %s", s);
   else if (pass < -1)
      sprintf(buf, "Port edge-merge pass %d: %s", -pass, s);
   Status(buf);
}

int bsp_num_planes=3;   // 3 axis aligned start planes

static void PortalizeTree(void *tree, Real world_size)
{
   int pass;
   PortalPolyhedron *s = PortalMakeCube(world_size);

     // proceed depth-first through the tree, refining
     // the polyhedron for each subtree into the polyhedra
     // for its children.  Then store them on the children
     // and continue the traversal.  Already visited locations
     // continue to be refined via subdivision of existing polygons
     // and edges to remove t-joints and t-faces

   Status("Portalize");

#if 0
{
   int i,j;
   for (i=0; i < bsp_num_planes; ++i) {
      BspPlane x;
      x.a = (float) all_planes[i].a;
      x.b = (float) all_planes[i].b;
      x.c = (float) all_planes[i].c;
      x.d = (float) all_planes[i].d;
      j = find_plane(&x);
      if (i != j) {
         mprintf("Plane %i doesn't map to itself:\n");
         mprintf("double: %lg %lg %lg %lg\n", all_planes[i].a,all_planes[i].b,
                         all_planes[i].c,all_planes[i].c);
         mprintf(" float: %lg %lg %lg %lg\n", x.a,x.b,x.c,x.d);
         if (j >= 0)
            mprintf(" match: %lg %lg %lg %lg\n", all_planes[j].a,
                   all_planes[j].b,all_planes[j].c,all_planes[j].c);
      }
   }
}
#endif

   //clear_dummy_nodes();

   PortalizeSubTree((BspNode *) tree, s, FALSE);

     // Now we've fully traversed the tree and stored the
     // polyhedra in the leaves.  Now we traverse the tree
     // and allocate IDs to the leaves.
     // and emit the final refined polyhedra.  We could have
     // put them in a linked list or something, but there isn't
     // much point

   Status("Port: analysis");

   MarkNode((BspNode *) tree);
#ifdef DBG_ON
   PortalTraverseVirtualLeaves((BspNode *) tree, CheckPolyhedronFinal);
#endif

   PortalTraverseVirtualLeaves((BspNode *) tree, compute_brface);
   stat_edge_merge = stat_poly_merge = 0;

   pass = 1;

   do {
      num_cells = num_points = num_poly = num_portal = 0;
      subdivide = 0;

      cur_dummy_nodes = num_dummy_nodes;

      FreeWR();
      resplit_cell = FALSE;

      port_status("number cells", pass);
      PortalTraverseVirtualLeaves((BspNode *) tree, portal_id_point_list);
      port_status("compute brushfaces", pass);
      PortalTraverseVirtualLeaves((BspNode *) tree, assign_pinfo);
      if (coplanar || merge_polys) {
        port_status("merge polys", pass);
        PortalTraverseVirtualLeaves((BspNode *) tree, emit_merge_cell_polys);
        if (post_edge_merge) {
          port_status("merge edges", pass);
          PortalTraverseVirtualLeaves((BspNode *) tree, emit_merge_cell_edges);
        }
      }
      ClearRegisteredTextures();
      stat_max_vertices = 0;
      port_status("build worldrep", pass);
      PortalTraverseVirtualLeaves((BspNode *) tree, emit_portal_polyhedron);

      // Set global world-rep variable here so deallocation can know how many
      //  cells it needs to deallocate
      wr_num_cells = num_cells;

      if (!resplit_cell) {
         if (post_edge_merge)
            break;
         post_edge_merge = TRUE;
         pass = -1;
      } else
         if (pass > 0) ++pass; else --pass;
   } while (1);

   post_edge_merge = FALSE;

   // Copy CSG BSP tree into WorldRep BSP Tree
   Status("Port: copying to worldrep BSP");
   wrBspTreeDeallocate();
   wrBspTreeCopy((BspNode *)tree, WRBSP_INVALID);
  
   // Set up links between cells and their leaves
   Status("Port: linking cells to leaves");
   wrBspTreeRefCells(0);

   mprintf("%d cells, %d portals, %d polygons; %d splits from large polys\n",
       num_cells, num_portal, num_poly, subdivide);
   mprintf("%d polys merged; %d colinear vertices deleted; %d vertices max\n",
       stat_poly_merge, stat_edge_merge, stat_max_vertices);
   mprintf("%d unique planes\n", bsp_num_planes);

   portalize_mem_reset();
}

extern Hep csg_hep, ab_hep;

extern void InsertPortalPolyhedronInNode(BspNode *b, PortalPolyhedron *z);

bool merge_nodes=TRUE;

//  Attach non-zero user_data values to nodes
//  which shouldn't really be split
static int leaf_count;
static void RecodeNode(BspNode *b)
{
   if (!IS_LEAF(b)) {
      RecodeNode(b->inside);
      RecodeNode(b->outside);
      if (merge_nodes && b->inside->medium == b->outside->medium)
         b->medium = b->inside->medium;
      else {
         b->medium = NO_MEDIUM;
         if (b->inside->medium != NO_MEDIUM)
            ++leaf_count;
         if (b->outside->medium != NO_MEDIUM)
            ++leaf_count;
      }
   }
}

extern void init_csgmedia(void);

void csg_memory_init(void)
{
   MakeHep(&Nodes, sizeof(BspNode));
   init_csgmedia();
   portalize_mem_init();
}

void csg_memory_reset(void)
{
   clear_dummy_nodes();
   ResetHep(&Nodes);
   ResetHep(&ab_hep);
   ResetHep(&csg_hep);
   portalize_mem_reset();
}

extern int num_brush_faces[MAX_FACE_BRUSHES];
extern BspPlane brush_faces[MAX_FACE_BRUSHES][MAX_FACES];

void save_csg_internal_database(CSGReadWriteFunc func)
{
   int i;
   
   (*func)(&wr_num_cells, sizeof(int), 1);
   for (i = 0; i < wr_num_cells; i++)
      (*func)(wr_brfaces[i], sizeof(int), wr_cell[i]->num_render_polys);

#if 0      
   (*func)(num_brush_faces, sizeof(int), MAX_FACE_BRUSHES);
   //for (i = 0; i < MAX_FACE_BRUSHES; i++)
   //   (*func)(brush_faces[i], sizeof(BspPlane), num_brush_faces[i]);
   (*func)(brush_faces, sizeof(BspPlane), MAX_FACE_BRUSHES * MAX_FACES);   
#endif
   
   (*func)(&csg_num_brushes, sizeof(int), 1);
   (*func)(num_brush_faces, sizeof(int), csg_num_brushes);
   // Slower, but saves up to a half-meg on big maps
   for (i=0; i<csg_num_brushes; i++)
      (*func)(brush_faces[i], sizeof(BspPlane), num_brush_faces[i]);

   (*func)(ref_count, sizeof(int), csg_num_brushes);
   for (i = 0; i < csg_num_brushes; i++)
      if (ref_count[i] > 0)
         (*func)(ref_locs[i], sizeof(SurfaceRef), ref_count[i]);
}

void free_csg_internal_database()
{
   ClearRegisteredTextures();
   FreeWR();
   csg_num_brushes = 0;
   csg_memory_reset();
}

void load_csg_internal_database(CSGReadWriteFunc func)
{
   int i;
   
   (*func)(&wr_num_cells, sizeof(int), 1);
   for (i = 0; i < wr_num_cells; i++)
   {
      wr_brfaces[i] = Malloc(sizeof(int) * wr_cell[i]->num_render_polys);
      (*func)(wr_brfaces[i], sizeof(int), wr_cell[i]->num_render_polys);
   }
      
#if 0
   (*func)(num_brush_faces, sizeof(int), MAX_FACE_BRUSHES);
   //for (i = 0; i < MAX_FACE_BRUSHES; i++)
   //   (*func)(brush_faces[i], sizeof(BspPlane), num_brush_faces[i]);
   (*func)(brush_faces, sizeof(BspPlane), MAX_FACE_BRUSHES * MAX_FACES);   
#endif
   
   (*func)(&csg_num_brushes, sizeof(int), 1);
   (*func)(num_brush_faces, sizeof(int), csg_num_brushes);

   for (i=0; i<csg_num_brushes; i++)
      (*func)(brush_faces[i], sizeof(BspPlane), num_brush_faces[i]);

   (*func)(ref_count, sizeof(int), csg_num_brushes);
   for (i = 0; i < csg_num_brushes; i++)
      if (ref_count[i] > 0)
      {
         ref_locs[i] = Malloc(sizeof(SurfaceRef) * ref_count[i]);
         (*func)(ref_locs[i], sizeof(SurfaceRef), ref_count[i]);
      }
}

// WARNING: i trash int *map!!!!
void remap_csg_database(int* map)
{
   int sourceIndex, i, k, highest = 0;
   
   if (wr_num_cells == 0)
      return;

   for (i = 0; i < wr_num_cells; i++)
      for (k = 0; k < wr_cell[i]->num_render_polys; k++)
         if (wr_brfaces[i][k] >= 0) 
            wr_brfaces[i][k] = (map[wr_brfaces[i][k] >> 8] << 8)  // brush
                               | (wr_brfaces[i][k]&255);   // face

   for (sourceIndex = 0; sourceIndex < MAX_FACE_BRUSHES; )
      if ((map[sourceIndex]==-1)||(map[sourceIndex]==sourceIndex))
      {
         sourceIndex++;
         if (sourceIndex + 1 > highest && map[sourceIndex] == sourceIndex)
            highest = sourceIndex + 1;
      }
      else
      {  // SWAP EVERYTHING... ICKY-POO (tm)
         SurfaceRef *tmp_sref;
         int destIndex = map[sourceIndex];
         int tmp_int;

         tmp_int=num_brush_faces[destIndex];
         num_brush_faces[destIndex] = num_brush_faces[sourceIndex];
         num_brush_faces[sourceIndex] = tmp_int;

         for (k = 0; k < MAX_FACES; k++)
         {
            BspPlane tmp = brush_faces[destIndex][k];
            brush_faces[destIndex][k] = brush_faces[sourceIndex][k];
            brush_faces[sourceIndex][k] = tmp;
         }
      
         tmp_sref=ref_locs[destIndex];
         ref_locs[destIndex] = ref_locs[sourceIndex];
         ref_locs[sourceIndex] = tmp_sref;

         tmp_int=ref_count[destIndex];
         ref_count[destIndex] = ref_count[sourceIndex];
         ref_count[sourceIndex] = tmp_int;

         if (destIndex + 1 > highest)
            highest = destIndex + 1;

         tmp_int=map[sourceIndex];
         map[sourceIndex]=map[destIndex];
         map[destIndex]=tmp_int;           // woo-woo
      }                            // now we have completely swapped them
                                   // so we can iterate and all should be ok
   csg_num_brushes = highest;
}

BOOL eq_planes(BspPlane *p, BspPlane *q)
{
   if (p->a*q->a + p->b*q->b + p->c*q->c >= VEC_DOT_ONE &&
       fabs(p->d - q->d) < PLANE_CONST_EPSILON)
      return TRUE;
   if (p->a*q->a + p->b*q->b + p->c*q->c <= -VEC_DOT_ONE &&
       fabs(p->d + q->d) < PLANE_CONST_EPSILON)
      return TRUE;
   return FALSE;
}

static bool eq_normals(BspPlane *p, BspPlane *q)
{
   if (p->a*q->a + p->b*q->b + p->c*q->c >= VEC_DOT_ONE)
      return TRUE;
   if (p->a*q->a + p->b*q->b + p->c*q->c <= -VEC_DOT_ONE)
      return TRUE;
   return FALSE;
}

int find_plane(BspPlane *p)
{
   int i;
   for (i=0; i < bsp_num_planes; ++i)
      if (eq_planes(&all_planes[i], p))
         return i;
   return -1;
}

int plane_brush(BspPlane *p)
{
   int i;
   for (i=0; i < bsp_num_planes; ++i)
      if (eq_planes(&all_planes[i], p))
         return plane_brushid[i];
   return -1;
}

#if 0
int find_plane_2(BspPlane *p)
{
   int i, best;
   double best_err;

   i = find_plane(p);
   if (i >= 0) return i;

   best_err = 0.01;
   best = -1;

   for (i=0; i < bsp_num_planes; ++i) {
      double err;
      err = fabs(all_planes[i].a - p->a)
          + fabs(all_planes[i].b - p->b);
          + fabs(all_planes[i].c - p->c);
          + fabs(all_planes[i].d - p->d);
      if (err < best_err) {
         best_err = err;
         best = i;
      }
   }
     
   if (best >= 0)
      mprintf("Warning: converting optimized plane to nearest match.\n");
   return best;
}
#endif

#define PEQ(x,y)   (fabs((x)-(y)) < REAL_EPSILON)

void show_match_plane(BspPlane *p)
{
   int i;
   for (i=0; i < bsp_num_planes; ++i) {
      if (PEQ(p->a, all_planes[i].a) &&
          PEQ(p->b, all_planes[i].b) &&
          PEQ(p->c, all_planes[i].c) &&
          PEQ(p->d, all_planes[i].d)) {
         mprintf("%lg %lg %lg %lg\n", all_planes[i].a,
                     all_planes[i].b, all_planes[i].c, all_planes[i].d);
      }
   }
}

extern int cur_brush;
void find_matched_plane(BspPlane *p)
{
   int i = find_plane(p);
   if (i == -1) {
      i = bsp_num_planes++;
      if (i == MAX_PLANES)
         Error(1, "Too many unique planes; increase MAX_PLANES.\n");
      all_planes[i] = *p;
      plane_brushid[i] = cur_brush;
      //mprintf("New plane: %g %g %g %g\n",p->a,p->b,p->c,p->d);
   } else {
      // if the constant d is within epsilon of an integer, we should
      // probably snap it...
      BspPlane *q = &all_planes[i], old = *p;
      if (p->a*q->a + p->b*q->b + p->c*q->c > 0)
         *p = *q;
      else {
         p->a = -q->a;
         p->b = -q->b;
         p->c = -q->c;
         p->d = -q->d;
      }
#if 0
mprintf("Coerced plane %g %g %g %g to %g %g %g %g\n",
        old.a,old.b,old.c,old.d,p->a,p->b,p->c,p->d);
#endif
   }
}

int find_normal(BspPlane *p)
{
   int i;
   for (i=0; i < bsp_num_planes; ++i)
      if (eq_normals(&all_planes[i], p))
         return i;
   return -1;
}

int normal_brush(BspPlane *p)
{
   int i;
   for (i=0; i < bsp_num_planes; ++i)
      if (eq_normals(&all_planes[i], p))
         return plane_brushid[i];
   return -1;
}

extern void find_matched_normal(BspPlane *p)
{
   int i = find_normal(p);
   if (i >= 0) {
      BspPlane *q = &all_planes[i], old = *p;
      if (p->a*q->a + p->b*q->b + p->c*q->c > 0) {
         p->a = q->a;
         p->b = q->b;
         p->c = q->c;
      } else {
         p->a = -q->a;
         p->b = -q->b;
         p->c = -q->c;
      }
#if 0
mprintf("Coerced normal %g %g %g to %g %g %g\n",
        old.a,old.b,old.c,p->a,p->b,p->c);
#endif
   }
}

void *tree;

int brush_count = 0;

extern void free_csg_data(void *head);
void init_csg_mem(void)
{
   static int init=1;
   if (init) {
      init = 0;
      csg_memory_init();
      CSGfree = free_csg_data;
   } else {
      if (tree)
         BspFreeTree(tree);
      tree = NULL;
      csg_memory_reset();
   }
}

extern void *BspReadTree(FILE *f);
int csg_clip_count;

void init_csg_internal_database(void)
{
   init_csg_mem();

   if (optimize_bsp) {
      FILE *f = fopen("bsp.out", "r");
      if (!f) {
         mprintf("No bsp.out file found.\n");
         tree = BspMakeTree();
      } else {
         tree = BspReadTree(f);
         if (!tree)
            tree = BspMakeTree();
         fclose(f);
      }
   } else
      tree = BspMakeTree();

   brush_count = 0;
   csg_clip_count = 0;
}

extern int base_medium;

extern void recursive_recompute_node(BspNode *b);
void default_csg_medium(int medium)
{
   if (medium != base_medium) {
      base_medium = medium;
      if (tree) {
         active = 0;
         recursive_recompute_node(tree);
         if (active)
            Error(1, "Active list non-empty after resetting csg medium.\n");
      }
   }
}

void cid_register_start(void)
{
   init_csg_mem();
   bsp_num_planes = 3;
}

extern void clear_surface_cache(void);
void free_portal_database(void)
{
   clear_surface_cache();
   reset_dynamic_lights();
   FreeWR();
   wrBspTreeDeallocate();
}

extern void recompute_node(BspNode *b);
void portalize_csg_internal_database(void)
{
   sAllocLimits alloc_limits;
   ulong old_alloc_cap;
   
   AllocGetLimits(&alloc_limits);
   old_alloc_cap = alloc_limits.allocCap;

   free_portal_database();
   
   AllocSetAllocCap(0x4000000); // 64 meg

   mprintf("Inserted %d brushes.\n", brush_count);

   // now that we know for sure if we have any clipping
   // brushes, recompute our medium

   recompute_node((BspNode *) tree);

     // merge leaves of the same type together
     // by setting userdata fields

   leaf_count = 0;
   RecodeNode((BspNode *) tree);

   mprintf("RAW cell count: %d\n", leaf_count);

   if (optimize_bsp) {
      FILE *f = fopen("bsp.out", "r");
      if (f) {
         BspCompareNode((BspNode *) tree, f);
         fclose(f);
      }
   }

     // now portalize it
   PortalizeTree(tree, 1000.0);

   // And free it because it is no longer needed
   BspFreeTree(tree);
   tree = NULL;

   AllocGetLimits(&alloc_limits);
   AllocSetAllocCap((alloc_limits.totalAlloc > old_alloc_cap) ? 
                        alloc_limits.totalAlloc : 
                        old_alloc_cap);
   _heapmin();
}
