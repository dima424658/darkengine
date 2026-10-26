/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/wrloop.cpp,v 1.73 2000/03/18 15:20:54 adurant Exp $

#include <wrloop.h>

#include <lg.h>
#include <loopapi.h>

#include <brloop.h>
#include <dbasemsg.h>
#include <dispbase.h>  
#include <loopmsg.h>
#include <objloop.h>
#include <rendloop.h>

#include <animlgt.h>
#include <csg.h>
#include <mprintf.h>
#include <physapi.h>
#include <tagfile.h>
#include <vernum.h>
#include <wr.h>
#include <wrmem.h>
#include <wrbsp.h>
#include <wrlimit.h>
#include <wrdbrend.h>
#include <editbr.h>
#include <gedit.h>
#include <dbfile.h>
#include <ged_csg.h>
#include <medmotn.h>
#include <porthlpr.h>
#include <portal.h>
#include <family.h>
#include <objpos.h>
#include <texsave.h>
#include <texmem.h>
#include <missrend.h>

#include <config.h>
#include <cfgdbg.h>
#include <allocapi.h>

// Must be last header 
#include <dbmem.h>

// This shouldn't be here either
EXTERN int *wr_brfaces[];

//
// Goofy Memory management
//


static struct sMemBuffer 
{
   uchar* buf;   // the buffer
   ulong size;   // The buffer size; 
   uchar* next; // next available byte 
   ulong overflow;  // how much we couldn't allocate
} MemBuf; 


static void ClearMemBuf()
{
   delete [] MemBuf.buf; 
   MemBuf.buf = NULL; 
   MemBuf.size = 0; 
   MemBuf.next = NULL; 
   MemBuf.overflow = 0;
}

static void AllocMemBuf(ulong n)
{
#ifdef EDITOR
   MemBuf.buf = NULL;
   MemBuf.size = 0;
#else
#if UINTPTR_MAX > UINT32_MAX
   // The stored size describes the original 32-bit in-memory records.  Native
   // 64-bit pointers make the reconstructed world representation larger.
   n += n / 2;
#endif
   MemBuf.buf = new uchar[n]; 
   MemBuf.size = n; 
#endif
   MemBuf.next = MemBuf.buf; 
   MemBuf.overflow = 0;
}; 

#define DoMalloc(x) (new char[x])
#define DoFree(x)   (delete [] (char*)(x))

static void* wrAlloc(ulong size)
{
   if (MemBuf.next == NULL || MemBuf.buf == NULL)
      return DoMalloc(size);
   // Do not return the arena's one-past-end pointer.  wrFree uses the usual
   // half-open address range to distinguish arena storage from spill blocks.
   if (size == 0 && MemBuf.next == MemBuf.buf + MemBuf.size)
      return DoMalloc(1);
   if (MemBuf.next + size > MemBuf.buf + MemBuf.size)
   {
      if (MemBuf.overflow == 0)
          Warning(("wrAlloc: need %d bytes, have %d\n",size,MemBuf.size - (MemBuf.next - MemBuf.buf)));
      MemBuf.overflow += size;
      return DoMalloc(size); 
   }

   void* retval = MemBuf.next;
   MemBuf.next += size;
   return retval; 
}

static void wrFree(void* ptr)
{
   if (ptr < MemBuf.buf || ptr >= MemBuf.buf + MemBuf.size)
      DoFree(ptr); 
}

#ifndef SHIP
static void wrAllocPrintOverflow(void)
{
   if (MemBuf.overflow != 0) {
      mprintf("WARNING: %d bytes of worldrep not allocated from main block\n",
           MemBuf.overflow);
   }
}
#endif

/////////////////////////////////////////////////////////////
// WORLD REP LOOP CLIENT
////////////////////////////////////////////////////////////

//////////////////
// CONSTANTS
//
// These are just here to separate out boiler-plate code and leave it untouched
//

#define MY_GUID   LOOPID_Wr


// My context data
typedef void Context;

// fields to this structure. 
// 
// My state
typedef struct _StateRecord
{
   Context* context; // a pointer to the context data I got.
   // State fields go here
} StateRecord;

////////////////////////////////////////
// DATABASE MESSAGE HANDLER
//

typedef void (*PortalReadWrite) (void *buf, size_t elsize, size_t nelem);

// WRRGB stores two obsolete pointer fields as 32-bit zero placeholders.
// Keep the mission format fixed while allowing the live pointers to follow
// the native architecture.
typedef struct sPortalLightMapDisk
{
   short base_u, base_v;
   short pixel_row;
   uchar h, w;
   uint32 data;
   uint32 dynamic_light;
   uint anim_light_bitmask;
} sPortalLightMapDisk;

static_assert(sizeof(sPortalLightMapDisk) == 20, "Unexpected WRRGB light-map record layout");

// The live render record has a 16-bit texture id for WREXT.  WR/WRRGB keep
// the original byte id and 48-byte layout on disk.
typedef struct sPortalPolygonRenderInfoDiskLegacy
{
   mxs_vector tex_u, tex_v;
   ushort u_base, v_base;
   uchar texture_id;
   uchar texture_anchor;
   short cached_surface;
   mxs_real texture_mag;
   mxs_vector center;
} sPortalPolygonRenderInfoDiskLegacy;

static_assert(sizeof(sPortalPolygonRenderInfoDiskLegacy) == 48,
              "Unexpected legacy WR render-info record layout");

// WR 0.23 stores monochrome lightmap texels and scalar object-light
// brightness.  The live engine remains RGB; these flags affect only decoding.
static BOOL gReadScalarWorldRep = FALSE;
static BOOL gReadExtendedWorldRep = FALSE;
static int gExtendedHeaderVersion = 4;
static int gExtendedHeaderFlags = 0;
static int gExtendedLightmapFormat = 0;
static int gExtendedLightmapScale = 1;
static uchar gExtendedCellZones[MAX_REGIONS] = { 0 };
float portal_worldrep_texture_scale = 1.0f;
float *portal_wrext_u_base[MAX_REGIONS] = { NULL };
float *portal_wrext_v_base[MAX_REGIONS] = { NULL };
#ifdef RGB_LIGHTING
static TagFileTag WRExtTag = { "WREXT" };
#endif

// NewDark WREXT replaces the fixed-point texture bases and byte texture id
// with floats and a 16-bit id.  Keep the disk record independent of the live
// portal structure.
typedef struct sWRExtRenderInfoDisk
{
   mxs_vector tex_u, tex_v;
   float u_base, v_base;
   ushort texture_id;
   ushort cached_surface;
   float texture_mag;
   mxs_vector center;
} sWRExtRenderInfoDisk;

static_assert(sizeof(sWRExtRenderInfoDisk) == 52,
              "Unexpected WREXT render-info record layout");

int portal_cell_ambient_zone(int cell)
{
   return cell >= 0 && cell < wr_num_cells
      ? (gExtendedCellZones[cell] >> 4) : 0;
}

typedef struct sScalarMultiLightDisk
{
   mxs_vector loc;
   mxs_vector dir;
   float bright;
   float inner;
   float outer;
   float radius;
} sScalarMultiLightDisk;

static_assert(sizeof(sScalarMultiLightDisk) == 40,
              "Unexpected WR 0.23 scalar-light record layout");

