#include <render_backend.h>
#include <dev2d.h>
#include <lgd3d.h>
#include "legacy_texture_driver.h"
#include <climits>
#include <cstdlib>
#include <cstring>
#include <rsdcfg.h>
#include <vector>
#include <material.h>

constexpr int kPaletteSlots = 256;
extern "C" {
void *g_legacyTextures[LGD3D_MAX_TEXTURES]{};
grs_bitmap *g_legacyBitmaps[LGD3D_MAX_TEXTURES]{};
uchar *lgd3d_clut = nullptr;
}
static uchar g_palettes[kPaletteSlots][768];
static uchar *g_textureClut = nullptr;
static ushort *g_alphaPalette = nullptr;
static tdrv_texture_info g_textureInfo[LGD3D_MAX_TEXTURES];
static BOOL g_textureInfoValid[LGD3D_MAX_TEXTURES];
static int g_chromaR = 255, g_chromaG = 0, g_chromaB = 255;

extern "C" void LegacyTexturesInit(void)
{
   std::memset(g_legacyTextures, 0, sizeof(g_legacyTextures));
   std::memset(g_legacyBitmaps, 0, sizeof(g_legacyBitmaps));
   std::memset(g_textureInfoValid, 0, sizeof(g_textureInfoValid));
   for (auto &palette : g_palettes) std::memcpy(palette, grd_pal, sizeof(palette));
}

extern "C" grs_bitmap *lgd3d_get_texture_bitmap(int id)
{
   return id >= 0 && id < LGD3D_MAX_TEXTURES ? g_legacyBitmaps[id] : nullptr;
}

