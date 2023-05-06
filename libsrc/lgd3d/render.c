// $Header: x:/prj/tech/libsrc/lgd3d/RCS/render.c 1.26 1998/07/06 15:32:20 buzzard Exp $
#include <mprintf.h>
#include <stdio.h>

#include <math.h>

#include <windows.h>
#include <memall.h>
#include <tmpalloc.h>
#include <d3d.h>
#include <d3dmacs.h>
#include <r3ds.h>
#include <g2.h>
#include <lgassert.h>
#include <lgd3d.h>
#include <tmgr.h>
#include <tdrv.h>
#include <dbg.h>

#include <setup.h>
#include <texture.h>

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


typedef struct d3dval {
   union {
      D3DVALUE drVal;
      DWORD    dwVal;
   };
} d3dval;

BOOL lgd3d_save_poly;
BOOL lgd3d_z_normal = TRUE;

static d3dval fog_tabledensity;

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

void lgd3d_clear_z_rect(int x0, int y0, int x1, int y1)
{
    // TODO
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
    return 0; // TODO
}

int lgd3d_is_zcompare_on(void)
{
    return 0; // TODO
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

static float x_offset = 0.0;
static float y_offset = 0.0;

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
   LPD3DTLVERTEX __dest = _dest; \
   if (zlinear) \
      __dest->sz = z2d; \
   else if (lgd3d_z_normal) \
      __dest->sz = z * inv_z_far; \
   else { \
      __dest->sz = z1 - z2 * w; \
      if (__dest->sz > 1.0) \
         __dest->sz = 1.0;  \
      else if (__dest->sz < 0.0) \
         __dest->sz = 0.0;  }\
   __dest->rhw = w;  \
} while (0)

#define setxyz(dest, src) \
do { \
   LPD3DTLVERTEX _dest = dest; \
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
   x_offset = x;
   y_offset = y;
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

void lgd3d_set_alpha(float alpha)
{
   lgd3d_alpha = alpha * 255;

   if (lgd3d_alpha > 255)
      lgd3d_alpha = 255;
   if (lgd3d_alpha < 0)
      lgd3d_alpha = 0;
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

static D3DCOLOR get_color(void)
{
   D3DCOLOR color;
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
         index = grd_gc.fill_parm;
      }

      // This hack effectively simulates the lighting table 
      // hacks done in flight2
      if (lgd3d_clut != NULL)
         index = lgd3d_clut[index];

      index *= 3;
      color = RGBA_MAKE(grd_pal[index],grd_pal[index+1],grd_pal[index+2],lgd3d_alpha);
   } else {
      switch (grd_gc.fill_type) {
      default:
         Warning(("lgd3d: unsupported fill type: %i\n", grd_gc.fill_type));
      case FILL_NORM:
         color = grd_gc.fcolor;
         break;
      case FILL_SOLID:
         color = grd_gc.fill_parm;
      }
      color = (color&0xffffff) + (lgd3d_alpha<<24);
   }
   return color;
}

#define d3d_device lpd3dDevice

#ifdef USE_D3D2_API

#ifndef SHIP
#define SetRenderState(name, data) \
do { \
   HRESULT hres = d3d_device->lpVtbl->SetRenderState(d3d_device, name, data); \
   AssertMsg1(!FAILED(hres), "SetRenderState failed: error %i", hres&0xffff); \
} while (0)
#define SetLightState(name, data) \
do { \
   HRESULT hres = d3d_device->lpVtbl->SetLightState(d3d_device, name, data); \
   AssertMsg1(!FAILED(hres), "SetLightState failed: error %i", hres&0xffff); \
} while (0)
#else
#define SetRenderState(name, data) \
   d3d_device->lpVtbl->SetRenderState(d3d_device, name, data)
#define SetLightState(name, data) \
   d3d_device->lpVtbl->SetLightState(d3d_device, name, data)
#endif

#else
#define SetRenderState(name,data)
#define SetLightState(name,data)
#endif

static D3DCOLOR fog_specular = (D3DCOLOR )D3DRGBA(0.0, 0.0, 0.0, 1.0);

void lgd3d_set_fog_level(float fl)
{
   fog_specular = D3DRGBA(0.0, 0.0, 0.0, 1.0 - fl);
}

