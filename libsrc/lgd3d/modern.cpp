#include <render_backend.h>
#include <dev2d.h>
#include <lgd3d.h>
#include <stdlib.h>
#include <string.h>
#include <tdrv.h>
#include <texture.h>
#include <tmgr.h>

#define MODERN_MAX_PALETTES 256

extern "C" {
texture_manager *g_tmgr = NULL;
void *g_ModernTextures[LGD3D_MAX_TEXTURES];
grs_bitmap *g_ModernBitmaps[LGD3D_MAX_TEXTURES];
void *g_ModernWhiteTexture = NULL;
int g_ModernTextureLevel = 0;
BOOL lgd3d_blend_trans = TRUE;
BOOL b_SS2_UseSageTexManager = FALSE;
uchar *lgd3d_clut = NULL;
void SynchD3D(void) {}
}

static uchar g_palettes[MODERN_MAX_PALETTES][768];
static uchar *g_textureClut = NULL;
static ushort *g_alphaPalette = NULL;
// Dark's portal and model UVs routinely extend beyond the base texture and
// rely on the primary texture stage repeating.  Starting stage zero clamped
// turns every out-of-range surface into a stretched edge row/column until a
// later effect happens to enable wrapping.  Lightmaps occupy an atlas, so the
// secondary stage must remain clamped.
static BOOL g_wrap[2] = {TRUE, FALSE};
static BOOL g_smooth[2] = {TRUE, TRUE};
static BOOL g_dither = FALSE, g_antialias = FALSE, g_shading = TRUE;
static int g_chromaR = 255, g_chromaG = 0, g_chromaB = 255;
static grs_bitmap *g_defaultBitmap = NULL;
static lgd3ds_device_info g_deviceInfo;
static short g_supportedModes[GRD_MODES + 1];
static const char g_deviceName[] = "Modern renderer";

extern "C" {
extern void SetTextureId(int n);
extern void UnsetTextureId(int n);
extern void lgd3d_render_init(lgd3ds_device_info *info);
extern void lgd3d_render_shutdown(void);
extern void lgd3d_render_start_frame(void);
extern void lgd3d_render_end_frame(void);
extern void lgd3d_render_flush(void);
extern void lgd3d_modern_reset_texture_state(void);
extern void lgd3d_set_znearfar(double znear, double zfar);
extern void ScrnClearHardwareOverlay(void);
}

static void Decode16(ushort p, ushort flags, uchar *r, uchar *g, uchar *b,
                     uchar *a) {
  switch (flags & 0x7f00) {
  case BMF_RGB_565:
    *r = (uchar)(((p >> 11) & 31) * 255 / 31);
    *g = (uchar)(((p >> 5) & 63) * 255 / 63);
    *b = (uchar)((p & 31) * 255 / 31);
    *a = 255;
    break;
  case BMF_RGB_4444:
    *a = (uchar)(((p >> 12) & 15) * 17);
    *r = (uchar)(((p >> 8) & 15) * 17);
    *g = (uchar)(((p >> 4) & 15) * 17);
    *b = (uchar)((p & 15) * 17);
    break;
  case BMF_RGB_1555:
    *a = (p & 0x8000) ? 255 : 0;
    *r = (uchar)(((p >> 10) & 31) * 255 / 31);
    *g = (uchar)(((p >> 5) & 31) * 255 / 31);
    *b = (uchar)((p & 31) * 255 / 31);
    break;
  default:
    *r = (uchar)(((p >> 10) & 31) * 255 / 31);
    *g = (uchar)(((p >> 5) & 31) * 255 / 31);
    *b = (uchar)((p & 31) * 255 / 31);
    *a = 255;
    break;
  }
}

