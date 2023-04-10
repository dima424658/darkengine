/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//  $Header: r:/t2repos/thief2/src/portal/portsurf.c,v 1.42 2000/02/19 13:18:58 toml Exp $
//
//  PORTAL
//
//  dynamic portal/cell-based renderer

#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <lg.h>
#include <r3d.h>
#include <mprintf.h>

#include <portal_.h>
#include <portclip.h>
#include <portdraw.h>

#include <pt.h>
#include <pt_clut.h>
#include <ptsurf.h>
#include <wrdbrend.h>

#include <profile.h>
#include <lgd3d.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

BOOL g_lgd3d;

#ifdef DBG_ON
  #define STATIC
#else
  #define STATIC static
#endif

#define BORDER_COLOR 255

#define STATS_ON

#ifdef DBG_ON
  #ifndef STATS_ON
  #define STATS_ON
  #endif
#endif

bool surface_cache=TRUE;   // no longer used

uchar temp_texture[256*512];

extern int stat_num_lit_pixels;
extern int stat_num_source_pixels;
bool palette_light;

#define OPAQUE_BM(b)    (!((b).flags & (BMF_TRANS | BMF_TLUC8)))


// globals for data schlepping
int _portal_surface_texture_row;
int _portal_surface_output_row;
int _portal_surface_lightmap_row;
uchar * _portal_surface_texture;
uchar * _portal_surface_output;
LightmapEntry * _portal_surface_lightmap;

#ifdef RGB_LIGHTING
extern ulong pl_red_mask, pl_green_mask, pl_blue_mask;
#endif

// This isn't being used any more, but is still here to preserve the
// decal code.
#ifdef DO_DECALS
static char tempbuf[256];

PortalDecal *compute_cached_block(uchar *dest, int row, int dx, int dy,
            int width, grs_bitmap *source, int align_x, int align_y,
            PortalLightMap *lt, int lx, int ly, int offset, PortalDecal *decal)
{
   int i,j;
   // load the light at each corner
   fix c0,c1,c2,c3, c, dc;
   uchar *l = (lt->dynamic ? lt->dynamic : lt->bits) + ly * lt->row + lx;
   uchar *s;

   uchar *bits_src;
   int bits_row;

   align_x = align_x & (source->w-1);
   align_y = align_y & (source->h-1);

   bits_src = source->bits + align_y*source->row + align_x;
   bits_row = source->row;

   // draw on the decals

   if (decal && offset == decal->offset) {
      // if this is the only decal, and it's opaque, fast case
      if (decal[1].offset != offset && OPAQUE_BM(decal->decal[mip_level])) {
         bits_src = decal->decal[mip_level].bits;
         bits_row = decal->decal[mip_level].row;
         ++decal;
      } else {
         grs_bitmap temp;
         grs_canvas dest;
         gr_init_bitmap(&temp, tempbuf, BMT_FLAT8, 0, width, width);
         gr_make_canvas(&temp, &dest);
         gr_push_canvas(&dest);
         if (!OPAQUE_BM(decal->decal[mip_level])) {
            grs_bitmap duh;
            gr_init_sub_bitmap(source, &duh, align_x, align_y, width, width);
            gr_bitmap(&duh, 0, 0);
         }

         do {
            gr_bitmap(&decal->decal[mip_level], 0, 0);
            ++decal;
         } while (offset == decal->offset);
         gr_pop_canvas();
         bits_src = tempbuf;
         bits_row = width;
      }
   }

   _portal_surface_texture = bits_src;
   _portal_surface_texture_row = bits_row;

   _portal_surface_output = dest + dy*row + dx;
   _portal_surface_output_row = row;

   _portal_surface_lightmap = l;
   _portal_surface_lightmap_row = lt->row;

   // We does what we does in the manner in which we chooses.
   switch (width)
   {
      case 0:
         goto worst_case;

      case 1:
         pt_surfbuild_1();
         stat_num_lit_pixels += 1;
         return decal;

      case 2:
         pt_surfbuild_2();
         stat_num_lit_pixels += 4;
         return decal;

      case 3:
         goto worst_case;

      case 4:
         pt_surfbuild_4();
         stat_num_lit_pixels += 16;
         return decal;

      case 5:
      case 6:
      case 7:
         goto worst_case;

      case 8:
      {
         pt_surfbuild_8();
         stat_num_lit_pixels += 64;
         return decal;
      }

      case 9:
      case 10:
      case 11:
      case 12:
      case 13:
      case 14:
      case 15:
         goto worst_case;

      case 16:
      {
         pt_surfbuild_16();
         stat_num_lit_pixels += 256;
         return decal;
      }

      default:
         break;
   }

worst_case:
   stat_num_lit_pixels += width * width;

   c0 = fix_make(l[0], 0x8000) / 16;
   c1 = fix_make(l[1], 0x8000) / 16;
   c2 = fix_make(l[lt->row], 0x8000) / 16;
   c3 = fix_make(l[lt->row+1], 0x8000) / 16;

   c2 = (c2 - c0) / width;
   c3 = (c3 - c1) / width;

   s = bits_src;
   for (j=0; j < width; ++j) {
      c = c0;
      dc = (c1 - c0) / width;
      for (i=0; i < width; ++i) {
         dest[(dy+j)*row+dx+i] = pt_light_table[256*fix_int(c) + s[i]];
         c += dc;
      }
      c0 += c2;
      c1 += c3;
      s += bits_row;
   }

   return decal;
}
#endif // DO_DECALS


