#version 450
#extension GL_EXT_nonuniform_qualifier : require
struct Material {vec4 baseColor;vec4 emissionRoughness;uint textureIndex;uint opacity;float metallic;float cutoff;uint emissionUsesBaseTexture;uint doubleSided;uint pad1;uint pad2;};
struct ShadowView {mat4 viewProjection;vec4 originNear;vec4 depthPlane;vec4 parameters;uvec4 metadata;};
layout(set=0,binding=1,std430) readonly buffer Shadows {ShadowView data[];} shadows;
layout(set=0,binding=2,std430) readonly buffer Materials {Material data[];} materials;
layout(set=0,binding=7) uniform sampler2D textures[];
layout(push_constant) uniform Draw {mat4 model;uint material;uint view;} draw;
layout(location=0) in vec3 worldPosition;
layout(location=1) in vec2 texcoord;
layout(location=2) flat in uint materialIndex;
void main() {
    Material material=materials.data[materialIndex];
    if(material.doubleSided==0u && !gl_FrontFacing)discard;
    if(material.opacity==2)discard;
    if(material.opacity==1 && texture(textures[nonuniformEXT(material.textureIndex)],texcoord).a*material.baseColor.a<material.cutoff)discard;
    ShadowView view=shadows.data[draw.view];
    float depth=view.metadata.x==0?length(worldPosition-view.originNear.xyz):dot(view.depthPlane,vec4(worldPosition,1));
    gl_FragDepth=clamp(depth/view.parameters.x,0,1);
}
