#include "vita_readable_zoom.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace DarkVitaReadableZoom
{
float ClampScale(float scale)
{
   return std::clamp(scale, 1.0f, 2.0f);
}

float ScaleStep(float scale, int direction)
{
   if (direction == 0)
      return ClampScale(scale);
   const float step = direction > 0 ? 0.25f : -0.25f;
   return ClampScale(scale + step);
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
      if (separator != std::string::npos &&
          line.substr(0, separator) == "readable_zoom")
      {
         std::stringstream parser(line.substr(separator + 1));
         float scale = 1.0f;
         if (parser >> scale)
            settings.scale = ClampScale(scale);
      }
   }
   return true;
}

bool Save(const std::string &path, const Settings &settings,
          std::string *error)
{
   if (path.empty())
   {
      if (error)
         *error = "invalid Vita readable zoom path";
      return false;
   }
   std::ofstream output(path, std::ios::trunc);
   if (!output)
   {
      if (error)
         *error = "cannot create Vita readable zoom file";
      return false;
   }
   output << "readable_zoom=" << ClampScale(settings.scale) << '\n';
   if (!output)
   {
      if (error)
         *error = "failed while writing Vita readable zoom file";
      return false;
   }
   if (error)
      error->clear();
   return true;
}
}
