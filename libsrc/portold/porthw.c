/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/libsrc/portold/porthw.c,v 1.9 1998/04/20 14:17:43 KEVIN Exp $
#include <string.h>

#include <dev2d.h>
#include <lgd3d.h>

#include <port.h>
#include <porthw.h>
#include <wrdb.h>

// only referenced if _not_ using hardware _and_ 
// AGGREGATE_LIGHTMAPS_IN_SOFTWARE is defined
BOOL pt_aggregate_lightmaps = FALSE;

extern void portsurf_update_bits(uchar *bits, int row, PortalLightMap *lt);

typedef struct hw_texture hw_texture;
typedef struct hw_cache_elem hw_cache_elem;

struct hw_texture {
   grs_bitmap bm;             // the actual bitmap we hand to lgd3d
   uchar *bits;               // the real bits pointer
                              // (bm.bits get spoofed by lgd3d)
   int flags;                 // size of component lightmaps, etc.
   hw_cache_elem **epl;       // list of component lightmaps
   int num_elem;              // number of component lightmaps allocated
   int max_elem;              // maximum number of component lightmaps
};

struct hw_cache_elem {
   short *ptr;                // back pointer to render->cached_surface
   grs_bitmap *bm0;           // this is a pointer to the temporary bitmap 
                              // used only as a stopgap for the first frame 
                              // a lightmap is rendered
   hw_texture *tex;           // texture in which this lightmap is aggregated
   int u0, v0;                // indexes into tex
   int frame;                 // last frame this lightmap was used
   int index;                 // we can eliminate this field when we're
                              // really sure this code is stable...
   int flags;
};

#define HWF_8           1     // 8x8 or smaller lightmaps
#define HWF_16          2     // 16x16 or smaller
#define HWF_20          3     // 20x20 or smaller
#define HWF_TYPE_MASK   3     
#define HWF_DYNAMIC     4     // Dynamic or animated lightmaps only
#define HWF_UPDATED     8     // Set if any lightmaps have been added or 
                              // updated in the past frame.

#define MAX_CACHE 3072

static hw_cache_elem hw_cache[MAX_CACHE];    // list of all allocated lightmaps
static int num_cache=1;                      // 0th element is bogus

#define MAX_TEXTURES 32                      // this should be plenty

static hw_texture *hw_tex_list[MAX_TEXTURES];
static int num_textures=0;
static int cur_frame = 0;

// what is says...
void porthw_start_frame(int frame)
{
   cur_frame = frame;
}

// reload any textures that have been modified this frame
void porthw_end_frame(void)
{
   int i;
   for (i=0; i<num_textures; i++) {
      hw_texture *tex = hw_tex_list[i];
      if (!(tex->flags & HWF_UPDATED))
         continue;
      gr_set_fill_type(FILL_BLEND);
      lgd3d_set_alpha_pal(pt_alpha_pal);

      if (tex->bm.flags & BMF_LOADED)
         g_tmgr->reload_texture(&tex->bm);
      else
         g_tmgr->load_texture(&tex->bm);

      gr_set_fill_type(FILL_NORM);
      tex->flags &= ~HWF_UPDATED;
   }
}

// call after lgd3d_init()
void porthw_init(void)
{
   hw_cache[0].bm0 = gr_alloc_bitmap(BMT_FLAT8, 0, 8, 8);
   memset(hw_cache[0].bm0->bits, 0xff, 64);

   num_cache = 1;
   num_textures = 0;
   clear_surface_cache();
}

// call before lgd3d_shutdown()
void porthw_shutdown(void)
{
   int i;

   for (i=0; i<num_cache; i++)
   {
      if (hw_cache[i].bm0!=NULL) {
         lgd3d_unload_texture(hw_cache[i].bm0);
         gr_free(hw_cache[i].bm0);
         hw_cache[i].bm0 = NULL;
      }

      if ((i>0) && (*(hw_cache[i].ptr) == i))
         *(hw_cache[i].ptr) = 0;
   }

   for (i=0; i<num_textures; i++) {
      hw_texture *tex = hw_tex_list[i];
      lgd3d_unload_texture(&tex->bm);
      Free(tex->bits);
      Free(tex->epl);
      Free(tex);
   }

   num_cache = 1;
   num_textures = 0;
}

// allocate a new texture
static hw_texture *alloc_texture(int flags)
{
   hw_texture *texture;

   AssertMsg(num_textures < MAX_TEXTURES, "Too many textures allocated.");

   texture = (hw_texture *)Malloc(sizeof(hw_texture));

   hw_tex_list[num_textures++] = texture;
   texture->bits = (uchar *)Malloc(128*128);
   switch (flags&HWF_TYPE_MASK) {
   case HWF_8:
      texture->max_elem = 256;
      break;
   case HWF_16:
      texture->max_elem = 64;
      break;
   case HWF_20:
      texture->max_elem = 36;
      break;
   }
   gr_init_bitmap(&texture->bm, texture->bits, BMT_FLAT8, 0, 128, 128);
   texture->flags = flags;
   texture->num_elem = 0;
   texture->epl = Malloc(texture->max_elem * sizeof (*texture->epl));
   return texture;
}