static D3DCOLOR fog_color = D3DRGB(0.8, 0.0, 0.0);

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

   fog_color = RGB_MAKE(r, g, b);
   put_mono('a');
   SetRenderState(D3DRENDERSTATE_FOGCOLOR, fog_color);
   put_mono('.');
}

#ifndef USE_D3D2_API
static void *get_exebuf_data(int index, int *size)
{
   HRESULT hres;
   D3DEXECUTEBUFFERDESC d3dExeBufDesc;
   LPD3DIEXECUTEBUFFER  ExeBuf;

   ExeBuf = index ? lpd3dExecuteBuffer2:lpd3dExecuteBuffer;
   ZeroMemory(&d3dExeBufDesc, sizeof(d3dExeBufDesc));
   d3dExeBufDesc.dwSize = sizeof(d3dExeBufDesc);
   hres = ExeBuf->lpVtbl->Lock(ExeBuf, &d3dExeBufDesc);
   *size = d3dExeBufDesc.dwBufferSize;
   return d3dExeBufDesc.lpData;
}

static void execute(int index, int vertex_count, int inst_offset, int inst_length)
{
   HRESULT hres;
   D3DEXECUTEDATA       d3dExecuteData;
   LPD3DIEXECUTEBUFFER  ExeBuf;

   ExeBuf = index ? lpd3dExecuteBuffer2:lpd3dExecuteBuffer;
   hres = ExeBuf->lpVtbl->Unlock(ExeBuf);

   ZeroMemory(&d3dExecuteData, sizeof(d3dExecuteData));
   d3dExecuteData.dwSize = sizeof(d3dExecuteData);
   d3dExecuteData.dwVertexCount       = vertex_count;
   d3dExecuteData.dwInstructionOffset = inst_offset;
   d3dExecuteData.dwInstructionLength = inst_length;

   hres = ExeBuf->lpVtbl->SetExecuteData(ExeBuf, &d3dExecuteData);
   AssertMsg1(!FAILED(hres), "SetExecuteData failed: error %i", hres&0xffff);

   hres = d3d_device->lpVtbl->Execute(lpd3dDevice, ExeBuf,
      lpd3dViewport, D3DEXECUTE_UNCLIPPED);
   AssertMsg1(!FAILED(hres), "Execute failed: error %i", hres&0xffff);
}
#endif

static void SetTransparent(int flags);

static BOOL zbuffer;
static BOOL zwrite;
static BOOL zcompare;

void lgd3d_set_zcompare(BOOL val)
{
   if (zcompare == val)
      return;

   put_mono('b');
   Flush();
   SetRenderState(D3DRENDERSTATE_ZFUNC, val?D3DCMP_LESSEQUAL:D3DCMP_ALWAYS);
   put_mono('.');
   zcompare = val;
}

void lgd3d_set_zwrite(BOOL val)
{
   if (zwrite == val)
      return;

   put_mono('c');
   Flush();
   SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, zwrite=val);
   put_mono('.');
}

void lgd3d_zclear(void)
{
   Warning(("zclear not yet implemented"));
}

void lgd3d_blend_normal(void)
{
   Flush();
   SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
   SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
}

void lgd3d_blend_multiply(int blend_mode)
{
   static int table[4] = {
       D3DBLEND_ZERO,
       D3DBLEND_ONE,
       D3DBLEND_SRCCOLOR,
       D3DBLEND_DESTCOLOR
   };
   int state1,state2;
   state1 = table[blend_mode & 3];
   state2 = table[(blend_mode >> 2) & 3];
 
   Flush();
   SetRenderState(D3DRENDERSTATE_SRCBLEND, state1);
   SetRenderState(D3DRENDERSTATE_DESTBLEND, state2);
}

//
//
// Library internal functions
//
//