void SpewCellPlanes(PortalCell *p)
{
   int      i;
   mprintf("num_vertices:%d\n", p->num_vertices);
   mprintf("num_polys:%d\n", p->num_polys);
   mprintf("num_render_polys:%d\n", p->num_render_polys);
   mprintf("num_portal_polys:%d\n", p->num_portal_polys);
   for (i = 0; i < p->num_planes; i++)
      mprintf("plane_list[i] %g,%g,%g, %g\n", p->plane_list[i].normal.x, p->plane_list[i].normal.y, p->plane_list[i].normal.z, p->plane_list[i].plane_constant);
}

void SpewCell(PortalCell *p)
{
   int      i;
   
   mprintf("num_vertices:%d\n", p->num_vertices);
   mprintf("num_polys:%d\n", p->num_polys);
   mprintf("num_render_polys:%d\n", p->num_render_polys);
   mprintf("num_portal_polys:%d\n", p->num_portal_polys);

   mprintf("num_planes:%d\n", p->num_planes);
   mprintf("medium:%d\n", p->medium);
   mprintf("flags:%d\n", p->flags);

   mprintf("portal_vertex_list:%d\n", p->portal_vertex_list);

   //mprintf("refs:%d", p->refs);

   mprintf("sphere_center:%g,%g,%g\n", p->sphere_center.x, p->sphere_center.y, p->sphere_center.z);
   mprintf("sphere_radius:%g\n", p->sphere_radius);

   for (i = 0; i < p->num_vertices; i++)
      mprintf("vpool[i]:%g,%g,%g\n", p->vpool[i].x, p->vpool[i].y, p->vpool[i].z);

   for (i = 0; i < p->num_polys; i++)
   {
      mprintf ("poly_list[i] flags:%d\n", p->poly_list[i].flags);
      // should be more here....
   }

   mprintf ("portal_poly_list[0] flags:%d\n", p->portal_poly_list[0]);
      
   for (i = 0; i < p->num_render_polys; i++)
   {
      mprintf("render_list u->raw:%g,%g,%g\n", p->render_list[i].tex_u.x, p->render_list[i].tex_u.y, p->render_list[i].tex_u.z);
      mprintf("render_list v->raw:%g,%g,%g\n", p->render_list[i].tex_v.x, p->render_list[i].tex_v.y, p->render_list[i].tex_v.z);
      mprintf("render_list u_base:%d\n", p->render_list[i].u_base);
      mprintf("render_list v_base:%d\n", p->render_list[i].v_base);
      mprintf("render_list texture_id:%d\n", p->render_list[i].texture_id);
      mprintf("render_list texture_anchor:%d\n", p->render_list[i].texture_anchor);
      mprintf("render_list cached_surface:%d\n", p->render_list[i].cached_surface);
   }
   
   int pi, voff = 0;
   for (pi = 0; pi < p->num_polys; ++pi)
      voff += p->poly_list[pi].num_vertices;
   for (i = 0; i < voff; i++)
      mprintf("vertex_list[i]:%d\n", p->vertex_list[i]);
      
   for (i = 0; i < p->num_planes; i++)
      mprintf("plane_list[i] normal %g,%g,%g\n", p->plane_list[i].normal.x, p->plane_list[i].normal.y, p->plane_list[i].normal.z);
      
//   for (i = 0; i < xxxx; i++)
//      mprintf("render_data[i]

   for (i = 0; i < p->num_render_polys; i++)
   {
      mprintf("light_list[i] base_u:%d\n", p->light_list[i].base_u);
      mprintf("light_list[i] base_u:%d\n", p->light_list[i].base_v);
      mprintf("light_list[i] h:%d\n", p->light_list[i].h);
      mprintf("light_list[i] w:%d\n", p->light_list[i].w);
      mprintf("light_list[i] bits[0]:%d\n", p->light_list[i].data[0]);
      mprintf("light_list[i] bits[h*w-1]:%d\n", p->light_list[i].data[p->light_list[i].h*p->light_list[i].w-1]);
   }

   for (i = 0; i < p->light_indices[0]; i++)
      mprintf("light_indices[i]:%d\n", p->light_indices[i]);
}

void SpewAllCells(void)
{
   int      ci;

   for (ci = 0; ci < wr_num_cells; ci++)
      if (wr_cell[ci] != NULL)
         SpewCell(wr_cell[ci]);
}

ulong ReadWriteNonPtrPortalCell(PortalReadWrite func, PortalCell *p)
{
   ulong retval = 0; 
   (*func)(&(p->num_vertices),     sizeof(uchar), 1);
   (*func)(&(p->num_polys),        sizeof(uchar), 1);
   (*func)(&(p->num_render_polys), sizeof(uchar), 1);
   (*func)(&(p->num_portal_polys), sizeof(uchar), 1);
   retval += 4*sizeof(uchar); 
   
   (*func)(&(p->num_planes), sizeof(uchar), 1);
   (*func)(&(p->medium),     sizeof(uchar), 1);
   (*func)(&(p->flags),      sizeof(uchar), 1);
   retval += 3* sizeof(uchar); 
   
   (*func)(&(p->portal_vertex_list), sizeof(int), 1);
   retval += sizeof(int); 
   
   (*func)(&(p->num_vlist), sizeof(ushort), 1);
   (*func)(&(p->num_anim_lights), sizeof(uchar), 1);
   (*func)(&(p->motion_index), sizeof(uchar), 1);
   retval += sizeof(ushort)+2*sizeof(uchar); 

   (*func)(&(p->sphere_center), sizeof(Vertex), 1);
   (*func)(&(p->sphere_radius), sizeof(mxs_real), 1);
   retval += sizeof(Vertex) + sizeof(mxs_real) ;

   return retval; 
}

#if 0
ulong WriteCachedVector(PortalReadWrite func, CachedVector *cv)
{
   (*func)(&(cv->raw), sizeof(Vector), 1);
   return sizeof(CachedVector); 
//   (*func)(cv->cached, sizeof(CachedData), 1);
}

void ReadAndAllocCachedVector(PortalReadWrite func, CachedVector **cv)
{
   *cv = (CachedVector*) wrAlloc(sizeof(CachedVector));
   (*func)(&((**cv).raw), sizeof(Vector), 1);
   
//   (**cv).cached = (st_CachedData*) DoMalloc(sizeof(CachedData));
//   (*func)((**cv).cached, sizeof(CachedData), 1);
}
#endif