// allocate (or free and reallocate) a new lightmap
static void hw_alloc_lightmap(PortalLightMap *lt, short *ptr)
{
   int i,w,h,flags;
   hw_texture *texture;
   hw_cache_elem *elem=NULL;

   w = lt->w;
   h = lt->h;

   if ((w>20)||(h>20)) {
      Warning(("Lightmap size out of range: w=%i h=%i\n",w,h));
      return;
   }

   while (w&(w-1))
      w++;
   while (h&(h-1))
      h++;

   if ((w<=8)&&(h<=8))
      flags = HWF_8;
   else if ((w<=16)&&(h<=16))
      flags = HWF_16;
   else if ((w<=32)&&(h<=32))
      flags = HWF_20;

   for (i=0; i<num_textures; i++)
   {
      texture = hw_tex_list[i];
      if ((texture->flags&~HWF_UPDATED) != flags)
         continue;

      if (texture->num_elem < texture->max_elem) {
         break;
      }
      else
      {
         int count=texture->num_elem;
         hw_cache_elem **epl = texture->epl;
         do {
            elem = *(epl++);
            AssertMsg(elem->index == *(elem->ptr), "cache element out of synch");
            if (elem->frame < (cur_frame-10)) {
               if (elem->bm0!=NULL) {
                  lgd3d_unload_texture(elem->bm0);
                  gr_free(elem->bm0);
                  elem->bm0 = NULL;
               }
               *(ptr) = elem->index;
               *(elem->ptr) = 0;
               break;
            }
         } while (--count);
         if (count > 0)
            break;
		 else
            elem = NULL;
      }
   }
   if (elem==NULL) {
      if (i==num_textures)
         texture = alloc_texture(flags);
      AssertMsg(num_cache < MAX_CACHE, "Out of cache elements");
      elem = &hw_cache[num_cache];
      elem->index = num_cache;
      *ptr = num_cache++;

      elem->tex = texture;
      switch (texture->flags & HWF_TYPE_MASK) {
      case HWF_8:
         elem->u0 = (texture->num_elem&0xf)<<3;
         elem->v0 = (texture->num_elem&0xf0)>>1;
         break;
      case HWF_16:
         elem->u0 = (texture->num_elem&0x7)<<4;
         elem->v0 = (texture->num_elem&0x38)<<1;
         break;
      case HWF_20:
         elem->u0 = (texture->num_elem%6)*20;
         elem->v0 = (texture->num_elem/6)*20;
         break;
      }
      texture->epl[texture->num_elem++] = elem;
   }
   elem->ptr = ptr;
   elem->bm0 = gr_alloc_bitmap(BMT_FLAT8, 0, w, h);
   elem->flags = (lt->dynamic) ? HWF_DYNAMIC : 0;

   portsurf_update_bits(elem->bm0->bits,
      elem->bm0->row, lt);
   portsurf_update_bits(texture->bits + elem->u0 + elem->v0 * texture->bm.row,
      texture->bm.row, lt);
   texture->flags |= HWF_UPDATED;
}


// invalidate a cached lightmap.  
// Called from uncache_surface() in portsurf.c.
// Basically only used to animate a lightmap
void porthw_invalidate_cached_lightmap(int n)
{
   hw_cache[n].frame = -1;
}


// Main entry point.  Allocates lightmaps and textures as 
// appropriate.  Sets hw->lm, hw->lm_u0, and hw->lm_v0.
void porthw_get_cached_lightmap(hw_render_info *hw,
      PortalPolygonRenderInfo *render,
      PortalLightMap *lt)
{
   hw_cache_elem *elem;

   if (render->cached_surface) {
      elem = &hw_cache[render->cached_surface];
      if ((elem->bm0!=NULL)&&(elem->frame < cur_frame)) {
         lgd3d_unload_texture(elem->bm0);
         gr_free(elem->bm0);
         elem->bm0 = NULL;
      }
      hw->lm = &(elem->tex->bm);
      hw->lm_u0 = elem->u0 + 0.5;
      hw->lm_v0 = elem->v0 + 0.5;
      if ((lt->dynamic)||(elem->frame == -1)||(elem->flags & HWF_DYNAMIC))
      {
         hw_texture *tex = elem->tex;
         tex->flags |= HWF_UPDATED;
         if (lt->dynamic)
            elem->flags |= HWF_DYNAMIC;
         else
            elem->flags &= ~HWF_DYNAMIC;

         portsurf_update_bits(tex->bits + elem->u0 + elem->v0 * tex->bm.row,
            tex->bm.row, lt);
      }
   } else {
      hw_alloc_lightmap(lt, &render->cached_surface);
      elem = &hw_cache[render->cached_surface];
      hw->lm = elem->bm0;
      hw->lm_u0 = hw->lm_v0 = 0.5;
   }
   elem->frame = cur_frame;
   AssertMsg(*(elem->ptr) == elem->index, "cache elem out of synch");
}
