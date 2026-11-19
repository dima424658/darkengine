#ifndef DARK_WORLD_MATERIAL
#define DARK_WORLD_MATERIAL
struct Material {
    vec4 baseColor;vec4 emissionRoughness;
    uint textureIndex;uint opacity;float metallic;float cutoff;
    uint emissionUsesBaseTexture;uint doubleSided;uint pbrFlags;uint pad2;
    uint normalTexture;uint roughnessTexture;uint metallicTexture;uint occlusionTexture;
    uint emissionTexture;uint ormTexture;uint pad3;uint pad4;
};
#endif