//
// Free worldrep structures
//
// NOTE: This does NOT free wrBspTree.  You must do this explicitly.
//       The reason it doesn't is that FreeWR gets called at the
//       beginning of each pass of the portalization, and we need
//       to preserve wrBspTree through all passes.
//
// NOTE: FreeWR seems to be getting called twice in a row typically.
//       Right now I'm not going to worry about it because it doesn't
//       seem to be hurting anything, but be aware of it.
void FreeWR(void)
{
   int         ci, i;
   PortalCell  *p;
   
   clear_surface_cache();
#ifdef RGB_LIGHTING
   dbCompatibilityRewriteTag(&WRExtTag,
      "world representation changed; source block will be regenerated");
   memset(gExtendedCellZones,0,sizeof(gExtendedCellZones));
#endif
   ObjDeleteAllRefs();
   reset_dynamic_lights();

   for (ci = 0; ci < wr_num_cells; ci++) {
      delete [] portal_wrext_u_base[ci];
      delete [] portal_wrext_v_base[ci];
      portal_wrext_u_base[ci] = NULL;
      portal_wrext_v_base[ci] = NULL;
      if (wr_cell[ci] != NULL) {
         p = wr_cell[ci];
           
         if (p->vpool != NULL)                wrFree(p->vpool);
         if (p->poly_list != NULL)            wrFree(p->poly_list);
         if (p->render_list != NULL) {
            wrFree(p->render_list);
         }
         if (p->vertex_list != NULL)          wrFree(p->vertex_list);
         if (p->plane_list != NULL)           wrFree(p->plane_list);

         if (p->render_data != NULL)             wrFree(p->render_data);
         if (p->anim_light_index_list != NULL)   wrFree(p->anim_light_index_list);
         if (p->light_list != NULL) {
            for (i = 0; i < p->num_render_polys; i++) {
               if (p->light_list[i].data != NULL) wrFree(p->light_list[i].data);
               if (p->light_list[i].dynamic_light != NULL) wrFree(p->light_list[i].dynamic_light);
            }
            wrFree(p->light_list);
         }         
         if (p->light_indices != NULL)        wrFree(p->light_indices);
    
#ifdef EDITOR               
         if (wr_brfaces[ci] != NULL)          wrFree(wr_brfaces[ci]);
#endif 
         
       
         wrFree(wr_cell[ci]);
         wr_cell[ci] = NULL; 
         // NOTE: doesn't free refs
      }
   }

   ClearMemBuf(); 

   wr_num_cells = 0;
}


// Given a rendered non-portal polygon in a cell, how many lightmaps?
// There's always the static one.  Each animated animated light which
// reaches this polygon contributes another.
static int lightmap_count(PortalCell *cell, int polygon_index)
{
   int count = 1;
   uint bitmask = cell->light_list[polygon_index].anim_light_bitmask;

   while (bitmask) {
      if (bitmask & 1)
         count++;
      bitmask >>= 1;
   }

   return count;
}

ulong WriteWR(PortalReadWrite func, BOOL extended = FALSE)
{
   int            ci, i;            // portal cell index
   int            pi, voff, num_light_indices;
   PortalCell     *p;

   ulong alloced_bytes_written = 0 ; 
   
   //SpewAllCells();
   
   // write the number of portal cells to be written
   (*func)(&wr_num_cells, sizeof(int), 1);

   for (ci = 0; ci < wr_num_cells; ci++)
   {
      p = wr_cell[ci];
      
      // write the portal cell
      ReadWriteNonPtrPortalCell(func, p);
      alloced_bytes_written += sizeof(*p); 

      // write the vpool
      (*func)(p->vpool, sizeof(Vertex), p->num_vertices);
      alloced_bytes_written += sizeof(Vertex)*p->num_vertices; 
      
      // write the poly_list
      (*func)(p->poly_list, sizeof(PortalPolygonCore), p->num_polys);
      alloced_bytes_written += sizeof(PortalPolygonCore)*p->num_polys; 
      
      // Keep the disk record independent of the widened live structure.
      for (i = 0; i < p->num_render_polys; ++i)
      {
         if (extended)
         {
            sWRExtRenderInfoDisk diskRender;
            diskRender.tex_u = p->render_list[i].tex_u;
            diskRender.tex_v = p->render_list[i].tex_v;
            diskRender.u_base = portal_wrext_u_base[ci]
               ? portal_wrext_u_base[ci][i]
               : p->render_list[i].u_base / 4096.0f;
            diskRender.v_base = portal_wrext_v_base[ci]
               ? portal_wrext_v_base[ci][i]
               : p->render_list[i].v_base / 4096.0f;
            diskRender.texture_id = p->render_list[i].texture_id;
            diskRender.cached_surface = 0;
            diskRender.texture_mag = p->render_list[i].texture_mag;
            diskRender.center = p->render_list[i].center;
            (*func)(&diskRender, sizeof(diskRender), 1);
         }
         else
         {
            sPortalPolygonRenderInfoDiskLegacy diskRender;
            diskRender.tex_u = p->render_list[i].tex_u;
            diskRender.tex_v = p->render_list[i].tex_v;
            diskRender.u_base = p->render_list[i].u_base;
            diskRender.v_base = p->render_list[i].v_base;
            if (p->render_list[i].texture_id >
                DARK_LIMIT_WR_LEGACY_TEXTURE_ID_MAX)
            {
               Warning(("Legacy WR cannot encode terrain texture id %u; writing texture 0\n",
                        p->render_list[i].texture_id));
               diskRender.texture_id = 0;
            }
            else
               diskRender.texture_id = (uchar)p->render_list[i].texture_id;
            diskRender.texture_anchor = p->render_list[i].texture_anchor;
            diskRender.cached_surface = 0;
            diskRender.texture_mag = p->render_list[i].texture_mag;
            diskRender.center = p->render_list[i].center;
            (*func)(&diskRender, sizeof(diskRender), 1);
         }
      }
      alloced_bytes_written += (extended ? sizeof(sWRExtRenderInfoDisk)
                                         : sizeof(sPortalPolygonRenderInfoDiskLegacy))
                               * p->num_render_polys;

      // calculate the highest index into the vertex_list used,
      // and then write the index, and then the vertex_list array.
      voff = 0;
      for (pi = 0; pi < p->num_polys; ++pi)
         voff += p->poly_list[pi].num_vertices;
      (*func)(&voff, sizeof(int), 1);
      (*func)(p->vertex_list, sizeof(uchar), voff);
      alloced_bytes_written += sizeof(uchar)*voff; 
      
      // write the plane_list
      (*func)(p->plane_list, sizeof(PortalPlane), p->num_planes);
      alloced_bytes_written += sizeof(PortalPlane)*p->num_planes; 

      if (p->num_anim_lights)
         (*func)(p->anim_light_index_list, sizeof(ushort), p->num_anim_lights);
      alloced_bytes_written += sizeof(ushort)*p->num_anim_lights;

      // write the light_list and the sub field bits parameter
      // zero out the pointers (.data, and .dynamic_light), since we
      // zero them out on load anyway, and they are meaningless to save.
      // If we don't, we keep getting a different WRRGB tag which sucks
      // for missdiff.exe
      sPortalLightMapDisk templist;
      for (i = 0; i < p->num_render_polys; i++)
      {
         templist.base_u = p->light_list[i].base_u;
         templist.base_v = p->light_list[i].base_v;
         templist.pixel_row = p->light_list[i].pixel_row;
         templist.h = p->light_list[i].h;
         templist.w = p->light_list[i].w;
         templist.data = 0;
         templist.dynamic_light = 0;
         templist.anim_light_bitmask = p->light_list[i].anim_light_bitmask;
         (*func)(&templist,sizeof(templist),1);
      }
      //(*func)(p->light_list, sizeof(PortalLightMap), p->num_render_polys);
      alloced_bytes_written += sizeof(PortalLightMap) * p->num_render_polys;
      for (i = 0; i < p->num_render_polys; i++)
      {
         const int layers = lightmap_count(p,i);
         const int height = p->light_list[i].h;
         const int width = p->light_list[i].w;
         const int row = p->light_list[i].pixel_row;
         int lightmap_entries = height * (extended ? width : row) * layers;
         LightmapEntry* packed = NULL;
         if (extended)
         {
            packed = new LightmapEntry[lightmap_entries];
            for (int layer = 0; layer < layers; ++layer)
               for (int y = 0; y < height; ++y)
                  memcpy(packed + (layer * height + y) * width,
                         p->light_list[i].data +
                            (layer * height + y) * row,
                         width * sizeof(LightmapEntry));
         }
         if (extended && gExtendedLightmapFormat != 0)
         {
            uint32* pixels = new uint32[lightmap_entries];
            for (int pixel = 0; pixel < lightmap_entries; ++pixel)
            {
               const uint value = packed[pixel];
               uint red = (value & 0x1f) * 255 / 31;
               uint green = ((value >> 5) & 0x1f) * 255 / 31;
               uint blue = ((value >> 10) & 0x1f) * 255 / 31;
               if (gExtendedLightmapFormat == 2)
               {
                  red >>= 1;
                  green >>= 1;
                  blue >>= 1;
               }
               pixels[pixel] = blue | (green << 8) | (red << 16) |
                               0xff000000;
            }
            (*func)(pixels,sizeof(uint32),lightmap_entries);
            delete [] pixels;
         }
         else if (extended)
            (*func)(packed,sizeof(LightmapEntry),lightmap_entries);
         else
            (*func)(p->light_list[i].data,sizeof(LightmapEntry),
                    lightmap_entries);
         delete [] packed;
         alloced_bytes_written +=
            (extended && gExtendedLightmapFormat != 0
               ? sizeof(uint32) : sizeof(LightmapEntry)) * lightmap_entries;
      }

      // write the light_indices
      num_light_indices = p->light_indices[0]+1;
      if (num_light_indices>1024)
         mprintf("ummm, dont take this the wrong way, but cell %d has %d light idx\n",ci,num_light_indices);
      (*func)(&num_light_indices, sizeof(int), 1);
      (*func)(p->light_indices, sizeof(ushort), num_light_indices);
      // a little silly to write out the first field twice, oh well
      alloced_bytes_written += sizeof(ushort) * (num_light_indices);

      // pointers in struct that are not being written to disk yet......
      //////////////////////////////////////////////////////////////////
      // vertex_list_dynamic
      // refs
      // decal ptr and dynamic ptr in PortalLightMap (same place as bits)
   }

   wrBspTreeWrite(func);

   return alloced_bytes_written; 
}


