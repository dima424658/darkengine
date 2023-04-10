/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/portal/wrdb.c,v 1.17 2000/02/19 13:18:53 toml Exp $

// World representation save/load

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <lg.h>
#include <mprintf.h>

#include <wrfunc.h>
#include <wrlimit.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

PortalCell *wr_cell[MAX_REGIONS];
int wr_num_cells;

//#define HACK_PORTAL

int poly_number, plane_count;
CachedVector *cv;

void WorldRepLoadPolygon(FILE *f, PortalCell *cell, PortalPolygonCore *pcore, PortalPlane *plane, int *vcount)
{
   int color, num_v, i, v, vc = *vcount, base = 64, j;

   fscanf(f, "%d%d", &color, &num_v);

   pcore->flags = 0;
   pcore->num_vertices = num_v;
   pcore->destination = color;
   pcore->clut_id = 0;

   for (i=0; i < num_v; ++i) {
      fscanf(f, "%d", &v);
      cell->vertex_list[vc] = v;
      cell->vertex_list_lighting[vc] = base;
      ++vc;
   }

   // load index of first vertex into v, then update count

   v = *vcount;
   *vcount = vc;

   // check if we lie along any of the existing planes
   for (i=0; i < plane_count; ++i) {
      plane = &cell->plane_list[i];
      for (j=0; j < num_v; ++j) {
         if (fabs(mx_dot_vec(&cell->vpool[cell->vertex_list[v+j]], &plane->norm->raw)+plane->plane_constant) > 0.10)
            break;
      }
      if (j == num_v) {
         cell->poly_list[poly_number].planeid = i;
         return;
      }
   }

   // we're not a new plane
   cell->plane_list[plane_count].norm = &cv[plane_count];
   cell->poly_list[poly_number].planeid = plane_count;

   // now compute the plane
   {
      // stick some values in local so the code is readable
      mxs_vector a,b;

      Vertex *pool = cell->vpool;          // all the vertices for this cell
      uchar *vlist = cell->vertex_list + v;  // the vertex list for this poly

      plane = &cell->plane_list[plane_count];

      mx_sub_vec(&a, &pool[vlist[1]], &pool[vlist[0]]);
      mx_sub_vec(&b, &pool[vlist[2]], &pool[vlist[1]]);
      mx_cross_norm_vec(&plane->norm->raw, &b, &a);

      // for a vertex, ax + by + cz + d = 0, so d = -(ax + by + cz)
      plane->plane_constant = -mx_dot_vec(&plane->norm->raw, &pool[vlist[0]]);
   }

   ++plane_count;
}

extern void wr_alloc_light_map(PortalCell *p, int surface, int vc);
void WorldRepLoadGodCell(FILE *f, int n, Vertex *vlist)
{
   // read the vertex palette
   int num_v, num_s, num_p, i, v, vcount;
   CachedVector *cv2;
   PortalCell *p;
   fscanf(f, "%d%d%d", &num_v, &num_s, &num_p);

   p = wr_cell[n] = Malloc(sizeof(PortalCell));
   p->num_vertices = num_v;
   p->num_polys = num_s + num_p;

#ifdef HACK_PORTAL
   ++num_s;
#endif

   p->num_render_polys = num_s;
   p->num_portal_polys = num_p;

   p->medium = 1;
   p->flags = 0;
  
   p->vpool = Malloc(sizeof(Vertex) * num_v);
   p->poly_list = Malloc(sizeof(PortalPolygonCore) * p->num_polys);
   p->portal_poly_list = p->poly_list + p->num_polys - num_p;
   p->render_list = Malloc(sizeof(PortalPolygonRenderInfo) * num_s);
   p->light_list = Malloc(sizeof(PortalLightMap) * num_s);

   // don't know how big vertex list is yet

   p->vertex_list = Malloc(256);
   p->vertex_list_lighting = Malloc(256);
   p->vertex_list_dynamic = 0;
   p->portal_vertex_list = 0;

   p->plane_list = Malloc(sizeof(PortalPlane) * (p->num_polys));
   p->num_planes = p->num_polys;
   p->render_data = 0;
   p->refs = 0;

   cv = Malloc(sizeof(CachedVector) * (p->num_polys));
   cv2 = Malloc(sizeof(CachedVector) * (num_s * 2));

   for (i=0; i < num_v; ++i) {
      fscanf(f, "%d", &v);      // convert vertex palette indexing global table
      p->vpool[i] = vlist[v];   //   into local palette per cell
   }

   vcount = 0;
   plane_count = 0;

      // read rendered surfaces
   for (i=0; i < num_s; ++i) {
      int vc = vcount;
      poly_number = i;
      WorldRepLoadPolygon(f, p, &p->poly_list[i], &p->plane_list[i], &vcount);
      p->render_list[i].texture_id = (i >= p->num_polys - num_p);

      p->render_list[i].u_base = p->render_list[i].v_base = 0;
      p->render_list[i].texture_anchor = 0;

      p->render_list[i].u = &cv2[i*2];
      p->render_list[i].v = &cv2[i*2+1];

      p->render_list[i].cached_surface = 0;

      mx_sub_vec(&p->render_list[i].u->raw, &p->vpool[p->vertex_list[vc+1]], &p->vpool[p->vertex_list[vc]]);
      mx_sub_vec(&p->render_list[i].v->raw, &p->vpool[p->vertex_list[vc+2]], &p->vpool[p->vertex_list[vc+1]]);
      mx_normeq_vec(&p->render_list[i].u->raw);
      mx_normeq_vec(&p->render_list[i].v->raw);
      mx_scaleeq_vec(&p->render_list[i].u->raw, 16.0);
      mx_scaleeq_vec(&p->render_list[i].v->raw, 16.0);
      if (i == p->num_polys-num_p - 1)
         p->portal_vertex_list = vcount;
      wr_alloc_light_map(p, i, vc);
   }

      // read portals
   for (i; i < p->num_polys; ++i) {
      poly_number = i;
      WorldRepLoadPolygon(f, p, &p->poly_list[i], &p->plane_list[i], &vcount);
   }

   p->num_planes = plane_count;

   p->num_vlist = vcount;
}

