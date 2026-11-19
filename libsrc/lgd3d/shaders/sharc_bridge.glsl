#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference_uvec2 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_shader_explicit_arithmetic_types_float16 : require
#extension GL_EXT_shader_atomic_int64 : require
#extension GL_KHR_shader_subgroup_ballot : require
#extension GL_EXT_shader_16bit_storage : require
#extension GL_EXT_control_flow_attributes : require
#include "rtxdi_bridge.glsl"
#include "surface_geometry.glsl"
// Trace fresh Update paths rather than feeding old cache lighting back into
// the next update. Query still terminates on eligible cached secondary hits.
#define SHARC_ENABLE_CACHE_RESAMPLING 0
#define SHARC_ENABLE_GLSL 1
#define SHARC_SEPARATE_EMISSIVE 1
#define SHARC_MATERIAL_DEMODULATION 1
#undef rcp
#include "SharcGlslHelpers.h"
#include "SharcCommon.h"
layout(set=1,binding=20,std430) buffer SharcStatistics {uint updateVertices;uint cacheQueries;uint cacheHits;uint referenceVertices;} sharcStats;
layout(set=1,binding=19) uniform sampler2D surfaceEmission;
layout(set=1,binding=18,std140) uniform SharcSettings {
    uvec4 addresses[3]; // hash keys (8 B), accumulation (16 B), resolved (16 B)
    vec4 tuning; // world scale, update roughness minimum, radiance scale, enabled
    uvec4 counts; // capacity, sparse tile width, accumulation frames, eviction frames
} gi;
SharcParameters buildSharcParameters() {
    SharcParameters p;
    p.hashGridParameters.cameraPosition=camera.positionExposure.xyz;
    p.hashGridParameters.logarithmBase=SHARC_GRID_LOGARITHM_BASE;
    p.hashGridParameters.sceneScale=gi.tuning.x;p.hashGridParameters.levelBias=SHARC_GRID_LEVEL_BIAS;
    p.hashGridData.capacity=gi.counts.x;
    p.hashGridData.hashEntriesBuffer=RWStructuredBuffer_uint64_t(gi.addresses[0].xy);
    p.accumulationBuffer=RWStructuredBuffer_SharcAccumulationData(gi.addresses[1].xy);
    p.resolvedBuffer=RWStructuredBuffer_SharcPackedData(gi.addresses[2].xy);
    p.radianceScale=gi.tuning.z;return p;
}
struct IndirectSurface {RAB_Surface surface;vec3 emission;vec3 geometryNormal;float distance;};
SharcHitData cacheHit(IndirectSurface hit) {
    SharcHitData data;data.positionWorld=hit.surface.position;data.normalWorld=hit.geometryNormal;
    data.materialDemodulation=max(hit.surface.material.base*(1-hit.surface.material.metal),vec3(.04));
    data.emissive=hit.emission;return data;
}
IndirectSurface rootSurface(ivec2 pixel) {
    IndirectSurface hit;hit.surface=RAB_GetGBufferSurface(pixel,false);
    hit.geometryNormal=hit.surface.valid?unpackSurfaceGeometry(hit.surface.geometry):hit.surface.normal;
    hit.emission=hit.surface.valid?texelFetch(surfaceEmission,pixel,0).rgb:vec3(0);
    hit.distance=0;return hit;
}
vec3 offsetRay(IndirectSurface hit,vec3 direction) {
    float scale=max(max(abs(hit.surface.position.x),abs(hit.surface.position.y)),abs(hit.surface.position.z));
    return hit.surface.position+hit.geometryNormal*(dot(direction,hit.geometryNormal)>=0?1:-1)*max(.002,scale*2.3841858e-7);
}
bool traceIndirect(IndirectSurface origin,vec3 direction,out IndirectSurface hit) {
    vec3 point=offsetRay(origin,direction);rayQueryEXT query;
    rayQueryInitializeEXT(query,rayScene,gl_RayFlagsCullBackFacingTrianglesEXT,2,point,.001,direction,1024);
    while(rayQueryProceedEXT(query))if(rayQueryGetIntersectionTypeEXT(query,false)==gl_RayQueryCandidateIntersectionTriangleEXT && rayAccept(query,0,true))rayQueryConfirmIntersectionEXT(query);
    hit.surface=RAB_EmptySurface();hit.emission=vec3(0);hit.geometryNormal=vec3(0,0,1);hit.distance=1024;
    if(rayQueryGetIntersectionTypeEXT(query,true)!=gl_RayQueryCommittedIntersectionTriangleEXT)return false;
    hit.distance=rayQueryGetIntersectionTEXT(query,true);
    uvec4 instance=rayInstances.data[rayQueryGetIntersectionInstanceCustomIndexEXT(query,true)];
    RayTriangle triangle=rayTriangles.data[instance.x+rayQueryGetIntersectionPrimitiveIndexEXT(query,true)];
    vec2 b=rayQueryGetIntersectionBarycentricsEXT(query,true);vec3 bary=vec3(1-b.x-b.y,b);
    mat3 normalMatrix=transpose(mat3(rayQueryGetIntersectionWorldToObjectEXT(query,true)));
    vec3 geometric=cross(triangle.positionU[1].xyz-triangle.positionU[0].xyz,triangle.positionU[2].xyz-triangle.positionU[0].xyz);
    hit.geometryNormal=normalize(normalMatrix*geometric);
    if(dot(hit.geometryNormal,direction)>0)hit.geometryNormal=-hit.geometryNormal;
    vec3 n=normalize(normalMatrix*(triangle.normalV[0].xyz*bary.x+triangle.normalV[1].xyz*bary.y+triangle.normalV[2].xyz*bary.z));
    if(dot(n,hit.geometryNormal)<0)n=-n;
    Material material=materials.data[instance.y];MaterialSample sampled=sampleMaterial(material,rayUV(triangle,bary));
    if(material.normalTexture!=0u) {
        mat3 world=mat3(rayQueryGetIntersectionObjectToWorldEXT(query,true));
        vec2 uv0=vec2(triangle.positionU[0].w,triangle.normalV[0].w);
        vec2 uv1=vec2(triangle.positionU[1].w,triangle.normalV[1].w)-uv0;
        vec2 uv2=vec2(triangle.positionU[2].w,triangle.normalV[2].w)-uv0;
        n=materialNormal(n,world*(triangle.positionU[1].xyz-triangle.positionU[0].xyz),world*(triangle.positionU[2].xyz-triangle.positionU[0].xyz),uv1,uv2,sampled.tangentNormal);
    }
    hit.surface.position=point+direction*hit.distance;hit.surface.normal=n;hit.surface.view=-direction;
    hit.surface.material.base=sampled.base.rgb;hit.surface.material.rough=sampled.roughness;hit.surface.material.metal=sampled.metallic;
    hit.surface.offset=0xffffffffu;hit.surface.lightCount=di.counts.x;hit.surface.valid=true;
    hit.emission=sampled.emission;return true;
}
vec3 indirectDirect(IndirectSurface hit,inout RTXDI_RandomSamplerState rng) {
    RAB_Surface surface=hit.surface;
    uint count=surface.offset==0xffffffffu?di.counts.x:surface.lightCount;
    if(count==0)return vec3(0);
    // A uniform light draw is unbiased; visibility is evaluated for the sampled
    // emitter only. Primary ReSTIR reuse is kept out of this secondary estimator.
    uint index=min(uint(RTXDI_GetNextRandom(rng)*float(count)),count-1);
    if(surface.offset!=0xffffffffu)index=links.data[surface.offset+index];
    RAB_LightSample light=RAB_SamplePolymorphicLight(RAB_LoadLightInfo(index,false),surface,vec2(RTXDI_GetNextRandom(rng),RTXDI_GetNextRandom(rng)));
    vec3 diffuse,specular;directBrdf(surface,light,diffuse,specular);
    return geometricVisibility(surface,light,hit.geometryNormal,false)?(diffuse+specular)*float(count):vec3(0);
}
vec3 cosineDirection(vec3 n,inout RTXDI_RandomSamplerState rng) {
    float r=sqrt(RTXDI_GetNextRandom(rng)),phi=6.2831853*RTXDI_GetNextRandom(rng);
    vec3 t=normalize(cross(n,abs(n.z)<.9?vec3(0,0,1):vec3(0,1,0)));
    return normalize(t*(r*cos(phi))+cross(n,t)*(r*sin(phi))+n*sqrt(max(1-r*r,0)));
}
// Sample the full diffuse+GGX mixture at cache vertices. Mixture PDF and BRDF
// are evaluated together so segment weights remain independent and unbiased.
bool sampleContinuation(IndirectSurface hit,inout RTXDI_RandomSamplerState rng,out vec3 direction,out vec3 weight,out bool glossy) {
    RAB_Surface s=hit.surface;float probability=mix(.25,.8,s.material.metal);glossy=RTXDI_GetNextRandom(rng)<probability;
    if(glossy) {
        float u=min(RTXDI_GetNextRandom(rng),.999999),phi=6.2831853*RTXDI_GetNextRandom(rng);
        float a=max(s.material.rough*s.material.rough,.002),c=sqrt((1-u)/(1+(a*a-1)*u));
        vec3 t=normalize(cross(s.normal,abs(s.normal.z)<.9?vec3(0,0,1):vec3(0,1,0)));
        vec3 h=t*(sqrt(max(1-c*c,0))*cos(phi))+cross(s.normal,t)*(sqrt(max(1-c*c,0))*sin(phi))+s.normal*c;
        direction=reflect(-s.view,h);
    } else direction=cosineDirection(s.normal,rng);
    float nl=dot(s.normal,direction);weight=vec3(0);if(nl<=0 || dot(hit.geometryNormal,direction)<=0)return false;
    vec3 h=normalize(s.view+direction);float nh=max(dot(s.normal,h),0),vh=max(dot(s.view,h),.0001);
    float a=max(s.material.rough*s.material.rough,.002),d=nh*nh*(a*a-1)+1;
    float pdf=mix(nl/3.14159265,(a*a/(3.14159265*d*d))*nh/(4*vh),probability);
    RAB_LightSample light=RAB_EmptyLightSample();light.direction=direction;light.radiance=vec3(1);
    vec3 diffuse,specular;directBrdf(s,light,diffuse,specular);weight=(diffuse+specular)/max(pdf,1e-8);
    return true;
}
