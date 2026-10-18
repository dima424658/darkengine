// $Header: x:/prj/tech/libsrc/lgd3d/RCS/render.c 1.26 1998/07/06 15:32:20 buzzard Exp $
#include <mprintf.h>
#include <stdio.h>

#include <math.h>

#include <memall.h>
#include <tmpalloc.h>
#include <r3ds.h>
#include <g2.h>
#include <lgassert.h>
#include <lgd3d.h>
#include <tmgr.h>
#include <tdrv.h>
#include <dbg.h>

#include <texture.h>
#include <render_backend.h>
extern void *g_ModernTextures[LGD3D_MAX_TEXTURES];
extern grs_bitmap *g_ModernBitmaps[LGD3D_MAX_TEXTURES];
extern void *g_ModernWhiteTexture;
extern int g_ModernTextureLevel;
extern uchar *lgd3d_clut;

#ifdef MONO_SPEW
#define put_mono(c) \
do { \
   uchar *p_mono = (uchar *)0xb0080; \
   *p_mono = c;                      \
} while (0)
#else
#define put_mono(c)
#endif

BOOL lgd3d_punt_buffer = FALSE;
static BOOL primitives_pending = FALSE;
static void flush_primitives(void);

#define Flush()               \
do {                          \
   if (primitives_pending)    \
      flush_primitives();     \
} while (0)


typedef struct sLgd3dVertex {
   float sx, sy, sz, rhw;
   uint32 color, specular;
   float tu, tv;
} sLgd3dVertex;

static uint32 PackColor(int r, int g, int b, int a)
{
   return ((uint32)(a & 0xff) << 24) | ((uint32)(r & 0xff) << 16) |
          ((uint32)(g & 0xff) << 8) | (uint32)(b & 0xff);
}

BOOL lgd3d_save_poly;
BOOL lgd3d_z_normal = TRUE;

static float fog_tabledensity;

typedef struct cliprect {
   float left, right, top, bot;
} cliprect;

cliprect lgd3d_clip;

#ifdef REALLY_EGREGIOUS_SPEW
#include <mprintf.h>
#define start(s) mprintf("tf%s",s)
#define end() mprint(".")
#else
#define start(s)
#define end()
#endif

double z2d = 1.0;
double w2d = 1.0;
static double z_near = 1.0;
static double z_far = 200.0;
static double inv_z_far = 1.0 / 200.0;
static double z1 = 200.0 / 199.0; // z_far / (z_far - z_near)
static double z2 = 200.0 / 199.0; // z_near * z_far / (z_far - z_near)
static double zbias = 0.0;
static BOOL zbuffer;
static BOOL zwrite;
static BOOL zcompare;

void lgd3d_clear_z_rect(int x0, int y0, int x1, int y1)
{
   RenderBackendClearDepth();
}

void lgd3d_set_z(float z)
{
   if (lgd3d_z_normal)
      z2d = z;
   else
      z2d = z1 - (z2 / z);

   w2d = 1.0 / z;
}

int lgd3d_is_zwrite_on(void)
{
    return zwrite;
}

int lgd3d_is_zcompare_on(void)
{
    return zcompare;
}

double lgd3d_set_zbias(double new_bias)
{
   double old_bias = zbias;
   z1 += old_bias - new_bias;
   zbias = new_bias;
   return old_bias;
}

void lgd3d_push_zbias_i(int nZBias)
{
    // TODO
}

void lgd3d_pop_zbias(void)
{
    // TODO
}


void lgd3d_set_znearfar(double znear, double zfar)
{
   z_near = znear;
   z_far = zfar;
   inv_z_far = 1.0 / z_far;
   z1 = (zfar / (zfar - znear)) - zbias;
   z2 = znear * z1;
}

static float x_offset = 0.0f;
static float y_offset = 0.0f;

#define setxy(p, __sx, __sy) \
do { \
   fix _sx = __sx + 0x8000;       \
   fix _sy = __sy + 0x8000;       \
   if (_sx > grd_gc.clip.f.right) \
      _sx = grd_gc.clip.f.right;  \
   if (_sx < grd_gc.clip.f.left)  \
      _sx = grd_gc.clip.f.left;   \
   if (_sy > grd_gc.clip.f.bot)   \
      _sy = grd_gc.clip.f.bot;    \
   if (_sy < grd_gc.clip.f.top)   \
      _sy = grd_gc.clip.f.top;    \
   (p)->sx = fix_float(_sx)+x_offset; \
   (p)->sy = fix_float(_sy)+y_offset; \
} while (0)

#define setz(_dest, z, w) \
do { \
   sLgd3dVertex *__dest = _dest; \
   if (zlinear) \
      __dest->sz = (float)z2d; \
   else if (lgd3d_z_normal) \
      __dest->sz = (float)(z * inv_z_far); \
   else { \
      __dest->sz = (float)(z1 - z2 * w); \
      if (__dest->sz > 1.0f) \
         __dest->sz = 1.0f;  \
      else if (__dest->sz < 0.0f) \
         __dest->sz = 0.0f;  }\
   __dest->rhw = (float)(w);  \
} while (0)

#define setxyz(dest, src) \
do { \
   sLgd3dVertex *_dest = dest; \
   r3s_point *_src = src;         \
   fix _sx = _src->grp.sx + 0x8000; \
   fix _sy = _src->grp.sy + 0x8000; \
   if (_sx > grd_gc.clip.f.right) \
      _sx = grd_gc.clip.f.right;  \
   if (_sx < grd_gc.clip.f.left)  \
      _sx = grd_gc.clip.f.left;   \
   if (_sy > grd_gc.clip.f.bot)   \
      _sy = grd_gc.clip.f.bot;    \
   if (_sy < grd_gc.clip.f.top)   \
      _sy = grd_gc.clip.f.top;    \
   _dest->sx = fix_float(_sx)+x_offset; \
   _dest->sy = fix_float(_sy)+y_offset; \
   setz(_dest, _src->p.z, _src->grp.w); \
} while (0)

