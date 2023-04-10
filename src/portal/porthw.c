/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/portal/porthw.c,v 1.24 2000/02/19 13:18:39 toml Exp $
#include <string.h>

#include <dev2d.h>
#include <lgd3d.h>

#include <palmgr.h>

#include <port.h>
#include <porthw.h>
#include <wrdb.h>
#include <wrdbrend.h>

#include <mprintf.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

EXTERN BOOL g_lgd3d;

// only referenced if _not_ using hardware _and_ 
// AGGREGATE_LIGHTMAPS_IN_SOFTWARE is defined
BOOL pt_aggregate_lightmaps = FALSE;

extern void portsurf_update_bits(uchar *bits, int row, PortalLightMap *lt);

typedef struct porthw_texture porthw_texture;
typedef struct porthw_strip porthw_strip;
typedef struct porthw_elem porthw_elem;

struct porthw_elem {
   short *ptr;
   int frame;
   int next_elem;
   char u0, v0;         // offset within texture
   uchar flags;         // are we dynamic?
   uchar texture_index;
};

struct porthw_strip {
   int elem_index;
   int next_strip;
   char w, h, v0, pad;
};

struct porthw_texture {
   grs_bitmap *bm;
   uchar *bits;
   int strip_index;  // index of first strip in this texture
   int frame;
   int h;            // unallocated height remaining in this texture
   int next_texture; // next in texture chain (ordered by increasing age)
   int flags;
};

#ifdef RGB_LIGHTING

#ifdef RGB_888
#define LM_BITMAP_TYPE BMT_FLAT32
#define LM_FLAGS BMF_RGB_8888
#else // RGB_888
#define LM_BITMAP_TYPE BMT_FLAT16
#define LM_FLAGS BMF_RGB_555
#endif // RGB_888

#else
#define LM_BITMAP_TYPE BMT_FLAT8
#define LM_FLAGS 0
#endif

#ifdef BIG_LIGHTMAPS
#define LM_MAX_SIZE 64
#else
#define LM_MAX_SIZE 20
#endif

#define TEX_SIZE 64

#define HWF_DYNAMIC     8     // Dynamic or animated lightmaps only
#define HWF_UPDATED    16     // Set if any lightmaps have been added or 
                              // updated in the past frame.

#define START_MAX_ELEMS 2048
#define START_MAX_TEXTURES 32			// so we have 2*64*64*32 = 256k of texture memory for lightmaps
#define START_MAX_STRIPS 384

static porthw_texture *texture_list;
static porthw_strip *strip_list;
static porthw_elem *elem_list;     // list of all allocated lightmaps

static int max_strips;
static int max_textures;
static int max_elems;

static int num_strips;
static int num_textures;
static int num_elems;

static int base_texture;
static int free_strip_index;
static int free_elem_index;

static int cur_frame = 0;

#ifndef RGB_LIGHTING
static int pt_lightmap_pal_index = -1;

static void pt_init_lightmap_pal(void)
{
   uchar *pal = Malloc(768);
   int i;

   for (i=0; i<256; i++)
      pal[3*i] = pal[3*i+1] = pal[3*i+2] = i;

   pt_lightmap_pal_index = palmgr_alloc_pal(pal);
   Free(pal);
}
#endif

static void free_texture(int texture_index)
{
    int i;
    porthw_texture *texture;
    porthw_strip *strip;
    int *p_tex_index = &base_texture;
    
    // first remove texture from texture chain...
    while ( *p_tex_index != -1 )
    {
        i = *p_tex_index;
        texture = &texture_list[i];
        if (i == texture_index) {
            *p_tex_index = texture->next_texture;   
            break;
        }
        p_tex_index = (int*)&texture->next_texture;
    }

   AssertMsg1(i!=-1, "free_texture(): couldn't find texture %i in texture chain!\n", texture_index);
   texture = &texture_list[texture_index];

   // now free strips and elements
   for (i = texture->strip_index;
        i != -1;
        i = strip->next_strip)
   {
      int j;
      porthw_elem *elem;

      strip = &strip_list[i];
      for (j = strip->elem_index;
           j != -1;
           j = elem->next_elem)
      {
         elem = &elem_list[j];
         AssertMsg(elem->ptr != NULL, "free_texture(): element already freed!\n");
         *(elem->ptr) = 0;
         elem->ptr = NULL;
      }
      elem->next_elem = free_elem_index;
      free_elem_index = strip->elem_index;
   }
   strip->next_strip = free_strip_index;
   free_strip_index = texture->strip_index;
}

