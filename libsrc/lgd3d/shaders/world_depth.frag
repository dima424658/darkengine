#version 450
#extension GL_EXT_nonuniform_qualifier : require
layout(location=2) in vec2 texcoord;
layout(location=3) flat in uint materialIndex;
struct Material {vec4 baseColor;vec4 emissionRoughness;uint textureIndex;uint opacity;float metallic;float cutoff;uint emissionUsesBaseTexture;uint doubleSided;uint pad1;uint pad2;};
layout(set=0,binding=2,std430) readonly buffer Materials {Material data[];} materials;
layout(set=0,binding=7) uniform sampler2D textures[];
void main() {
    Material material=materials.data[materialIndex];
    if(material.doubleSided==0u && !gl_FrontFacing)discard;
    if(material.opacity==1u && texture(textures[nonuniformEXT(material.textureIndex)],texcoord).a*material.baseColor.a<material.cutoff)discard;
}
