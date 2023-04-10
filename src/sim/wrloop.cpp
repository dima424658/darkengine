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
#include <wrdbrend.h>
#include <editbr.h>
#include <gedit.h>
#include <ged_csg.h>
#include <medmotn.h>
#include <porthlpr.h>
#include <portal.h>
#include <family.h>
#include <objpos.h>
#include <texsave.h>

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
   } else if (MemBuf.next != MemBuf.buf + MemBuf.size) {
      mprintf("WARNING: %d bytes of worldrep alloc unused.\n",
           (MemBuf.next - MemBuf.buf) + MemBuf.size);
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
   ObjDeleteAllRefs();
   reset_dynamic_lights();

   for (ci = 0; ci < wr_num_cells; ci++) {
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

ulong WriteWR(PortalReadWrite func)
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
      
      // write render_list
      (*func)(p->render_list, sizeof(PortalPolygonRenderInfo), p->num_render_polys);
      alloced_bytes_written += sizeof(PortalPolygonRenderInfo)*p->num_render_polys;

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
      PortalLightMap templist;
      for (i = 0; i < p->num_render_polys; i++)
      {
         memcpy(&templist,&(p->light_list[i]),sizeof(PortalLightMap));
         templist.data = 0;
         templist.dynamic_light = 0;
         (*func)(&templist,sizeof(PortalLightMap),1);
      }
      //(*func)(p->light_list, sizeof(PortalLightMap), p->num_render_polys);
      alloced_bytes_written += sizeof(PortalLightMap) * p->num_render_polys;
      for (i = 0; i < p->num_render_polys; i++)
      {
         int lightmap_size = (p->light_list[i].h)
                           * (p->light_list[i].pixel_row)
                           * sizeof(LightmapEntry)
                           * (lightmap_count(p, i));

         (*func)(p->light_list[i].data, 1, lightmap_size);
         alloced_bytes_written += /* sizeof(LightmapEntry) * */ lightmap_size;
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
      (*func)(p->render_list, sizeof(PortalPolygonRenderInfo), p->num_render_polys);
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
      (*func)(p->light_list, sizeof(PortalLightMap), p->num_render_polys);

      for (i = 0; i < p->num_render_polys; i++)
      {
         int lightmap_size = (p->light_list[i].h)
                           * (p->light_list[i].pixel_row)
                           * sizeof(LightmapEntry)
                           * (lightmap_count(p, i));

         p->light_list[i].data = (LightmapEntry*) wrAlloc(lightmap_size);
         (*func)(p->light_list[i].data, 1 /* sizeof(LightmapEntry) */, lightmap_size);

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

#define MAX_STATIC   768

EXTERN int num_light;
EXTERN int num_dyn;
EXTERN mls_multi_light light_data[];
EXTERN mls_multi_light light_this[];

// @TODO: Shouldn't this be using num_light for the number of elements?
void ReadWriteLight(LightReadWrite func)
{
   // object lighting
   (*func)(&num_light, sizeof(int), 1);
   (*func)(&num_dyn, sizeof(int), 1);
   (*func)(light_data, sizeof(mls_multi_light), MAX_STATIC);
   (*func)(light_this, sizeof(mls_multi_light), 32);

   // mappings from animated lights to cells
   (*func)(&g_iCurAnimLightToCell, sizeof(int), 1);
   (*func)(&g_aAnimLightToCell[0], sizeof(sAnimLightToCell),
            g_iCurAnimLightToCell);
}

// end of light save/load stuff
//////////////////////////////////////////////////


//////////////////////////////////////////////////
// management of PortalCellMotion structures


static void ReadWriteWaterMotion(PortalReadWrite func)
{
   (*func)(portal_cell_motion,
           sizeof(portal_cell_motion[0]), MAX_CELL_MOTION);
   (*func)(g_aMedMoCellMotion,
           sizeof(g_aMedMoCellMotion[0]), MAX_CELL_MOTION);
}


static void ClearWaterMotion()
{
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
   ITagFile_Move(tagfile,(char*)buf,elsize*nelem);
}

enum eMinorVersions
{
   kSizeHeader = 18, 
}; 

#ifdef RGB_LIGHTING
TagFileTag WrTag = { "WRRGB" };
#else
TagFileTag WrTag = { "WR" };
#endif
TagVersion WrVersion = { 0, 24 };
TagVersion AcceptWrVersion = { 0, 24 }; 

static BOOL setup_tagfile(ITagFile* file, TagFileTag *tag, 
                          TagVersion *version, TagVersion* accept)
{
   HRESULT result;
   TagVersion old_version = *version;
   tagfile = file;

   result = file->OpenBlock(tag, version);
   if (VersionNumsCompare(version,accept) < 0)
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
            if (setup_tagfile(data.save, &WaterMotionTag, &v, &AcceptWaterMotionVersion) )
            {
               ReadWriteWaterMotion(movefunc);
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
               TagVersion v = WrVersion;
               if (setup_tagfile(file, &WrTag, &v,&AcceptWrVersion))
               {             
                  ulong size = 0; 

                  // make room for the size 
                  file->Write((char*)&size,sizeof(size));

                  // write the world rep
                  size = WriteWR(movefunc);

                  // write the light stuff
                  ReadWriteLight(movefunc);

                  // write the csg editting stuff
#ifdef EDITOR
                  save_csg_internal_database(movefunc);
#endif 

                  // backpatch the writen size 
                  file->Seek(0,kTagSeekFromStart); 
                  file->Write((char*)&size,sizeof(size)); 

                  cleanup_tagfile(data.save);
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
            if (setup_tagfile(file, &WrTag, &v, &AcceptWrVersion))
            {
               ulong size;
               if (v.minor >= kSizeHeader)
                  file->Read((char*)&size,sizeof(size)); 
               else 
                  size = file->BlockSize(&WrTag);

               AllocMemBuf(size); 

               // load the world rep
               ReadWR(movefunc);

               // load the light stuff
               ReadWriteLight(movefunc);

               // load the csg editting type stuff
#ifdef EDITOR
               load_csg_internal_database(movefunc);
#endif 

               cleanup_tagfile(data.load);

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

#pragma off(unreferenced)
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

#pragma off(unreferenced)
static ILoopClient* LGAPI _CreateClient(sLoopClientDesc * pDesc, tLoopClientData data)
{
   StateRecord* state;
   // allocate space for our state, and fill out the fields
   state = (StateRecord*)DoMalloc(sizeof(StateRecord));
   state->context = (Context*)data;
   
   return CreateSimpleLoopClient(_LoopFunc,state,pDesc);
}
#pragma on(unreferenced)

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