void lgd3d_render_init(lgd3ds_device_info *info)
{
   zbuffer = info->flags&LGD3DF_ZBUFFER;
   fog_tabledensity.drVal  = 0.025;

#ifdef USE_D3D2_API
   put_mono('d');
   SetRenderState(D3DRENDERSTATE_TEXTUREMAPBLEND, D3DTBLEND_MODULATE);
   SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD);
   SetRenderState(D3DRENDERSTATE_SPECULARENABLE, FALSE);
   SetRenderState(D3DRENDERSTATE_TEXTUREPERSPECTIVE, TRUE);
   SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
   SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
   SetRenderState(D3DRENDERSTATE_WRAPU, FALSE);
   SetRenderState(D3DRENDERSTATE_WRAPV, FALSE);
   SetRenderState(D3DRENDERSTATE_ZENABLE, (info->flags & LGD3DF_ZBUFFER) ? TRUE : FALSE);
   SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, zwrite = FALSE);
   SetRenderState(D3DRENDERSTATE_ZFUNC, D3DCMP_ALWAYS);
   zcompare = FALSE;

   // disable fogtable on @#$! powerVR!
   if (info->device_desc->dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_ZBUFFERLESSHSR)
      info->device_desc->dpcTriCaps.dwRasterCaps &= ~D3DPRASTERCAPS_FOGTABLE;
   if (info->device_desc->dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_FOGTABLE)
   {  // Setup table fog
      SetRenderState(D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_EXP);
      SetRenderState(D3DRENDERSTATE_FOGTABLEDENSITY, fog_tabledensity.dwVal);
      SetLightState(D3DLIGHTSTATE_FOGMODE, D3DFOG_EXP);
      SetLightState(D3DLIGHTSTATE_FOGDENSITY, fog_tabledensity.dwVal);
   } else
   { // use vertex fog
      SetRenderState(D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_NONE);
      SetLightState(D3DLIGHTSTATE_FOGMODE, D3DFOG_NONE);
   }

   put_mono('.');
#else
{
   void *data, *start;
   int length, size;

   start = data = get_exebuf_data(0, &size);

   OP_STATE_RENDER(1, data);
      STATE_DATA(D3DRENDERSTATE_FOGENABLE, TRUE, data);
   OP_STATE_RENDER(1, data);
      STATE_DATA(D3DRENDERSTATE_FOGCOLOR, fog_color, data);
   OP_STATE_RENDER(1, data);
      STATE_DATA(D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_EXP, data);
   OP_STATE_RENDER(1, data);
      STATE_DATA(D3DRENDERSTATE_FOGTABLEDENSITY, fog_tabledensity.dwVal, data);

   OP_STATE_LIGHT(1, data);
      STATE_DATA(D3DLIGHTSTATE_FOGMODE, D3DFOG_EXP, data);
   OP_STATE_LIGHT(1, data);
      STATE_DATA(D3DLIGHTSTATE_FOGDENSITY, fog_tabledensity.dwVal, data);

   OP_STATE_RENDER(1, data);
      STATE_DATA(D3DRENDERSTATE_TEXTUREMAPBLEND, D3DTBLEND_MODULATE, data);
   OP_STATE_RENDER(1, data);
      STATE_DATA(D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD, data);
   OP_STATE_RENDER(1, data);
      STATE_DATA(D3DRENDERSTATE_SPECULARENABLE, TRUE, data);
   OP_STATE_RENDER(1, data);
      STATE_DATA(D3DRENDERSTATE_TEXTUREPERSPECTIVE, TRUE, data);
   OP_STATE_RENDER(1, data);
      STATE_DATA(D3DRENDERSTATE_COLORKEYENABLE, FALSE, data);
   OP_STATE_RENDER(1, data);
      STATE_DATA(D3DRENDERSTATE_BLENDENABLE, FALSE, data);
   OP_STATE_RENDER(1, data);
      STATE_DATA(D3DRENDERSTATE_ALPHABLENDENABLE, FALSE, data);
   OP_STATE_RENDER(1, data);
//      STATE_DATA(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA, data);
      STATE_DATA(D3DRENDERSTATE_SRCBLEND, D3DBLEND_ONE, data);
   OP_STATE_RENDER(1, data);
//      STATE_DATA(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA, data);
      STATE_DATA(D3DRENDERSTATE_DESTBLEND, D3DBLEND_ZERO, data);
   OP_STATE_RENDER(1, data);
      STATE_DATA(D3DRENDERSTATE_WRAPU, FALSE, data);
   OP_STATE_RENDER(1, data);
      STATE_DATA(D3DRENDERSTATE_WRAPV, FALSE, data);
   OP_EXIT(data);

   length = (char *)data - (char *)start;
   AssertMsg(length < size, "Yikes! out of exebuf space!");

   memset(data, 0, size-length);

   execute(0, 0, 0, length);
}
#endif

   // force init
   lgd3d_trans = BMF_TRANS;
   SetTransparent(0);
}

