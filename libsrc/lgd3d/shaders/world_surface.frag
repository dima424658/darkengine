#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require
#define NRD_GLSL 1
#include "NRD_glsl.hlsli"
#include "world_material.glsl"
#include "surface_geometry.glsl"
struct SurfaceDraw {mat4 model;mat4 previousModel;uvec4 metadata;uvec4 identity;};
layout(set=0,binding=2,std430) readonly buffer Materials {Material data[];} materials;
layout(set=0,binding=7) uniform sampler2D textures[];
#include "material_sampling.glsl"
layout(set=0,binding=17,std430) readonly buffer SurfaceDraws {SurfaceDraw data[];} draws;
layout(set=0,binding=19,std140) uniform SurfaceCamera {
    mat4 current;mat4 previous;vec4 eye;vec4 forward;vec4 previousEye;vec4 previousForward;vec4 jitter;
} camera;
layout(location=0) in vec3 worldPosition;
layout(location=1) in vec3 worldNormal;
layout(location=2) in vec2 texcoord;
layout(location=3) in vec4 previousClip;
layout(location=4) in vec2 viewDepth;
layout(location=5) flat in uint drawIndex;
layout(location=0) out vec4 positionDepth;
layout(location=1) out vec4 normalRoughness;
layout(location=2) out vec4 baseMetallic;
layout(location=3) out vec4 emission;
layout(location=4) out vec4 motion;
layout(location=5) out uvec4 identity;
void main() {
    // Evaluate derivatives before any alpha/backface discard. The geometric
    // normal must remain independent of interpolation and normal-map tilts.
    vec3 positionDx=dFdx(worldPosition),positionDy=dFdy(worldPosition);
    vec2 uvDx=dFdx(texcoord),uvDy=dFdy(texcoord);
    SurfaceDraw draw=draws.data[drawIndex];Material material=materials.data[draw.metadata.x];
    if(material.doubleSided==0u && !gl_FrontFacing)discard;
    MaterialSample sampled=sampleMaterial(material,texcoord);
    vec4 base=sampled.base;
    if(material.opacity==1u && base.a<material.cutoff)discard;
    vec3 n=normalize(worldNormal);
    if(dot(n,camera.eye.xyz-worldPosition)<0)n=-n;
    vec3 geometric=cross(positionDx,positionDy);
    geometric=dot(geometric,geometric)>1e-20?normalize(geometric):n;
    if(dot(geometric,camera.eye.xyz-worldPosition)<0)geometric=-geometric;
    if(material.normalTexture!=0u)n=materialNormal(n,positionDx,positionDy,uvDx,uvDy,sampled.tangentNormal);
    // Use the SDK front end so encoding stays identical to RELAX.
    normalRoughness=NRD_FrontEnd_PackNormalAndRoughness(n,sampled.roughness,0);
    positionDepth=vec4(worldPosition,max(viewDepth.x,0));
    baseMetallic=vec4(base.rgb,sampled.metallic);
    emission=vec4(sampled.emission,((draw.metadata.w&2u)!=0u?1:0)+(1-sampled.occlusion)*.25);
    vec4 clip=camera.current*vec4(worldPosition,1);
    vec2 currentUV=clip.xy/clip.w*.5+.5;
    vec2 oldUV=previousClip.w>0?previousClip.xy/previousClip.w*.5+.5:currentUV;
    bool valid=(draw.metadata.w&1u)!=0u && previousClip.w>0 && viewDepth.y>0 &&
        all(greaterThanEqual(oldUV,vec2(0))) && all(lessThanEqual(oldUV,vec2(1)));
    // Invalid surfaces point outside history even for consumers that ignore the flag.
    motion=vec4(valid?oldUV-currentUV:vec2(2),valid?viewDepth.y:0,valid?1:0);
    identity=uvec4(draw.identity.xy,drawIndex,packSurfaceGeometry(geometric,valid));
}