void lgd3d_set_offsets(int x, int y)
{
   x_offset = (float)x;
   y_offset = (float)y;
}

//
// Macros for internal use
//

#define make_scolor(pColor, c0, intensity) \
do { \
   ulong __c0 = (c0);                                          \
   float __i = (intensity);                                    \
   *(pColor) = ((ulong )((__c0 & 0xff) * __i)) +               \
             (((ulong )(((__c0 >> 8) & 0xff) * __i)) << 8) +   \
             (((ulong )(((__c0 >> 16) & 0xff) * __i)) << 16) + \
             (__c0 & 0xff000000);                              \
} while (0)

static BOOL lgd3d_blend = FALSE;
static BOOL lgd3d_trans = BMF_TRANS;
static int lgd3d_alpha = 255;
static BOOL lgd3d_iterated_alpha = FALSE;

void lgd3d_set_alpha(float alpha)
{
   lgd3d_alpha = (int)(alpha * 255.0f);

   if (lgd3d_alpha > 255)
      lgd3d_alpha = 255;
   if (lgd3d_alpha < 0)
      lgd3d_alpha = 0;
}

void lgd3d_set_iterated_alpha(BOOL enabled)
{
   lgd3d_iterated_alpha = enabled;
}

static int get_vertex_alpha(const g2s_point *point)
{
   float alpha = (float)lgd3d_alpha;

   if (lgd3d_iterated_alpha)
      alpha *= point->a;
   if (alpha > 255.0)
      return 255;
   if (alpha < 0.0)
      return 0;
   return (int)alpha;
}

static BOOL use_palette = TRUE;

void lgd3d_disable_palette(void)
{
   use_palette = FALSE;
}

void lgd3d_enable_palette(void)
{
   use_palette = TRUE;
}

static uint32 get_color(void)
{
   uint32 color;
   if (use_palette) {
      int index = grd_gc.fcolor&0xff;

      switch (grd_gc.fill_type) {
      default:
         Warning(("lgd3d: unsupported fill type: %i\n", grd_gc.fill_type));
      case FILL_NORM:
         break;
      case FILL_CLUT:
         index = ((uchar *)grd_gc.fill_parm)[index];
         break;
      case FILL_SOLID:
         index = (int)grd_gc.fill_parm;
      }

      // This hack effectively simulates the lighting table 
      // hacks done in flight2
      if (lgd3d_clut != NULL)
         index = lgd3d_clut[index];

      index *= 3;
      color = PackColor(grd_pal[index], grd_pal[index+1],
                        grd_pal[index+2], lgd3d_alpha);
   } else {
      switch (grd_gc.fill_type) {
      default:
         Warning(("lgd3d: unsupported fill type: %i\n", grd_gc.fill_type));
      case FILL_NORM:
         color = grd_gc.fcolor;
         break;
      case FILL_SOLID:
         color = (uint32)grd_gc.fill_parm;
      }
      color = (color&0xffffff) + (lgd3d_alpha<<24);
   }
   return color;
}

static uint32 fog_specular = 0xff000000;

void lgd3d_set_fog_level(float fl)
{
   fog_specular = PackColor(0, 0, 0, (int)((1.0f - fl) * 255.0f));
}

static uint32 fog_color = 0x00cc0000;
static BOOL fog_enabled = FALSE;

void lgd3d_set_fog_color(int r, int g, int b)
{
   if (r > 255)
      r = 255;
   else if (r <0)
      r = 0;
   if (g > 255)
      g = 255;
   else if (g <0)
      g = 0;
   if (b > 255)
      b = 255;
   else if (b <0)
      b = 0;

   // Fog color is part of queued draw state in the active rendering backend.  Submit
   // primitives using the old color before changing it.
   Flush();
   fog_color = PackColor(r, g, b, 0);
   put_mono('a');
   RenderBackendSetFog(fog_enabled, fog_color);
   put_mono('.');
}

static void SetTransparent(int flags);

void lgd3d_set_zcompare(BOOL val)
{
   if (zcompare == val)
      return;

   put_mono('b');
   Flush();
   put_mono('.');
   zcompare = val;
   RenderBackendSetDepth(zcompare, zwrite);
}

void lgd3d_set_zwrite(BOOL val)
{
   if (zwrite == val)
      return;

   put_mono('c');
   Flush();
   zwrite=val;
   RenderBackendSetDepth(zcompare, zwrite);
   put_mono('.');
}

void lgd3d_zclear(void)
{
   RenderBackendClearDepth();
}

void lgd3d_blend_normal(void)
{
   Flush();
   RenderBackendSetBlend(lgd3d_blend || lgd3d_trans ? kRenderBackendBlendAlpha : kRenderBackendBlendOpaque);
}

void lgd3d_blend_multiply(int blend_mode)
{
   Flush();
   RenderBackendSetBlend(blend_mode == BLEND_SRC_DEST ? kRenderBackendBlendMultiply : kRenderBackendBlendAdd);
}

//
//
// Library internal functions
//
//

void lgd3d_render_init(lgd3ds_device_info *info)
{
   zbuffer = info->flags&LGD3DF_ZBUFFER;
   fog_tabledensity = 0.025f;
   zwrite = FALSE;
   zcompare = FALSE;
   RenderBackendSetDepth(zcompare, zwrite);

   // force init
   lgd3d_trans = BMF_TRANS;
   SetTransparent(0);
}

