#version 450
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 uv;
layout(set=0,binding=4,std140) uniform Camera {
    mat4 viewProjection;
    vec4 positionExposure;
    vec4 ambientScaleSize;
} camera;
layout(push_constant) uniform Draw { mat4 model; uint material; uint receivesFog; uint lightOffset; uint lightCount; } draw;
layout(location=0) out vec3 worldPosition;
layout(location=1) out vec3 worldNormal;
layout(location=2) out vec2 texcoord;
layout(location=3) flat out uint materialIndex;
layout(location=4) flat out uint receivesFog;
layout(location=5) flat out uvec2 receiverLights;
void main() {
    vec4 world=draw.model*vec4(position,1);
    worldPosition=world.xyz;
    worldNormal=transpose(inverse(mat3(draw.model)))*normal;
    texcoord=uv;
    materialIndex=draw.material;
    receivesFog=draw.receivesFog;
    receiverLights=uvec2(draw.lightOffset,draw.lightCount);
    gl_Position=camera.viewProjection*world;
}