static porthw_texture *init_texture(int texture_index)
{
   porthw_texture *texture = &texture_list[texture_index];
   texture->bm = gr_alloc_bitmap(LM_BITMAP_TYPE, LM_FLAGS, TEX_SIZE, TEX_SIZE);
   texture->bits = texture->bm->bits;
#ifndef RGB_LIGHTING
   if (pt_lightmap_pal_index < 0)
      pt_init_lightmap_pal();
   texture->bm->align = pt_lightmap_pal_index;
#endif
   return texture;
}

static int get_free_texture(void)
{
   int texture_index;
   int min_frame = cur_frame;
   int retval = -1;
   porthw_texture *texture;

   for (texture_index = base_texture;
      texture_index != -1;
      texture_index = texture->next_texture)
   {
      texture = &texture_list[texture_index];
      if (texture->frame <= min_frame) {
         min_frame = texture->frame;
         retval = texture_index;
      }
   }
   AssertMsg(retval != -1, "get_free_texture(): no textures to free!\n");

   if (min_frame < cur_frame)
      free_texture(retval);
   else
   {
      max_textures += max_textures>>2;
      Warning(("get_free_texture(): growing texture_list to %i entries...\n", max_textures));
#ifndef SHIP
      if (max_textures > 256)
         Warning(("porthw: get_free_texture(): Scene has more textures than we can use!\n"));
#endif
      texture_list = (porthw_texture *)Realloc(texture_list, max_textures * sizeof(porthw_texture));
      retval = num_textures++;
      texture = init_texture(retval);
   } 

   return retval;
}



// allocate a new texture
static porthw_texture *alloc_texture(void)
{
   porthw_texture *texture;
   int texture_index;

   if (num_textures < max_textures) {
      texture_index = num_textures++;
      texture = init_texture(texture_index);
   } else {
      texture_index = get_free_texture();
      texture = &texture_list[texture_index];
   }

   texture->flags = 0;
   texture->next_texture = base_texture;
   texture->h = TEX_SIZE;
   texture->strip_index = -1;

   base_texture = texture_index;
   return texture;
}

// allocate a new strip from the specified texture
static porthw_strip *alloc_strip(porthw_texture *texture, int h)
{
   porthw_strip *strip;
   int strip_index;

   if (free_strip_index != -1) {
      strip_index = free_strip_index;
      free_strip_index = strip_list[strip_index].next_strip;
   } else {
      if (num_strips >= max_strips) {
         max_strips += max_strips>>2;
         Warning(("porthw.c  alloc_strip(): growing strip_list to %i entries...\n", max_strips));
         strip_list = (porthw_strip *)Realloc(strip_list, max_strips * sizeof(porthw_strip));
      }
      strip_index = num_strips++;
   }

   strip = &strip_list[strip_index];
   strip->next_strip = texture->strip_index;
   strip->h = h;
   strip->w = texture->bm->w;
   strip->elem_index = -1;
   strip->v0 = texture->bm->h - texture->h;
   texture->strip_index = strip_index;
   texture->h -= h;

   return strip;
}

