#pragma once
#include "image_pixels.hpp"
#include <cstddef>
// Decode the top mip of a single 2D DDS. Arrays, cubes, volumes and HDR are
// rejected explicitly. Output retains original 8-bit color/data channels.
bool ResDecodeDds(const uint8_t *bytes,size_t size,ResImagePixels &output);