void ReadWR(PortalReadWrite func)
{
   int            ci, i;
   int            num_vertex_list, num_light_indices;
   PortalCell     *p;

   // read the number of portal cells to be read
   (*func)(&wr_num_cells, sizeof(int), 1);

   if (wr_num_cells < 0 || wr_num_cells > MAX_REGIONS)
   {
      Warning(("Worldrep cell count %d exceeds limit %d\n",
               wr_num_cells, MAX_REGIONS));
      wr_num_cells = 0;
      ged_validate_level(FALSE);
      return;
   }

   if (wr_num_cells > 0)
   {
      ged_validate_level(TRUE);

   }
   else
   {
      ged_validate_level(FALSE); 
   }
   
   for (ci = 0; ci < wr_num_cells; ci++)
   {
      // alloc and read the portal cell
      wr_cell[ci] = (PortalCell*) wrAlloc(sizeof(PortalCell));
      p = wr_cell[ci];
      ReadWriteNonPtrPortalCell(func, p);

      // alloc and read the vpool      
      p->vpool = (Vertex*) wrAlloc(sizeof(Vertex) * p->num_vertices);
      (*func)(p->vpool, sizeof(Vertex), p->num_vertices);
      
      // alloc and read the poly_list
      p->poly_list = (PortalPolygonCore*) wrAlloc(sizeof(PortalPolygonCore) * p->num_polys);
      (*func)(p->poly_list, sizeof(PortalPolygonCore), p->num_polys);

      // alloc and read the render_list
      p->render_list = (PortalPolygonRenderInfo*) wrAlloc(sizeof(PortalPolygonRenderInfo)* p->num_render_polys);
      if (gReadExtendedWorldRep)
      {
         portal_wrext_u_base[ci] = new float[p->num_render_polys];
         portal_wrext_v_base[ci] = new float[p->num_render_polys];
         for (i = 0; i < p->num_render_polys; ++i)
         {
            sWRExtRenderInfoDisk diskRender;
            (*func)(&diskRender, sizeof(diskRender), 1);
            p->render_list[i].tex_u = diskRender.tex_u;
            p->render_list[i].tex_v = diskRender.tex_v;
            // The native renderer consumes 12.4 fixed-point bases relative
            // to texture_anchor.  WREXT stores the same values as floats and
            // anchors them to the polygon's first vertex.
            p->render_list[i].u_base = (ushort)(int)(diskRender.u_base * 4096.0f);
            p->render_list[i].v_base = (ushort)(int)(diskRender.v_base * 4096.0f);
            portal_wrext_u_base[ci][i] = diskRender.u_base;
            portal_wrext_v_base[ci][i] = diskRender.v_base;
            if (diskRender.texture_id > DARK_LIMIT_WREXT_TEXTURE_ID_MAX ||
                diskRender.texture_id >= TEXMEM_MAX)
            {
               Warning(("WREXT texture id %u exceeds the %u-entry texture table\n",
                        diskRender.texture_id, TEXMEM_MAX));
               p->render_list[i].texture_id = 0;
            }
            else
               p->render_list[i].texture_id = diskRender.texture_id;
            p->render_list[i].texture_anchor = 0;
            p->render_list[i].cached_surface = 0;
            p->render_list[i].texture_mag = diskRender.texture_mag;
            p->render_list[i].center = diskRender.center;
         }
      }
      else
      {
         for (i = 0; i < p->num_render_polys; ++i)
         {
            sPortalPolygonRenderInfoDiskLegacy diskRender;
            (*func)(&diskRender, sizeof(diskRender), 1);
            p->render_list[i].tex_u = diskRender.tex_u;
            p->render_list[i].tex_v = diskRender.tex_v;
            p->render_list[i].u_base = diskRender.u_base;
            p->render_list[i].v_base = diskRender.v_base;
            p->render_list[i].texture_id = diskRender.texture_id;
            p->render_list[i].texture_anchor = diskRender.texture_anchor;
            p->render_list[i].cached_surface = 0;
            p->render_list[i].texture_mag = diskRender.texture_mag;
            p->render_list[i].center = diskRender.center;
         }
      }
      for (i=0; i < p->num_render_polys; ++i)
          p->render_list[i].cached_surface = 0;

      // set the portal_list_ptr
      p->portal_poly_list = p->poly_list + p->num_polys - p->num_portal_polys;
      
      // read the size of the vertex_list and then alloc and read the array
      (*func)(&num_vertex_list, sizeof(int), 1);
      p->vertex_list = (uchar*) wrAlloc(sizeof(uchar) * num_vertex_list);
      (*func)(p->vertex_list, sizeof(uchar), num_vertex_list);
      
      // alloc and read the plane_list, and then read in the 
      // cached vectors inside the plane_list
      p->plane_list = (PortalPlane*) wrAlloc(sizeof(PortalPlane) * p->num_planes);
      (*func)(p->plane_list, sizeof(PortalPlane), p->num_planes);

#ifdef DEBUG
      g_pMalloc->PushCredit("WR Light Maps",0); 
#endif 
      
      // DoMalloc and read the anim_light_index_list
      if (p->num_anim_lights) {
         p->anim_light_index_list
            = (ushort*) wrAlloc(sizeof(ushort) * p->num_anim_lights);
         (*func)(p->anim_light_index_list, sizeof(ushort), p->num_anim_lights);
      } else
         p->anim_light_index_list = NULL;
      
      // DoMalloc and read the light_list and the sub field bits pointer
      p->light_list = (PortalLightMap*) wrAlloc(sizeof(PortalLightMap)
                                             * p->num_render_polys);
      for (i = 0; i < p->num_render_polys; ++i)
      {
         sPortalLightMapDisk diskLightMap;
         (*func)(&diskLightMap, sizeof(diskLightMap), 1);
         p->light_list[i].base_u = diskLightMap.base_u;
         p->light_list[i].base_v = diskLightMap.base_v;
         p->light_list[i].pixel_row = diskLightMap.pixel_row;
         p->light_list[i].h = diskLightMap.h;
         p->light_list[i].w = diskLightMap.w;
         if (gReadExtendedWorldRep &&
             p->light_list[i].pixel_row < p->light_list[i].w)
            p->light_list[i].pixel_row = p->light_list[i].w;
         p->light_list[i].data = NULL;
         p->light_list[i].dynamic_light = NULL;
         p->light_list[i].anim_light_bitmask = diskLightMap.anim_light_bitmask;
      }

      for (i = 0; i < p->num_render_polys; i++)
      {
         const int layers = lightmap_count(p,i);
         const int height = p->light_list[i].h;
         const int width = p->light_list[i].w;
         const int row = p->light_list[i].pixel_row;
         const int serializedEntries = height *
            (gReadExtendedWorldRep ? width : row) * layers;
         const int storageEntries = height * row * layers;
         const int lightmap_size = storageEntries * sizeof(LightmapEntry);

         p->light_list[i].data = (LightmapEntry*) wrAlloc(lightmap_size);
         memset(p->light_list[i].data,0,lightmap_size);
         LightmapEntry* packed = gReadExtendedWorldRep
            ? new LightmapEntry[serializedEntries] : p->light_list[i].data;
         if (gReadScalarWorldRep)
         {
            uchar* scalar = new uchar[serializedEntries];
            (*func)(scalar, 1, serializedEntries);
            for (int pixel = 0; pixel < serializedEntries; ++pixel)
            {
               ushort component = (ushort)(scalar[pixel] >> 3);
               packed[pixel] =
                  (LightmapEntry)(component | (component << 5) | (component << 10));
            }
            delete [] scalar;
         }
         else if (gReadExtendedWorldRep && gExtendedLightmapFormat != 0)
         {
            uint32* extended = new uint32[serializedEntries];
            (*func)(extended, sizeof(uint32), serializedEntries);
            for (int pixel = 0; pixel < serializedEntries; ++pixel)
            {
               uint32 value = extended[pixel];
               uint red = (value >> 16) & 0xff;
               uint green = (value >> 8) & 0xff;
               uint blue = value & 0xff;
               // Format 2 uses half-range channels.  Expand them before
               // reducing the BGR888 source to the renderer's RGB555 entry.
               if (gExtendedLightmapFormat == 2)
               {
                  red = red > 127 ? 255 : red * 2;
                  green = green > 127 ? 255 : green * 2;
                  blue = blue > 127 ? 255 : blue * 2;
               }
               // Preserve the nearest representable RGB555 value.  A shift
               // always rounds down; at NewDark's common low-light floor of
               // 14 that produced 1/31 instead of the much closer 2/31 and
               // made dark WREXT cells appear nearly black.
               red = (red * 31 + 127) / 255;
               green = (green * 31 + 127) / 255;
               blue = (blue * 31 + 127) / 255;
               packed[pixel] = (LightmapEntry)
                  (red | (green << 5) | (blue << 10));
            }
            delete [] extended;
         }
         else
            (*func)(packed,sizeof(LightmapEntry),serializedEntries);

         if (gReadExtendedWorldRep)
         {
            for (int layer = 0; layer < layers; ++layer)
               for (int y = 0; y < height; ++y)
                  memcpy(p->light_list[i].data +
                            (layer * height + y) * row,
                         packed + (layer * height + y) * width,
                         width * sizeof(LightmapEntry));
            delete [] packed;
         }

         p->light_list[i].dynamic_light = NULL;
      }
      
      // DoMalloc and read the light_indices
      (*func)(&num_light_indices, sizeof(int), 1);
      p->light_indices = (ushort*) wrAlloc(sizeof(ushort) * num_light_indices);
      (*func)(p->light_indices, sizeof(ushort), num_light_indices);

      // pointers in the struct which are not being read from disk yet....
      /////////////////////////////////////////////////////////////////////
      p->refs = NULL;
      p->render_data = NULL;
      p->changed_anim_light_bitmask = 0;
      p->num_full_bright = 0;
      // and decal and dynamic in PortalLightMap

#ifdef DEBUG
      g_pMalloc->PopCredit(); 
#endif 
   }

#ifdef DEBUG
   g_pMalloc->PushCredit("WR BSP",0); 
#endif 

   wrBspTreeRead(func);
   wrBspTreeRefCells(WRBSP_HEAD);

#ifdef DEBUG
   g_pMalloc->PopCredit(); 
#endif 
#ifndef SHIP
   wrAllocPrintOverflow();
#endif

   //SpewAllCells();
}

