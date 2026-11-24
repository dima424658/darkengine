#include "vita_acoustics.hpp"

#include <algorithm>
#include <cmath>

namespace DarkVitaAcoustics
{
namespace
{
constexpr Preset kPresets[] = {
   {1.0f,1.0f,.32f,.89f,1.49f,.83f},
   {1.0f,1.0f,.25f,.10f,.17f,.10f},
   {1.0f,1.0f,.42f,.59f,.40f,.83f},
   {1.0f,1.0f,.32f,.25f,1.49f,.54f},
   {1.0f,1.0f,.32f,.00f,.50f,.10f},
   {1.0f,1.0f,.50f,.71f,2.31f,.64f},
   {1.0f,1.0f,.40f,.58f,4.32f,.59f},
   {1.0f,1.0f,.50f,.56f,3.92f,.70f},
   {1.0f,1.0f,.50f,1.00f,2.91f,1.30f},
   {1.0f,1.0f,.36f,.45f,7.24f,.33f},
   {1.0f,1.0f,.50f,.32f,10.05f,.23f},
   {1.0f,1.0f,.25f,.01f,.30f,.10f},
   {1.0f,1.0f,.36f,.71f,1.49f,.59f},
   {1.0f,1.0f,.44f,.64f,2.70f,.79f},
   {1.0f,.30f,.32f,.73f,1.49f,.86f},
   {1.0f,.30f,.15f,.54f,1.49f,.54f},
   {1.0f,.50f,.32f,.67f,1.49f,.67f},
   {1.0f,.27f,.27f,.21f,1.49f,.21f},
   {1.0f,1.0f,1.00f,.32f,1.49f,.83f},
   {1.0f,.21f,.21f,.50f,1.49f,.50f},
   {1.0f,1.0f,1.00f,1.00f,1.65f,1.50f},
   {1.0f,.80f,.32f,.32f,2.81f,.14f},
   {1.0f,1.0f,.36f,.01f,1.49f,.10f},
   {1.0f,.50f,.43f,1.00f,8.39f,1.39f},
   {1.0f,.60f,.36f,.63f,17.23f,.56f},
   {1.0f,.50f,.49f,.84f,7.56f,.91f},
};
constexpr int kPresetCount =
   static_cast<int>(sizeof(kPresets) / sizeof(kPresets[0]));
}

Preset ForRoomType(int roomType)
{
   const int index = std::max(0, std::min(roomType, kPresetCount - 1));
   return kPresets[index];
}

Preset ForPropagation(float distanceMeters, float occlusion,
                      float underwater)
{
   Preset preset = kPresets[0];
   const float distance = std::max(0.0f, distanceMeters);
   const float roomScale = std::min(1.0f, distance / 20.0f);
   preset.decay = std::clamp(preset.decay * (0.65f + roomScale * 0.35f),
                             0.1f, 20.0f);
   preset.gain = std::clamp(preset.gain * (1.0f -
                             std::clamp(occlusion, 0.0f, 1.0f) * 0.55f),
                            0.0f, 1.0f);
   if (underwater > 0.0f)
   {
      preset.gainHF *= 0.25f;
      preset.decay = std::max(preset.decay,
                              kPresets[0].decay * 1.18f);
   }
   return preset;
}

float HeadphoneWetMix(float baseWetMix)
{
   return std::clamp(baseWetMix * 0.82f, 0.0f, 1.0f);
}
}
