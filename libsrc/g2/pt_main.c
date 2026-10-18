/*
 * Portable portal texture-mapper setup and span loops.
 *
 * The original implementation selected self-modifying x86 routines here.
 * The modern hardware renderer normally owns 3D drawing, but these C loops
 * retain the legacy software path for tools and fallback rendering.
 */

#include <string.h>

#include <lg.h>
#include <fix.h>
#include <dev2d.h>
#include <tmapd.h>
#include "ptmap.h"
#include <pt.h>

uchar *g2pt_tmap_ptr;
uchar *g2pt_clut;
uchar *g2pt_tluc_table;
ulong g2pt_tmap_mask;
ulong g2pt_tmap_row;

int g2pt16_mask;
int g2pt_dither;
int g2pt_preload;
fix g2pt_light;
fix g2pt_dlight;
fix g2pt_toggle;

#define RECIP_TABLE_SIZE 8192
fix g2pt_reciprocal_table_24[RECIP_TABLE_SIZE + 1];
float g2pt_int_table[32];

bool g2pt_span_clip = FALSE;
bool g2pt_project_space = TRUE;

/* pt_map.c preserves the old ABI by writing fixed-point offsets here. */
double g2pt_u_offset;
double g2pt_v_offset;

static grs_bitmap *g2pt_bitmap;
static fix g2pt_du;
static fix g2pt_dv;

extern BOOL g2pt_poly_lit;
extern double g2pt_fda, g2pt_fdb, g2pt_fdc;
extern int g2pt_unlit_perspective_poly(int n, g2s_point **vp);
extern int g2pt_lit_perspective_poly(int n, g2s_point **vp);

static fix offset_fix(const double *offset)
{
   fix value;
   memcpy(&value, offset, sizeof(value));
   return value;
}

static int wrap_coord(fix value, int size)
{
   int coordinate;
   if (size <= 1)
      return 0;
   coordinate = value >> 16;
   coordinate %= size;
   if (coordinate < 0)
      coordinate += size;
   return coordinate;
}

static uchar sample8(fix u, fix v)
{
   int x = wrap_coord(u + offset_fix(&g2pt_u_offset), g2pt_bitmap->w);
   int y = wrap_coord(v + offset_fix(&g2pt_v_offset), g2pt_bitmap->h);
   return g2pt_tmap_ptr[y * g2pt_bitmap->row + x];
}

static ushort sample16(fix u, fix v)
{
   int x = wrap_coord(u + offset_fix(&g2pt_u_offset), g2pt_bitmap->w);
   int y = wrap_coord(v + offset_fix(&g2pt_v_offset), g2pt_bitmap->h);
   return *(ushort *)(g2pt_tmap_ptr + y * g2pt_bitmap->row + 2 * x);
}

static int light_level(fix light)
{
   int level = fix_int(light);
   if (level < 0)
      level = 0;
   if (level > grd_light_table_size)
      level = grd_light_table_size;
   return level;
}

static void draw_span(uchar *dest, int count, fix u, fix v,
                      fix du, fix dv, bool lit)
{
   int i;

   if (g2pt_bitmap == NULL || g2pt_tmap_ptr == NULL)
      return;

   if (grd_bm.type == BMT_FLAT16) {
      ushort *dest16 = (ushort *)dest;
      for (i = 0; i < count; ++i) {
         if (g2pt_bitmap->type == BMT_FLAT8 ||
             g2pt_bitmap->type == BMT_TLUC8) {
            uchar source = sample8(u, v);
            if (!(g2pt_bitmap->flags & BMF_TRANS) || source != 0) {
               if (lit && grd_ltab816 != NULL)
                  dest16[i] = grd_ltab816[(light_level(g2pt_light) << 8) + source];
               else
                  dest16[i] = ((ushort *)pixpal)[source];
            }
         } else {
            ushort source = sample16(u, v);
            if (!(g2pt_bitmap->flags & BMF_TRANS) || source != 0)
               dest16[i] = source;
         }
         u += du;
         v += dv;
         g2pt_light += g2pt_dlight;
      }
   } else {
      for (i = 0; i < count; ++i) {
         uchar source = sample8(u, v);
         if (g2pt_clut != NULL)
            source = g2pt_clut[source];
         if (!(g2pt_bitmap->flags & BMF_TRANS) || source != 0) {
            if (lit && grd_light_table != NULL)
               source = grd_light_table[(light_level(g2pt_light) << 8) + source];
            if (g2pt_bitmap->type == BMT_TLUC8 && g2pt_tluc_table != NULL)
               dest[i] = g2pt_tluc_table[((unsigned)source << 8) + dest[i]];
            else
               dest[i] = source;
         }
         u += du;
         v += dv;
         g2pt_light += g2pt_dlight;
      }
   }
}