//////////////////////////////////////////////////
// World Rep Save/Load Stuff

//////////////////////////////////////////////////
// light save/load stuff -- anim light scripts are next

typedef void (*LightReadWrite) (void *buf, size_t elsize, size_t nelem);

#include <mlight.h>

#define LEGACY_LIGHT_SLOTS DARK_LIMIT_WR_LEGACY_LIGHT_SLOTS
#define LEGACY_ANIM_LIGHT_TO_CELLS \
   DARK_LIMIT_WR_LEGACY_ANIM_LIGHT_CELL_PAIRS

EXTERN int num_light;
EXTERN int num_dyn;
EXTERN mls_multi_light light_data[];
EXTERN mls_multi_light light_this[];

// @TODO: Shouldn't this be using num_light for the number of elements?
void ReadWriteLight(LightReadWrite func, BOOL reading)
{
   // object lighting
   int storedNumLight = num_light;
   int storedNumDyn = num_dyn;
   if (!reading && storedNumLight + storedNumDyn > LEGACY_LIGHT_SLOTS)
   {
      Warning(("Legacy WR writer cannot store %d static/dynamic lights; "
               "writing the first %d slots\n",
               storedNumLight + storedNumDyn, LEGACY_LIGHT_SLOTS));
      storedNumLight = storedNumLight > LEGACY_LIGHT_SLOTS
                     ? LEGACY_LIGHT_SLOTS : storedNumLight;
      storedNumDyn = storedNumDyn > LEGACY_LIGHT_SLOTS - storedNumLight
                   ? LEGACY_LIGHT_SLOTS - storedNumLight : storedNumDyn;
   }
   (*func)(&storedNumLight, sizeof(int), 1);
   (*func)(&storedNumDyn, sizeof(int), 1);
   if (reading)
   {
      if (storedNumLight < 0 || storedNumDyn < 0)
      {
         Warning(("Invalid legacy WR light counts %d/%d\n",
                  storedNumLight, storedNumDyn));
         storedNumLight = storedNumDyn = 0;
      }
      num_light = storedNumLight > MAX_STATIC ? MAX_STATIC : storedNumLight;
      num_dyn = storedNumDyn;
      if (num_light + num_dyn > MAX_STATIC)
         num_dyn = MAX_STATIC - num_light;
      memset(light_data, 0, sizeof(mls_multi_light) * MAX_STATIC);
   }

   if (gReadScalarWorldRep)
   {
      sScalarMultiLightDisk* scalar = new sScalarMultiLightDisk[LEGACY_LIGHT_SLOTS];
      (*func)(scalar, sizeof(*scalar), LEGACY_LIGHT_SLOTS);
      for (int i = 0; i < LEGACY_LIGHT_SLOTS; ++i)
      {
         light_data[i].loc = scalar[i].loc;
         light_data[i].dir = scalar[i].dir;
         light_data[i].bright.x = scalar[i].bright;
         light_data[i].bright.y = scalar[i].bright;
         light_data[i].bright.z = scalar[i].bright;
         light_data[i].inner = scalar[i].inner;
         light_data[i].outer = scalar[i].outer;
         light_data[i].radius = scalar[i].radius;
      }
      delete [] scalar;

      scalar = new sScalarMultiLightDisk[32];
      (*func)(scalar, sizeof(*scalar), 32);
      for (int i = 0; i < 32; ++i)
      {
         light_this[i].loc = scalar[i].loc;
         light_this[i].dir = scalar[i].dir;
         light_this[i].bright.x = scalar[i].bright;
         light_this[i].bright.y = scalar[i].bright;
         light_this[i].bright.z = scalar[i].bright;
         light_this[i].inner = scalar[i].inner;
         light_this[i].outer = scalar[i].outer;
         light_this[i].radius = scalar[i].radius;
      }
      delete [] scalar;
   }
   else
   {
      mls_multi_light* legacy = new mls_multi_light[LEGACY_LIGHT_SLOTS];
      if (!reading)
         memcpy(legacy, light_data, sizeof(*legacy) * LEGACY_LIGHT_SLOTS);
      (*func)(legacy, sizeof(*legacy), LEGACY_LIGHT_SLOTS);
      if (reading)
         memcpy(light_data, legacy, sizeof(*legacy) * LEGACY_LIGHT_SLOTS);
      delete [] legacy;
      (*func)(light_this, sizeof(mls_multi_light), 32);
   }

   // mappings from animated lights to cells
   int storedPairings = g_iCurAnimLightToCell;
   if (!reading && storedPairings > LEGACY_ANIM_LIGHT_TO_CELLS)
   {
      Warning(("Legacy WR writer cannot store %d animated-light/cell pairings; "
               "writing %d\n", storedPairings, LEGACY_ANIM_LIGHT_TO_CELLS));
      storedPairings = LEGACY_ANIM_LIGHT_TO_CELLS;
   }
   (*func)(&storedPairings, sizeof(int), 1);
   if (reading)
   {
      if (storedPairings < 0 || storedPairings > 1048576)
      {
         Warning(("Invalid legacy WR animated-light/cell count %d\n",
                  storedPairings));
         storedPairings = 0;
      }
      g_iCurAnimLightToCell = storedPairings > MAX_ANIM_LIGHT_TO_CELLS
                           ? MAX_ANIM_LIGHT_TO_CELLS : storedPairings;
      (*func)(&g_aAnimLightToCell[0], sizeof(sAnimLightToCell),
              g_iCurAnimLightToCell);
      int remaining = storedPairings - g_iCurAnimLightToCell;
      sAnimLightToCell discard[256];
      while (remaining > 0)
      {
         int count = remaining > 256 ? 256 : remaining;
         (*func)(discard, sizeof(discard[0]), count);
         remaining -= count;
      }
   }
   else
      (*func)(&g_aAnimLightToCell[0], sizeof(sAnimLightToCell), storedPairings);
}