void lgd3d_set_fog_enable(BOOL enable)
{
   put_mono('e');
   Flush();
   SetRenderState(D3DRENDERSTATE_FOGENABLE, enable);
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
   fog_tabledensity.drVal = density * z_far;

   put_mono('f');
   Flush();
   SetRenderState(D3DRENDERSTATE_FOGTABLEDENSITY, fog_tabledensity.dwVal);
   SetLightState(D3DLIGHTSTATE_FOGDENSITY, fog_tabledensity.dwVal);
   put_mono('.');
}

int lgd3d_is_fog_on(void)
{
    return 0; // TODO
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
    // TODO
}

//
// Interface to texture manager
//


// can't bilinear filter and colorkey at the same time
// (actually, we can; it just looks really stupid)
static void SetTransparent(int trans)
{
   DWORD filter;

   if (lgd3d_trans == (int )trans)
      return;

   lgd3d_trans = trans;
   filter = (trans&BMF_TRANS) ? D3DFILTER_NEAREST : D3DFILTER_LINEAR;

#ifdef USE_D3D2_API
   put_mono('g');
   Flush();
   SetRenderState(D3DRENDERSTATE_COLORKEYENABLE, trans&BMF_TRANS);
   SetRenderState(D3DRENDERSTATE_BLENDENABLE, trans&BMF_TRANS);
   SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, trans||lgd3d_blend);
   SetRenderState(D3DRENDERSTATE_TEXTUREMAG, filter);
   SetRenderState(D3DRENDERSTATE_TEXTUREMIN, filter);
   put_mono('.');
#else
   void *data, *start;
   int size, length;

   data = start = get_exebuf_data(0, &size);
   OP_STATE_RENDER(1, data);
      STATE_DATA(D3DRENDERSTATE_TEXTUREMAG, filter, data);
   OP_STATE_RENDER(1, data);
      STATE_DATA(D3DRENDERSTATE_TEXTUREMIN, filter, data);
   OP_EXIT(data);

   length = (char *)data - (char *)start;

   memset(data, 0, size - length);

   execute(0, 0, 0, length);
#endif
}


static int next_id=TDRV_ID_INVALID;

#define SetPoly() next_id = TDRV_ID_SOLID

void SetTextureId(int n)
{
   next_id = n;
}

// used when a texture is released to make sure state is reset

static int tex_id=TDRV_ID_INVALID;

static void doPolySetup(int n)
{
#ifdef USE_D3D2_API
   int trans;

   put_mono('h');
   Flush();
   if (n==TDRV_ID_SOLID) {
      put_mono('1');
      SetLightState(D3DLIGHTSTATE_MATERIAL, hPolyMaterial);
      put_mono('2');
      SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, 0);
      trans = 0;
   } else {
      AssertMsg1( (n>=0)&&(n<LGD3D_MAX_TEXTURES), "Invalid texture id: %i", n);
      put_mono('3');
      SetLightState(D3DLIGHTSTATE_MATERIAL, MaterialHandle[n]);
      put_mono('4');
      SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, TextureHandle[n]);
      trans = TdrvBitmap[n]->flags & (BMF_TRANS|BMF_TLUC);
   }
   put_mono('5');
   SetTransparent(trans);
   put_mono('.');
#endif
}

