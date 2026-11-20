#pragma once
#include <array>
#include <cstdint>
#include <cstddef>
#include <functional>

struct sPushConstants
{
   float viewportSize[2];
   float gamma;
   uint32_t useTexture1;
   uint32_t fogEnabled;
   uint32_t alphaTest;
   uint32_t transparentBlack;
   uint32_t canvasMode;
   float fogColor[4];
};
static_assert(sizeof(sPushConstants) == 48,
              "SPIR-V push constant layout mismatch");
static_assert(offsetof(sPushConstants, canvasMode) == 28 &&
                 offsetof(sPushConstants, fogColor) == 32,
              "SPIR-V canvas constant offsets mismatch");
