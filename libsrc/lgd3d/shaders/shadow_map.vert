#version 450
layout(location=0) in vec3 position;
layout(location=2) in vec2 uv;
struct ShadowView {mat4 viewProjection;vec4 originNear;vec4 depthPlane;vec4 parameters;uvec4 metadata;};
layout(set=0,binding=1,std430) readonly buffer Shadows {ShadowView data[];} shadows;
layout(push_constant) uniform Draw {mat4 model;uint material;uint view;} draw;
struct Instance {mat4 model;uvec4 material;};
layout(set=0,binding=10,std430) readonly buffer Instances {Instance data[];} instances;
layout(location=0) out vec3 worldPosition;
layout(location=1) out vec2 texcoord;
layout(location=2) flat out uint materialIndex;
void main() {
    bool indirect=draw.material==0xffffffffu;
    mat4 model=indirect?instances.data[gl_InstanceIndex].model:draw.model;
    materialIndex=indirect?instances.data[gl_InstanceIndex].material.x:draw.material;
    vec4 world=model*vec4(position,1);
    worldPosition=world.xyz;texcoord=uv;
    gl_Position=shadows.data[draw.view].viewProjection*world;
}
