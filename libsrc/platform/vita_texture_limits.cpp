#include "vita_texture_limits.hpp"

#include <algorithm>
#include <fstream>

namespace DarkVitaTextures
{
namespace
{
std::uint32_t NextPowerOfTwo(std::uint32_t value)
{
   if (value <= 1)
      return 1;
   std::uint32_t result = 1;
   while (result < value)
      result <<= 1;
   return result;
}
}

bool ParseProfile(const std::string &name, Profile &profile)
{
   if (name == "256")
      profile = Profile::Size256;
   else if (name == "512")
      profile = Profile::Size512;
   else if (name == "1024")
      profile = Profile::Size1024;
   else if (name == "unlimited")
      profile = Profile::Unlimited;
   else
      return false;
   return true;
}

const char *ProfileName(Profile profile)
{
   switch (profile)
   {
   case Profile::Size256: return "256";
   case Profile::Size512: return "512";
   case Profile::Size1024: return "1024";
   case Profile::Unlimited: return "unlimited";
   }
   return "512";
}

Dimensions BoundDimensions(Profile profile, std::uint32_t width,
                           std::uint32_t height)
{
   if (profile == Profile::Unlimited)
      return {width, height};
   const std::uint32_t limit =
      static_cast<std::uint32_t>(profile);
   const std::uint32_t boundedWidth =
      std::min(NextPowerOfTwo(width), limit);
   const std::uint32_t boundedHeight =
      std::min(NextPowerOfTwo(height), limit);
   return {boundedWidth, boundedHeight};
}

std::uint64_t EstimatedRgbaBytes(std::uint32_t width,
                                 std::uint32_t height)
{
   return static_cast<std::uint64_t>(width) *
          static_cast<std::uint64_t>(height) * 4;
}

Settings Load(const std::string &path)
{
   Settings settings;
   std::ifstream input(path);
   std::string line;
   while (std::getline(input, line))
   {
      const size_t separator = line.find('=');
      if (separator == std::string::npos)
         continue;
      if (line.substr(0, separator) == "max_texture_size")
      {
         ParseProfile(line.substr(separator + 1), settings.profile);
      }
   }
   return settings;
}

bool Save(const std::string &path, const Settings &settings,
          std::string *error)
{
   if (path.empty())
   {
      if (error)
         *error = "invalid Vita texture settings path";
      return false;
   }
   std::ofstream output(path, std::ios::trunc);
   if (!output)
   {
      if (error)
         *error = "cannot create Vita texture settings file";
      return false;
   }
   output << "max_texture_size=" << ProfileName(settings.profile) << '\n';
   if (!output)
   {
      if (error)
         *error = "failed while writing Vita texture settings file";
      return false;
   }
   if (error)
      error->clear();
   return true;
}
}
