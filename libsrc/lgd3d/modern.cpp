#include <render_backend.h>
#include <dev2d.h>
#include <lgd3d.h>
#include <climits>
#include <cstddef>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <tdrv.h>
#include <texture.h>
#include <tmgr.h>
#include <vector>

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
}

static uchar g_palettes[MODERN_MAX_PALETTES][768];
static uchar *g_textureClut = NULL;
static ushort *g_alphaPalette = NULL;
static tdrv_texture_info g_ModernTextureInfo[LGD3D_MAX_TEXTURES];
static BOOL g_ModernTextureInfoValid[LGD3D_MAX_TEXTURES];
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
static std::vector<lgd3ds_device_info> g_deviceInfos;
static std::vector<std::string> g_deviceNames;
static short g_supportedModes[GRD_MODES + 1];

extern "C" {
extern int RenderBackendVulkanEnumeratePhysicalDevices(void);
extern const char *RenderBackendVulkanPhysicalDeviceName(int index);
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

static BOOL MatchesChromaKey16(ushort pixel, ushort flags) {
  ushort key;
  ushort mask;
  switch (flags & 0x7f00) {
  case BMF_RGB_565:
    key = (ushort)(((g_chromaR >> 3) << 11) |
                   ((g_chromaG >> 2) << 5) | (g_chromaB >> 3));
    mask = 0xffff;
    break;
  case BMF_RGB_4444:
    key = (ushort)(((g_chromaR >> 4) << 8) |
                   ((g_chromaG >> 4) << 4) | (g_chromaB >> 4));
    mask = 0x0fff;
    break;
  case BMF_RGB_1555:
    key = (ushort)(((g_chromaR >> 3) << 10) |
                   ((g_chromaG >> 3) << 5) | (g_chromaB >> 3));
    mask = 0x7fff;
    break;
  default:
    key = (ushort)(((g_chromaR >> 3) << 10) |
                   ((g_chromaG >> 3) << 5) | (g_chromaB >> 3));
    mask = 0x7fff;
    break;
  }
  return (pixel & mask) == key;
}

static BOOL GetTextureStorageSize(const tdrv_texture_info *info,
                                 size_t *rowBytes, size_t *totalBytes) {
  if (!info || !info->bm || info->bm->w <= 0 || info->bm->h <= 0 ||
      info->w <= 0 || info->h <= 0 || info->scale_w < 0 ||
      info->scale_h < 0 || !rowBytes || !totalBytes)
    return FALSE;

  size_t expectedWidth = (size_t)info->bm->w;
  size_t expectedHeight = (size_t)info->bm->h;
  for (int i = 0; i < info->scale_w; ++i) {
    if (expectedWidth > (size_t)INT_MAX / 2)
      return FALSE;
    expectedWidth *= 2;
  }
  for (int i = 0; i < info->scale_h; ++i) {
    if (expectedHeight > (size_t)INT_MAX / 2)
      return FALSE;
    expectedHeight *= 2;
  }
  if (expectedWidth != (size_t)info->w ||
      expectedHeight != (size_t)info->h ||
      expectedWidth > (size_t)INT_MAX / 4)
    return FALSE;

  const size_t stride = expectedWidth * 4;
  if (expectedHeight > (size_t)INT_MAX / stride)
    return FALSE;
  *rowBytes = stride;
  *totalBytes = stride * expectedHeight;
  return TRUE;
}

static uchar *ConvertTexture(tdrv_texture_info *info) {
  grs_bitmap *bm = info ? info->bm : NULL;
  size_t outputRowBytes, totalBytes;
  if (!GetTextureStorageSize(info, &outputRowBytes, &totalBytes))
    return NULL;

  size_t sourcePixelBytes = 0;
  if (bm->type == BMT_FLAT8)
    sourcePixelBytes = 1;
  else if (bm->type == BMT_FLAT16)
    sourcePixelBytes = 2;
  else if (bm->type == BMT_FLAT24)
    sourcePixelBytes = 3;
  if (sourcePixelBytes &&
      (size_t)bm->row < (size_t)bm->w * sourcePixelBytes)
    return NULL;

  const uchar *bits = (bm->flags & BMF_LOADED) ? info->bits : bm->bits;
  if (sourcePixelBytes && !bits)
    return NULL;

  const size_t scaleWidth = (size_t)1 << info->scale_w;
  const size_t scaleHeight = (size_t)1 << info->scale_h;
  const size_t sourceRowBytes = bm->row;
  uchar *out = (uchar *)malloc(totalBytes);
  if (!out)
    return NULL;
  for (int y = 0; y < bm->h; ++y)
    for (int x = 0; x < bm->w; ++x) {
      uchar r = 255, g = 255, b = 255, a = 255;
      const uchar *source = bits
          ? bits + (size_t)y * sourceRowBytes + (size_t)x * sourcePixelBytes
          : NULL;
      if (bm->type == BMT_FLAT8) {
        uchar index = source[0];
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
        ushort p;
        memcpy(&p, source, sizeof(p));
        Decode16(p, bm->flags, &r, &g, &b, &a);
        if ((info->flags & TF_TRANS) && MatchesChromaKey16(p, bm->flags))
          a = 0;
      } else if (bm->type == BMT_FLAT24) {
        // The PCX decoder interleaves its red, green, and blue planes in
        // RGB order.  Vulkan uploads the expanded RGBA data unchanged.
        r = source[0];
        g = source[1];
        b = source[2];
        if ((info->flags & TF_TRANS) && r == g_chromaR &&
            g == g_chromaG && b == g_chromaB)
          a = 0;
      }
      const size_t destination =
          ((size_t)y * scaleHeight * (size_t)info->w +
           (size_t)x * scaleWidth) * 4;
      for (size_t repeatY = 0; repeatY < scaleHeight; ++repeatY)
        for (size_t repeatX = 0; repeatX < scaleWidth; ++repeatX) {
          uchar *pixel = out + destination +
              repeatY * outputRowBytes + repeatX * 4;
          pixel[0] = r;
          pixel[1] = g;
          pixel[2] = b;
          pixel[3] = a;
        }
    }
  return out;
}

static void CookInfo(tdrv_texture_info *info) {
  if (!info)
    return;
  // The texture manager owns flags, dimensions, scaling, and the original
  // bits pointer. In particular, loaded bitmaps temporarily store a texture
  // ID in bm->bits; replacing info->bits here corrupts animated reloads.
  size_t rowBytes, totalBytes;
  info->size = GetTextureStorageSize(info, &rowBytes, &totalBytes)
      ? (int)totalBytes : 0;
  if (!info->bm)
    return;
  // Size chains can contain different dimensions with the same byte count
  // (2048x512 and 1024x1024, for example).  Hash complete dimensions rather
  // than truncating both to eight bits, or a swapped-out large texture can be
  // incorrectly treated as an exact reload candidate.
  info->cookie = (ulong)(info->bm->type & 0xff) |
      ((ulong)(info->flags & 0xff) << 8);
  info->cookie ^= (ulong)info->w * 0x9e3779b1u;
  info->cookie ^= (ulong)info->h * 0x85ebca6bu;
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
  if (g_ModernTextures[info->id]) {
    g_ModernTextureInfo[info->id] = *info;
    g_ModernTextureInfoValid[info->id] = TRUE;
  }
  return g_ModernTextures[info->id] ? TDRV_SUCCESS : TDRV_FAILURE;
}
static void ReloadTexture(tdrv_texture_info *info) {
  lgd3d_render_flush();
  uchar *pixels = ConvertTexture(info);
  if (!pixels)
    return;
  if (!RenderBackendUpdateTexture(g_ModernTextures[info->id], info->w, info->h,
                                pixels, info->w * 4)) {
    if (g_ModernTextures[info->id]) {
      // Replacing a failed animated-light atlas must invalidate the legacy
      // stage cache as well as the backend binding. Otherwise the unchanged
      // texture ID prevents the newly created object from being rebound.
      UnsetTextureId(info->id);
      RenderBackendDestroyTexture(g_ModernTextures[info->id]);
    }
    g_ModernTextures[info->id] =
        RenderBackendCreateTexture(info->w, info->h, pixels, info->w * 4);
  }
  free(pixels);
  g_ModernBitmaps[info->id] = info->bm;
  if (g_ModernTextures[info->id]) {
    g_ModernTextureInfo[info->id] = *info;
    g_ModernTextureInfoValid[info->id] = TRUE;
  }
}
static void UnloadTexture(int id) {
  if (id >= 0 && id < LGD3D_MAX_TEXTURES) {
    g_ModernBitmaps[id] = NULL;
    g_ModernTextureInfoValid[id] = FALSE;
  }
}
static void ReleaseTexture(int id) {
  if (id >= 0 && id < LGD3D_MAX_TEXTURES) {
    UnsetTextureId(id);
    if (g_ModernTextures[id])
      RenderBackendDestroyTexture(g_ModernTextures[id]);
    g_ModernTextures[id] = NULL;
    g_ModernBitmaps[id] = NULL;
    g_ModernTextureInfoValid[id] = FALSE;
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
  const int deviceCount = RenderBackendVulkanEnumeratePhysicalDevices();
  if (deviceCount <= 0)
    return 0;
  try {
    g_deviceInfos.resize((size_t)deviceCount);
    g_deviceNames.resize((size_t)deviceCount);
    for (int device = 0; device < deviceCount; ++device) {
      const char *name = RenderBackendVulkanPhysicalDeviceName(device);
      g_deviceNames[(size_t)device] = name ? name : "Vulkan device";
    }
    for (int device = 0; device < deviceCount; ++device) {
      lgd3ds_device_info &info = g_deviceInfos[(size_t)device];
      memset(&info, 0, sizeof(info));
      info.supported_modes = g_supportedModes;
      info.name = g_deviceNames[(size_t)device].c_str();
      info.flags = LGD3DF_CAN_DO_ZBUFFER | LGD3DF_CAN_DO_VERTEX_FOG |
                   LGD3DF_CAN_DO_SINGLE_PASS_MT | LGD3DF_CAN_DO_WINDOWED |
                   LGD3DF_CAN_DO_ITERATE_ALPHA;
    }
    return deviceCount;
  } catch (...) {
    g_deviceInfos.clear();
    g_deviceNames.clear();
    return 0;
  }
}
extern "C" lgd3ds_device_info *lgd3d_get_device_info(int device) {
  return device >= 0 && (size_t)device < g_deviceInfos.size()
      ? &g_deviceInfos[(size_t)device] : NULL;
}

extern "C" BOOL lgd3d_init(lgd3ds_device_info *info) {
  uint32 white = 0xffffffff;
  int i;
  if (!RenderBackendAvailable())
    return FALSE;
  memset(g_ModernTextures, 0, sizeof(g_ModernTextures));
  memset(g_ModernBitmaps, 0, sizeof(g_ModernBitmaps));
  memset(g_ModernTextureInfoValid, 0, sizeof(g_ModernTextureInfoValid));
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
extern "C" void lgd3d_clean_render_surface(BOOL depth) {
  if (depth)
    RenderBackendClearDepth();
}
extern "C" BOOL lgd3d_overlays_master_switch(BOOL on) { return TRUE; }

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

  // The legacy palettized texture path attached a palette to each
  // loaded surface, so SetEntries recolored existing textures immediately.
  // This backend expands indices to RGBA during upload; reproduce the same
  // semantics by re-expanding every resident bitmap that uses the changed
  // palette.  Without this, models cached before a mission palette switch
  // retain menu/previous-mission colors while newly loaded terrain is right.
  for (i = 0; i < LGD3D_MAX_TEXTURES; ++i) {
    grs_bitmap *bm = g_ModernBitmaps[i];
    if (bm && bm->type == BMT_FLAT8 && bm->align == slot &&
        (bm->flags & BMF_LOADED) && g_ModernTextureInfoValid[i])
      // Do not ask the legacy manager to recook this texture.  Its cookie
      // depends on the current global fill mode, which need not match the
      // mode used when this resident texture was loaded.  Reuse the original
      // upload description and only re-expand its palette indices.
      ReloadTexture(&g_ModernTextureInfo[i]);
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
  g_chromaR = r < 0 ? 0 : r > 255 ? 255 : r;
  g_chromaG = g < 0 ? 0 : g > 255 ? 255 : g;
  g_chromaB = b < 0 ? 0 : b > 255 ? 255 : b;
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
extern "C" void lgd3d_set_light_map_method(ulong flag) {}
