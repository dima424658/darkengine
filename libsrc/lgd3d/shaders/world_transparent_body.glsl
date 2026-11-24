#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_ray_query : require
// Primary visibility has already established opaque depth.
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
#include "world_material.glsl"
struct Light { vec4 positionRange; vec4 directionInner; vec4 colorIntensity; vec4 outerType; vec4 rayParameters; };
layout(set=0,binding=2,std430) readonly buffer Materials { Material data[]; } materials;
layout(set=0,binding=3,std430) readonly buffer Lights { Light data[]; } lights;
layout(set=0,binding=4,std140) uniform Camera { mat4 viewProjection; vec4 positionExposure; vec4 ambientScaleSize; vec4 cameraDepthPlane; uvec4 lightingCounts; vec4 fogColorDistance; } camera;
layout(set=0,binding=5,std430) readonly buffer Tiles { uvec2 data[]; } tiles;
layout(set=0,binding=6,std430) readonly buffer TileLights { uint data[]; } tileLights;
layout(set=0,binding=7) uniform sampler2D textures[];
#include "material_sampling.glsl"
vec3 receiverPlaneNormal;
#include "ray_scene.glsl"
// Transparent effects are forward shaded, without a screen-space denoiser.
// Use one visibility ray to the emitter center; opaque soft shadows use RTXDI.
float lightVisibility(Light light,vec3 point) {
    if(light.outerType.z<0)return 1;
    bool sun=light.outerType.y==2;
    vec3 delta=sun?-normalize(light.directionInner.xyz):light.positionRange.xyz-point;
    float distance=sun?100000:length(delta);
    return distance<=.002 || !rayOccluded(point,normalize(delta),max(distance-.002,.001),floatBitsToInt(light.rayParameters.x))?1:0;
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
    vec3 positionDx=dFdx(worldPosition),positionDy=dFdy(worldPosition);
    vec2 uvDx=dFdx(texcoord),uvDy=dFdy(texcoord);
    vec3 geometricNormal=cross(positionDx,positionDy);
    receiverPlaneNormal=dot(geometricNormal,geometricNormal)>1e-20?normalize(geometricNormal):normalize(worldNormal);
    Material material=materials.data[materialIndex];
    if(material.doubleSided==0u && !gl_FrontFacing)discard;
    MaterialSample sampled=sampleMaterial(material,texcoord);
    vec4 base=sampled.base;
    if(material.opacity==1 && base.a<material.cutoff)discard;
    vec3 n=normalize(worldNormal),v=normalize(camera.positionExposure.xyz-worldPosition);
    if(dot(receiverPlaneNormal,v)<0)receiverPlaneNormal=-receiverPlaneNormal;
    if(dot(n,v)<0)n=-n;
    if(material.normalTexture!=0u)n=materialNormal(n,positionDx,positionDy,uvDx,uvDy,sampled.tangentNormal);
    vec3 result=base.rgb*camera.ambientScaleSize.x*sampled.occlusion+sampled.emission;
    // Floating-point position error depends on world coordinates, not the
    // camera's distance. Keep this lookup point stable as the player moves.
    float coordinateScale=max(max(abs(worldPosition.x),abs(worldPosition.y)),abs(worldPosition.z));
    vec3 shadowPoint=worldPosition+receiverPlaneNormal*max(.002,coordinateScale*2.3841858e-7);
    uint columns=(uint(camera.ambientScaleSize.z)+15)/16;
    uvec2 tile=uvec2(gl_FragCoord.xy)/16;
    uvec2 range=tiles.data[tile.y*columns+tile.x];
    uint globalCount=camera.lightingCounts.w;
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
        if(dot(n,l)<=0 || dot(receiverPlaneNormal,l)<=0)continue;
        // A finite two-foot source region prevents inverse-square hotspots on
        // nearby windows/walls from staying saturated as intensity is reduced.
        float attenuation=directional?1:1/(distance*distance+4.0);
        if(!directional) {
            // Keep the source region substantially dimmer, with a smaller
            // reduction near the edge. Unbounded lights use eight world units.
            float falloffDistance=light.positionRange.w>0?light.positionRange.w:8.0;
            attenuation*=mix(0.25,0.85,smoothstep(0.0,1.0,distance/falloffDistance));
        }
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
        result+=brdf(base.rgb,sampled.metallic,sampled.roughness,n,v,l)*
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
