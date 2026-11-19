#pragma once
#ifdef __cplusplus
extern "C"
{
#endif
   struct grs_bitmap;
   // -1 leaves image/engine defaults unchanged. Animation flags match sAnimTexProp.
   typedef struct sTextureMaterial
   {
      int terrainWidth, terrainHeight;
      float tileU, tileV;
      int forceOpaque, fullAlpha, alphaKey, noMipmap, uvClamp, localPalette;
      int animationRate, animationFrames, animationMode, forceAnimation;
      float illumination;
   } sTextureMaterial;
   const sTextureMaterial *ResMaterialGet(const struct grs_bitmap *bitmap);
   void ResMaterialClone(const struct grs_bitmap *source, struct grs_bitmap *destination);
   void ResMaterialForget(const struct grs_bitmap *bitmap);
   void ResMaterialSetTerrain(const struct grs_bitmap *bitmap, const struct grs_bitmap *base);
   int ResMaterialIsTerrain(const struct grs_bitmap *bitmap);
   void ResMaterialUvScale(const struct grs_bitmap *bitmap, float *u, float *v);
   // Renderer-owned attachment; released with the last image/mipmap reference.
   void *ResMaterialRenderData(const struct grs_bitmap *bitmap);
   void ResMaterialSetRenderData(const struct grs_bitmap *bitmap, void *data,
                                 void (*release)(void *));
   void ResMaterialReleaseRenderData(void);
   void ResMaterialReset(void);
   // Lazily locked illumination image. Owned by the base material until Forget.
   struct grs_bitmap *ResMaterialIllumination(const struct grs_bitmap *bitmap);
#ifdef __cplusplus
}
struct IRes;
struct IResManHelper;
void ResMaterialLoad(IResManHelper *manager, IRes *image, grs_bitmap *bitmap);
#endif