void lgd3d_set_fog_enable(BOOL enable)
{
   put_mono('e');
   Flush();
   fog_enabled = enable;
   RenderBackendSetFog(enable, fog_color);
   put_mono('.');
}


//
// Called once per frame from lgd3d_start_frame() to initialize render state.
//

int num_polys, tri_index, num_poly_verts, num_points;

void lgd3d_render_start_frame(void)
{
   num_points = num_polys = tri_index = num_poly_verts = 0;
   lgd3d_clip.left = fix_float(grd_gc.clip.f.left)+x_offset;
   lgd3d_clip.right = fix_float(grd_gc.clip.f.right)+x_offset;
   lgd3d_clip.top = fix_float(grd_gc.clip.f.top)+y_offset;
   lgd3d_clip.bot = fix_float(grd_gc.clip.f.bot)+y_offset;

   lgd3d_set_fog_enable(FALSE);
   lgd3d_set_fog_level(0.0);
}

void lgd3d_render_end_frame(void)
{
   Flush();
}

void lgd3d_set_fog_density(float density)
{
   fog_tabledensity = (float)(density * z_far);

   put_mono('f');
   Flush();
   put_mono('.');
}

int lgd3d_is_fog_on(void)
{
    return fog_enabled;
}

int lgd3d_use_linear_table_fog(int bUseIt)
{
    // TODO
    return 0;
}

void lgd3d_set_linear_fog_distance(float fDistance)
{
    // TODO
}

void lgd3d_set_texture_level(int n)
{
   g_ModernTextureLevel = (n != 0);
}

//
// Interface to texture manager
//


static void SetTransparent(int trans)
{
   // BMF_TRANS is a binary color key. FILL_BLEND textures instead carry
   // continuous alpha (cloud masks, water, overlays), so applying the
   // color-key alpha test to them discards valid values below 0.5. Do this
   // before the legacy state-cache early-out because fill type can change
   // while the bitmap flags remain identical.
   RenderBackendSetAlphaTest((trans & BMF_TRANS) != 0 &&
                           gr_get_fill_type() != FILL_BLEND);

   if (lgd3d_trans == (int )trans)
      return;

   lgd3d_trans = trans;
   // The fixed-function renderers could not combine color-keying with useful
   // bilinear filtering.  The modern backend converts the key to alpha and
   // performs the alpha test in its shader, so point-sampling cutout foliage
   // only creates severe crawling and hard silhouettes as the camera moves.
   RenderBackendSetSampler(0,
                         lgd3d_get_texture_wrapping(0),
                         TRUE);
   RenderBackendSetBlend((trans || lgd3d_blend) ? kRenderBackendBlendAlpha : kRenderBackendBlendOpaque);
}


static int next_id=TDRV_ID_INVALID;
static int modern_next_id1=TDRV_ID_INVALID;
static int modern_tex_id1=TDRV_ID_INVALID;

#define SetPoly() next_id = TDRV_ID_SOLID

void SetTextureId(int n)
{
   // The legacy manager exposes one callback bitmap globally.  Deferring a
   // callback until draw time loses the first bitmap when both multitexture
   // levels request a new upload before the draw.  Resolve it while the
   // manager's callback bitmap still identifies this texture and retain the
   // resulting concrete ID in the appropriate texture level.
   if (n == TDRV_ID_CALLBACK) {
      g_tmgr->set_texture_callback();
      return;
   }
   if (g_ModernTextureLevel != 0) {
      modern_next_id1 = n;
      return;
   }
   next_id = n;
}

// used when a texture is released to make sure state is reset

static int tex_id=TDRV_ID_INVALID;

void lgd3d_modern_reset_texture_state(void)
{
   next_id=tex_id=TDRV_ID_INVALID;
   modern_next_id1=modern_tex_id1=TDRV_ID_INVALID;
}

static void doPolySetup(int n)
{
   int trans = 0;
   Flush();
   if (n == TDRV_ID_SOLID) {
      RenderBackendBindTexture(0, g_ModernWhiteTexture);
   } else {
      AssertMsg1((n>=0)&&(n<LGD3D_MAX_TEXTURES), "Invalid texture id: %i", n);
      RenderBackendBindTexture(0, g_ModernTextures[n]);
      if (g_ModernBitmaps[n] != NULL)
         trans = g_ModernBitmaps[n]->flags & (BMF_TRANS|BMF_TLUC);
   }
   SetTransparent(trans);
}

void UnsetTextureId(int n)
{
   extern void SynchD3D(void);

   if (n==tex_id) {
      doPolySetup(tex_id=TDRV_ID_SOLID);
      SynchD3D(); // flush it, cause the old handle's about to be released!
   }
   if (n==modern_tex_id1) {
      Flush();
      modern_tex_id1=TDRV_ID_SOLID;
      modern_next_id1=TDRV_ID_SOLID;
      RenderBackendBindTexture(1,g_ModernWhiteTexture);
   }
}

BOOL lgd3d_punt_d3d = FALSE;

static void prim_setup(void)
{
   if (lgd3d_punt_d3d)
      return;

   if (next_id == TDRV_ID_CALLBACK) {
      Flush();
      g_tmgr->set_texture_callback();
   }

   if (tex_id != next_id)
      doPolySetup(tex_id = next_id);
}

static void modern_setup_second_texture(void)
{
   int n;
   if (modern_tex_id1 == modern_next_id1)
      return;
   Flush();
   n = modern_tex_id1 = modern_next_id1;
   if (n == TDRV_ID_SOLID)
      RenderBackendBindTexture(1, g_ModernWhiteTexture);
   else {
      AssertMsg1((n>=0)&&(n<LGD3D_MAX_TEXTURES), "Invalid lightmap id: %i", n);
      RenderBackendBindTexture(1, g_ModernTextures[n]);
   }
   RenderBackendSetSampler(1, lgd3d_get_texture_wrapping(1), TRUE);
}

