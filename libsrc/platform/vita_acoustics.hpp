#pragma once

#include <cstdint>

namespace DarkVitaAcoustics
{
struct Preset
{
   float density;
   float diffusion;
   float gain;
   float gainHF;
   float decay;
   float decayHF;
};

Preset ForRoomType(int roomType);
Preset ForPropagation(float distanceMeters, float occlusion,
                      float underwater);
float HeadphoneWetMix(float baseWetMix);
}
