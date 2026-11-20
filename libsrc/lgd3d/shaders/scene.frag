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
    uint canvasMode;
    vec4 fogColor;
} scene;

layout(location = 0) out vec4 fragmentColor;

vec4 canvasPixel(ivec2 position)
{
    position = clamp(position, ivec2(0), ivec2(scene.viewportSize) - 1);
    uint depth = scene.canvasMode & 255u;
    int pixelsPerTexel = depth == 8u ? 4 : 2;
    uvec4 bytes = uvec4(round(texelFetch(texture0,
        ivec2(position.x / pixelsPerTexel, position.y), 0) * 255.0));
    uint raw;
    vec3 rgb;
    if (depth == 8u)
    {
        raw = bytes[position.x & 3];
        rgb = texelFetch(texture1, ivec2(int(raw), 0), 0).rgb;
    }
    else
    {
        int component = (position.x & 1) * 2;
        raw = bytes[component] | (bytes[component + 1] << 8u);
        uvec3 channels = depth == 16u
            ? uvec3((raw >> 11u) & 31u, (raw >> 5u) & 63u, raw & 31u)
            : uvec3((raw >> 10u) & 31u, (raw >> 5u) & 31u, raw & 31u);
        // Match the legacy integer expansion, including its truncation.
        uvec3 maximum = depth == 16u ? uvec3(31, 63, 31) : uvec3(31);
        rgb = vec3(channels * 255u / maximum) / 255.0;
    }
    float alpha = (scene.canvasMode & 256u) != 0u && raw == 0u ? 0.0 : 1.0;
    return vec4(rgb, alpha);
}

vec4 canvasSample(vec2 uv)
{
    vec2 pixel = uv * scene.viewportSize;
    if ((scene.canvasMode & 512u) == 0u)
        return canvasPixel(ivec2(floor(pixel)));
    pixel -= 0.5;
    ivec2 origin = ivec2(floor(pixel));
    vec2 blend = fract(pixel);
    return mix(mix(canvasPixel(origin), canvasPixel(origin + ivec2(1, 0)), blend.x),
               mix(canvasPixel(origin + ivec2(0, 1)),
                   canvasPixel(origin + ivec2(1, 1)), blend.x), blend.y);
}

void main()
{
    float weight = max(linearWeight, 0.000001);
    vec4 texel = scene.canvasMode != 0u
        ? canvasSample(texcoord0) : texture(texture0, texcoord0);
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
