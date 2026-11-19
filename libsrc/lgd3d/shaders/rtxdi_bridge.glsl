#extension GL_EXT_ray_query : require
#extension GL_EXT_nonuniform_qualifier : require
#define RTXDI_GLSL 1
#define rsqrt inversesqrt
#include <Rtxdi/RtxdiParameters.h>
#include <Rtxdi/DI/ReSTIRDIParameters.h>
#include <Rtxdi/Utils/RandomSamplerState.hlsli>
#define NRD_GLSL 1
#include "NRD_glsl.hlsli"
#include "surface_geometry.glsl"
#include "world_material.glsl"
struct Light {vec4 positionRange;vec4 directionInner;vec4 colorIntensity;vec4 outerType;vec4 rayParameters;};
struct SurfaceDraw {mat4 model;mat4 previousModel;uvec4 metadata;uvec4 identity;};
layout(set=0,binding=2,std430) readonly buffer Materials {Material data[];} materials;
layout(set=0,binding=3,std430) readonly buffer Lights {Light data[];} lights;
layout(set=0,binding=4,std140) uniform Camera {mat4 matrix;vec4 positionExposure;vec4 ambientScaleSize;vec4 depthPlane;uvec4 lightingCounts;vec4 fog;} camera;
layout(set=0,binding=6,std430) readonly buffer Links {uint data[];} links;
layout(set=0,binding=7) uniform sampler2D textures[];
#define MATERIAL_COMPUTE 1
#include "material_sampling.glsl"
layout(set=0,binding=17,std430) readonly buffer Draws {SurfaceDraw data[];} draws;
#define RAY_SCENE_HISTORY 1
#include "ray_scene.glsl"
layout(set=1,binding=0) uniform sampler2D surfacePosition;
layout(set=1,binding=1) uniform sampler2D surfaceNormal;
layout(set=1,binding=2) uniform sampler2D surfaceBase;
layout(set=1,binding=3) uniform sampler2D surfaceMotion;
layout(set=1,binding=4) uniform usampler2D surfaceIdentity;
layout(set=1,binding=5) uniform sampler2D previousPosition;
layout(set=1,binding=6) uniform sampler2D previousNormal;
layout(set=1,binding=7) uniform sampler2D previousBase;
layout(set=1,binding=8) uniform usampler2D previousIdentity;
layout(set=1,binding=9,std430) buffer Reservoirs {RTXDI_PackedDIReservoir data[];} reservoirs;
layout(set=1,binding=10,std430) readonly buffer Neighbors {vec2 data[];} neighbors;
layout(set=1,binding=11,std430) readonly buffer OldLights {Light data[];} oldLights;
layout(set=1,binding=12,std430) readonly buffer OldDraws {SurfaceDraw data[];} oldDraws;
layout(set=1,binding=13,std430) readonly buffer OldLinks {uint data[];} oldLinks;
layout(set=1,binding=14,std430) readonly buffer LightRemap {int data[];} lightRemap;
layout(set=1,binding=15,std140) uniform DirectSettings {
    uvec4 dimensionsFrame; // width, height, frame, history available
    uvec4 counts; // current lights, previous lights, initial candidates, spatial output slot
    vec4 previousEye;
    RTXDI_ReservoirBufferParameters addressing;
    RTXDI_RuntimeParameters runtime;
} di;
layout(set=1,binding=16,rgba16f) uniform image2D diffuseOutput;
layout(set=1,binding=17,rgba16f) uniform image2D specularOutput;
#define RTXDI_LIGHT_RESERVOIR_BUFFER reservoirs.data
#define RTXDI_DI_NEIGHBOR_OFFSETS_BUFFER neighbors.data
struct RAB_Material {vec3 base;float metal;float rough;};
struct RAB_Surface {vec3 position;vec3 normal;vec3 view;float depth;RAB_Material material;uvec2 identity;uint offset;uint lightCount;uint geometry;bool valid;bool previous;};
struct RAB_LightInfo {Light light;uint index;bool previous;};
struct RAB_LightSample {vec3 direction;vec3 radiance;float distance;uint index;bool previous;};
RAB_Surface RAB_EmptySurface() {
    RAB_Surface s;s.position=vec3(0);s.normal=vec3(0,0,1);s.view=s.normal;s.depth=0;
    s.material.base=vec3(0);s.material.metal=0;s.material.rough=1;s.identity=uvec2(0);s.offset=0;s.lightCount=0;s.geometry=0;s.valid=false;s.previous=false;return s;
}
RAB_Surface RAB_GetGBufferSurface(ivec2 pixel,bool previous) {
    RAB_Surface s=RAB_EmptySurface();s.previous=previous;
    if(any(lessThan(pixel,ivec2(0))) || any(greaterThanEqual(pixel,ivec2(di.dimensionsFrame.xy))) || (previous && di.dimensionsFrame.w==0u))return s;
    vec4 p=previous?texelFetch(previousPosition,pixel,0):texelFetch(surfacePosition,pixel,0);
    uvec4 id=previous?texelFetch(previousIdentity,pixel,0):texelFetch(surfaceIdentity,pixel,0);
    if(p.w<=0 || p.w>=500000 || all(equal(id.xy,uvec2(0))))return s;
    vec4 packed=previous?texelFetch(previousNormal,pixel,0):texelFetch(surfaceNormal,pixel,0);
    vec4 nr=NRD_FrontEnd_UnpackNormalAndRoughness(packed);
    vec4 base=previous?texelFetch(previousBase,pixel,0):texelFetch(surfaceBase,pixel,0);
    SurfaceDraw draw=previous?oldDraws.data[id.z]:draws.data[id.z];
    s.position=p.xyz;s.depth=p.w;s.normal=normalize(nr.xyz);s.identity=id.xy;s.geometry=id.w;
    s.material.base=base.rgb;s.material.metal=base.a;s.material.rough=nr.w;
    vec3 view=(previous?di.previousEye.xyz:camera.positionExposure.xyz)-s.position;
    s.view=dot(view,view)>1e-12?normalize(view):s.normal;
    s.offset=draw.identity.z;s.lightCount=draw.identity.w;s.valid=true;return s;
}
bool RAB_IsSurfaceValid(RAB_Surface s){return s.valid;}
float RAB_GetSurfaceLinearDepth(RAB_Surface s){return s.depth;}
vec3 RAB_GetSurfaceNormal(RAB_Surface s){return s.normal;}
RAB_Material RAB_GetMaterial(RAB_Surface s){return s.material;}
bool RAB_AreMaterialsSimilar(RAB_Material a,RAB_Material b){return abs(a.rough-b.rough)<.15 && abs(a.metal-b.metal)<.1 && length(a.base-b.base)<.3;}
ivec2 RAB_ClampSamplePositionIntoView(ivec2 pixel,bool previous){return clamp(pixel,ivec2(0),ivec2(di.dimensionsFrame.xy)-1);}
RAB_LightInfo RAB_EmptyLightInfo(){RAB_LightInfo l;l.light.positionRange=vec4(0);l.light.directionInner=vec4(0);l.light.colorIntensity=vec4(0);l.light.outerType=vec4(0);l.light.rayParameters=vec4(0);l.index=0;l.previous=false;return l;}
RAB_LightInfo RAB_LoadLightInfo(uint index,bool previous) {RAB_LightInfo l=RAB_EmptyLightInfo();l.index=index;l.previous=previous;if(index<(previous?di.counts.y:di.counts.x))l.light=previous?oldLights.data[index]:lights.data[index];return l;}
RAB_LightSample RAB_EmptyLightSample(){RAB_LightSample l;l.direction=vec3(0,0,1);l.radiance=vec3(0);l.distance=0;l.index=0;l.previous=false;return l;}
int RAB_TranslateLightIndex(uint index,bool currentToPrevious) {
    if(currentToPrevious)return index<di.counts.x?lightRemap.data[index]:-1;
    return index<di.counts.y?lightRemap.data[di.counts.x+index]:-1;
}
bool directLightLinked(RAB_Surface surface,uint index) {
    if(surface.offset==0xffffffffu)return true;
    uint first=0,last=surface.lightCount;
    while(first<last) {
        uint middle=first+(last-first)/2;
        uint value=surface.previous?oldLinks.data[surface.offset+middle]:links.data[surface.offset+middle];
        if(value==index)return true;
        if(value<index)first=middle+1;else last=middle;
    }
    return false;
}
RAB_LightSample RAB_SamplePolymorphicLight(RAB_LightInfo info,RAB_Surface surface,vec2 uv) {
    RAB_LightSample result=RAB_EmptyLightSample();result.index=info.index;result.previous=info.previous;
    Light light=info.light;bool sun=light.outerType.y==2;
    vec3 delta=sun?-normalize(light.directionInner.xyz):light.positionRange.xyz-surface.position;
    float distance=sun?100000:length(delta);
    if(distance<.002)return result;
    vec3 direction=normalize(delta);
    vec3 tangent=normalize(cross(direction,abs(direction.z)<.9?vec3(0,0,1):vec3(0,1,0)));
    vec3 bitangent=cross(direction,tangent);
    float radius=sun?tan(light.outerType.w):light.outerType.w;
    vec2 disk=sqrt(uv.x)*vec2(cos(6.2831853*uv.y),sin(6.2831853*uv.y));
    vec3 offset=(disk.x*tangent+disk.y*bitangent)*radius;
    vec3 target=sun?direction+offset:delta+offset;
    result.direction=normalize(target);result.distance=sun?distance:length(target);
    float attenuation=sun?1:1/(distance*distance+4);
    if(!sun) {
        float range=light.positionRange.w>0?light.positionRange.w:8;
        attenuation*=mix(.25,.85,smoothstep(0,1,distance/range));
        if(light.positionRange.w>0){float fade=max(1-pow(distance/light.positionRange.w,4),0);attenuation*=fade*fade;}
    }
    if(light.outerType.y==1)attenuation*=clamp((dot(-direction,normalize(light.directionInner.xyz))-light.outerType.x)/max(light.directionInner.w-light.outerType.x,.0001),0,1);
    result.radiance=light.colorIntensity.rgb*(light.colorIntensity.w*camera.ambientScaleSize.y*attenuation);return result;
}
void directBrdf(RAB_Surface surface,RAB_LightSample light,out vec3 diffuse,out vec3 specular) {
    vec3 n=surface.normal,v=surface.view,l=light.direction;float nl=max(dot(n,l),0),nv=max(dot(n,v),.0001);
    // A non-contributing light needs no GGX evaluation. This also avoids
    // normalizing v+l == 0 for a source directly behind the surface.
    if(nl<=0){diffuse=vec3(0);specular=vec3(0);return;}
    vec3 h=normalize(v+l);float nh=max(dot(n,h),0),vh=max(dot(v,h),0);
    float a=max(surface.material.rough*surface.material.rough,.002),a2=a*a;
    float d=nh*nh*(a2-1)+1,distribution=a2/(3.14159265*d*d),k=(surface.material.rough+1)*(surface.material.rough+1)/8;
    float g=(nv/(nv*(1-k)+k))*(nl/(nl*(1-k)+k));
    vec3 f=mix(vec3(.04),surface.material.base,surface.material.metal);f+=(1-f)*pow(1-vh,5);
    diffuse=(1-f)*(1-surface.material.metal)*surface.material.base*(nl/3.14159265)*light.radiance;
    specular=f*(distribution*g/max(4*nv*nl,.0001))*nl*light.radiance;
}
float directTargetPdf(RAB_LightSample light,RAB_Surface surface) {
    vec3 diffuse,specular;directBrdf(surface,light,diffuse,specular);
    return max(dot(diffuse+specular,vec3(.2126,.7152,.0722)),0);
}
float RAB_GetLightSampleTargetPdfForSurface(RAB_LightSample light,RAB_Surface surface) {
    if(!surface.valid || dot(surface.normal,light.direction)<=0 || all(lessThanEqual(light.radiance,vec3(0))) || !directLightLinked(surface,light.index))return 0;
    if(dot(unpackSurfaceGeometry(surface.geometry),light.direction)<=0)return 0;
    return directTargetPdf(light,surface);
}
bool geometricVisibility(RAB_Surface surface,RAB_LightSample light,vec3 geometricNormal,bool previousScene) {
    if(dot(geometricNormal,light.direction)<=0)return false;
    RAB_LightInfo info=RAB_LoadLightInfo(light.index,light.previous);
    if(info.light.outerType.z<0)return true;
    float scale=max(max(abs(surface.position.x),abs(surface.position.y)),abs(surface.position.z));
    vec3 point=surface.position+geometricNormal*max(.002,scale*2.3841858e-7);
    return !rayOccluded(point,light.direction,max(light.distance-.003,.001),floatBitsToInt(info.light.rayParameters.x),previousScene);
}
bool RAB_GetConservativeVisibility(RAB_Surface surface,RAB_LightSample light) {
    return geometricVisibility(surface,light,unpackSurfaceGeometry(surface.geometry),false);
}
bool RAB_GetTemporalConservativeVisibility(RAB_Surface current,RAB_Surface previous,RAB_LightSample light) {
    return geometricVisibility(previous,light,unpackSurfaceGeometry(previous.geometry),true);
}
// TemporalResampling tests this macro but does not supply a default. Define it
// before either header; SpatialResampling's later default is too late.
#define RTXDI_ALLOWED_BIAS_CORRECTION RTXDI_BIAS_CORRECTION_RAY_TRACED
#include <Rtxdi/DI/TemporalResampling.hlsli>
#include <Rtxdi/DI/SpatialResampling.hlsli>