bool surf_256 = TRUE;
bool cache_feedback = FALSE;

#define SURFACE_CACHE_SIZE  (1024*2048)
#define FRAME_MAX (SURFACE_CACHE_SIZE*3/4)
#define SURFACE_CACHE_ENTRIES  512

struct
{
   grs_bitmap bitmap;
   uchar *bits;
   short *ptr;     // thing that points to this entry
   ushort w, h;
   ushort mip_level;
   uchar active;
   uchar texture_id;
} surf_cache[SURFACE_CACHE_ENTRIES];

static int surf_first_free = 0;
static int surf_max = 0;
static int surf_mem = 0;
int surfs_alloced = 0;

static int get_surface(void)
{
   if (surf_first_free) {
      int n = surf_first_free;
      surf_first_free = surf_cache[n].mip_level;
      return n;
   } else {
      if (surf_max >= SURFACE_CACHE_ENTRIES-1)
         return 0;
      return ++surf_max;
   }
}

static void free_surface(int n)
{
   if (n == 0)
      Error(1, "Tried to free surface #0.\n");
   surf_cache[n].mip_level = surf_first_free;
   surf_first_free = n;
}

static void free_surface_cache(int n)
{
   surf_mem -= surf_cache[n].w * surf_cache[n].h;
   if (surf_256)
      portal_free_mem_rect(surf_cache[n].bits,
          surf_cache[n].w, surf_cache[n].h);
   else
      Free(surf_cache[n].bits);

   surf_cache[n].bits = 0;
   free_surface(n);
}

void clear_surface_cache(void)
{
   int i;
   for (i=1; i < SURFACE_CACHE_ENTRIES; ++i)
      if (surf_cache[i].bits) {
         *(surf_cache[i].ptr) = 0;
         free_surface_cache(i);
      }
}

// When we animate textures, we want to invalidate all surface cache
// entries which have a particular texture id.
void clear_surfaces_for_texture(uchar texture_id)
{
   int i;

   for (i = 1; i < SURFACE_CACHE_ENTRIES; ++i)
      if (surf_cache[i].bits && surf_cache[i].texture_id == texture_id) {
         *(surf_cache[i].ptr) = 0;
         free_surface_cache(i);
      }
}

