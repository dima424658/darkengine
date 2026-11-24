#pragma once

#include <cstdint>
#include <string>

namespace DarkVitaGhostCheck
{
struct Stats
{
   std::uint32_t detections = 0;
   std::uint32_t alarms = 0;
   std::uint32_t bodiesFound = 0;
   std::uint32_t kills = 0;
};

struct Result
{
   bool ghost = false;
   bool perfect = false;
   std::uint32_t penalties = 0;
};

Result Evaluate(const Stats &stats);
bool Load(const std::string &path, Stats &stats);
bool Save(const std::string &path, const Stats &stats,
          std::string *error);
}