static uchar *ConvertTexture(tdrv_texture_info *info) {
  grs_bitmap *bm = info->bm;
  int w = info->w, h = info->h, x, y;
  const uchar *bits = (bm->flags & BMF_LOADED) ? info->bits : bm->bits;
  uchar *out = (uchar *)malloc(w * h * 4);
  if (!out)
    return NULL;
  for (y = 0; y < h; ++y)
    for (x = 0; x < w; ++x) {
      uchar r = 255, g = 255, b = 255, a = 255;
      if (bm->type == BMT_FLAT8) {
        uchar index = bits[y * bm->row + x];
        if (g_textureClut && !(info->flags & TF_ALPHA))
          index = g_textureClut[index];
        if (info->flags & TF_ALPHA) {
          ushort p = g_alphaPalette ? g_alphaPalette[index]
                                    : (ushort)((index >> 4) << 12 | 0x0fff);
          Decode16(p, BMF_RGB_4444, &r, &g, &b, &a);
        } else {
          int slot = bm->align;
          if (slot < 0 || slot >= MODERN_MAX_PALETTES)
            slot = 0;
          r = g_palettes[slot][index * 3];
          g = g_palettes[slot][index * 3 + 1];
          b = g_palettes[slot][index * 3 + 2];
          if ((info->flags & TF_TRANS) && index == 0)
            a = 0;
        }
      } else if (bm->type == BMT_FLAT16) {
        ushort p = ((const ushort *)(bits + y * bm->row))[x];
        Decode16(p, bm->flags, &r, &g, &b, &a);
        if ((info->flags & TF_TRANS) && p == 0)
          a = 0;
      } else if (bm->type == BMT_FLAT24) {
        const uchar *p = bits + y * bm->row + x * 3;
        b = p[0];
        g = p[1];
        r = p[2];
      }
      out[(y * w + x) * 4] = r;
      out[(y * w + x) * 4 + 1] = g;
      out[(y * w + x) * 4 + 2] = b;
      out[(y * w + x) * 4 + 3] = a;
    }
  return out;
}

static void CookInfo(tdrv_texture_info *info) {
  // The texture manager owns flags, dimensions, scaling, and the original
  // bits pointer. In particular, loaded bitmaps temporarily store a texture
  // ID in bm->bits; replacing info->bits here corrupts animated reloads.
  info->size = info->w * info->h * 4;
  info->cookie =
      (ulong)(info->bm->type & 0xff) | ((ulong)(info->flags & 0xff) << 8) |
      ((ulong)(info->w & 0xff) << 16) | ((ulong)(info->h & 0xff) << 24);
}
static int LoadTexture(tdrv_texture_info *info) {
  uchar *pixels;
  if (info->id < 0 || info->id >= LGD3D_MAX_TEXTURES)
    return TDRV_FAILURE;
  pixels = ConvertTexture(info);
  if (!pixels)
    return TDRV_FAILURE;
  if (g_ModernTextures[info->id])
    RenderBackendDestroyTexture(g_ModernTextures[info->id]);
  g_ModernTextures[info->id] =
      RenderBackendCreateTexture(info->w, info->h, pixels, info->w * 4);
  free(pixels);
  g_ModernBitmaps[info->id] = info->bm;
  return g_ModernTextures[info->id] ? TDRV_SUCCESS : TDRV_FAILURE;
}
static void ReloadTexture(tdrv_texture_info *info) {
  lgd3d_render_flush();
  uchar *pixels = ConvertTexture(info);
  if (!pixels)
    return;
  if (!RenderBackendUpdateTexture(g_ModernTextures[info->id], info->w, info->h,
                                pixels, info->w * 4)) {
    if (g_ModernTextures[info->id])
      RenderBackendDestroyTexture(g_ModernTextures[info->id]);
    g_ModernTextures[info->id] =
        RenderBackendCreateTexture(info->w, info->h, pixels, info->w * 4);
  }
  free(pixels);
  g_ModernBitmaps[info->id] = info->bm;
}
static void UnloadTexture(int id) {
  if (id >= 0 && id < LGD3D_MAX_TEXTURES)
    g_ModernBitmaps[id] = NULL;
}
static void ReleaseTexture(int id) {
  if (id >= 0 && id < LGD3D_MAX_TEXTURES) {
    UnsetTextureId(id);
    if (g_ModernTextures[id])
      RenderBackendDestroyTexture(g_ModernTextures[id]);
    g_ModernTextures[id] = NULL;
    g_ModernBitmaps[id] = NULL;
  }
}
static void Synchronize(void) {}
static void DriverStartFrame(int frame) { RenderBackendBeginFrame(); }
static void DriverEndFrame(void) { RenderBackendEndFrame(); }