extern "C" BOOL lgd3d_texture_is_resident(int id)
{
   return id >= 0 && id < LGD3D_MAX_TEXTURES && g_legacyTextures[id] != nullptr;
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

  const uchar *bits = (bm->flags & BMF_LOADED) ? info->bits : bm->bits;
  grs_bitmap decodedBitmap;
  std::vector<uchar> decodedPixels;
  if (bm->type == BMT_RSD8) {
    // RSD8 bitmaps store compressed data, so gr_init_bitmap() gives them a
    // zero row stride. gd_rsd8_convert() uses row as the decoded output stride
    // and expects a full flat bitmap buffer in the unpack configuration.
    if (!bits || bm->w <= 0 || bm->h <= 0 || bm->h > INT_MAX / bm->w)
      return NULL;
    const size_t decodedSize = (size_t)bm->w * bm->h;
    try {
      decodedPixels.resize(decodedSize);
    } catch (...) {
      return NULL;
    }

    grs_bitmap encodedBitmap = *bm;
    encodedBitmap.bits = const_cast<uchar *>(bits);
    encodedBitmap.row = bm->w;
    gds_rsd_config previousConfig;
    gd_get_rsd_config(&previousConfig);
    gds_rsd_config decodedConfig = {
        decodedPixels.data(), (int)decodedSize, (int)decodedSize, 0};
    gd_set_rsd_config(&decodedConfig);
    const int result = gd_rsd8_convert(&encodedBitmap, &decodedBitmap);
    gd_set_rsd_config(&previousConfig);
    if (result != GR_UNPACK_RSD8_OK)
      return NULL;
    decodedBitmap.bits = decodedPixels.data();
    bm = &decodedBitmap;
    bits = decodedBitmap.bits;
  }

  size_t sourcePixelBytes = 0;
  if (bm->type == BMT_FLAT8 || bm->type == BMT_TLUC8)
    sourcePixelBytes = 1;
  else if (bm->type == BMT_FLAT16)
    sourcePixelBytes = 2;
  else if (bm->type == BMT_FLAT24)
    sourcePixelBytes = 3;
  if (sourcePixelBytes &&
      (size_t)bm->row < (size_t)bm->w * sourcePixelBytes)
    return NULL;

  if (sourcePixelBytes && !bits)
    return NULL;

  const auto *material = ResMaterialGet(info->bm);
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
      if (bm->type == BMT_FLAT8 || bm->type == BMT_TLUC8) {
        uchar index = source[0];
        if (g_textureClut && !(info->flags & TF_ALPHA))
          index = g_textureClut[index];
        if (info->flags & TF_ALPHA) {
          ushort p = g_alphaPalette ? g_alphaPalette[index]
                                    : (ushort)((index >> 4) << 12 | 0x0fff);
          Decode16(p, BMF_RGB_4444, &r, &g, &b, &a);
        } else {
          int slot = bm->align;
          if (slot < 0 || slot >= kPaletteSlots)
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
        if ((info->flags & TF_TRANS) && (!material || material->fullAlpha != 1) &&
            MatchesChromaKey16(p, bm->flags))
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
      if (material)
      {
         if (material->forceOpaque == 1)
            a = 255;
         else if (material->alphaKey == 1)
            a = a >= 128 ? 255 : 0;
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

extern "C" void LegacyTextureCook(tdrv_texture_info *info) {
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
extern "C" int LegacyTextureLoad(tdrv_texture_info *info) {
  uchar *pixels;
  if (info->id < 0 || info->id >= LGD3D_MAX_TEXTURES)
    return TDRV_FAILURE;
  pixels = ConvertTexture(info);
  if (!pixels)
    return TDRV_FAILURE;
  if (g_legacyTextures[info->id])
    RenderBackendDestroyTexture(g_legacyTextures[info->id]);
  g_legacyTextures[info->id] =
      RenderBackendCreateTexture(info->w, info->h, pixels, info->w * 4);
  free(pixels);
  g_legacyBitmaps[info->id] = info->bm;
  if (g_legacyTextures[info->id]) {
    g_textureInfo[info->id] = *info;
    g_textureInfoValid[info->id] = TRUE;
  }
  return g_legacyTextures[info->id] ? TDRV_SUCCESS : TDRV_FAILURE;
}
extern "C" void LegacyTextureReload(tdrv_texture_info *info) {
  lgd3d_render_flush();
  uchar *pixels = ConvertTexture(info);
  if (!pixels)
    return;
  if (!RenderBackendUpdateTexture(g_legacyTextures[info->id], info->w, info->h,
                                pixels, info->w * 4)) {
    if (g_legacyTextures[info->id]) {
      // Replacing a failed animated-light atlas must invalidate the legacy
      // stage cache as well as the backend binding. Otherwise the unchanged
      // texture ID prevents the newly created object from being rebound.
      UnsetTextureId(info->id);
      RenderBackendDestroyTexture(g_legacyTextures[info->id]);
    }
    g_legacyTextures[info->id] =
        RenderBackendCreateTexture(info->w, info->h, pixels, info->w * 4);
  }
  free(pixels);
  g_legacyBitmaps[info->id] = info->bm;
  if (g_legacyTextures[info->id]) {
    g_textureInfo[info->id] = *info;
    g_textureInfoValid[info->id] = TRUE;
  }
}
extern "C" void LegacyTextureUnload(int id) {
  if (id >= 0 && id < LGD3D_MAX_TEXTURES) {
    g_legacyBitmaps[id] = NULL;
    g_textureInfoValid[id] = FALSE;
  }
}
extern "C" void LegacyTextureRelease(int id) {
  if (id >= 0 && id < LGD3D_MAX_TEXTURES) {
    UnsetTextureId(id);
    if (g_legacyTextures[id])
      RenderBackendDestroyTexture(g_legacyTextures[id]);
    g_legacyTextures[id] = NULL;
    g_legacyBitmaps[id] = NULL;
    g_textureInfoValid[id] = FALSE;
  }
}
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
    grs_bitmap *bm = g_legacyBitmaps[i];
    if (bm && (bm->type == BMT_FLAT8 || bm->type == BMT_TLUC8 ||
               bm->type == BMT_RSD8) && bm->align == slot &&
        (bm->flags & BMF_LOADED) && g_textureInfoValid[i])
      // Do not ask the legacy manager to recook this texture.  Its cookie
      // depends on the current global fill mode, which need not match the
      // mode used when this resident texture was loaded.  Reuse the original
      // upload description and only re-expand its palette indices.
      LegacyTextureReload(&g_textureInfo[i]);
  }
}

extern "C" void lgd3d_set_pal_slot(uint start, uint n, uchar *pal, int slot) {
  if (slot < 0 || slot >= kPaletteSlots || start >= 256)
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

extern "C" void *LegacyMaterialIllumination(const grs_bitmap *bitmap)
{
   if (void *cached = ResMaterialRenderData(bitmap))
      return cached;
   grs_bitmap *image = ResMaterialIllumination(bitmap);
   if (!image)
      return nullptr;
   tdrv_texture_info info{};
   info.bm = image;
   info.w = image->w;
   info.h = image->h;
   info.bits = image->bits;
   // A referenced image can already be resident through ordinary geometry.
   if (image->flags & BMF_LOADED)
   {
      bool found = false;
      for (int i = 0; i < LGD3D_MAX_TEXTURES; ++i)
         if (g_legacyBitmaps[i] == image && g_textureInfoValid[i])
         {
            info.bits = g_textureInfo[i].bits;
            found = true;
            break;
         }
      if (!found)
         return nullptr;
   }
   uchar *pixels = ConvertTexture(&info);
   if (!pixels)
      return nullptr;
   void *texture = RenderBackendCreateTexture(image->w, image->h, pixels, image->w * 4);
   std::free(pixels);
   if (texture)
   {
      ResMaterialSetRenderData(bitmap, texture, RenderBackendDestroyTexture);
      RenderBackendTrace("material-illumination pixels=%dx%d", image->w, image->h);
   }
   return texture;
}