static void ModernConvertVertex(sRenderBackendVertex *d, const sLgd3dVertex *s)
{
   d->x=s->sx; d->y=s->sy; d->z=s->sz; d->rhw=s->rhw;
   d->r=((s->color>>16)&255)/255.0f;
   d->g=((s->color>>8)&255)/255.0f;
   d->b=(s->color&255)/255.0f;
   d->a=((s->color>>24)&255)/255.0f;
   d->fog=((s->specular>>24)&255)/255.0f;
   d->u0=s->tu; d->v0=s->tv; d->u1=d->v1=0.0f;
}

static void ModernSubmitList(int primitive, int n, const sLgd3dVertex *src)
{
   int i;
   sRenderBackendVertex *dst=(sRenderBackendVertex *)temp_malloc(n*sizeof(*dst));
   for(i=0;i<n;++i) ModernConvertVertex(&dst[i],&src[i]);
   RenderBackendDraw(primitive,dst,n,FALSE);
   temp_free(dst);
}

static void ModernSubmitFan(int n, const sLgd3dVertex *src)
{
   int i,j=0,count=(n-2)*3;
   sRenderBackendVertex *dst;
   if(n<3)return;
   dst=(sRenderBackendVertex *)temp_malloc(count*sizeof(*dst));
   for(i=2;i<n;++i) {
      ModernConvertVertex(&dst[j++],&src[0]);
      ModernConvertVertex(&dst[j++],&src[i-1]);
      ModernConvertVertex(&dst[j++],&src[i]);
   }
   RenderBackendDraw(kRenderBackendTriangles,dst,count,FALSE);
   temp_free(dst);
}

static void ModernSubmitIndexed(int count,const ushort *indices,const sLgd3dVertex *src)
{
   int i;
   sRenderBackendVertex *dst=(sRenderBackendVertex *)temp_malloc(count*sizeof(*dst));
   for(i=0;i<count;++i)ModernConvertVertex(&dst[i],&src[indices[i]]);
   RenderBackendDraw(kRenderBackendTriangles,dst,count,FALSE);
   temp_free(dst);
}

static void do_points(int n, sLgd3dVertex *vlist)
{
   if (lgd3d_punt_d3d) return;
   prim_setup(); ModernSubmitList(kRenderBackendPoints,n,vlist);
}

//
// guts of all polygon drawing routines
//

static void do_trifan(int n, sLgd3dVertex *vlist)
{
   AssertMsg(next_id>TDRV_ID_INVALID, "Current Texture is invalid!");
   if (lgd3d_punt_d3d) return;
   prim_setup(); ModernSubmitFan(n,vlist);
}

#define MAX_POLY_VERTS 50
sLgd3dVertex poly_vertex_buffer[MAX_POLY_VERTS];
ushort tri_index_buffer[3*MAX_POLY_VERTS];

static void flush_polys(void)
{
   if (!num_polys)
      return;

   if (!lgd3d_punt_d3d) {
      if(num_polys==1)ModernSubmitFan(num_poly_verts,poly_vertex_buffer);
      else ModernSubmitIndexed(tri_index,tri_index_buffer,poly_vertex_buffer);
   }

   num_polys = tri_index = num_poly_verts = 0;
}

#define MAX_POINTS 50
sLgd3dVertex point_buffer[MAX_POINTS];

static void flush_points(void)
{
   if (!num_points)
      return;

   if (!lgd3d_punt_d3d) {
      ModernSubmitList(kRenderBackendPoints,num_points,point_buffer);
   }
   num_points = 0;
}

static void flush_primitives(void)
{
   flush_points();
   flush_polys();
   primitives_pending = FALSE;
}

void lgd3d_render_flush(void)
{
   Flush();
}


static sLgd3dVertex *PolyMalloc(int n)
{
   sLgd3dVertex *retval;
   int i;

   if (lgd3d_punt_buffer)
      return (sLgd3dVertex *)temp_malloc(n*sizeof(sLgd3dVertex));

   prim_setup();

   if (num_poly_verts + n > MAX_POLY_VERTS)
      flush_polys();

   AssertMsg(num_poly_verts + n <= MAX_POLY_VERTS, "PolyMalloc(): poly too large!");

   retval = &poly_vertex_buffer[num_poly_verts];

   // Do the fan index thing...
   tri_index_buffer[tri_index] = num_poly_verts;
   tri_index_buffer[tri_index+1] = num_poly_verts+1;
   tri_index_buffer[tri_index+2] = num_poly_verts+2;
   tri_index+=3;
   for (i=3; i<n; i++)
   {
      tri_index_buffer[tri_index] = num_poly_verts;
      tri_index_buffer[tri_index+1] = num_poly_verts+i-1;
      tri_index_buffer[tri_index+2] = num_poly_verts+i;
      tri_index+=3;
   }

   num_polys++;
   num_poly_verts += n;
   primitives_pending = TRUE;

   return retval;
}

static void PolyFree(int n, sLgd3dVertex *vlist)
{
   if (!lgd3d_punt_buffer)
      return;

   do_trifan(n, vlist);
   temp_free(vlist);
}