// WREXT stores one weather byte per cell, followed by only the used static
// and dynamic lights (plus the 32 scratch lights).  Legacy WR always writes
// the entire fixed-capacity static-light array.
void ReadWriteWRExtLight(LightReadWrite func, BOOL reading)
{
   int staticLights = reading ? 0 : num_light;
   int dynamicLights = reading ? 0 : num_dyn;
   int animatedCount = reading ? 0 : g_iCurAnimLightToCell;
   (*func)(gExtendedCellZones, sizeof(uchar), wr_num_cells);

   (*func)(&staticLights, sizeof(int), 1);
   (*func)(&dynamicLights, sizeof(int), 1);
   if (staticLights < 0 || dynamicLights < 0 ||
       staticLights + dynamicLights > 1048576)
   {
      Warning(("Invalid WREXT light counts %d/%d\n", staticLights, dynamicLights));
      if (reading)
         num_light = num_dyn = 0;
      return;
   }

   if (reading)
   {
      memset(light_data, 0, sizeof(mls_multi_light) * MAX_STATIC);
      num_light = staticLights > MAX_STATIC ? MAX_STATIC : staticLights;
      num_dyn = dynamicLights;
      if (num_light + num_dyn > MAX_STATIC)
         num_dyn = MAX_STATIC - num_light;
   }

   const int serializedLights = staticLights + dynamicLights;
   for (int i = 0; i < serializedLights; ++i)
   {
      mls_multi_light light = {};
      if (!reading && i < MAX_STATIC)
         light = light_data[i];
      (*func)(&light, sizeof(light), 1);
      if (reading && i < MAX_STATIC)
         light_data[i] = light;
   }
   (*func)(light_this, sizeof(mls_multi_light), 32);

   (*func)(&animatedCount, sizeof(int), 1);
   if (animatedCount < 0 || animatedCount > 1048576)
   {
      Warning(("Invalid WREXT animated-light/cell count %d\n", animatedCount));
      animatedCount = 0;
   }
   if (reading)
   {
      g_iCurAnimLightToCell = animatedCount;
      if (g_iCurAnimLightToCell > MAX_ANIM_LIGHT_TO_CELLS)
         g_iCurAnimLightToCell = MAX_ANIM_LIGHT_TO_CELLS;
   }
   for (int i = 0; i < animatedCount; ++i)
   {
      sAnimLightToCell entry = {};
      if (!reading && i < MAX_ANIM_LIGHT_TO_CELLS)
         entry = g_aAnimLightToCell[i];
      (*func)(&entry, sizeof(entry), 1);
      if (reading && i < MAX_ANIM_LIGHT_TO_CELLS)
         g_aAnimLightToCell[i] = entry;
   }
}

// end of light save/load stuff
//////////////////////////////////////////////////


//////////////////////////////////////////////////
// management of PortalCellMotion structures


static float gWaterProjectionThreshold = 0.005f;

static void ReadWriteWaterMotion(PortalReadWrite func, BOOL readExtended)
{
   (*func)(portal_cell_motion,
           sizeof(portal_cell_motion[0]), MAX_CELL_MOTION);
   (*func)(g_aMedMoCellMotion,
           sizeof(g_aMedMoCellMotion[0]), MAX_CELL_MOTION);
   if (readExtended)
      (*func)(&gWaterProjectionThreshold,
              sizeof(gWaterProjectionThreshold), 1);
}


