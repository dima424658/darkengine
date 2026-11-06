#version 450

layout(location = 0) in vec4 linearColor;
layout(location = 1) in float linearFog;
layout(location = 2) in float linearWeight;
layout(location = 3) in vec2 texcoord0;
layout(location = 4) in vec2 texcoord1;

layout(set = 0, binding = 0) uniform sampler2D texture0;
layout(set = 0, binding = 1) uniform sampler2D texture1;

layout(push_constant) uniform ScenePushConstants {
    vec2 viewportSize;
    float gammaLevel;
    uint useTexture1;
    uint fogEnabled;
    uint alphaTest;
    uint transparentBlack;
    vec4 fogColor;
} scene;

layout(location = 0) out vec4 fragmentColor;

void main()
{
    float weight = max(linearWeight, 0.000001);
    vec4 texel = texture(texture0, texcoord0);
    if (scene.transparentBlack != 0u &&
        all(equal(texel.rgb, vec3(0.0))))
        texel.a = 0.0;

    vec4 color = (linearColor / weight) * texel;
    if (scene.useTexture1 != 0u)
        color *= texture(texture1, texcoord1);
    if (scene.alphaTest != 0u && color.a < 0.5)
        discard;
    if (scene.fogEnabled != 0u)
        color.rgb = mix(scene.fogColor.rgb, color.rgb,
            clamp(linearFog / weight, 0.0, 1.0));
    color.rgb = pow(max(color.rgb, vec3(0.0)),
        vec3(scene.gammaLevel));
    fragmentColor = color;
}