static sLgd3dVertex *PointMalloc(int n)
{
   sLgd3dVertex *retval;

   if (lgd3d_punt_buffer)
      return (sLgd3dVertex *)temp_malloc(n*sizeof(sLgd3dVertex));

   prim_setup();
   if (num_points + n > MAX_POINTS)
      flush_points();

   AssertMsg(num_points + n <= MAX_POINTS, "PointMalloc(): too many points!");

   retval = &point_buffer[num_points];

   num_points += n;
   primitives_pending = TRUE;

   return retval;
}

static void PointFree(int n, sLgd3dVertex *vlist)
{
   if (!lgd3d_punt_buffer)
      return;

   do_points(n, vlist);
   temp_free(vlist);
}

static BOOL linear = FALSE;
void lgd3d_set_linear(BOOL lin)
{
   linear = lin;
}

static BOOL zlinear = FALSE;
void lgd3d_set_zlinear(BOOL lin)
{
   zlinear = lin;
}

void lgd3d_set_blend(BOOL blend_enable)
{
   put_mono('k');
   if (lgd3d_blend != blend_enable) {
      lgd3d_blend = blend_enable;
      Flush();
      RenderBackendSetBlend((blend_enable||lgd3d_trans) ? kRenderBackendBlendAlpha : kRenderBackendBlendOpaque);
   }
   put_mono('.');
}

void lgd3d_draw_line(r3s_point *p0, r3s_point *p1)
{
   fix x10, y10;
   sLgd3dVertex *vlist;
   int i, save_id;
   uint32 c=get_color();
   fix temp, left, right, top, bot;
   float x0, x1, y0, y1;

   left = grd_gc.clip.f.left;
   right = grd_gc.clip.f.right;
   top = grd_gc.clip.f.top;
   bot = grd_gc.clip.f.bot;

   if (bot - top < FIX_UNIT)
      return;

   if (right - left < FIX_UNIT)
      return;

   save_id = next_id;

   SetPoly();

   vlist = PolyMalloc(4);

   for (i=0; i<4; i++) {
      vlist[i].color = c;
      vlist[i].specular = fog_specular;
      setz(&vlist[i], p0->p.z, p0->grp.w);
      vlist[i].tu = vlist[i].tv = 0.0;
   }

   x10 = p1->grp.sx - p0->grp.sx;
   y10 = p1->grp.sy - p0->grp.sy;
   if (x10 < 0) x10 = -x10;
   if (y10 < 0) y10 = -y10;

   if (y10 > x10) {
      left += 0x8000;
      right -= 0x8000;

      if (p0->grp.sy > p1->grp.sy) {
         r3s_point *tmp = p0;
         p0 = p1;
         p1 = tmp;
      }
   } else {
      top += 0x8000;
      bot -= 0x8000;
      if (p0->grp.sx > p1->grp.sx) {
         r3s_point *tmp = p0;
         p0 = p1;
         p1 = tmp;
      }
   }

   temp = p0->grp.sx + 0x8000;
   if (temp < left)
      temp = left;
   if (temp > right)
      temp = right;
   x0 = fix_float(temp)+x_offset;

   temp = p1->grp.sx + 0x8000;
   if (temp < left)
      temp = left;
   if (temp > right)
      temp = right;
   x1 = fix_float(temp)+x_offset;

   temp = p0->grp.sy + 0x8000;
   if (temp < top)
      temp = top;
   if (temp > bot)
      temp = bot;
   y0 = fix_float(temp)+y_offset;

   temp = p1->grp.sy + 0x8000;
   if (temp < top)
      temp = top;
   if (temp > bot)
      temp = bot;
   y1 = fix_float(temp)+y_offset;

   if (y10 > x10) {
      vlist[0].sx = x0 - 0.5f;
      vlist[1].sx = x0 + 0.5f;
      vlist[2].sx = x1 + 0.5f;
      vlist[3].sx = x1 - 0.5f;
      vlist[0].sy = y0;
      vlist[1].sy = y0;
      vlist[2].sy = y1;
      vlist[3].sy = y1;
   } else {
      vlist[0].sy = y0 + 0.5f;
      vlist[1].sy = y0 - 0.5f;
      vlist[2].sy = y1 - 0.5f;
      vlist[3].sy = y1 + 0.5f;
      vlist[0].sx = x0;
      vlist[1].sx = x0;
      vlist[2].sx = x1;
      vlist[3].sx = x1;
   }

   PolyFree(4, vlist);
   next_id = save_id;
}

static grs_bitmap *hack_light_bm = NULL;
static ushort hack_light_alpha_pal[16] =
{
   0x0fff, 0x1fff, 0x2fff, 0x3fff, 0x4fff, 0x5fff, 0x6fff, 0x7fff,
   0x8fff, 0x9fff, 0xafff, 0xbfff, 0xcfff, 0xdfff, 0xefff, 0xffff,
};

void lgd3d_render_shutdown(void)
{
   if (hack_light_bm != NULL) {
      lgd3d_unload_texture(hack_light_bm);
      gr_free(hack_light_bm);
      hack_light_bm = NULL;
   }
}


static void init_hack_light_bm(void)
{
   uchar *bits;
   int i,j;

   hack_light_bm = gr_alloc_bitmap(BMT_FLAT8, BMF_TLUC, 32, 32);
   bits = hack_light_bm->bits;
   for (i=0; i<32; i++) {
      for (j=0; j<32; j++) {
         float alpha = 8.0f * 16.0f / ((i-15.5f)*(i-15.5f) + (j-15.5f)*(j-15.5f));
         uchar val;
         if (alpha > 15.0f)
            val = 15;
         else {
            val = (uchar)alpha;
         }
         bits[j] = val;
      }
      bits += 32;
   }
}


