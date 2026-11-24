#pragma once

#include <cstdint>
#include <string>

namespace DarkVitaPower
{
enum class Profile
{
   Fps30,
   Fps60,
};

struct Settings
{
   Profile profile = Profile::Fps60;
};

struct Clock
{
   int armMegahertz;
   int busMegahertz;
   int gpuMegahertz;
};

bool ParseProfile(const std::string &name, Profile &profile);
const char *ProfileName(Profile profile);
Clock ApplyProfile(Profile profile);
bool Load(const std::string &path, Settings &settings);
bool Save(const std::string &path, const Settings &settings,
          std::string *error);
}
