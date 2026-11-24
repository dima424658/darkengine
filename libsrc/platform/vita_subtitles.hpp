#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace DarkVitaSubtitles
{
enum class TextEncoding
{
   Auto,
   Utf8,
   Windows1250
};

enum class SoundType
{
   Any,
   TwoD,
   ThreeD
};

struct Cue
{
   uint32_t timeMs = 0;
   uint32_t lengthMs = 0;
   bool hasTime = false;
   bool hasLength = false;
   bool hasText = false;
   std::string text;
};

struct Entry
{
   std::string id;
   std::string type;
   std::string description;
   std::string srtFile;
   SoundType soundType = SoundType::Any;
   bool singleton = false;
   bool noVolume = false;
   bool multi = false;
   std::vector<Cue> cues;
};

struct ParseResult
{
   std::vector<Entry> entries;
   std::vector<std::string> errors;
   std::vector<std::string> warnings;

   bool ok() const { return errors.empty(); }
};

ParseResult Parse(std::string_view source,
                  TextEncoding encoding = TextEncoding::Auto);
}