void g2pt_init(void)
{
   int i;
   g2pt_u_offset = 0.0;
   g2pt_v_offset = 0.0;
   g2pt_reciprocal_table_24[0] = 0x7fffffff;
   for (i = 1; i <= RECIP_TABLE_SIZE; ++i)
      g2pt_reciprocal_table_24[i] = fix_make(256, 0) / i;
   for (i = 0; i < 32; ++i)
      g2pt_int_table[i] = (float)i;
}

int g2ptmap_setup(grs_bitmap *bm)
{
   if (bm == NULL)
      return G2PTC_FAIL;
   if (grd_bm.type != BMT_FLAT8 && grd_bm.type != BMT_FLAT16)
      return G2PTC_FAIL;

   g2pt_bitmap = bm;
   g2pt_tmap_ptr = bm->bits;
   g2pt_tmap_row = bm->row;
   g2pt_tmap_mask = (bm->h - 1) * 256 + (bm->w - 1);
   g2pt16_mask = grd_bm.type == BMT_FLAT16 ? -1 : 0;
   if (grd_bm.type == BMT_FLAT16 && bm->type == BMT_FLAT8)
      pixpal = (void *)grd_pal16_list[bm->align];
   return G2PTC_OK;
}

void g2ptmap_affine_duv(fix du, fix dv)
{
   g2pt_du = du;
   g2pt_dv = dv;
}

void g2ptmap_run(uchar *dest, int count, fix u, fix v)
{
   draw_span(dest, count, u, v, g2pt_du, g2pt_dv, FALSE);
}

void g2ptmap_lit_run(uchar *dest, int count, fix u, fix v)
{
   draw_span(dest, count, u, v, g2pt_du, g2pt_dv, TRUE);
}

void g2ptmap_perspective_run(int count, double *abc, uchar *dest)
{
   int i;
   double a = abc[0];
   double b = abc[1];
   double c = abc[2];

   for (i = 0; i < count; ++i) {
      if (c != 0.0) {
         fix u = (fix)(a / c);
         fix v = (fix)(b / c);
         draw_span(dest, 1, u, v, 0, 0, g2pt_poly_lit);
      }
      dest += grd_bm.type == BMT_FLAT16 ? 2 : 1;
      a += g2pt_fda;
      b += g2pt_fdb;
      c += g2pt_fdc;
   }
}

void g2ptmap_setup_unlit(grs_bitmap *bm)
{
   g2d_pp.u_scale = bm->w * 65536.0f;
   g2d_pp.v_scale = bm->h * 65536.0f;
   g2d_pp.poly_func = g2pt_unlit_perspective_poly;
   g2pt_poly_lit = FALSE;
   g2pt_clut = gr_get_fill_type() == FILL_CLUT
      ? (uchar *)gr_get_fill_parm() : NULL;
   g2ptmap_setup(bm);
}

void g2ptmap_setup_lit(grs_bitmap *bm)
{
   g2d_pp.u_scale = bm->w * 65536.0f;
   g2d_pp.v_scale = bm->h * 65536.0f;
   g2d_pp.poly_func = g2pt_lit_perspective_poly;
   g2pt_poly_lit = TRUE;
   g2ptmap_setup(bm);
}

void g2ptmap_setup_unlit16(grs_bitmap *bm)
{
   g2pt_clut = NULL;
   g2ptmap_setup_unlit(bm);
}

void g2ptmap_setup_lit16(grs_bitmap *bm)
{
   g2pt_clut = NULL;
   g2ptmap_setup_lit(bm);
}