static void do_quad_light(r3s_point *p, float r, grs_bitmap *bm)
{
   float x = fix_float(p->grp.sx)+x_offset;
   float y = fix_float(p->grp.sy)+y_offset;
   float x_right, x_left, y_top, y_bot;
   float u_right, u_left, v_top, v_bot;
   sLgd3dVertex *vl;
   uint32 c;

   if ((x+r < lgd3d_clip.left) || (x-r > lgd3d_clip.right) ||
       (y+r < lgd3d_clip.top) || (y-r > lgd3d_clip.bot))
      return;

   c = get_color()|0xff000000;

   gr_set_fill_type(FILL_BLEND);
   lgd3d_set_alpha_pal(hack_light_alpha_pal);
   lgd3d_set_texture(bm);

   vl = PolyMalloc(4);
   x_right = x+r; x_left = x-r;
   u_left = 0; u_right = 1.0f;
   if (x_left < lgd3d_clip.left) {
      u_left = (lgd3d_clip.left - x_left) / (2*r);
      x_left = lgd3d_clip.left;
   }
   if (x_right > lgd3d_clip.right) {
      u_right = u_left + (u_right - u_left)*(lgd3d_clip.right - x_left) / (x_right - x_left);
      x_right = lgd3d_clip.right;
   }
   y_bot = y+r; y_top = y-r;
   v_top = 0; v_bot = 1.0f;
   if (y_top < lgd3d_clip.top) {
      v_top = (lgd3d_clip.top - y_top) / (2*r);
      y_top = lgd3d_clip.top;
   }
   if (y_bot > lgd3d_clip.bot) {
      v_bot = v_top + (1.0f - v_top)*(lgd3d_clip.bot - y_top) / (y_bot - y_top);
      y_bot = lgd3d_clip.bot;
   }

   vl[0].tu = u_left;
   vl[0].tv = v_top;
   vl[0].sx = x_left;
   vl[0].sy = y_top;
   vl[0].color = c;
   vl[0].specular = fog_specular;
   setz(&vl[0], p->p.z, p->grp.w);
   vl[1].tu = u_right;
   vl[1].tv = v_top;
   vl[1].sx = x_right;
   vl[1].sy = y_top;
   vl[1].color = c;
   vl[1].specular = fog_specular;
   setz(&vl[1], p->p.z, p->grp.w);
   vl[2].tu = u_right;
   vl[2].tv = v_bot;
   vl[2].sx = x_right;
   vl[2].sy = y_bot;
   vl[2].color = c;
   vl[2].specular = fog_specular;
   setz(&vl[2], p->p.z, p->grp.w);
   vl[3].tu = u_left;
   vl[3].tv = v_bot;
   vl[3].sx = x_left;
   vl[3].sy = y_bot;
   vl[3].color = c;
   vl[3].specular = fog_specular;
   setz(&vl[3], p->p.z, p->grp.w);

   PolyFree(4, vl);

   gr_set_fill_type(FILL_NORM);
}

int lgd3d_draw_point(r3s_point *p)
{
   fix sx = p->grp.sx + 0x8000;
   fix sy = p->grp.sy + 0x8000;
   sLgd3dVertex *vp;
   uint32 c=get_color()|0xff000000;
   int save_id;

   if ((sx > grd_gc.clip.f.right) || (sx < grd_gc.clip.f.left) ||
       (sy > grd_gc.clip.f.bot) || (sy < grd_gc.clip.f.top))
      return CLIP_ALL;

   save_id = next_id;
   SetPoly();

   vp = PointMalloc(1);
   vp->sx = fix_float(sx) + x_offset;
   vp->sy = fix_float(sy) + y_offset;
   vp->color = c;
   vp->specular = fog_specular;
   setz(vp, p->p.z, p->grp.w);

   PointFree(1, vp);

   next_id = save_id;
   return CLIP_NONE;
}

int lgd3d_draw_point_alpha(r3s_point *p, float alpha)
{
   fix sx = p->grp.sx + 0x8000;
   fix sy = p->grp.sy + 0x8000;
   sLgd3dVertex *vp;
   uint32 c=get_color()&0xffffff;
   int save_id;

   if ((sx > grd_gc.clip.f.right) || (sx < grd_gc.clip.f.left) ||
       (sy > grd_gc.clip.f.bot) || (sy < grd_gc.clip.f.top))
      return CLIP_ALL;

   c += ((int) (255 * alpha)) << 24;

   save_id = next_id;
   SetPoly();

   vp = PointMalloc(1);
   vp->sx = fix_float(sx) + x_offset;
   vp->sy = fix_float(sy) + y_offset;
   vp->color = c;
   vp->specular = fog_specular;
   setz(vp, p->p.z, p->grp.w);

   PointFree(1, vp);

   next_id = save_id;
   return CLIP_NONE;
}


void lgd3d_hack_light(r3s_point *p, float r)
{
   if (r <= 1.0) {
      lgd3d_draw_point(p);
   } else {
      if (hack_light_bm == NULL)
         init_hack_light_bm();

      do_quad_light(p,r, hack_light_bm);
   }
}

void lgd3d_hack_light_extra(r3s_point *p, float r, grs_bitmap *bm)
{
   if (r <= 1.0) {
      float alpha = r*r;
      // scale by the overall alpha for this guy: what a hack:
   alpha *= bm->bits[bm->row*(bm->h>>1) + (bm->w>>1)] / 15.0f;
      lgd3d_draw_point(p);
   } else
      do_quad_light(p,r,bm);
}

//
// r3d-like api
//

static int lgd3d_poly(int n, r3s_point **ppl)
{
   sLgd3dVertex *vlist;
   int j;
   uint32 c = get_color();

   vlist = PolyMalloc(n);
   for (j=0; j<n; j++) {
      vlist[j].color = c;
      vlist[j].specular = fog_specular;
      setxyz(&vlist[j], ppl[j]);
   }

   PolyFree(n, vlist);

   return CLIP_NONE;
}