static int cache_step=1;
void make_room_in_cache(int amt)
{
   int count = 0;
   while (amt + surf_mem >= SURFACE_CACHE_SIZE) {
      if (surf_cache[cache_step].bits) {
         if (surf_cache[cache_step].active)
            --surf_cache[cache_step].active;
         else {
            *(surf_cache[cache_step].ptr) = 0;
            free_surface_cache(cache_step);
         }
      }
      if (++cache_step == SURFACE_CACHE_ENTRIES)
         cache_step = 1;
      if (++count > SURFACE_CACHE_ENTRIES*2) {
         if (cache_feedback)
            mprintf("cache_feedback: Two passes through surface cache.\n");
         return;
      }
   }
}

// make sure there's at least one free id
void free_slot_in_cache(void)
{
   int count = 0;
   for(;;) {
      if (surf_cache[cache_step].bits) {
         if (surf_cache[cache_step].active)
            --surf_cache[cache_step].active;
         else {
            *(surf_cache[cache_step].ptr) = 0;
            free_surface_cache(cache_step);
            return;
         }
      }
      if (++cache_step == SURFACE_CACHE_ENTRIES)
         cache_step = 1;
      if (++count > SURFACE_CACHE_ENTRIES*2) {
         if (cache_feedback)
            mprintf("cache_feedback: Two passes through surface cache.\n");
         return;
      }
   }
}

int cache_mem_this_frame;
static int alloc_surface_cache(int x, int y, short *ptr, uchar texture_id)
{
   int i, n, sz, w_bytes;
   uchar *bits;

   if (cache_mem_this_frame > FRAME_MAX) return 0;

   n = get_surface();
   if (n == 0) {
      free_slot_in_cache();
      n = get_surface();
      if (n == 0) {
         if (cache_feedback)
            mprintf("cache_feedback: out of surface ids.\n");
         return 0;
      }
   }

   x += 4;
   y += 4;

   // increase y until it's 2 << n or 3 << n

   if (surf_256) {
      if (y > 8) {
         for (i=2; i < 9; ++i) {
            if ((2 << i) >= y) {
               y = 2 << i;
               break;
            } else if ((3 << i) >= y) {
               y = 3 << i;
               break;
            }
         }
      }
   }
   else
   {
      // Make surface power of 2 width
      for (i=8; i<512; i+=i) {
         if (i < y)
            continue;
         y = i;
         break;
      }
      for (i=8; i<512; i+=i) {
         if (i < x)
            continue;
         x = i;
         break;
      }
   }

   sz = x * y;
   if (sz + surf_mem > SURFACE_CACHE_SIZE) {
      make_room_in_cache(x * y);
   }

   if (grd_bpp > 8)
      w_bytes = 2*x;
   else
      w_bytes = x;

   if (surf_256)
      bits = portal_allocate_mem_rect(w_bytes,y);
   else
      bits = Malloc(w_bytes * y);

   if (!bits) {
      if (cache_feedback)
         mprintf("cache_feedback: Out of memory.\n");
      free_surface(n);
      return 0;
   }

   surf_mem += sz;
   cache_mem_this_frame += sz;

   surf_cache[n].w = x;
   surf_cache[n].h = y;
   surf_cache[n].bits = bits;
   surf_cache[n].ptr = ptr;
   surf_cache[n].active = (cache_mem_this_frame < FRAME_MAX ? 3 : 2);
   surf_cache[n].texture_id = texture_id;

   *ptr = n;

   if (surf_256)
      gr_init_bitmap(&surf_cache[n].bitmap, bits, BMT_DEVICE_VIDMEM_TO_FLAT, 0, 256, 256);
   else
      gr_init_bitmap(&surf_cache[n].bitmap, bits, BMT_DEVICE_VIDMEM_TO_FLAT, 0, x, y);

   return n;
}


// We add lightmaps and clamp the result in a temporary buffer.
// A lightmap, currently, can be up to 17x17=289, or 64x64=4096 in
// hardware.
#ifdef BIG_LIGHTMAPS
LightmapEntry temp_lightmap_bits[4096];
#else // BIG_LIGHTMAPS
LightmapEntry temp_lightmap_bits[320];
#endif // BIG_LIGHTMAPS

uint _portal_lightmap_width;
uint _portal_lightmap_height;
uint _portal_lightmap_row;

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   The temporary lightmap srarts off as a copy of the static lightmap
   for our polygon.  Then we add in the dynamic lightmaps.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