void UnsetTextureId(int n)
{
   extern void SynchD3D(void);

   if (n==tex_id) {
      doPolySetup(tex_id=TDRV_ID_SOLID);
      SynchD3D(); // flush it, cause the old handle's about to be released!
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

static void do_points(int n, LPD3DTLVERTEX vlist)
{
   HRESULT hres;

   if (lgd3d_punt_d3d)
      return;

   prim_setup();

   put_mono('i');
   hres = d3d_device->lpVtbl->DrawPrimitive(d3d_device, D3DPT_POINTLIST, D3DVT_TLVERTEX,
      (LPVOID)vlist, n, D3DDP_DONOTCLIP);
   put_mono('.');

   AssertMsg1(!FAILED(hres), "DrawPrimitive failed: error %i", hres&0xffff);
}

//
// guts of all polygon drawing routines
//

static void do_trifan(int n, LPD3DTLVERTEX vlist)
{
   HRESULT hres;
   AssertMsg(next_id>TDRV_ID_INVALID, "Current Texture is invalid!");

   if (lgd3d_punt_d3d)
      return;

   prim_setup();

   put_mono('j');
   hres = d3d_device->lpVtbl->DrawPrimitive(d3d_device, D3DPT_TRIANGLEFAN, D3DVT_TLVERTEX,
      (LPVOID)vlist, n, D3DDP_DONOTCLIP);
   put_mono('.');

   AssertMsg1(!FAILED(hres), "DrawPrimitive failed: error %i", hres&0xffff);
}

#define MAX_POLY_VERTS 50
D3DTLVERTEX poly_vertex_buffer[MAX_POLY_VERTS];
ushort tri_index_buffer[3*MAX_POLY_VERTS];

static void flush_polys(void)
{
   if (!num_polys)
      return;

   if (!lgd3d_punt_d3d) {
      HRESULT hres;
      if (num_polys==1)
         hres = d3d_device->lpVtbl->DrawPrimitive(d3d_device, D3DPT_TRIANGLEFAN, D3DVT_TLVERTEX,
            (LPVOID)poly_vertex_buffer, num_poly_verts, D3DDP_DONOTCLIP);
      else
         hres = d3d_device->lpVtbl->DrawIndexedPrimitive(d3d_device, D3DPT_TRIANGLELIST, D3DVT_TLVERTEX,
            (LPVOID)poly_vertex_buffer, num_poly_verts, tri_index_buffer, tri_index, D3DDP_DONOTCLIP);
      AssertMsg1(!FAILED(hres), "DrawPrimitive failed: error %i", hres&0xffff);
   }

   num_polys = tri_index = num_poly_verts = 0;
}

#define MAX_POINTS 50
D3DTLVERTEX point_buffer[MAX_POINTS];

static void flush_points(void)
{
   if (!num_points)
      return;

   if (!lgd3d_punt_d3d) {
      HRESULT hres;
      hres = d3d_device->lpVtbl->DrawPrimitive(d3d_device, D3DPT_POINTLIST, D3DVT_TLVERTEX,
         (LPVOID)point_buffer, num_points, D3DDP_DONOTCLIP);
      AssertMsg1(!FAILED(hres), "DrawPrimitive failed: error %i", hres&0xffff);
   }
   num_points = 0;
}

static void flush_primitives(void)
{
   flush_points();
   flush_polys();
   primitives_pending = FALSE;
}


static LPD3DTLVERTEX PolyMalloc(int n)
{
   LPD3DTLVERTEX retval;
   int i;

   if (lgd3d_punt_buffer)
      return (LPD3DTLVERTEX )temp_malloc(n*sizeof(D3DTLVERTEX));

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

static void PolyFree(int n, LPD3DTLVERTEX vlist)
{
   if (!lgd3d_punt_buffer)
      return;

   do_trifan(n, vlist);
   temp_free(vlist);
}

static LPD3DTLVERTEX PointMalloc(int n)
{
   LPD3DTLVERTEX retval;

   if (lgd3d_punt_buffer)
      return (LPD3DTLVERTEX )temp_malloc(n*sizeof(D3DTLVERTEX));

   prim_setup();
   if (num_points + n > MAX_POINTS)
      flush_points();

   AssertMsg(num_points + n <= MAX_POINTS, "PointMalloc(): too many points!");

   retval = &point_buffer[num_points];

   num_points += n;
   primitives_pending = TRUE;

   return retval;
}

static void PointFree(int n, LPD3DTLVERTEX vlist)
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
      SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, blend_enable||lgd3d_trans);
   }
   put_mono('.');
}

