#pragma once

#include <string>

namespace DarkVitaVideo
{
struct Settings
{
   int brightnessSlider = 50;
   int gammaSlider = 50;
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
Color ApplyCorrection(Color color, const Settings &settings);
const char *VertexShaderSource();
const char *FragmentShaderSource();
LoadResult Load(const std::string &path);
bool Save(const std::string &path, const Settings &settings,
          std::string *error);
}
