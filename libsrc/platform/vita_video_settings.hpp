#pragma once

#include <string>

namespace DarkVitaVideo
{
enum class SpatialUpscale
{
   Native100 = 0,
   Balanced85,
   Uncharted,
   Extreme,
};

struct Settings
{
   int brightnessSlider = 50;
   int gammaSlider = 50;
   SpatialUpscale spatialUpscale = SpatialUpscale::Native100;
};

struct Color
{
   float red;
   float green;
   float blue;
};

struct LoadResult
{
   Settings settings;
   bool found = false;
   std::string error;
};

float BrightnessOffset(const Settings &settings);
float GammaValue(const Settings &settings);
SpatialUpscale ParseSpatialUpscale(const std::string &name);
const char *SpatialUpscaleName(SpatialUpscale mode);
void SpatialRenderResolution(SpatialUpscale mode,
                             int *width, int *height);
bool SpatialUpscaleNeedsFsr(SpatialUpscale mode);
Color ApplyCorrection(Color color, const Settings &settings);
const char *VertexShaderSource();
const char *FragmentShaderSource();
const char *FsrVertexShaderSource();
const char *FsrFragmentShaderSource();
LoadResult Load(const std::string &path);
bool Save(const std::string &path, const Settings &settings,
          std::string *error);
}
