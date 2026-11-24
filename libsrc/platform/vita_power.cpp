#include "vita_power.hpp"

#include <fstream>

#if defined(__vita__) || defined(__vita__)
#include <psp2/power.h>
#endif

namespace DarkVitaPower
{
bool ParseProfile(const std::string &name, Profile &profile)
{
   if (name == "30")
   {
      profile = Profile::Fps30;
      return true;
   }
   if (name == "60")
   {
      profile = Profile::Fps60;
      return true;
   }
   return false;
}

const char *ProfileName(Profile profile)
{
   return profile == Profile::Fps30 ? "30" : "60";
}

Clock ApplyProfile(Profile profile)
{
   Clock clock{444, 222, 222};
   if (profile == Profile::Fps30)
      clock = {333, 166, 166};
#if defined(__vita__)
   scePowerSetArmClockFrequency(clock.armMegahertz);
   scePowerSetBusClockFrequency(clock.busMegahertz);
   scePowerSetGpuClockFrequency(clock.gpuMegahertz);
#endif
   return clock;
}

bool Load(const std::string &path, Settings &settings)
{
   std::ifstream input(path);
   if (!input)
      return false;
   std::string line;
   while (std::getline(input, line))
   {
      const size_t separator = line.find('=');
      if (separator == std::string::npos)
         continue;
      if (line.substr(0, separator) == "fps")
         ParseProfile(line.substr(separator + 1), settings.profile);
   }
   return true;
}

bool Save(const std::string &path, const Settings &settings,
          std::string *error)
{
   if (path.empty())
   {
      if (error)
         *error = "invalid Vita power settings path";
      return false;
   }
   std::ofstream output(path, std::ios::trunc);
   if (!output)
   {
      if (error)
         *error = "cannot create Vita power settings file";
      return false;
   }
   output << "fps=" << ProfileName(settings.profile) << '\n';
   if (!output)
   {
      if (error)
         *error = "failed while writing Vita power settings file";
      return false;
   }
   if (error)
      error->clear();
   return true;
}
}
