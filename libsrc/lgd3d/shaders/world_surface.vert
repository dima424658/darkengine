#version 450
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 uv;
struct SurfaceDraw {mat4 model;mat4 previousModel;uvec4 metadata;uvec4 identity;};
layout(set=0,binding=17,std430) readonly buffer SurfaceDraws {SurfaceDraw data[];} draws;
layout(set=0,binding=18,std430) readonly buffer PreviousVertices {vec4 data[];} previousVertices;
layout(set=0,binding=19,std140) uniform SurfaceCamera {
    mat4 current;mat4 previous;vec4 eye;vec4 forward;vec4 previousEye;vec4 previousForward;vec4 jitter;
} camera;
layout(location=0) out vec3 worldPosition;
layout(location=1) out vec3 worldNormal;
layout(location=2) out vec2 texcoord;
layout(location=3) out vec4 previousClip;
layout(location=4) out vec2 viewDepth;
layout(location=5) flat out uint drawIndex;
void main() {
    SurfaceDraw draw=draws.data[gl_InstanceIndex];
    vec4 world=draw.model*vec4(position,1);
    uint vertexIndex=uint(gl_VertexIndex)-draw.metadata.z;
    vec4 oldWorld=draw.previousModel*vec4(previousVertices.data[draw.metadata.y+vertexIndex].xyz,1);
    worldPosition=world.xyz;worldNormal=transpose(inverse(mat3(draw.model)))*normal;texcoord=uv;
    gl_Position=camera.current*world;
    gl_Position.xy+=camera.jitter.xy*gl_Position.w;
    previousClip=camera.previous*oldWorld;
    viewDepth=vec2(dot(world.xyz-camera.eye.xyz,camera.forward.xyz),dot(oldWorld.xyz-camera.previousEye.xyz,camera.previousForward.xyz));
    drawIndex=gl_InstanceIndex;
}