void set_up_temp_lightmap(PortalLightMap *map)
{
   _portal_lightmap_width = map->w;
   _portal_lightmap_height = map->h;
   _portal_lightmap_row = map->pixel_row;

   if (map->dynamic_light)
      memcpy(&temp_lightmap_bits[0], map->dynamic_light, map->h * map->pixel_row*sizeof(LightmapEntry));
   else
      memcpy(&temp_lightmap_bits[0], map->data, map->h * map->pixel_row*sizeof(LightmapEntry));
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   We only add lightmaps to the temporary one.  We assume that the
   lightmaps have the same row value.

   The lightmap points are 0-255, but when we weight them here we
   cheat, right-shifting the product as though the range was 0-256.

   This will be replaced with ASM & use lookup tables, one bright day.


\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#define RED_SHIFT 0
#define GREEN_SHIFT 5
#define BLUE_SHIFT 10


void add_to_temp_lightmap(LightmapEntry *source, uchar intensity)
{
   int r,g,b;
   int i, j;
   uint v;
   LightmapEntry *dest = &temp_lightmap_bits[0];

#ifndef RGB_LIGHTING
   for (i = 0; i < _portal_lightmap_height; i++) {
      for (j = 0; j < _portal_lightmap_width; j++) {
         v = ((intensity * source[j]) >> 8) + dest[j];
         if (v > 255)
            v = 255;
         dest[j] = v;
      }
      source += _portal_lightmap_row;
      dest += _portal_lightmap_row;
   }
#else
   for (i = 0; i < _portal_lightmap_height; i++) {
      for (j = 0; j < _portal_lightmap_width; j++) {

#ifdef RGB_888
         v = ((intensity * source[j].A[0]) >> 8) + dest[j].A[0];
         if (v > 255)
            v = 255;
         dest[j].A[0] = v;

         v = ((intensity * source[j].A[1]) >> 8) + dest[j].A[1];
         if (v > 255)
            v = 255;
         dest[j].A[1] = v;

         v = ((intensity * source[j].A[2]) >> 8) + dest[j].A[2];
         if (v > 255)
            v = 255;
         dest[j].A[2] = v;
#else // RGB_888
         // as an optimization for RGB lighting, we add it with no scaling
         int cur,plus,sum;
         cur = dest[j];
         plus = source[j];

         r = ((((plus & pl_red_mask)>>RED_SHIFT)*intensity)>>8)<<RED_SHIFT;
         g = ((((plus & pl_green_mask)>>GREEN_SHIFT)*intensity)>>8)<<GREEN_SHIFT;
         b = ((((plus & pl_blue_mask)>>BLUE_SHIFT)*intensity)>>8)<<BLUE_SHIFT;
         plus = r+g+b;

         sum = cur + plus;

         // test for overflow
         //    how can we tell if overflow occured?  if we carried
         //    into a given bit (the next up bit).  How can we tell
         //    if we carried?  Because bit #n =
         //                (cur-n ^ plus-n ^ carry-n)
         //    so we can infer the carries with:
         //        (cur-n ^ plus-n ^ sum-n)
         if ((cur ^ plus ^ sum) & (32 + (32<<5) + (32<<10))) {
            // overflow, so do it the slow way
            r = (cur & pl_red_mask) + (plus & pl_red_mask);
            g = (cur & pl_green_mask) + (plus & pl_green_mask);
            b = (cur & pl_blue_mask) + (plus & pl_blue_mask);
            if (r > pl_red_mask) r = pl_red_mask;
            if (g > pl_green_mask) g = pl_green_mask;
            if (b > pl_blue_mask) b = pl_blue_mask;
            sum = r + g + b;
         }

         dest[j] = sum;
#endif RGB_888
      }
      source += _portal_lightmap_row;
      dest += _portal_lightmap_row;
   }
#endif
}


// Animated lightmaps at or below this intensity are not added in.
#define LIGHTMAP_ADD_CUTOFF 0

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   This routine builds the current lightmap for a given polygon's
   surface.  It's only called if the polygon is reached by at least
   one animated light.

   The first PortalLightMap in lightmap_list is the static one, so
   we skip over it here.

   *intensities is a weighting 0-255 for all lights which reach our
   polygon's _cell_.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
void compute_lightmap(PortalLightMap *lightmap)
{
   ushort *anim_light_index = cur_anim_light_index_list;
   LightmapEntry *bits = lightmap->data;
   int size = lightmap->h * lightmap->pixel_row;
   uint light_bitmask = lightmap->anim_light_bitmask;
   uchar intensity;

   set_up_temp_lightmap(lightmap);

   while (light_bitmask) {
      if (light_bitmask & 1) {
#ifndef SHIP
         if (!cur_anim_light_index_list) {
            Warning(("Lightmap thinks it's animated but no anim lights reach cell!"));
            return;
         }
#endif // ~SHIP

         bits += size;             // advance to next lightmap data

         intensity = portal_anim_light_intensity[*anim_light_index];
         if (intensity > LIGHTMAP_ADD_CUTOFF)
            add_to_temp_lightmap(bits, intensity);
      }
      anim_light_index++;
      light_bitmask >>= 1;
   }
}


#ifdef DO_DECALS
// This probably isn't the approach we want in the final setup.
void decal_dummy_func()
{
}

// The indices for these two arrays correspond to MIP levels.
static void (*decal_build_funcs[5])() = {
   decal_dummy_func, decal_dummy_func, decal_dummy_func,
   decal_dummy_func, decal_dummy_func
};

#endif

static void (*surf8_build_funcs[5])() = {
   pt_surfbuild_16_asm, pt_surfbuild_8_asm, pt_surfbuild_4_asm,
   pt_surfbuild_2_asm, pt_surfbuild_1_asm
};

static void (*surf8_setup_funcs[5])() = {
   pt_surfbuild_setup_16_asm, pt_surfbuild_setup_8_asm,
   pt_surfbuild_setup_4_asm, pt_surfbuild_setup_2_asm,
   pt_surfbuild_setup_1_asm
};

extern void pt16_surfbuild_16_asm(void);
extern void pt16_surfbuild_8_asm(void);
extern void pt16_surfbuild_4_asm(void);
extern void pt16_surfbuild_2_asm(void);
extern void pt16_surfbuild_1_asm(void);

extern void pt16_surfbuild_setup_1_asm(void);
extern void pt16_surfbuild_setup_2_asm(void);
extern void pt16_surfbuild_setup_4_asm(void);
extern void pt16_surfbuild_setup_8_asm(void);
extern void pt16_surfbuild_setup_16_asm(void);

void pt16_surfbuild_setup(void);
void pt16_surfbuild_1(void);
void pt16_surfbuild_2(void);
void pt16_surfbuild_4(void);
void pt16_surfbuild_8(void);
void pt16_surfbuild_16(void);

static void (*surf16_build_funcs[5])() = {
#ifdef RGB_LIGHTING
   pt16_surfbuild_16, pt16_surfbuild_8, pt16_surfbuild_4,
   pt16_surfbuild_2, pt16_surfbuild_1
#else
   pt16_surfbuild_16_asm, pt16_surfbuild_8_asm, pt16_surfbuild_4_asm,
   pt16_surfbuild_2_asm, pt16_surfbuild_1_asm
#endif
};

static void (*surf16_setup_funcs[5])() = {
#ifdef RGB_LIGHTING
   pt16_surfbuild_setup,
   pt16_surfbuild_setup,
   pt16_surfbuild_setup,
   pt16_surfbuild_setup,
   pt16_surfbuild_setup
#else
   pt16_surfbuild_setup_16_asm, pt16_surfbuild_setup_8_asm,
   pt16_surfbuild_setup_4_asm, pt16_surfbuild_setup_2_asm,
   pt16_surfbuild_setup_1_asm
#endif
};

void uncache_surface(PortalPolygonRenderInfo *render)
{
   if (render->cached_surface && (!g_lgd3d))
   {
      free_surface_cache(render->cached_surface);
      render->cached_surface = 0;
   }
}

#ifdef RGB_LIGHTING
#ifndef RGB_888
ulong pl_red_convert[32], pl_blue_convert[32], pl_green_convert[32];
#define RED_SHIFT 0
#define GREEN_SHIFT 5
#define BLUE_SHIFT 10

ushort portal_color_convert(ushort color)
{
   int r,g,b;
   r = (color & pl_red_mask) >> RED_SHIFT;
   g = (color & pl_green_mask) >> GREEN_SHIFT;
   b = (color & pl_blue_mask) >> BLUE_SHIFT;
   return pl_red_convert[r] + pl_blue_convert[b] + pl_green_convert[g];
}
#endif // ~RGB_888
#endif

// This is called only from porthw.c, and is only called
// to produce the final output format--we never add dynamic
// light on top of this or anything
void portsurf_update_bits(uchar *dst, int row, PortalLightMap *lt)
{
   int y,x;
   LightmapEntry *src;

   if (lt->anim_light_bitmask) {
      compute_lightmap(lt);
      src = &temp_lightmap_bits[0];
   } else
      src = (lt->dynamic_light) ? lt->dynamic_light : lt->data;

   for (y=0; y < lt->h; y++) {
      for (x=0; x < lt->w; ++x) {
#ifdef RGB_LIGHTING
#ifdef RGB_888
//         ((LightmapEntry *) dst)[x].I = src[x].I;
         ((LightmapEntry *) dst)[x].A[0] = src[x].A[2];
         ((LightmapEntry *) dst)[x].A[1] = src[x].A[1];
         ((LightmapEntry *) dst)[x].A[2] = src[x].A[0];
#else // RGB_888
         LightmapEntry color = portal_color_convert(src[x]);
         ((LightmapEntry *) dst)[x] = color;
#endif // RGB_888
#else
         dst[x] = src[x];
#endif
      }
      src += lt->pixel_row;
      dst += row;
   }
}

// get a cached surface.
grs_bitmap *get_cached_surface(PortalPolygonRenderInfo *render,
                               PortalLightMap *lt, grs_bitmap *texture,
                               int mip_level)
{
   grs_bitmap *new_tex;
   int pixels_per_lm, w, h, row, num_pixels;
   int x, y, n, xtop, ytop;
   bool cache;
   uchar *data, *bits;
   uchar log_bpp;
#ifdef DO_DECALS
   void (*decal_build_func)();
#endif
   void (*surface_build_func)();

#ifndef SHIP
#ifdef BIG_LIGHTMAPS
   if (lt->w > 64 || lt->h > 64) {
      Warning(("LightMap size out of range!  %d x %d.\n", lt->w, lt->h));
      return 0;
   }
#else
   if (lt->w > 17 || lt->h > 17) {
      Warning(("LightMap size out of range!  %d x %d.\n", lt->w, lt->h));
      return 0;
   }
#endif
#endif // ~SHIP

   pixels_per_lm = 64 >> (mip_level + 2);
   w = (lt->w-1) * pixels_per_lm;
   h = (lt->h-1) * pixels_per_lm;

   num_pixels = w * h;

   if (render->cached_surface) {

      n = render->cached_surface;

      if (!lt->dynamic_light && surf_cache[n].mip_level == pixels_per_lm) {

         stat_num_source_pixels += num_pixels;
         surf_cache[n].active = 1;

         return &surf_cache[n].bitmap;
      }

      if (cache_feedback)
         mprintf("cache_feedback: Throw out mip level %d for %d.\n",
                 surf_cache[n].mip_level, pixels_per_lm);
      free_surface_cache(n);
      render->cached_surface = 0;
   }

     // too fine to surface cache
   if (pixels_per_lm < 1) {
      Warning(("Warning: not lighting--mipmap too fine.\n"));
      return texture;
   }

     // too big to surface cache!
   if (w > 256-16 || h > 256-16) {
#ifndef BIG_LIGHTMAPS
      Warning(("Surface too big: %d %d (%d %d)\n", w, h, lt->w, lt->h));
#endif
      return texture;
   }

#ifdef STATS_ON
   stat_num_source_pixels += num_pixels;
#endif

   cache = surface_cache;
   if (lt->dynamic_light)
      cache = FALSE;

   if (cache &&
         (n = alloc_surface_cache(w, h, &render->cached_surface,
                                  render->texture_id)) != 0) {
      new_tex = &surf_cache[n].bitmap;
      surf_cache[n].mip_level = pixels_per_lm;
   } else {
      static grs_bitmap temp_tex;
      if (cache_feedback)
         mprintf("cache_feedback: failed to cache surface.\n");

      new_tex = &temp_tex;
      gr_init_bitmap(new_tex, temp_texture, BMT_DEVICE_VIDMEM_TO_FLAT, 0, 256, 256);
   }

   row = new_tex->row;
   bits = new_tex->bits;
   log_bpp = (grd_bpp > 8) ? 1:0;
   data = bits + (2<<log_bpp) + row * 2;

   {  // surface cache for real!
#ifdef DO_DECALS
      decal_build_func = decal_build_funcs[mip_level];
#endif
      surface_build_func =
         (log_bpp ? surf16_build_funcs : surf8_build_funcs)[mip_level];

      // set surface building up for current lighting table and
      // texture size
      {
         uchar *texture_pos = texture->bits;
         uchar *output_pos = data;
         LightmapEntry *lightmap_pos =
                 (lt->dynamic_light)? lt->dynamic_light : lt->data;
         int bytes_per_lm = (log_bpp) ? 2*pixels_per_lm : pixels_per_lm;
         int lightmap_u_limit = lt->w - 1;
         int lightmap_v_limit = lt->h - 1;
         int lightmap_dy = lt->pixel_row;

         int output_dy = row * pixels_per_lm;

         int texture_x_mask = texture->w - 1;
         int texture_y_mask = texture->row * texture->h - 1;
         int texture_dy = texture->row * pixels_per_lm;
         int texture_x_offset
            = (lt->base_u * pixels_per_lm) & texture_x_mask;
         int texture_x_offset_base = texture_x_offset;
         int texture_y_offset
            = (lt->base_v * pixels_per_lm * texture->row) & texture_y_mask;

         // add in any animated light
         if (lt->anim_light_bitmask) {
            compute_lightmap(lt);
            lightmap_pos = &temp_lightmap_bits[0];
         }

         // The row sizes are constant over the surface.
         _portal_surface_texture_row = texture->row;
         _portal_surface_output_row = row;
         _portal_surface_lightmap_row = lt->pixel_row;

         // Some of our surface building uses self-modifying code.
         if (log_bpp)
         {
            AssertMsg (grd_ltab816_list!=NULL,
               "get_cached_surface(): Need 16 bit lighting table list to build 16 bit surface!");
            grd_ltab816 = grd_ltab816_list[texture->align];
            AssertMsg (grd_ltab816!=NULL,
               "get_cached_surface(): NULL 16 bit lighting table!");
            surf16_setup_funcs[mip_level]();
         }
         else
            surf8_setup_funcs[mip_level]();

         for (y = 0; y < lightmap_v_limit; ++y) {
            _portal_surface_output = output_pos;
            _portal_surface_lightmap = lightmap_pos;

            for (x = 0; x < lightmap_u_limit; ++x) {
               _portal_surface_texture
                  = texture_pos + texture_x_offset + texture_y_offset;

               // do the work
#ifdef DO_DECALS
               decal_build_func();
#endif
               surface_build_func();

               // advance our inner-loop pointers
               texture_x_offset
                  = (texture_x_offset + pixels_per_lm) & texture_x_mask;
               _portal_surface_output += bytes_per_lm;
               _portal_surface_lightmap++;
            }

            texture_x_offset = texture_x_offset_base;
            texture_y_offset
               = (texture_y_offset + texture_dy) & texture_y_mask;
            output_pos += output_dy;
            lightmap_pos += lightmap_dy;
         }
      }
#ifdef STATS_ON
      stat_num_lit_pixels += num_pixels;
#endif
   }

   ytop = 0;
   xtop = 0;

   // duplicate the data at the edges
#if 1
   memset(bits + row*ytop, BORDER_COLOR, (w+4)<<log_bpp);
   memcpy(bits + row*(ytop+1), bits + row*(ytop+2), w<<log_bpp);
   memcpy(bits + row * (h+2), bits + row * (h+1), w<<log_bpp);
   memset(bits + row*(h+3), BORDER_COLOR, (w+4)<<log_bpp);
#else
   memcpy(bits + row*(ytop+0), bits + row*(ytop+1), w);
   memcpy(bits + row * (h+1), bits + row * h, w);
#endif

   if (log_bpp) {
      ushort *b = (ushort *)bits;
      int r = row>>1;
      for (y=0; y <= h+3; ++y) {
         b[y*r+1]    = b[y*r+2];
         b[y*r+w+2]  = b[y*r+w+1];
         b[y*r] = b[y*r+w+3] = BORDER_COLOR;
      }
   } else
      for (y=0; y <= h+3; ++y) {
         bits[y*row+xtop+1] = bits[y*row+xtop+2];
         bits[y*row+w+2] = bits[y*row+w+1];
         bits[y*row+xtop] = bits[y*row+w+3] = BORDER_COLOR;
      }

   return new_tex;
}

static void pt16_surfbuild(int slog)
{
#ifdef SOFTWARE_RGB
   int srow, lrow, drow;
   uchar *src;
   LightmapEntry *light;
   ushort *dest;
   //int cl, cr, dcl, dcr, c, dc;
   int size, i,j;

   dest  = (ushort *)_portal_surface_output;
   src   = _portal_surface_texture;
   light = _portal_surface_lightmap;
   srow  = _portal_surface_texture_row;
   drow  = _portal_surface_output_row>>1;
   lrow  = _portal_surface_lightmap_row;

   // light map pixels start out 4.4; we want c to be 4.8, so we shift by 4
   // and shift deltas by the appropriate amount to compensate for size.

   size = 1<<slog;

#if 0
   cl = light[0];
   cr = light[1];
   dcl = (light[lrow] - cl)<<(4-slog);
   dcr = (light[lrow+1] - cr)<<(4-slog);
   cl <<= 4;
   cr <<= 4;

   for (j=0;j<size;j++)
   {
      c = cl;
      dc = (cr-cl)>>slog;
      for (i=0;i<size;i++) {
         dest[i] = grd_ltab816[(c&0xff00) + src[i]];
         c += dc;
      }
      cl += dcl;
      cr += dcr;
      dest += drow;
      src += srow;
   }
#else // 0
   // just use the tl color
   {
      extern uchar light_ipal[];
#ifdef RGB_888
      ushort n555 = (light[0].A[0] >> 3)
                  + ((light[0].A[1] >> 3) << 5)
                  + ((light[0].A[2] >> 3) << 10);
      uchar color = light_ipal[n555];
#else // RGB_888
      uchar color = light_ipal[light[0]];
#endif // RGB_888
      ushort *clut = &grd_ltab816[(color+24) << 8];
      for (j=0; j < size; ++j) {
         for (i=0; i < size; ++i) {
            dest[i] = clut[src[i]];
         }
         dest += drow;
         src  += srow;
      }
   }
#endif // 0

#endif // SOFTWARE_RGB
}

void pt16_surfbuild_setup(void) {}

void pt16_surfbuild_1(void)
{
   pt16_surfbuild(0);
}

void pt16_surfbuild_2(void)
{
   pt16_surfbuild(1);
}

void pt16_surfbuild_4(void)
{
   pt16_surfbuild(2);
}

void pt16_surfbuild_8(void)
{
   pt16_surfbuild(3);
}

void pt16_surfbuild_16(void)
{
   pt16_surfbuild(4);
}