static void InitTextureManager(lgd3ds_device_info *info) {
  static texture_driver driver;
  driver.load_texture = LoadTexture;
  driver.release_texture = ReleaseTexture;
  driver.set_texture_id = SetTextureId;
  driver.unload_texture = UnloadTexture;
  driver.synchronize = Synchronize;
  driver.start_frame = DriverStartFrame;
  driver.end_frame = DriverEndFrame;
  driver.reload_texture = ReloadTexture;
  driver.cook_info = CookInfo;
  g_tmgr = get_dopey_texture_manager(&driver);
  g_tmgr->init(g_defaultBitmap, LGD3D_MAX_TEXTURES, NULL, 0,
               (info->flags & LGD3DF_SPEW) ? TMGRF_SPEW : 0);
}

extern "C" int lgd3d_enumerate_devices(void) {
  // Enumeration happens before the platform renderer creates its output.
  int i;
  int supportedModeCount = gr_get_registered_mode_count(16);
  for (i = 0; i < supportedModeCount && i < GRD_MODES; ++i)
    g_supportedModes[i] = (short)gr_get_registered_mode(i, 16);
  g_supportedModes[i] = -1;
  memset(&g_deviceInfo, 0, sizeof(g_deviceInfo));
  g_deviceInfo.supported_modes = g_supportedModes;
  g_deviceInfo.name = g_deviceName;
  g_deviceInfo.flags = LGD3DF_CAN_DO_ZBUFFER | LGD3DF_CAN_DO_VERTEX_FOG |
                       LGD3DF_CAN_DO_SINGLE_PASS_MT | LGD3DF_CAN_DO_WINDOWED |
                       LGD3DF_CAN_DO_ITERATE_ALPHA;
  return 1;
}
extern "C" int lgd3d_enumerate_devices_capable_of(ulong flags) {
  return lgd3d_enumerate_devices();
}
extern "C" void lgd3d_unenumerate_devices(void) {}
extern "C" lgd3ds_device_info *lgd3d_get_device_info(int device) {
  return device == 0 ? &g_deviceInfo : NULL;
}
extern "C" void lgd3d_set_hardware(void) {}
extern "C" void lgd3d_set_software(void) {}
extern "C" void lgd3d_set_RGB(void) {}
extern "C" BOOL lgd3d_is_RGB(void) { return TRUE; }
extern "C" BOOL lgd3d_is_hardware(void) { return TRUE; }
extern "C" void lgd3d_texture_set_RGB(bool isRGB) {}

