#include "vita_ghost_check.hpp"

#include <fstream>

namespace DarkVitaGhostCheck
{
namespace
{
bool ParseUnsigned(const std::string &text, std::uint32_t &value)
{
   if (text.empty())
      return false;
   std::uint64_t parsed = 0;
   for (char character : text)
   {
      if (character < '0' || character > '9')
         return false;
      parsed = parsed * 10 + static_cast<unsigned>(character - '0');
      if (parsed > 0xffffffffu)
         return false;
   }
   value = static_cast<std::uint32_t>(parsed);
   return true;
}
}

Result Evaluate(const Stats &stats)
{
   Result result;
   result.penalties = stats.detections + stats.alarms +
                      stats.bodiesFound + stats.kills;
   result.ghost = result.penalties == 0;
   result.perfect = result.ghost;
   return result;
}

bool Load(const std::string &path, Stats &stats)
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
      const std::string key = line.substr(0, separator);
      const std::string value = line.substr(separator + 1);
      std::uint32_t parsed = 0;
      if (ParseUnsigned(value, parsed))
      {
         if (key == "detections") stats.detections = parsed;
         else if (key == "alarms") stats.alarms = parsed;
         else if (key == "bodies_found") stats.bodiesFound = parsed;
         else if (key == "kills") stats.kills = parsed;
      }
   }
   return true;
}

bool Save(const std::string &path, const Stats &stats,
          std::string *error)
{
   if (path.empty())
   {
      if (error)
         *error = "invalid Vita Ghost Check path";
      return false;
   }
   std::ofstream output(path, std::ios::trunc);
   if (!output)
   {
      if (error)
         *error = "cannot create Vita Ghost Check file";
      return false;
   }
   output << "detections=" << stats.detections << '\n'
          << "alarms=" << stats.alarms << '\n'
          << "bodies_found=" << stats.bodiesFound << '\n'
          << "kills=" << stats.kills << '\n';
   if (!output)
   {
      if (error)
         *error = "failed while writing Vita Ghost Check file";
      return false;
   }
   if (error)
      error->clear();
   return true;
}
}
