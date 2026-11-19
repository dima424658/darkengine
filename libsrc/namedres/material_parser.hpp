#pragma once
#include "material.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace DarkMaterial
{
struct Material
{
   sTextureMaterial settings{};
   Material()
   {
      settings.tileU = 1;
      settings.tileV = 1;
      settings.forceOpaque = -1;
      settings.fullAlpha = -1;
      settings.alphaKey = -1;
      settings.noMipmap = -1;
      settings.uvClamp = -1;
      settings.animationMode = -1;
   }
   std::string illumination;
   std::vector<std::string> unsupported;
};
using Reader = std::function<bool(const std::string &, std::string &)>;
using Condition = std::function<bool(const std::string &)>;
inline std::string Lower(std::string value)
{
   for (auto &c : value)
      if (c >= 'A' && c <= 'Z')
         c += 'a' - 'A';
   return value;
}
inline bool Integer(const std::string &text, int &value)
{
   auto start = text.data();
   if (!text.empty() && *start == '+')
      ++start;
   auto r = std::from_chars(start, text.data() + text.size(), value);
   return r.ec == std::errc{} && r.ptr == text.data() + text.size();
}
inline bool Number(const std::string &text, float &value)
{
   char *end = nullptr;
   value = std::strtof(text.c_str(), &end);
   return end != text.c_str() && *end == 0 && std::isfinite(value);
}
inline bool PowerOfTwo(int value)
{
   return value > 0 && value <= 16384 && !(value & (value - 1));
}
// Includes are textual: condition state is shared across include boundaries.
class Parser
{
   Material result;
   Reader reader;
   Condition condition;
   struct Block
   {
      bool parent, test, alternate = false;
   };
   std::vector<Block> blocks;
   std::vector<std::string> stack;
   bool active = true;
   size_t totalBytes = 0;
   int braceDepth = 0;
   bool passOpening = false;
   void Unsupported(const std::string &name)
   {
      if (std::find(result.unsupported.begin(), result.unsupported.end(), name) ==
          result.unsupported.end())
         result.unsupported.push_back(name);
   }
   void Line(const std::vector<std::string> &t)
   {
      if (t.empty())
         return;
      const auto key = Lower(t[0]);
      if (key == ".ifdef" || key == ".ifndef")
      {
         bool test =
             t.size() == 2 &&
             (t[1] == "1" || (t[1].front() == '$' && condition && condition(t[1].substr(1))));
         if (key == ".ifndef")
            test = !test;
         blocks.push_back({active, test});
         active = active && test;
         return;
      }
      if (key == ".else")
      {
         if (!blocks.empty() && !blocks.back().alternate)
         {
            blocks.back().alternate = true;
            active = blocks.back().parent && !blocks.back().test;
         }
         return;
      }
      if (key == ".endif")
      {
         if (!blocks.empty())
         {
            active = blocks.back().parent;
            blocks.pop_back();
         }
         return;
      }
      if (!active)
         return;
      // Unimplemented pass blocks must not alter base-surface properties.
      if (passOpening)
      {
         passOpening = false;
         if (t[0] == "{")
         {
            braceDepth = 1;
            for (size_t i = 1; i < t.size(); ++i)
            {
               if (t[i] == "{")
                  ++braceDepth;
               else if (t[i] == "}")
                  --braceDepth;
            }
            return;
         }
      }
      if (braceDepth)
      {
         for (const auto &token : t)
         {
            if (token == "{")
               ++braceDepth;
            else if (token == "}")
               --braceDepth;
         }
         return;
      }
      if (key == "render_pass")
      {
         Unsupported(key);
         passOpening = true;
         for (size_t i = 1; i < t.size(); ++i)
         {
            if (t[i] == "{")
            {
               ++braceDepth;
               passOpening = false;
            }
            else if (t[i] == "}")
               --braceDepth;
         }
         return;
      }
      if (key == "{")
         return;
      if (key == "include" && t.size() == 2)
      {
         // Includes live in the same canonical folder; no absolute/parent paths.
         if (t[1].find_first_of("/\\:") == std::string::npos && t[1] != "..")
            File(t[1]);
         return;
      }
      auto &s = result.settings;
      int v = 0, w = 0;
      if (key == "terrain_scale")
      {
         if ((t.size() == 2 || t.size() == 3) && Integer(t[1], v) && PowerOfTwo(v) &&
             (t.size() == 2 || (Integer(t[2], w) && PowerOfTwo(w))))
         {
            s.terrainWidth = v;
            s.terrainHeight = t.size() == 2 ? v : w;
         }
         return;
      }
      if (key == "tile_factor")
      {
         if ((t.size() == 2 || t.size() == 3) && Integer(t[1], v) && v && v >= -16384 &&
             v <= 16384 && (t.size() == 2 || (Integer(t[2], w) && w && w >= -16384 && w <= 16384)))
         {
            if (t.size() == 2)
               w = v;
            s.tileU = v > 0 ? 1.0f / v : float(-v);
            s.tileV = w > 0 ? 1.0f / w : float(-w);
         }
         return;
      }
      if (key == "ani_rate" || key == "ani_frames")
      {
         if (t.size() == 2 && Integer(t[1], v) && v >= 0)
         {
            if (key == "ani_rate" && v > 0)
               s.animationRate = v;
            if (key == "ani_frames")
               s.animationFrames = v;
         }
         return;
      }
      if (key == "ani_mode")
      {
         int mode = 1;
         bool valid = t.size() > 1;
         for (size_t i = 1; i < t.size(); ++i)
         {
            auto flag = Lower(t[i]);
            if (flag == "wrap")
               mode |= 1;
            else if (flag == "reverse")
               mode |= 4;
            else if (flag == "pingpong")
               mode &= ~1;
            else
               valid = false;
         }
         if (valid)
            s.animationMode = mode;
         return;
      }
      if (key == "illum_map")
      {
         float intensity;
         if (t.size() == 3 && Number(t[1], intensity) && intensity >= 0)
         {
            s.illumination = intensity;
            result.illumination = t[2];
         }
         return;
      }
      int *flag = nullptr;
      if (key == "force_opaque")
         flag = &s.forceOpaque;
      else if (key == "force_full_alpha")
         flag = &s.fullAlpha;
      else if (key == "force_alpha_key")
         flag = &s.alphaKey;
      else if (key == "no_mipmap")
         flag = &s.noMipmap;
      else if (key == "uv_clamp")
         flag = &s.uvClamp;
      else if (key == "force_local_pal")
         flag = &s.localPalette;
      else if (key == "force_ani_settings")
         flag = &s.forceAnimation;
      // Chroma-key conversion and edge padding require pixel processing.
      if (flag)
      {
         if (t.size() == 1)
            *flag = 1;
         else if (t.size() == 2 && Integer(t[1], v) && (v == 0 || v == 1))
            *flag = v;
         return;
      }
      Unsupported(key);
   }
   void Text(std::string_view text)
   {
      if (text.substr(0, 3) == "\xef\xbb\xbf")
         text.remove_prefix(3);
      while (!text.empty())
      {
         auto end = text.find('\n');
         auto line = text.substr(0, end);
         if (end == std::string_view::npos)
            text = {};
         else
            text.remove_prefix(end + 1);
         std::vector<std::string> tokens;
         std::string token;
         bool quoted = false;
         for (size_t i = 0; i < line.size(); ++i)
         {
            char c = line[i];
            if (!quoted && (c == '#' || (c == '/' && i + 1 < line.size() && line[i + 1] == '/')))
               break;
            if (c == '"')
            {
               quoted = !quoted;
               continue;
            }
            if (!quoted && (c == ' ' || c == '\t' || c == '\r' || c == '{' || c == '}'))
            {
               if (!token.empty())
               {
                  tokens.push_back(token);
                  token.clear();
               }
               if (c == '{' || c == '}')
                  tokens.emplace_back(1, c);
            }
            else
               token += c;
         }
         if (!token.empty())
            tokens.push_back(token);
         if (!quoted)
            Line(tokens);
      }
   }
   void File(const std::string &name)
   {
      const auto key = Lower(name);
      if (stack.size() >= 16 || std::find(stack.begin(), stack.end(), key) != stack.end())
      {
         Unsupported("include cycle/depth");
         return;
      }
      std::string text;
      if (!reader || !reader(name, text))
      {
         Unsupported("missing include: " + name);
         return;
      }
      totalBytes += text.size();
      if (totalBytes > 1024 * 1024)
      {
         Unsupported("material size limit");
         return;
      }
      stack.push_back(key);
      Text(text);
      stack.pop_back();
   }

 public:
   Parser(Reader read, Condition config = {})
       : reader(std::move(read)), condition(std::move(config))
   {
   }
   Material Parse(const std::string &name)
   {
      File(name);
      return std::move(result);
   }
};
} // namespace DarkMaterial