extern "C" BOOL lgd3d_init(lgd3ds_device_info *info) {
  uint32 white = 0xffffffff;
  int i;
  if (!RenderBackendAvailable())
    return FALSE;
  memset(g_ModernTextures, 0, sizeof(g_ModernTextures));
  memset(g_ModernBitmaps, 0, sizeof(g_ModernBitmaps));
  for (i = 0; i < MODERN_MAX_PALETTES; ++i)
    memcpy(g_palettes[i], grd_pal, 768);
  g_defaultBitmap = gr_alloc_bitmap(BMT_FLAT8, 0, 2, 2);
  if (!g_defaultBitmap)
    return FALSE;
  memset(g_defaultBitmap->bits, 255, 4);
  g_ModernWhiteTexture = RenderBackendCreateTexture(1, 1, &white, 4);
  if (!g_ModernWhiteTexture) {
    gr_free(g_defaultBitmap);
    g_defaultBitmap = NULL;
    return FALSE;
  }
  g_ModernTextureLevel = 0;
  lgd3d_modern_reset_texture_state();
  RenderBackendSetSampler(0, g_wrap[0], g_smooth[0]);
  RenderBackendSetSampler(1, g_wrap[1], g_smooth[1]);
  RenderBackendSetDepth(FALSE, FALSE);
  RenderBackendSetBlend(kRenderBackendBlendOpaque);
  RenderBackendSetAlphaTest(FALSE);
  info->flags |=
      LGD3DF_ZBUFFER | LGD3DF_MULTI_TEXTURING | LGD3DF_MULTITEXTURE_COLOR;
  InitTextureManager(info);
  RenderBackendBeginFrame();
  lgd3d_render_init(info);
  RenderBackendEndFrame();
  // Renderer initialization needs a temporary hardware frame for state
  // setup, but it is not a rendered scene.  Leaving it active makes menu
  // canvases composite as transparent overlays over an empty/stale scene.
  RenderBackendDeactivateScene();
  return TRUE;
}
extern "C" void lgd3d_shutdown(void) {
  int i;
  RenderBackendDeactivateScene();
  lgd3d_render_shutdown();
  if (g_tmgr) {
    g_tmgr->shutdown();
    g_tmgr = NULL;
  }
  for (i = 0; i < LGD3D_MAX_TEXTURES; ++i)
    ReleaseTexture(i);
  if (g_ModernWhiteTexture)
    RenderBackendDestroyTexture(g_ModernWhiteTexture);
  g_ModernWhiteTexture = NULL;
  if (g_defaultBitmap)
    gr_free(g_defaultBitmap);
  g_defaultBitmap = NULL;
}
extern "C" void lgd3d_start_frame(int frame) {
  g_ModernTextureLevel = 0;
  ScrnClearHardwareOverlay();
  RenderBackendBeginFrame();
  lgd3d_render_start_frame();
  if (g_tmgr)
    g_tmgr->start_frame(frame);
}
extern "C" void lgd3d_end_frame(void) {
  lgd3d_render_end_frame();
  if (g_tmgr)
    g_tmgr->end_frame();
  RenderBackendEndFrame();
}
extern "C" void lgd3d_scene_suspend(void) { RenderBackendDeactivateScene(); }
extern "C" BOOL lgd3d_attach_to_lgsurface(ILGSurface *surface) {
  return RenderBackendAvailable();
}
extern "C" void lgd3d_clean_render_surface(BOOL depth) {
  if (depth)
    RenderBackendClearDepth();
}
extern "C" BOOL lgd3d_overlays_master_switch(BOOL on) { return TRUE; }
extern "C" void lgd3d_blit(void) {}

extern "C" void lgd3d_set_texture_clut(uchar *clut) {
  g_textureClut = lgd3d_clut = clut;
  if (g_tmgr)
    g_tmgr->set_clut(clut);
}
extern "C" uchar *lgd3d_set_clut(uchar *clut) {
  uchar *old = lgd3d_clut;
  lgd3d_set_texture_clut(clut);
  return old;
}
extern "C" void lgd3d_set_alpha_pal(ushort *pal) { g_alphaPalette = pal; }

static void ReloadPaletteTextures(int slot) {
  int i;

  if (!g_tmgr)
    return;

  // The legacy palettized texture path attached a DirectDraw palette to each
  // loaded surface, so SetEntries recolored existing textures immediately.
  // This backend expands indices to RGBA during upload; reproduce the same
  // semantics by re-expanding every resident bitmap that uses the changed
  // palette.  Without this, models cached before a mission palette switch
  // retain menu/previous-mission colors while newly loaded terrain is right.
  for (i = 0; i < LGD3D_MAX_TEXTURES; ++i) {
    grs_bitmap *bm = g_ModernBitmaps[i];
    if (bm && bm->type == BMT_FLAT8 && bm->align == slot &&
        (bm->flags & BMF_LOADED))
      g_tmgr->reload_texture(bm);
  }
}