static void ClearWaterMotion()
{
   gWaterProjectionThreshold = 0.005f;
   memset(&g_aMedMoCellMotion[0], 0,
          sizeof(g_aMedMoCellMotion[0]) * MAX_CELL_MOTION);

   memset(&portal_cell_motion[0], 0,
          sizeof(portal_cell_motion[0]) * MAX_CELL_MOTION);

   for (int i = 0; i < MAX_CELL_MOTION; ++i) {
      portal_cell_motion[i].major_axis = MEDIUM_AXIS_Z;
      portal_cell_motion[i].in_motion = TRUE;
   }
}


// end of management of PortalCellMotion structures
//////////////////////////////////////////////////


//////////////////////////////////////////////////
// management of fog

static void ReadWriteFog(PortalReadWrite func)
{
   (*func)(portal_fog_color, sizeof(portal_fog_color), 1);
   (*func)(&portal_fog_dist, sizeof(portal_fog_dist), 1);
}

static void ClearFog()
{
   portal_fog_dist = 0;
   portal_fog_color[0] = 128;
   portal_fog_color[1] = 128;
   portal_fog_color[2] = 128;
}

// end of management of fog
//////////////////////////////////////////////////


//////////////////////////////////////////////////
// Tag File Stuff

static ITagFile* tagfile = NULL;
static void movefunc(void *buf, size_t elsize, size_t nelem)
{
   ITagFile_Move(tagfile,(char*)buf,(int)(elsize*nelem));
}

enum eMinorVersions
{
   kSizeHeader = 18, 
}; 

#ifdef RGB_LIGHTING
TagFileTag WrTag = { "WRRGB" };
static TagFileTag LegacyWrTag = { "WR" };
static TagVersion AcceptLegacyWrVersion = { 0, 23 };
static TagVersion WRExtVersion = { 0, 30 };
static TagVersion AcceptWRExtVersion = { 0, 30 };
#else
TagFileTag WrTag = { "WR" };
#endif
TagVersion WrVersion = { 0, 24 };
TagVersion AcceptWrVersion = { 0, 24 }; 

static BOOL setup_tagfile(ITagFile* file, TagFileTag *tag, 
                          TagVersion *version, TagVersion* oldest,
                          TagVersion* newest = NULL)
{
   HRESULT result;
   TagVersion maxRead = newest ? *newest : *version;
   if (file->OpenMode() == kTagOpenRead)
      *version = maxRead;
   tagfile = file;

   result = file->OpenBlock(tag, version);
   if (SUCCEEDED(result) &&
       (VersionNumsCompare(version,oldest) < 0 ||
        (file->OpenMode() == kTagOpenRead &&
         VersionNumsCompare(version,&maxRead) > 0)))
   {
      file->CloseBlock(); 
      return FALSE; 
   }
      
   return SUCCEEDED(result); 

}


static void cleanup_tagfile(ITagFile* file)
{
   file->CloseBlock(); 
}

//////////////////////////////////////////////////
// Tag File Stuff

//////////////////////////////////////////////////
// water tag file thingbobs

TagFileTag WaterMotionTag = { "CELL_MOTION" };
TagVersion WaterMotionVersion = { 0, 1 };
TagVersion AcceptWaterMotionVersion = { 0, 1 };
static TagVersion ReadWaterMotionVersion = { 0, 2 };




//////////////////////////////////////////////////
// fog tag file thingbobs

static TagFileTag FogTag = { "FOG" };
static TagVersion FogVersion = { 0, 1 };
static TagVersion AcceptFogVersion = { 0, 1 };




//////////////////////////////////////////////////

// @TODO: something more graceful here
extern "C"
{
   BOOL wr_db_disabled = FALSE; 
}

static void water_db_message(DispatchData* msg)
{
   msgDatabaseData data;
   data.raw = msg->data;

   switch (DB_MSG(msg->subtype))
   {
      case kDatabaseReset:
         ClearWaterMotion();
         break; 

      case kDatabaseLoad:
      case kDatabaseSave:
         if (msg->subtype & kDBMap)
         {
            TagVersion v = WaterMotionVersion;
            if (data.save->OpenMode() == kTagOpenWrite)
            {
               if (dbFormatGetWriteProfile() == kDBWritePreserveSource)
                  dbFormatSourceTag(&WaterMotionTag,&v,NULL);
               else if (dbFormatUsesNewDark())
                  v = ReadWaterMotionVersion;
            }
            const BOOL writingRetainedRaw =
               data.save->OpenMode() == kTagOpenWrite &&
               dbCompatibilityUseRawTag(&WaterMotionTag);
            if (!writingRetainedRaw &&
                setup_tagfile(data.save, &WaterMotionTag, &v,
                              &AcceptWaterMotionVersion,
                              &ReadWaterMotionVersion) )
            {
               const BOOL extended =
                  VersionNumsCompare(&v,&ReadWaterMotionVersion) == 0;
               ReadWriteWaterMotion(movefunc,extended);
               if (extended && data.save->OpenMode() == kTagOpenRead)
                  dbCompatibilityRewriteTag(&WaterMotionTag,
                     "0.2 fully decoded; source version is writable");
               cleanup_tagfile(data.save);
            }
         }
         break; 

   }
}

