#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in float inFog;
layout(location = 3) in vec2 inTexcoord0;
layout(location = 4) in vec2 inTexcoord1;

layout(push_constant) uniform ScenePushConstants {
    vec2 viewportSize;
    float gammaLevel;
    uint useTexture1;
    uint fogEnabled;
    uint alphaTest;
    uint transparentBlack;
    vec4 fogColor;
} scene;

layout(location = 0) out vec4 linearColor;
layout(location = 1) out float linearFog;
layout(location = 2) out float linearWeight;
layout(location = 3) out vec2 texcoord0;
layout(location = 4) out vec2 texcoord1;

void main()
{
    float q = 1.0 / max(inPosition.w, 0.000001);
    float x = inPosition.x / scene.viewportSize.x * 2.0 - 1.0;
    float y = inPosition.y / scene.viewportSize.y * 2.0 - 1.0;

    // Vulkan's clip-space depth range is [0, 1], unlike OpenGL's [-1, 1].
    gl_Position = vec4(x * q, y * q, inPosition.z * q, q);
    linearColor = inColor * q;
    linearFog = inFog * q;
    linearWeight = q;
    texcoord0 = inTexcoord0;
    texcoord1 = inTexcoord1;
}
