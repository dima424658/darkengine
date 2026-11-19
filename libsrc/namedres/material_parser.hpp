#pragma once

#include "material.h"
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace DarkMaterial
{
namespace Detail
{
enum class Directive;
class Grammar;
}

struct Material
{
   Material();
   sTextureMaterial settings{};
   std::string illumination;
   std::vector<std::string> unsupported;
};

using Reader = std::function<bool(const std::string &, std::string &)>;
using Condition = std::function<bool(const std::string &)>;
std::string Lower(std::string value);

// Includes share condition and render-pass state with the including file.
class Parser
{
public:
   Parser(Reader read, Condition config = {});
   Material Parse(const std::string &name);

private:
   friend class Detail::Grammar;

   struct Block
   {
      bool parent;
      bool test;
      bool alternate = false;
   };

   void Unsupported(const std::string &name);
   void Apply(Detail::Directive directive, const std::vector<std::string> &tokens);
   void Text(std::string_view text);
   void File(const std::string &name);

   Material result;
   Reader reader;
   Condition condition;
   std::vector<Block> blocks;
   std::vector<std::string> stack;
   bool active = true;
   size_t totalBytes = 0;
   int braceDepth = 0;
   bool passOpening = false;
};
} // namespace DarkMaterial