static void db_message(DispatchData* msg)
{
   msgDatabaseData data;
   data.raw = msg->data;

   if (wr_db_disabled)
      return ;

   switch (DB_MSG(msg->subtype))
   {
      case kDatabaseReset:
      {
         portal_worldrep_texture_scale = 1.0f;
         gExtendedHeaderVersion = 4;
         gExtendedHeaderFlags = 0;
         gExtendedLightmapFormat = 0;
         gExtendedLightmapScale = 0;
         memset(gExtendedCellZones,0,sizeof(gExtendedCellZones));
         FreeWR();

         wrBspTreeDeallocate();

#ifdef EDITOR
         free_csg_internal_database();
#endif 

         AnimLightClear();

         portal_cleanup_water_hack(); 

         ClearFog();

         family_clear_all();
         break;
      }

      case kDatabaseSave:
      {
         if (msg->subtype & kDBMap)    // Save world info into the MAP file.
         {
            if (!gedit_editted && wr_num_cells > 0)
            {
               ITagFile* file = data.save; 
               const BOOL writeExtended = dbFormatTargetHasTag(&WRExtTag);
               if (writeExtended)
               {
                  if (!dbCompatibilityUseRawTag(&WRExtTag))
                  {
                     TagVersion v = WRExtVersion;
                     if (dbFormatGetWriteProfile() == kDBWritePreserveSource)
                        dbFormatSourceTag(&WRExtTag,&v,NULL);
                     if (setup_tagfile(file,&WRExtTag,&v,&AcceptWRExtVersion))
                     {
                        const sMissionLightmapParams* params =
                           GetMissionLightmapParams();
                        if (dbFormatUsesNewDark() &&
                            !dbFormatSourceTag(&WRExtTag,NULL,NULL))
                        {
                           // New WREXT exports use NewDark's 32-bit BGR
                           // lightmaps.  LM_PARAM.shadow_depth controls the
                           // lighting algorithm and is not the disk format.
                           gExtendedLightmapFormat = 1;
                           gExtendedLightmapScale = params->lightmap_scale;
                        }
                        int headerSize = 20;
                        ulong size = 0;
                        file->Write((char*)&headerSize,sizeof(headerSize));
                        file->Write((char*)&gExtendedHeaderVersion,
                                    sizeof(gExtendedHeaderVersion));
                        file->Write((char*)&gExtendedHeaderFlags,
                                    sizeof(gExtendedHeaderFlags));
                        file->Write((char*)&gExtendedLightmapFormat,
                                    sizeof(gExtendedLightmapFormat));
                        file->Write((char*)&gExtendedLightmapScale,
                                    sizeof(gExtendedLightmapScale));
                        file->Write((char*)&size,sizeof(size));
                        size = WriteWR(movefunc,TRUE);
                        ReadWriteWRExtLight(movefunc,FALSE);
                        file->Seek(headerSize,kTagSeekFromStart);
                        file->Write((char*)&size,sizeof(size));
                        cleanup_tagfile(data.save);
                     }
                  }
               }
               else
               {
                  TagVersion v = WrVersion;
                  if (setup_tagfile(file, &WrTag, &v,&AcceptWrVersion))
                  {
                     ulong size = 0;
                     file->Write((char*)&size,sizeof(size));
                     size = WriteWR(movefunc);
                     ReadWriteLight(movefunc, FALSE);
#ifdef EDITOR
                     save_csg_internal_database(movefunc);
#endif 
                     file->Seek(0,kTagSeekFromStart);
                     file->Write((char*)&size,sizeof(size));
                     cleanup_tagfile(data.save);
                  }
               }
            }

            TagVersion v = FogVersion;
            if (setup_tagfile(data.save, &FogTag, &v, &AcceptFogVersion))
            {
               ReadWriteFog(movefunc);
               cleanup_tagfile(data.save);
            }

            texture_Save(data.save); 
         }
         break;
      }

      case kDatabaseLoad:
      {
         if (msg->subtype & kDBMap)    // Load world info from the MAP file.
         {
            ITagFile* file = data.load;
            TagVersion v = WrVersion;
            gReadScalarWorldRep = FALSE;
            gReadExtendedWorldRep = FALSE;
            BOOL opened = setup_tagfile(file, &WrTag, &v, &AcceptWrVersion);
#ifdef RGB_LIGHTING
            if (!opened)
            {
               v = AcceptLegacyWrVersion;
               opened = setup_tagfile(file, &LegacyWrTag, &v, &AcceptLegacyWrVersion);
               gReadScalarWorldRep = opened;
            }
            if (!opened)
            {
               v = WRExtVersion;
               opened = setup_tagfile(file, &WRExtTag, &v, &AcceptWRExtVersion);
               gReadExtendedWorldRep = opened;
               if (opened)
                  dbCompatibilityRetainRawTag(&WRExtTag,
                     "decoded; source block retained until the world changes");
            }
#endif
            if (opened)
            {
               ulong size;
               if (gReadExtendedWorldRep)
               {
                  int headerSize = 0;
                  file->Read((char*)&headerSize, sizeof(headerSize));
                  file->Read((char*)&gExtendedHeaderVersion,
                             sizeof(gExtendedHeaderVersion));
                  file->Read((char*)&gExtendedHeaderFlags,
                             sizeof(gExtendedHeaderFlags));
                  file->Read((char*)&gExtendedLightmapFormat, sizeof(gExtendedLightmapFormat));
                  file->Read((char*)&gExtendedLightmapScale, sizeof(gExtendedLightmapScale));
                  portal_worldrep_texture_scale = gExtendedLightmapScale > 0
                     ? (float)gExtendedLightmapScale
                     : (gExtendedLightmapScale < 0
                        ? -1.0f / (float)gExtendedLightmapScale : 1.0f);
                  if (headerSize < 20)
                  {
                     Warning(("Invalid WREXT header size %d\n", headerSize));
                     cleanup_tagfile(data.load);
                     gReadExtendedWorldRep = FALSE;
                     ged_validate_level(FALSE);
                     break;
                  }
                  if (headerSize > 20)
                     file->Seek(headerSize - 20, kTagSeekFromHere);
               }
               if (v.minor >= kSizeHeader)
                  file->Read((char*)&size,sizeof(size)); 
               else 
                  size = file->BlockSize(&WrTag);

               AllocMemBuf(size); 

               // load the world rep
               ReadWR(movefunc);

               // load the light stuff
               if (gReadExtendedWorldRep)
                  ReadWriteWRExtLight(movefunc,TRUE);
               else
                  ReadWriteLight(movefunc, TRUE);

               // load the csg editting type stuff
#ifdef EDITOR
               if (gReadExtendedWorldRep)
                  initialize_csg_internal_database_from_world();
               else
                  load_csg_internal_database(movefunc);
#endif 

               cleanup_tagfile(data.load);
               gReadScalarWorldRep = FALSE;
               gReadExtendedWorldRep = FALSE;

               ConfigSpew("wr_load_spew",("Wr is %d bytes, %d bytes unused\n",MemBuf.next- MemBuf.buf,MemBuf.size - (MemBuf.next - MemBuf.buf))); 
            }
            else 
               ged_validate_level(FALSE); 

            v = FogVersion;
            if (setup_tagfile(data.load, &FogTag, &v, &AcceptFogVersion))
            {
               ReadWriteFog(movefunc);
               cleanup_tagfile(data.load);
            }

            texture_Load(data.load); 
         }

         break;
      }
   }
}

////////////////////////////////////////
//
// LOOP/DISPATCH callback
// Here's where we do the dirty work.
//

static eLoopMessageResult LGAPI _LoopFunc(void* data, eLoopMessage msg, tLoopMessageData hdata)
{
   // useful stuff for most clients
   eLoopMessageResult result = kLoopDispatchContinue; 
   StateRecord* state = (StateRecord*)data;
   LoopMsg info;

   info.raw = hdata; 


   switch(msg)
   {

      case kMsgDatabase:
         db_message(info.dispatch);
         water_db_message(info.dispatch); 
         break; 

      case kMsgAppInit:
         family_init();      // a reset but an init
         break; 

      case kMsgAppTerm:
         family_term();      
         break; 

      case kMsgEnd:
         DoFree(state);
         break;   
   }
   return result;
}

////////////////////////////////////////////////////////////
// 
// Loop client factory function. 
//

static ILoopClient* LGAPI _CreateClient(sLoopClientDesc * pDesc, tLoopClientData data)
{
   StateRecord* state;
   // allocate space for our state, and fill out the fields
   state = (StateRecord*)DoMalloc(sizeof(StateRecord));
   state->context = (Context*)data;
   
   return CreateSimpleLoopClient(_LoopFunc,state,pDesc);
}

///////////////
// DESCRIPTOR
// 

sLoopClientDesc WrLoopClientDesc =
{
   &LOOPID_Wr,                           // GUID
   "World Rep Client",                    // NAME        
   kPriorityNormal,                    // PRIORITY          
   kMsgEnd | kMsgDatabase | kMsgsAppOuter,   // INTERESTS      

   kLCF_Callback,
   _CreateClient,
   
   NO_LC_DATA,

   {
      {kConstrainAfter, &LOOPID_Render, kMsgDatabase|kMsgAppTerm },  // for texture archetypes
      {kNullConstraint} // terminator
   }
};