void lgd3d_draw_line(r3s_point *p0, r3s_point *p1)
{
   fix x10, y10;
   LPD3DTLVERTEX vlist;
   int i, save_id;
   D3DCOLOR c=get_color();
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
      vlist[0].sx = x0 - 0.5;
      vlist[1].sx = x0 + 0.5;
      vlist[2].sx = x1 + 0.5;
      vlist[3].sx = x1 - 0.5;
      vlist[0].sy = y0;
      vlist[1].sy = y0;
      vlist[2].sy = y1;
      vlist[3].sy = y1;
   } else {
      vlist[0].sy = y0 + 0.5;
      vlist[1].sy = y0 - 0.5;
      vlist[2].sy = y1 - 0.5;
      vlist[3].sy = y1 + 0.5;
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
         float alpha = 8.0 * 16.0 / ((i-15.5)*(i-15.5) + (j-15.5)*(j-15.5));
         uchar val;
         if (alpha > 15.0)
            val = 15;
         else {
            val = alpha;
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
   LPD3DTLVERTEX vl;
   D3DCOLOR c;

   if ((x+r < lgd3d_clip.left) || (x-r > lgd3d_clip.right) ||
       (y+r < lgd3d_clip.top) || (y-r > lgd3d_clip.bot))
      return;

   c = get_color()|0xff000000;

   gr_set_fill_type(FILL_BLEND);
   lgd3d_set_alpha_pal(hack_light_alpha_pal);
   lgd3d_set_texture(bm);

   vl = PolyMalloc(4);
   x_right = x+r; x_left = x-r;
   u_left = 0; u_right = 1.0;
   if (x_left < lgd3d_clip.left) {
      u_left = (lgd3d_clip.left - x_left) / (2*r);
      x_left = lgd3d_clip.left;
   }
   if (x_right > lgd3d_clip.right) {
      u_right = u_left + (u_right - u_left)*(lgd3d_clip.right - x_left) / (x_right - x_left);
      x_right = lgd3d_clip.right;
   }
   y_bot = y+r; y_top = y-r;
   v_top = 0; v_bot = 1.0;
   if (y_top < lgd3d_clip.top) {
      v_top = (lgd3d_clip.top - y_top) / (2*r);
      y_top = lgd3d_clip.top;
   }
   if (y_bot > lgd3d_clip.bot) {
      v_bot = v_top + (1.0 - v_top)*(lgd3d_clip.bot - y_top) / (y_bot - y_top);
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
   LPD3DTLVERTEX vp;
   D3DCOLOR c=get_color()|0xff000000;
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
   LPD3DTLVERTEX vp;
   D3DCOLOR c=get_color()&0xffffff;
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
      alpha *= bm->bits[bm->row*(bm->h>>1) + (bm->w>>1)] / 15.0;
      lgd3d_draw_point(p);
   } else
      do_quad_light(p,r,bm);
}

//
// r3d-like api
//

static int lgd3d_poly(int n, r3s_point **ppl)
{
   LPD3DTLVERTEX vlist;
   int j;
   D3DCOLOR c = get_color();

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
   LPD3DTLVERTEX vlist;
   int j;
   D3DCOLOR c0 = get_color();

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
   LPD3DTLVERTEX vlist;
   int j;
   D3DCOLOR c0 = get_color();
   int rc,gc,bc;
   rc = (c0 >> 16) & 255;
   gc = (c0 >>  8) & 255;
   bc = (c0 >>  0) & 255;

   start("l");
   vlist = PolyMalloc(n);
   for (j=0; j<n; j++) {
      int r,g,b;
      g2s_point *g2p = (g2s_point *) &ppl[j]->grp;
      r = rc*g2p->i; if (r>255) r = 255;
      g = gc*g2p->h; if (g>255) g = 255;
      b = bc*g2p->d; if (b>255) b = 255;
      vlist[j].color = RGBA_MAKE(r,g,b,lgd3d_alpha);
      vlist[j].specular = fog_specular;
      setxyz(&vlist[j], ppl[j]);
   }

   PolyFree(n, vlist);

   end();
   return CLIP_NONE;
}

int lgd3d_lit_trifan(int n, r3s_point **ppl)
{
   LPD3DTLVERTEX vlist;
   int j;
   D3DCOLOR c0 = (lgd3d_alpha << 24) + 0xffffff;

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
   LPD3DTLVERTEX vlist;
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
      r = 255*g2p->i; if (r>255) r = 255;
      g = 255*g2p->h; if (g>255) g = 255;
      b = 255*g2p->d; if (b>255) b = 255;
      vlist[j].color = RGBA_MAKE(r,g,b,lgd3d_alpha);
      vlist[j].specular = fog_specular;
      setxyz(&vlist[j], ppl[j]);
   }

   PolyFree(n, vlist);

   end();
   return CLIP_NONE;
}

