#include "material_parser.hpp"
#include "material_parser_internal.hpp"
#include "material_grammar.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <utility>

namespace DarkMaterial
{
Material::Material()
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

std::string Lower(std::string value)
{
   for (auto &c : value)
      if (c >= 'A' && c <= 'Z')
         c += 'a' - 'A';
   return value;
}
namespace
{
bool Integer(const std::string &text, int &value)
{
   auto start = text.data();
   if (!text.empty() && *start == '+')
      ++start;
   auto r = std::from_chars(start, text.data() + text.size(), value);
   return r.ec == std::errc{} && r.ptr == text.data() + text.size();
}
bool Number(const std::string &text, float &value)
{
   char *end = nullptr;
   value = std::strtof(text.c_str(), &end);
   return end != text.c_str() && *end == 0 && std::isfinite(value);
}
bool PowerOfTwo(int value)
{
   return value > 0 && value <= 16384 && !(value & (value - 1));
}
} // namespace

void Parser::Unsupported(const std::string &name)
{
   if (std::find(result.unsupported.begin(), result.unsupported.end(), name) ==
       result.unsupported.end())
      result.unsupported.push_back(name);
}

void Parser::Apply(Detail::Directive directive, const std::vector<std::string> &t)
{
   if (t.empty())
      return;
   const auto key = Lower(t[0]);
   if (directive == Detail::Directive::Ifdef || directive == Detail::Directive::Ifndef)
   {
      bool test =
          t.size() == 2 &&
          (t[1] == "1" || (t[1].front() == '$' && condition && condition(t[1].substr(1))));
      if (directive == Detail::Directive::Ifndef)
         test = !test;
      blocks.push_back({active, test});
      active = active && test;
      return;
   }
   if (directive == Detail::Directive::Else)
   {
      if (!blocks.empty() && !blocks.back().alternate)
      {
         blocks.back().alternate = true;
         active = blocks.back().parent && !blocks.back().test;
      }
      return;
   }
   if (directive == Detail::Directive::Endif)
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
   if (directive == Detail::Directive::RenderPass)
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
   if (directive == Detail::Directive::Include && t.size() == 2)
   {
      // Includes live in the same canonical folder; no absolute/parent paths.
      if (t[1].find_first_of("/\\:") == std::string::npos && t[1] != "..")
         File(t[1]);
      return;
   }
   auto &s = result.settings;
   int v = 0;
   int w = 0;
   if (directive == Detail::Directive::TerrainScale)
   {
      if ((t.size() == 2 || t.size() == 3) && Integer(t[1], v) && PowerOfTwo(v) &&
          (t.size() == 2 || (Integer(t[2], w) && PowerOfTwo(w))))
      {
         s.terrainWidth = v;
         s.terrainHeight = t.size() == 2 ? v : w;
      }
      return;
   }
   if (directive == Detail::Directive::TileFactor)
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
   if (directive == Detail::Directive::AnimationRate ||
       directive == Detail::Directive::AnimationFrames)
   {
      if (t.size() == 2 && Integer(t[1], v) && v >= 0)
      {
         if (directive == Detail::Directive::AnimationRate && v > 0)
            s.animationRate = v;
         if (directive == Detail::Directive::AnimationFrames)
            s.animationFrames = v;
      }
      return;
   }
   if (directive == Detail::Directive::AnimationMode)
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
   if (directive == Detail::Directive::IlluminationMap)
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
   if (directive == Detail::Directive::ForceOpaque)
      flag = &s.forceOpaque;
   else if (directive == Detail::Directive::FullAlpha)
      flag = &s.fullAlpha;
   else if (directive == Detail::Directive::AlphaKey)
      flag = &s.alphaKey;
   else if (directive == Detail::Directive::NoMipmap)
      flag = &s.noMipmap;
   else if (directive == Detail::Directive::UvClamp)
      flag = &s.uvClamp;
   else if (directive == Detail::Directive::LocalPalette)
      flag = &s.localPalette;
   else if (directive == Detail::Directive::ForceAnimation)
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

void Parser::File(const std::string &name)
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

void Parser::Text(std::string_view text)
{
   Detail::Lexer lexer(text);
   Detail::Grammar grammar(*this, lexer);
   grammar.parse();
}

Parser::Parser(Reader read, Condition config)
   : reader(std::move(read)), condition(std::move(config))
{
}

Material Parser::Parse(const std::string &name)
{
   File(name);
   return std::move(result);
}
} // namespace DarkMaterial
