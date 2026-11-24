// Include after the scene texture descriptor declaration. Color maps use sRGB
// image views; data maps use UNORM, so every consumer sees the same linear data.
vec4 materialTexel(uint index,vec2 uv) {
#ifdef MATERIAL_COMPUTE
    return textureLod(textures[nonuniformEXT(index)],uv,0);
#else
    return texture(textures[nonuniformEXT(index)],uv);
#endif
}
struct MaterialSample {vec4 base;vec3 emission;vec3 tangentNormal;float roughness;float metallic;float occlusion;};
MaterialSample sampleMaterial(Material material,vec2 uv) {
    MaterialSample s;vec4 texel=materialTexel(material.textureIndex,uv);
    s.base=texel*material.baseColor;s.roughness=material.emissionRoughness.w;s.metallic=material.metallic;s.occlusion=1;
    s.emission=material.emissionRoughness.rgb*(material.emissionUsesBaseTexture!=0u?texel.rgb:vec3(1));
    s.tangentNormal=vec3(0,0,1);
    if(material.pbrFlags==0u)return s;
    if(material.ormTexture!=0u) {
        vec3 orm=materialTexel(material.ormTexture,uv).rgb;
        s.occlusion=orm.r;s.roughness=orm.g;s.metallic=orm.b;
    }
    if(material.roughnessTexture!=0u)s.roughness=materialTexel(material.roughnessTexture,uv).r;
    if(material.metallicTexture!=0u)s.metallic=materialTexel(material.metallicTexture,uv).r;
    if(material.occlusionTexture!=0u)s.occlusion=materialTexel(material.occlusionTexture,uv).r;
    if(material.emissionTexture!=0u)s.emission+=materialTexel(material.emissionTexture,uv).rgb;
    if(material.normalTexture!=0u) {
        vec2 xy=materialTexel(material.normalTexture,uv).rg*2-1;
        // Reconstruct +Z for both BC5 RG and conventional RGB normal maps.
        s.tangentNormal=normalize(vec3(xy,sqrt(max(1-dot(xy,xy),0))));
    }
    s.roughness=clamp(s.roughness,0,1);s.metallic=clamp(s.metallic,0,1);s.occlusion=clamp(s.occlusion,0,1);
    return s;
}
vec3 materialNormal(vec3 n,vec3 edge1,vec3 edge2,vec2 uv1,vec2 uv2,vec3 tangentNormal) {
    float determinant=uv1.x*uv2.y-uv1.y*uv2.x;
    if(abs(determinant)<1e-12)return n;
    vec3 t=(edge1*uv2.y-edge2*uv1.y)/determinant;
    vec3 b=(edge2*uv1.x-edge1*uv2.x)/determinant;
    t-=n*dot(n,t);if(dot(t,t)<1e-16 || dot(b,b)<1e-16)return n;
    t=normalize(t);b=normalize(cross(n,t))*(dot(cross(n,t),b)<0?-1:1);
    return normalize(t*tangentNormal.x+b*tangentNormal.y+n*tangentNormal.z);
}