int lgd3d_trifan(int n, r3s_point **ppl)
{
   LPD3DTLVERTEX vlist;
   D3DCOLOR c0 = (lgd3d_alpha << 24) + 0xffffff;
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

#pragma off(unreferenced)
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
#pragma on(unreferenced)


//
// g2 - like interface: take g2s_point handle lists
//

int lgd3d_g2utrifan(int n, g2s_point **ppl)
{
   LPD3DTLVERTEX vlist;
   D3DCOLOR c0 = (lgd3d_alpha << 24) + 0xffffff;
   int i;

   vlist = PolyMalloc(n);
   for (i=0; i<n; i++) {
      float j = ppl[i]->i;
      setxy(&vlist[i], ppl[i]->sx, ppl[i]->sy);
      vlist[i].sz = z2d;
      vlist[i].rhw = w2d;
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
   LPD3DTLVERTEX vlist;
   int j;
   D3DCOLOR c=get_color();

   vlist = PolyMalloc(n);
   for (j=0; j<n; j++) {
      vlist[j].color = c;
      vlist[j].specular = fog_specular;
      setxy(&vlist[j], ppl[j]->sx, ppl[j]->sy);
      vlist[j].sz = z2d;
      vlist[j].rhw = w2d;
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
   LPD3DTLVERTEX vlist;
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
      vlist[i].sz = z2d;
      vlist[i].rhw = w2d;
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

#ifdef OLD_AND_UNSUPPORTED
extern int lgd3d_clip_poly(int n, LPD3DTLVERTEX *vpl, LPD3DTLVERTEX **cvpl);

static void clip_poly_array(int n, LPD3DTLVERTEX vlist)
{
   LPD3DTLVERTEX *vpl;
   LPD3DTLVERTEX *cvpl=NULL;
   int i, m;

   vpl = (LPD3DTLVERTEX *)temp_malloc(n*sizeof(*vpl));

   for (i=0; i<n; i++)
      vpl[i] = &vlist[i];

   m = lgd3d_clip_poly(n, vpl, &cvpl);

   if (m<3) {
      if (cvpl!=NULL)
         temp_free(cvpl);
      temp_free(vpl);
      return;
   }
   if (cvpl!=vpl) {
      LPD3DTLVERTEX poly = (LPD3DTLVERTEX )temp_malloc(m*sizeof(D3DTLVERTEX));
      for (i=0; i<m; i++)
         poly[i] = *cvpl[i];

      do_trifan(m, poly);

      temp_free(poly);
      temp_free(cvpl);
   } else
      do_trifan(n, vlist);

   temp_free(vpl);
}


#ifdef FILTER_POLYS
static int bad_poly = -1;
static int test_poly = -1;
static int poly = 0;

void lgd3d_set_bad_poly(int i)
{
   bad_poly = i;
   if (bad_poly >= 0)
      mprintf("bad poly: %i\n", bad_poly);
}

void lgd3d_set_test_poly(int i)
{
   test_poly = i;
   if (test_poly >= 0)
      mprintf("test poly: %i\n", test_poly);
}

void ReadPoly(void)
{
   int n;
   LPD3DTLVERTEX vlist;
   static FILE *id = NULL;
   if (id == NULL)
      id = fopen("badpoly.bin", "rb");

   if (feof(id))
      return;

   fread(&n, sizeof(n), 1, id);
   vlist = (LPD3DTLVERTEX )temp_malloc(n * sizeof(*vlist));
   fread(vlist, sizeof(*vlist), n, id);

   do_trifan(n, vlist);
   temp_free(vlist);
}
#endif
#endif

