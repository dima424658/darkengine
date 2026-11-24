#pragma once

#include <cstdint>
#include <string>

namespace DarkVitaTextures
{
enum class Profile
{
   Size256 = 256,
   Size512 = 512,
   Size1024 = 1024,
   Unlimited = 0,
};

struct Settings
{
   Profile profile = Profile::Size512;
};

struct Dimensions
{
   std::uint32_t width;
   std::uint32_t height;
};

bool ParseProfile(const std::string &name, Profile &profile);
const char *ProfileName(Profile profile);
Dimensions BoundDimensions(Profile profile, std::uint32_t width,
                           std::uint32_t height);
std::uint64_t EstimatedRgbaBytes(std::uint32_t width,
                                 std::uint32_t height);
Settings Load(const std::string &path);
bool Save(const std::string &path, const Settings &settings,
          std::string *error);
}
