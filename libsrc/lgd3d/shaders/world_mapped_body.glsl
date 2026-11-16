#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require
// Lighting pipelines test depth without writing it; the separate depth pass
// already handles alpha cutouts. Reject hidden fragments before PCSS work.
layout(early_fragment_tests) in;
layout(location=0) in vec3 worldPosition;
layout(location=1) in vec3 worldNormal;
layout(location=2) in vec2 texcoord;
layout(location=3) flat in uint materialIndex;
layout(location=4) flat in uint receivesFog;
layout(location=5) flat in uvec2 receiverLights;
layout(location=0) out vec4 radiance;
#ifdef NATIVE_DEPTH_EXPORT
layout(location=1) out vec4 nativeViewDistance;
#endif
struct Material { vec4 baseColor; vec4 emissionRoughness; uint textureIndex; uint opacity; float metallic; float cutoff;uint emissionUsesBaseTexture;uint doubleSided;uint pad1;uint pad2; };
struct ShadowView {mat4 viewProjection;vec4 originNear;vec4 depthPlane;vec4 parameters;uvec4 metadata;};
struct Light { vec4 positionRange; vec4 directionInner; vec4 colorIntensity; vec4 outerType; };
layout(set=0,binding=0) uniform sampler2DArray localShadowMaps;
layout(set=0,binding=8) uniform sampler2DArray sunShadowMaps;
layout(set=0,binding=1,std430) readonly buffer Shadows {ShadowView data[];} shadows;
layout(set=0,binding=2,std430) readonly buffer Materials { Material data[]; } materials;
layout(set=0,binding=3,std430) readonly buffer Lights { Light data[]; } lights;
layout(set=0,binding=4,std140) uniform Camera { mat4 viewProjection; vec4 positionExposure; vec4 ambientScaleSize; vec4 cameraDepthPlane; uvec4 shadowSettings; vec4 fogColorDistance; } camera;
layout(set=0,binding=5,std430) readonly buffer Tiles { uvec2 data[]; } tiles;
layout(set=0,binding=6,std430) readonly buffer TileLights { uint data[]; } tileLights;
layout(set=0,binding=7) uniform sampler2D textures[];
layout(set=0,binding=9) uniform sampler2DArray cachedShadowMaps;
vec3 receiverPlaneNormal;
#include "shadow_sampling.glsl"
float lightVisibility(Light light,vec3 point) {
    if(light.outerType.z<0)return 1;
    uint first=uint(light.outerType.z),count=uint(light.outerType.w);
    uint selected=first;
    float depth=dot(camera.cameraDepthPlane,vec4(point,1));
    if(light.outerType.y==2) {
        while(selected+1u<first+count && depth>shadows.data[selected].parameters.w)++selected;
        if(depth>shadows.data[selected].parameters.w)return 1;
    }
    float result=filteredShadow(selected,point,camera.shadowSettings.x,camera.shadowSettings.y,camera.shadowSettings.z);
    if(light.outerType.y==2) {
        float far=shadows.data[selected].parameters.w;
        float near=selected==first?0:shadows.data[selected-1u].parameters.w;
        float fade=smoothstep(mix(near,far,.9),far,depth);
        // The adjacent cascade contributes only inside the transition band.
        // Avoid a second blocker search/filter for every other sunlit pixel.
        if(fade>0) {
            float next=selected+1u<first+count?filteredShadow(selected+1u,point,camera.shadowSettings.x,camera.shadowSettings.y,camera.shadowSettings.z):1;
            result=mix(result,next,fade);
        }
    }
    return result;
}
vec3 brdf(vec3 base,float metallic,float roughness,vec3 n,vec3 v,vec3 l) {
    float nv=max(dot(n,v),0.0001),nl=max(dot(n,l),0);
    vec3 h=normalize(v+l);float nh=max(dot(n,h),0),vh=max(dot(v,h),0);
    float a=max(roughness*roughness,0.002),a2=a*a;
    float denom=nh*nh*(a2-1)+1;
    float distribution=a2/(3.14159265*denom*denom);
    float k=(roughness+1)*(roughness+1)/8;
    float geometry=(nv/(nv*(1-k)+k))*(nl/(nl*(1-k)+k));
    vec3 f0=mix(vec3(0.04),base,metallic);
    vec3 fresnel=f0+(1-f0)*pow(1-vh,5);
    return ((1-fresnel)*(1-metallic)*base/3.14159265+
        fresnel*(distribution*geometry/max(4*nv*nl,0.0001)))*nl;
}
void main() {
    vec3 geometricNormal=cross(dFdx(worldPosition),dFdy(worldPosition));
    receiverPlaneNormal=dot(geometricNormal,geometricNormal)>1e-20?normalize(geometricNormal):normalize(worldNormal);
    Material material=materials.data[materialIndex];
    if(material.doubleSided==0u && !gl_FrontFacing)discard;
    vec4 texel=texture(textures[nonuniformEXT(material.textureIndex)],texcoord);
    vec4 base=texel*material.baseColor;
    if(material.opacity==1 && base.a<material.cutoff)discard;
    vec3 n=normalize(worldNormal),v=normalize(camera.positionExposure.xyz-worldPosition);
    if(dot(n,v)<0)n=-n;
    vec3 result=base.rgb*camera.ambientScaleSize.x+material.emissionRoughness.rgb*
        (material.emissionUsesBaseTexture!=0u?texel.rgb:vec3(1));
    // Floating-point position error depends on world coordinates, not the
    // camera's distance. Keep this lookup point stable as the player moves.
    float coordinateScale=max(max(abs(worldPosition.x),abs(worldPosition.y)),abs(worldPosition.z));
    vec3 shadowPoint=worldPosition+n*max(.002,coordinateScale*2.3841858e-7);
    uint columns=(uint(camera.ambientScaleSize.z)+15)/16;
    uvec2 tile=uvec2(gl_FragCoord.xy)/16;
    uvec2 range=tiles.data[tile.y*columns+tile.x];
    uint globalCount=camera.shadowSettings.w;
    bool linked=receiverLights.x!=0xffffffffu;
    for(uint group=0u;group<(linked?1u:2u);++group) {
      uint first=linked?receiverLights.x:(group==0u?0u:range.x);
      uint count=linked?receiverLights.y:(group==0u?globalCount:range.y);
      for(uint i=0;i<count;++i) {
        Light light=lights.data[tileLights.data[first+i]];
        bool directional=light.outerType.y==2;
        vec3 delta=light.positionRange.xyz-worldPosition;
        float distance=length(delta);
        if(!directional && (distance<0.0001 || (light.positionRange.w>0 && distance>=light.positionRange.w)))continue;
        vec3 l=directional?-normalize(light.directionInner.xyz):delta/distance;
        if(dot(n,l)<=0)continue;
        float attenuation=directional?1:1/max(distance*distance,0.01);
        if(!directional && light.positionRange.w>0) {
            float fade=max(1-pow(distance/light.positionRange.w,4),0);attenuation*=fade*fade;
        }
        if(light.outerType.y==1) {
            float cosine=dot(-l,normalize(light.directionInner.xyz));
            attenuation*=clamp((cosine-light.outerType.x)/max(light.directionInner.w-light.outerType.x,0.0001),0,1);
        }
        if(attenuation<=0)continue;
        float visibility=lightVisibility(light,shadowPoint);
        if(visibility<=0)continue;
        result+=brdf(base.rgb,material.metallic,material.emissionRoughness.w,n,v,l)*
            light.colorIntensity.rgb*light.colorIntensity.w*camera.ambientScaleSize.y*attenuation*visibility;
    }
    }
    if(receivesFog!=0u && camera.fogColorDistance.w>0) {
        float depth=max(0,dot(camera.cameraDepthPlane,vec4(worldPosition,1)));
        result=mix(result,camera.fogColorDistance.rgb,clamp(depth/camera.fogColorDistance.w,0,1));
    }
    radiance=vec4(clamp(result,vec3(0),vec3(65504)),material.opacity==2?base.a:1);
#ifdef NATIVE_DEPTH_EXPORT
    vec3 forward=normalize(vec3(camera.viewProjection[0][3],camera.viewProjection[1][3],camera.viewProjection[2][3]));
    // A separate alpha lets identical blend states preserve opaque distance
    // while HDR color still blends normally for translucent materials.
    nativeViewDistance=material.opacity==2u?vec4(0):vec4(max(0,dot(forward,worldPosition-camera.positionExposure.xyz)),0,0,1);
#endif
}
