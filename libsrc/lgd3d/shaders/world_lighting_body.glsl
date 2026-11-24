#define NRD_GLSL 1
#include "NRD_glsl.hlsli"
layout(set=0,binding=4,std140) uniform Camera {
    mat4 viewProjection;vec4 positionExposure;vec4 ambientScaleSize;
    vec4 depthPlane;uvec4 lightingCounts;vec4 fog;
} camera;
layout(set=1,binding=0) uniform sampler2D surfacePosition;
layout(set=1,binding=1) uniform sampler2D surfaceNormal;
layout(set=1,binding=2) uniform sampler2D surfaceBase;
layout(set=1,binding=3) uniform sampler2D surfaceEmission;
layout(set=1,binding=4) uniform sampler2D denoisedDiffuse;
layout(set=1,binding=5) uniform sampler2D denoisedSpecular;
layout(location=0) in vec2 uv;
layout(location=0) out vec4 radiance;
#ifdef NATIVE_DEPTH_EXPORT
layout(location=1) out vec4 nativeViewDistance;
#endif
void main() {
    ivec2 pixel=ivec2(gl_FragCoord.xy);vec4 position=texelFetch(surfacePosition,pixel,0);
    // Leave the native sky/background visible through the HDR compositor.
    if(position.w<=0 || position.w>=500000)discard;
    vec4 normal=NRD_FrontEnd_UnpackNormalAndRoughness(texelFetch(surfaceNormal,pixel,0));
    vec4 base=texelFetch(surfaceBase,pixel,0),emission=texelFetch(surfaceEmission,pixel,0);
    vec3 v=normalize(camera.positionExposure.xyz-position.xyz);
    vec3 diffuseFactor,specularFactor;
    NRD_MaterialFactors(normal.xyz,v,base.rgb*(1-base.a),mix(vec3(.04),base.rgb,base.a),normal.w,diffuseFactor,specularFactor);
    float occlusion=clamp(1-4*(emission.a-(emission.a>.5?1:0)),0,1);
    vec3 result=texelFetch(denoisedDiffuse,pixel,0).rgb*diffuseFactor+
        texelFetch(denoisedSpecular,pixel,0).rgb*specularFactor+emission.rgb+base.rgb*camera.ambientScaleSize.x*occlusion;
    if(emission.a>.5 && camera.fog.w>0) {
        float depth=max(0,dot(camera.depthPlane,vec4(position.xyz,1)));
        result=mix(result,camera.fog.rgb,clamp(depth/camera.fog.w,0,1));
    }
    radiance=vec4(clamp(result,vec3(0),vec3(65504)),1);
#ifdef NATIVE_DEPTH_EXPORT
    nativeViewDistance=vec4(position.w,0,0,1);
#endif
}