#define EPS  0.01

int src_portal, dest_cell;

void FindMatchingPortal(int src, PortalCell *r, PortalPlane *plane)
{
   PortalPolygonCore *p = r->portal_poly_list;
   int i,n = r->num_portal_polys;

   for (i=0; i < n; ++i) {
      if (p->destination == src) {
         PortalPlane *plane2 = &r->plane_list[p->planeid];
         mxs_vector *n1 = &plane->norm->raw, *n2 = &plane2->norm->raw;
         if (fabs(n1->x+n2->x) > EPS || fabs(n1->y+n2->y) > EPS || fabs(n1->z+n2->z) > EPS) {
            mprintf("Bad portal from cell %d portal %d\n", src, src_portal);
            mprintf("to cell %d portal %d\n", dest_cell, i);
            mprintf("Source normal: %f %f %f\n", (int) (65536.0 * n1->x), (int) (65536.0 * n1->y), (int) (65536.0 * n1->z));
            mprintf("Dest   normal: %f %f %f\n", (int) (65536.0 * n2->x), (int) (65536.0 * n2->y), (int) (65536.0 * n2->z));
            exit(1);
         }
         break;
      }
      ++p;
   }
   if (i == n) {
      mprintf("Bad portal from %d: no back-portal!\n");
      exit(1);
   }
}

void ValidateCellPointers(int s, PortalCell *r)
{
   PortalPolygonCore *p = r->portal_poly_list;
   int i,n = r->num_portal_polys;

   if (n > 128) Error(1, "Bad cell: %d portal polys.\n");

   for (i=0; i < n; ++i) {
      src_portal = i;
      dest_cell = p->destination;
      FindMatchingPortal(s, wr_cell[p->destination], &r->plane_list[p->planeid]);
      ++p;
   }
}

void WorldRepLoadGodFile(char *name)
{
   Vertex *v;
   int num,i;

   FILE *f = fopen(name, "r");
   if (!f)
      Error(1, "Couldn't open file %s\n", name);

     // let num = number of unique vertices
   fscanf(f, "%d", &num);
   
   v = Malloc(sizeof(Vertex) * num);
   for (i=0; i < num; ++i)
      fscanf(f, "%f%f%f", &v[i].x, &v[i].y, &v[i].z);

   fscanf(f, "%d", &num);
   wr_num_cells = num;

mprintf("%d cells\n", num);

   for (i=0; i < num; ++i)
      WorldRepLoadGodCell(f, i, v);

   fclose(f);

   for (i=0; i < num; ++i) {
      ValidateCellPointers(i, wr_cell[i]);
   }
}

void *MallocSafe(size_t sz)
{
   void *p = Malloc(sz);
   if (sz == 0) { mprintf("Allocated 0.\n"); return 0; }
   if (!p) Error(1, "Out of memory.\n");
   return p;
}

char *read_into(FILE *f, void **data, char *mem, int sz, int count)
{
   *data = mem;
   fread(mem, count, sz, f);
   return mem + count*sz;   
}

void bad_cell(PortalCell *p)
{
   mprintf("Bad cell:\n");
   mprintf("Num verts: %d\n", p->num_vertices);
   mprintf("Num polys; %d\n", p->num_polys);
   mprintf("Num render polys: %d\n", p->num_render_polys);
   mprintf("Num planes: %d\n", p->num_planes);
   Error(1, "Bad cell\n");
}

