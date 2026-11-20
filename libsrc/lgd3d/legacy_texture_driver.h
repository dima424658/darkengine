// $Header: x:/prj/tech/libsrc/lgd3d/RCS/tdrv.h 1.8 1998/03/06 13:02:31 KEVIN Exp $
#ifndef DARK_LEGACY_TEXTURE_DRIVER_H
#define DARK_LEGACY_TEXTURE_DRIVER_H

#include <types.h>
#include <texture.h>

typedef struct grs_bitmap grs_bitmap;

typedef struct tdrv_texture_info {
   grs_bitmap *bm;
   int id;
   int flags;
   int scale_w;
   int scale_h;
   int w, h;
   int size;
   uchar *bits;
   ulong cookie;
} tdrv_texture_info;


#define TDRV_FAILURE -1
#define TDRV_SUCCESS 0

#define TDRV_ID_SOLID     -1
#define TDRV_ID_CALLBACK  -2
#define TDRV_ID_INVALID   -3

// texture flags
#define TF_ALPHA 1
#define TF_RGB 2
#define TF_TRANS 4

#ifdef __cplusplus
extern "C" {
#endif
int LegacyTextureLoad(tdrv_texture_info *info);
void LegacyTextureReload(tdrv_texture_info *info);
void LegacyTextureRelease(int id);
void LegacyTextureUnload(int id);
void LegacyTextureCook(tdrv_texture_info *info);
void SetTextureId(int id);
void UnsetTextureId(int id);
void lgd3d_render_flush(void);
void lgd3d_reset_texture_bindings(void);
void LegacyTexturesInit(void);
void *LegacyMaterialIllumination(const grs_bitmap *bitmap);
extern void *g_legacyTextures[LGD3D_MAX_TEXTURES];
extern grs_bitmap *g_legacyBitmaps[LGD3D_MAX_TEXTURES];
extern uchar *lgd3d_clut;
#ifdef __cplusplus
}
#endif

#endif
