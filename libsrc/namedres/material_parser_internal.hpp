#pragma once

#include "material_parser.hpp"

namespace DarkMaterial::Detail
{
enum class Directive
{
   Ifdef,
   Ifndef,
   Else,
   Endif,
   RenderPass,
   Include,
   TerrainScale,
   TileFactor,
   AnimationRate,
   AnimationFrames,
   AnimationMode,
   IlluminationMap,
   ForceOpaque,
   FullAlpha,
   AlphaKey,
   NoMipmap,
   UvClamp,
   LocalPalette,
   ForceAnimation,
   Unknown
};

struct Command
{
   Directive directive;
   std::string name;
};

// Each lexer owns one file; nested parsers share only the material state.
class Lexer
{
public:
   enum class Token
   {
      Word,
      EndLine,
      Invalid,
      End
   };

   explicit Lexer(std::string_view text);
   Token Next(std::string &value, bool &first);

private:
   void ReadLine();

   std::string_view remaining;
   std::vector<std::string> tokens;
   size_t nextToken = 0;
   bool haveLine = false;
   bool invalidLine = false;
};
} // namespace DarkMaterial::Detail