void WorldRepLoadWrCell(FILE *f, int s)
{
   PortalCell p, *q;
   CachedVector *cv;
   char *mem;
   int vl, sz, i;

   fread(&vl, 1, sizeof(vl), f);
   fread(&p, 1, sizeof(p), f);

   if ( // p.num_vertices > 128 || p.num_polys > 64 ||
       p.num_polys > 128 || 
       p.num_render_polys > p.num_polys || p.num_planes > p.num_polys)
       bad_cell(&p);

   // now figure out how much memory we need
   sz = sizeof(p) +
        p.num_vertices * sizeof(Vertex) +
        p.num_polys * sizeof(PortalPolygonCore) +
        p.num_render_polys * sizeof(PortalPolygonRenderInfo) +
        p.num_planes * sizeof(PortalPlane) +
        p.num_planes * sizeof(CachedVector) +
        p.num_render_polys * 2 * sizeof(CachedVector) +
        vl + p.num_vlist +
        p.num_render_polys * sizeof(PortalLightMap);

   mem = MallocSafe(sz);

   q = (PortalCell *) mem;
   *q = p;

   mem += sizeof(p);
   mem = read_into(f, &q->vpool, mem, sizeof(Vertex), p.num_vertices);
   mem = read_into(f, &q->poly_list, mem,
           sizeof(PortalPolygonCore), p.num_polys);
   mem = read_into(f, &q->render_list, mem,
           sizeof(PortalPolygonRenderInfo), p.num_render_polys);
   mem = read_into(f, &q->plane_list, mem, sizeof(PortalPlane), p.num_planes);
   mem = read_into(f, &q->vertex_list, mem, 1, vl);
   mem = read_into(f, &q->vertex_list_lighting, mem, 1, p.num_vlist);

   q->portal_poly_list = q->poly_list + q->num_polys - q->num_portal_polys;

   cv = (CachedVector *) mem;
   mem += sizeof(CachedVector) * 2 * p.num_render_polys;

   for (i=0; i < p.num_render_polys; ++i) {
      q->render_list[i].u = &cv[i*2+0];
      q->render_list[i].v = &cv[i*2+1];
      q->render_list[i].cached_surface = 0;
      fread(&q->render_list[i].u->raw, sizeof(Vector), 1, f);
      fread(&q->render_list[i].v->raw, sizeof(Vector), 1, f);
   }

   cv = (CachedVector *) mem;
   mem += sizeof(CachedVector) * p.num_planes;

   for (i=0; i < p.num_planes; ++i) {
      q->plane_list[i].norm = &cv[i];
      fread(&q->plane_list[i].norm->raw, sizeof(Vector), 1, f);
   }

   mem = read_into(f, &q->light_list, mem, sizeof(PortalLightMap),
               p.num_render_polys);

   if ((char *) q + sz != mem)
      Error(1, "read_cell: buffer size different from read amount.");

   // now read light maps

   for (i=0; i < p.num_render_polys; ++i) {
      int len = q->light_list[i].h * q->light_list[i].w;
      if (len > 60*60 || len < 0) bad_cell(&p);
      q->light_list[i].bits = Malloc(len);
      fread(q->light_list[i].bits, 1, len, f);
   }

   wr_cell[s] = q;
}

void WorldRepLoadWrFile(char *name)
{
   int i;
   FILE *f = fopen(name, "rb");

   if (!f)
      Error(1, "Couldn't open file %s\n", name);

   fread(&wr_num_cells, sizeof(wr_num_cells), 1, f);

mprintf("%d cells\n", wr_num_cells);

   for (i=0; i < wr_num_cells; ++i)
      WorldRepLoadWrCell(f, i);

   fclose(f);

   for (i=0; i < wr_num_cells; ++i) 
      ValidateCellPointers(i, wr_cell[i]);
}

#if 0

#define vector_scale  (2 * FIX_UNIT)

void compute_normal(g3s_vector *norm, g3s_vector *p0, g3s_vector *p1, g3s_vector *p2)
{
   g3s_vector delta1, delta2, result;

   delta1.x = p1->x - p0->x;
   delta1.y = p1->y - p0->y;
   delta1.z = p1->z - p0->z;

   delta2.x = p2->x - p1->x;
   delta2.y = p2->y - p1->y;
   delta2.z = p2->z - p1->z;

   result.x = fix_mul(delta1.z, delta2.y) - fix_mul(delta1.y, delta2.z);
   result.y = fix_mul(delta1.x, delta2.z) - fix_mul(delta1.z, delta2.x);
   result.z = fix_mul(delta1.y, delta2.x) - fix_mul(delta1.x, delta2.y);

   *norm = result;
}

void compute_surface_tmapping(int r, int n)
{
   int a,b,c,i;
   int p = region[r].vertex_palette;
   fix len;
   g3s_vector temp;

   a = v_palette[p + surf_v[surface[n].vlist_offset + 0]];
   b = v_palette[p + surf_v[surface[n].vlist_offset + 1]];
   c = v_palette[p + surf_v[surface[n].vlist_offset + 2]];

   compute_texture_vertex(&surface[n].u_vect, &vertex[a].point, a, b);
   compute_texture_vertex(&surface[n].v_vect, &surface[n].u_vect, b, c);

   // now compute normal to surface

   for(i=2; i < surface[n].num_vertices; ++i) {
      c = v_palette[p + surf_v[surface[n].vlist_offset + i]];
      compute_normal(&temp, &vertex[a].point, &vertex[b].point, &vertex[c].point);
      len = fix_mul(temp.x, temp.x) + fix_mul(temp.y,temp.y) + fix_mul(temp.z,temp.z);
      if (len > 64) break;
      // probably colinear, so try next point
   }
     
   g3_vec_normalize(&temp);
   surface[n].normal = temp;
}

#endif