extern "C" void lgd3d_set_pal_slot(uint start, uint n, uchar *pal, int slot) {
  if (slot < 0 || slot >= MODERN_MAX_PALETTES || start >= 256)
    return;
  if (start + n > 256)
    n = 256 - start;
  if (!n || !memcmp(&g_palettes[slot][start * 3], pal, n * 3))
    return;
  memcpy(&g_palettes[slot][start * 3], pal, n * 3);
  ReloadPaletteTextures(slot);
}
extern "C" void lgd3d_set_pal_slot_flags(uint start, uint n, uchar *pal,
                                         int slot, int flags) {
  lgd3d_set_pal_slot(start, n, pal, slot);
}
extern "C" void lgd3d_set_pal(uint start, uint n, uchar *pal) {
  lgd3d_set_pal_slot(start, n, pal, 0);
}
extern "C" BOOL lgd3d_get_texture_wrapping(uint level) {
  return level < 2 ? g_wrap[level] : FALSE;
}
extern "C" BOOL lgd3d_set_texture_wrapping(uint level, BOOL wrap) {
  BOOL old;
  if (level >= 2)
    return FALSE;
  lgd3d_render_flush();
  old = g_wrap[level];
  g_wrap[level] = wrap;
  RenderBackendSetSampler(level, wrap, g_smooth[level]);
  return old;
}
extern "C" void lgd3d_set_chromakey(int r, int g, int b) {
  g_chromaR = r;
  g_chromaG = g;
  g_chromaB = b;
}
extern "C" void lgd3d_get_opaque_texture_bitmask(grs_rgb_bitmask *m) {
  m->red = 0xf800;
  m->green = 0x07e0;
  m->blue = 0x001f;
}
extern "C" void lgd3d_get_trans_texture_bitmask(grs_rgb_bitmask *m) {
  lgd3d_get_opaque_texture_bitmask(m);
}
extern "C" void lgd3d_get_alpha_texture_bitmask(grs_rgb_bitmask *m) {
  m->red = 0x0f00;
  m->green = 0x00f0;
  m->blue = 0x000f;
}
extern "C" void lgd3d_set_dithering(int on) { g_dither = on; }
extern "C" int lgd3d_is_dithering_on(void) { return g_dither; }
extern "C" void lgd3d_set_antialiasing(int on) { g_antialias = on; }
extern "C" int lgd3d_is_antialiasing_on(void) { return g_antialias; }
extern "C" BOOL lgd3d_set_shading(BOOL smooth) {
  BOOL old = g_shading;
  lgd3d_render_flush();
  g_shading = smooth;
  g_smooth[0] = g_smooth[1] = smooth;
  RenderBackendSetSampler(0, g_wrap[0], smooth);
  RenderBackendSetSampler(1, g_wrap[1], smooth);
  return old;
}
extern "C" BOOL lgd3d_is_smooth_shading_on(void) { return g_shading; }
extern "C" BOOL lgd3d_enable_specular(BOOL use) { return FALSE; }
extern "C" void lgd3d_set_texture_map_method(ulong flag) {}
extern "C" void lgd3d_set_light_map_method(ulong flag) {}
extern "C" void lgd3d_get_texblending_modes(ulong *a, ulong *b) {
  if (a)
    *a = LGD3DTB_MODULATE;
  if (b)
    *b = LGD3D_MULTITEXTURE_COLOR;
}
extern "C" BOOL lgd3d_get_error(uint32 *code, uint32 *result) {
  if (code)
    *code = LGD3D_EC_OK;
  if (result)
    *result = 0;
  return FALSE;
}