static int lgd3d_spoly(int n, r3s_point **ppl)
{
   sLgd3dVertex *vlist;
   int j;
   uint32 c0 = get_color();

//   if (grd_gc.fill_type == FILL_CLUT)
//      Warning(("lgd3d_spoly in FILL_CLUT mode!\n"));

   vlist = PolyMalloc(n );
   for (j=0; j<n; j++) {
      float i;

      i = ppl[j]->grp.i;
      if (i>1.0) i = 1.0;
      make_scolor(&vlist[j].color, c0, i);
      vlist[j].specular = fog_specular;
      setxyz(&vlist[j], ppl[j]);
   }

   PolyFree(n, vlist);

   return CLIP_NONE;
}

static int lgd3d_rgb_poly(int n, r3s_point **ppl)
{
   sLgd3dVertex *vlist;
   int j;
   uint32 c0 = get_color();
   int rc,gc,bc;
   rc = (c0 >> 16) & 255;
   gc = (c0 >>  8) & 255;
   bc = (c0 >>  0) & 255;

   start("l");
   vlist = PolyMalloc(n);
   for (j=0; j<n; j++) {
      int r,g,b;
      g2s_point *g2p = (g2s_point *) &ppl[j]->grp;
      r = (int)(rc*g2p->i); if (r>255) r = 255;
      g = (int)(gc*g2p->h); if (g>255) g = 255;
      b = (int)(bc*g2p->d); if (b>255) b = 255;
      vlist[j].color = PackColor(r, g, b, get_vertex_alpha(g2p));
      vlist[j].specular = fog_specular;
      setxyz(&vlist[j], ppl[j]);
   }

   PolyFree(n, vlist);

   end();
   return CLIP_NONE;
}

int lgd3d_lit_trifan(int n, r3s_point **ppl)
{
   sLgd3dVertex *vlist;
   int j;
   uint32 c0 = (lgd3d_alpha << 24) + 0xffffff;

   start("l");
   vlist = PolyMalloc(n);
   for (j=0; j<n; j++) {
      float u,v,i;
      u = ppl[j]->grp.u;
      v = ppl[j]->grp.v;
      i = ppl[j]->grp.i;
      if (i>1.0) i = 1.0;
      vlist[j].tu = u; 
      vlist[j].tv = v;
      make_scolor(&vlist[j].color, c0, i);
      vlist[j].specular = fog_specular;
      setxyz(&vlist[j], ppl[j]);
   }

   PolyFree(n, vlist);

   end();
   return CLIP_NONE;
}

static int lgd3d_rgblit_trifan(int n, r3s_point **ppl)
{
   sLgd3dVertex *vlist;
   int j;

   start("l");
   vlist = PolyMalloc(n);
   for (j=0; j<n; j++) {
      float u,v;
      int r,g,b;
      g2s_point *g2p = (g2s_point *) &ppl[j]->grp;
      u = ppl[j]->grp.u;
      v = ppl[j]->grp.v;
      vlist[j].tu = u; 
      vlist[j].tv = v;
      r = (int)(255*g2p->i); if (r>255) r = 255; else if (r<0) r = 0;
      g = (int)(255*g2p->h); if (g>255) g = 255; else if (g<0) g = 0;
      b = (int)(255*g2p->d); if (b>255) b = 255; else if (b<0) b = 0;
      vlist[j].color = PackColor(r, g, b, get_vertex_alpha(g2p));
      vlist[j].specular = fog_specular;
      setxyz(&vlist[j], ppl[j]);
   }

   PolyFree(n, vlist);

   end();
   return CLIP_NONE;
}

int lgd3d_trifan(int n, r3s_point **ppl)
{
   sLgd3dVertex *vlist;
   uint32 c0 = (lgd3d_alpha << 24) + 0xffffff;
   int i;

   start("f");
   vlist = PolyMalloc(n);

   for (i=0; i<n; i++) {
      float u,v;
      u = ppl[i]->grp.u;
      v = ppl[i]->grp.v;

      vlist[i].tu = u; 
      vlist[i].tv = v;
      setxyz(&vlist[i], ppl[i]);
      vlist[i].color = c0;
      vlist[i].specular = fog_specular;
   }

   PolyFree(n, vlist);

   end();
   return CLIP_NONE;
}

//
// actual r3d interface: setup functions + global function ptr
//

int (*lgd3d_draw_poly_func)(int n, r3s_phandle *pl);

void lgd3d_lit_tmap_setup(grs_bitmap *bm)
{
   g_tmgr->set_texture(bm);
   lgd3d_draw_poly_func = lgd3d_lit_trifan;
}

void lgd3d_tmap_setup(grs_bitmap *bm)
{
   g_tmgr->set_texture(bm);
   lgd3d_draw_poly_func = lgd3d_trifan;
}

void lgd3d_rgblit_tmap_setup(grs_bitmap *bm)
{
   g_tmgr->set_texture(bm);
   lgd3d_draw_poly_func = lgd3d_rgblit_trifan;
}

void lgd3d_poly_setup(grs_bitmap *bm)
{
   SetPoly();
   lgd3d_draw_poly_func = lgd3d_poly;
}

void lgd3d_spoly_setup(grs_bitmap *bm)
{
   SetPoly();
   lgd3d_draw_poly_func = lgd3d_spoly;
}

void lgd3d_rgb_poly_setup(grs_bitmap *bm)
{
   SetPoly();
   lgd3d_draw_poly_func = lgd3d_rgb_poly;
}


