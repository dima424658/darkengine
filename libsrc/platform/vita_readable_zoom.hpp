#pragma once

#include <cstdint>
#include <string>

namespace DarkVitaReadableZoom
{
struct Settings
{
   float scale = 1.0f;
};

float ClampScale(float scale);
float ScaleStep(float scale, int direction);
bool Load(const std::string &path, Settings &settings);
bool Save(const std::string &path, const Settings &settings,
          std::string *error);
}