static porthw_elem *alloc_elem(porthw_texture *texture, int texture_index, porthw_strip *strip, int w, short *ptr)
{
   int elem_index;
   porthw_elem *elem;

   if (free_elem_index != -1) {
      elem_index = free_elem_index;
      free_elem_index = elem_list[elem_index].next_elem;
   } else {
      if (num_elems >= max_elems) {
         max_elems += max_elems>>2;
         Warning(("porthw.c  alloc_elem(): growing elem_list to %i entries...\n", max_elems));
         elem_list = (porthw_elem *)Realloc(elem_list, max_elems * sizeof(porthw_elem));
      }
      elem_index = num_elems++;
   }

   elem = &elem_list[elem_index];
   elem->next_elem = strip->elem_index;
   elem->u0 = texture->bm->w - strip->w;
   elem->v0 = strip->v0;
   elem->texture_index = texture_index;
   elem->ptr = ptr;
   strip->elem_index = *ptr = elem_index;
   strip->w -= w;
   return elem;
}


static void porthw_alloc_lightmap(PortalLightMap *lt, short *ptr)
{
   int texture_index, w, h;
   porthw_texture *texture;
   porthw_strip *strip;
   porthw_elem *elem;
   uchar *bits;

   w = lt->w;
   h = lt->h;
   if ((w > LM_MAX_SIZE) || (h > LM_MAX_SIZE)) {
      Warning(("Lightmap size out of range: w=%i h=%i\n",w,h));
      return;
   }
   if (h<32)
      h = (h+3) & (~3);
   else
      h = (h+7) & (~7);

   for (
      texture_index=base_texture;
      texture_index!=-1;
      texture_index = texture_list[texture_index].next_texture)
   {
      int strip_index;

      texture = &texture_list[texture_index];
      for (
         strip_index = texture->strip_index;
         strip_index >=0;
         strip_index = strip->next_strip)
      {
         strip = &strip_list[strip_index];
         if ((strip->h == h) && (strip->w >= w))
            // This should do it!
            break;
      }
      if (strip_index != -1)
         break;

      if (texture->h >= h)
      {  // allocate new strip
         strip = alloc_strip(texture, h);
         break;
      }
   }
   if (texture_index==-1) {
      texture = alloc_texture();
      texture_index = base_texture;
      strip = alloc_strip(texture, h);
   }
   elem = alloc_elem(texture, texture_index, strip, w, ptr);
   bits = texture->bits + elem->u0 * sizeof(LightmapEntry) + elem->v0 * texture->bm->row;
   portsurf_update_bits(bits, texture->bm->row, lt);
   texture->flags |= HWF_UPDATED;
   elem->flags = lt->dynamic_light ? HWF_DYNAMIC : 0;
}

// what is says...
void porthw_start_frame(int frame)
{
   if (elem_list==NULL)
   {
      if (g_lgd3d)
         Warning(("porthw_start_frame(): porthw not initialized!\n"));
      return;
   }

   cur_frame = frame;
   if (g_tmgr != NULL)
      g_tmgr->start_frame(frame);
}

// reload any textures that have been modified this frame
void porthw_end_frame(void)
{
   extern BOOL portal_multitexture;
   static BOOL last_multitexture=0;
   int i;
   bool drop_all = last_multitexture != portal_multitexture;

   if (last_multitexture)
      lgd3d_set_texture_level(1);

   if (drop_all)
      for (i=0; i<num_textures; i++) {
         porthw_texture *tex = &texture_list[i];
         g_tmgr->unload_texture(tex->bm);
         tex->flags |= HWF_UPDATED;
      }

   if (portal_multitexture)
      lgd3d_set_texture_level(1);
   else
      lgd3d_set_texture_level(0);

   for (i=0; i<num_textures; i++) {
      porthw_texture *tex = &texture_list[i];

      if (!(tex->flags & HWF_UPDATED))
         continue;

      if (tex->bm->flags & BMF_LOADED)
         g_tmgr->reload_texture(tex->bm);
      else
         g_tmgr->load_texture(tex->bm);

      tex->flags &= ~HWF_UPDATED;
   }
   lgd3d_set_texture_level(0);
   last_multitexture = portal_multitexture;
}

