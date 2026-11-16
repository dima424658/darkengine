#version 450
layout(location=0) in vec2 uv;
layout(location=0) out vec4 color;
layout(set=0,binding=0) uniform sampler2D hdr;
layout(set=0,binding=1) uniform sampler2D worldDepth;
layout(push_constant) uniform Exposure {
    float value; uint srgbAttachment;
    float nativeLinear; float nativeReciprocal; float nativeConstant; uint writeDepth;
} exposure;
void main() {
    gl_FragDepth=gl_FragCoord.z;
    if(exposure.writeDepth!=0u) {
        float distance=texture(worldDepth,uv).r;
        gl_FragDepth=distance<=0.0?1.0:clamp(exposure.nativeLinear*distance+
            exposure.nativeReciprocal/distance+exposure.nativeConstant,0.0,1.0);
    }
    vec4 sampleColor=texture(hdr,uv);
    float coverage=clamp(sampleColor.a,0,1);
    // HDR blending/filtering stores premultiplied radiance. Recover straight
    // color for the nonlinear tone map, then blend with the native background.
    vec3 x=max(sampleColor.rgb/max(coverage,0.000001)*exposure.value,vec3(0));
    x=clamp((x*(2.51*x+0.03))/(x*(2.43*x+0.59)+0.14),0,1);
    if(exposure.srgbAttachment==0) x=mix(12.92*x,1.055*pow(x,vec3(1.0/2.4))-0.055,step(vec3(0.0031308),x));
    color=vec4(x,coverage);
}