//
// g2 - like interface: take g2s_point handle lists
//

int lgd3d_g2utrifan(int n, g2s_point **ppl)
{
   sLgd3dVertex *vlist;
   uint32 c0 = (lgd3d_alpha << 24) + 0xffffff;
   int i;

   vlist = PolyMalloc(n);
   for (i=0; i<n; i++) {
      float j = ppl[i]->i;
      setxy(&vlist[i], ppl[i]->sx, ppl[i]->sy);
      vlist[i].sz = (float)z2d;
      vlist[i].rhw = (float)w2d;
      make_scolor(&vlist[i].color, c0, j);
      vlist[i].specular = fog_specular;
      vlist[i].tu = ppl[i]->u;
      vlist[i].tv = ppl[i]->v;
   }

   PolyFree(n, vlist);

   return CLIP_NONE;
}


int lgd3d_g2upoly(int n, g2s_point **ppl)
{
   sLgd3dVertex *vlist;
   int j;
   uint32 c=get_color();

   vlist = PolyMalloc(n);
   for (j=0; j<n; j++) {
      vlist[j].color = c;
      vlist[j].specular = fog_specular;
      setxy(&vlist[j], ppl[j]->sx, ppl[j]->sy);
      vlist[j].sz = (float)z2d;
      vlist[j].rhw = (float)w2d;
//      vlist[j].tu = vlist[j].tv = 0.0;
   }

   PolyFree(n, vlist);

   return CLIP_NONE;
}


int lgd3d_g2poly(int n, g2s_point **ppl)
{
   int m, code;
   g2s_point **cpl=NULL;

   m = g2_clip_poly(n, G2C_CLIP_NONE, ppl, &cpl);

   if (m<3) {
      code = CLIP_ALL;
   } else {
      lgd3d_g2upoly(m, cpl);
      code = CLIP_NONE;
   }
   if ((cpl!=NULL)&&(cpl!=ppl))
      temp_free(cpl);

   return code;
}

int lgd3d_g2trifan(int n, g2s_point **ppl)
{
   int m, code;
   g2s_point **cpl=NULL;

   m = g2_clip_poly(n, G2C_CLIP_UVI, ppl, &cpl);

   if (m<3) {
      code = CLIP_ALL;
   } else {
      lgd3d_g2utrifan(m, cpl);
      code = CLIP_NONE;
   }
   if ((cpl!=NULL)&&(cpl!=ppl))
      temp_free(cpl);

   return code;
}

void lgd3d_clear(int c)
{
   sLgd3dVertex *vlist;
   float x0, y0, x1, y1;
   int i, save_id;
   int fc_save = gr_get_fcolor();

   save_id = next_id;
   SetPoly();

   vlist = PolyMalloc(4);

   gr_set_fcolor(c);

   c = get_color();

   gr_set_fcolor(fc_save);

   x0 = fix_float(grd_gc.clip.f.left) + x_offset;
   x1 = fix_float(grd_gc.clip.f.right) + x_offset;
   y0 = fix_float(grd_gc.clip.f.top) + y_offset;
   y1 = fix_float(grd_gc.clip.f.bot) + y_offset;

   for (i=0; i<4; i++)
      {
      vlist[i].sz = (float)z2d;
      vlist[i].rhw = (float)w2d;
      vlist[i].color = c;
      vlist[i].specular = fog_specular;
      }

   vlist[0].sx = x0;
   vlist[0].sy = y0;
   vlist[1].sx = x1;
   vlist[1].sy = y0;
   vlist[2].sx = x1;
   vlist[2].sy = y1;
   vlist[3].sx = x0;
   vlist[3].sy = y1;

   PolyFree(4, vlist);

   next_id = save_id;
}

int lgd3d_TrifanMTD(int n, r3s_point **ppl, LGD3D_tex_coord **uv2)
{
   int i,j=0,count;
   sRenderBackendVertex *v;
   if(n<3)return CLIP_ALL;
   prim_setup();
   modern_setup_second_texture();
   count=(n-2)*3;
   v=(sRenderBackendVertex *)temp_malloc(count*sizeof(*v));
   for(i=2;i<n;++i) {
      int k,indices[3]={0,i-1,i};
      for(k=0;k<3;++k) {
         r3s_point *p=ppl[indices[k]];
         sLgd3dVertex old;
         old.color=(lgd3d_alpha<<24)|0xffffff;
         old.specular=fog_specular;
         old.tu=p->grp.u;old.tv=p->grp.v;
         setxyz(&old,p);
         ModernConvertVertex(&v[j],&old);
         v[j].u1=uv2[indices[k]]->u;
         v[j].v1=uv2[indices[k]]->v;
         ++j;
      }
   }
   RenderBackendDraw(kRenderBackendTriangles,v,count,TRUE);
   temp_free(v);
   return CLIP_NONE;
}

int lgd3d_LitTrifanMTD(int n,r3s_point **p,LGD3D_tex_coord **u){return lgd3d_TrifanMTD(n,p,u);}
int lgd3d_RGBlitTrifanMTD(int n,r3s_point **p,LGD3D_tex_coord **u){return lgd3d_TrifanMTD(n,p,u);}
int lgd3d_RGBAlitTrifanMTD(int n,r3s_point **p,LGD3D_tex_coord **u){return lgd3d_TrifanMTD(n,p,u);}
int lgd3d_RGBAFoglitTrifanMTD(int n,r3s_point **p,LGD3D_tex_coord **u){return lgd3d_TrifanMTD(n,p,u);}
int lgd3d_DiffuseSpecularMTD(int n,r3s_point **p,LGD3D_tex_coord **u){return lgd3d_TrifanMTD(n,p,u);}