// call after lgd3d_init()
void porthw_init(void)
{
   if (!g_lgd3d)
      return;

   if (elem_list != NULL) {
      Warning(("porthw_init(): already initialized!\n"));
      return;
   }

   max_elems = START_MAX_ELEMS;
   max_strips = START_MAX_STRIPS;
   max_textures = START_MAX_TEXTURES;

   elem_list = (porthw_elem *)Malloc(max_elems * sizeof(porthw_elem));
   strip_list = (porthw_strip *)Malloc(max_strips * sizeof(porthw_strip));
   texture_list = (porthw_texture *)Malloc(max_textures * sizeof(porthw_texture));

   num_elems = 1;             // 0th element is bogus
   num_strips = 0;
   num_textures = 0;

   base_texture = -1;
   free_strip_index = -1;
   free_elem_index = -1;

   clear_surface_cache();
}

// call before lgd3d_shutdown()
void porthw_shutdown(void)
{
   int i;

   if (elem_list == NULL) {
      if (g_lgd3d)
         Warning(("porthw_shutdown(): Already shutdown!\n"));
      return;
   }

   // clear cache
   for (i=1; i<num_elems; i++)
   {
      if ((elem_list[i].ptr != NULL) && (*(elem_list[i].ptr) == i))
         *(elem_list[i].ptr) = 0;
   }

   // free textures
   for (i=0; i<num_textures; i++) {
      porthw_texture *tex = &texture_list[i];
      lgd3d_unload_texture(tex->bm);
      gr_free(tex->bm);
   }

   num_elems = 1;
   num_strips = 0;
   num_textures = 0;

   base_texture = -1;
   free_strip_index = -1;
   free_elem_index = -1;

   Free(elem_list);
   elem_list = NULL;

   Free(strip_list);
   strip_list = NULL;

   Free(texture_list);
   texture_list = NULL;
}


void porthw_uncache_lightmap(PortalPolygonRenderInfo *render)
{
   int i = render->cached_surface;

   if (i>=num_elems)
      Warning((
         "porthw_uncache_lightmap(): index %i greater than num_elems %i.\n",
         i, num_elems));
   else if (i>0)
      elem_list[i].frame = -1;
}


// preload all lightmaps to be used this frame:
void porthw_preload_lightmap(
         PortalPolygonRenderInfo *render,
         PortalLightMap *lt)
{
   porthw_elem *elem;

   if (elem_list == NULL)
      return;

   if (render->cached_surface == 0) {
      porthw_alloc_lightmap(lt, &render->cached_surface);
      elem = &elem_list[render->cached_surface];
   } else {
      elem = &elem_list[render->cached_surface];
      if ((lt->dynamic_light) ||
          (elem->frame == -1) ||
          (elem->flags & HWF_DYNAMIC))
      {
         porthw_texture *tex = &texture_list[elem->texture_index];
         tex->flags |= HWF_UPDATED;
         if (lt->dynamic_light)
            elem->flags |= HWF_DYNAMIC;
         else
            elem->flags &= ~HWF_DYNAMIC;

         portsurf_update_bits(tex->bits + elem->u0*sizeof(LightmapEntry)
                         + elem->v0 * tex->bm->row, tex->bm->row, lt);
      }
   }
   texture_list[elem->texture_index].frame = elem->frame = cur_frame;
}


bool punt_hardware_lighting=FALSE;

// Main entry point.  Allocates lightmaps and textures as 
// appropriate.  Sets hw->lm, hw->lm_u0, and hw->lm_v0.
void porthw_get_cached_lightmap(hw_render_info *hw,
      PortalPolygonRenderInfo *render,
      PortalLightMap *lt)
{
   if ((punt_hardware_lighting)||(elem_list==NULL)) {
      hw->lm = NULL;
   } else if (render->cached_surface==0) {
      Warning(("porthw_get_cached_lightmap(): Found unpreloaded lightmap!\n"));
      hw->lm = NULL;
   } else {
      porthw_elem *elem = &elem_list[render->cached_surface];
      hw->lm = texture_list[elem->texture_index].bm;
      hw->lm_u0 = elem->u0 + 0.5;
      hw->lm_v0 = elem->v0 + 0.5;
   }
}
