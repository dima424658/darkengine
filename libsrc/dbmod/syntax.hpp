#pragma once

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace DarkDbMod
{
enum class Phase { Pre, Post };
struct Argument
{
   std::string text;
   bool quoted = false;
};
struct Statement
{
   std::vector<Argument> arguments;
   std::vector<Statement> fields;
   size_t line = 0;
   Phase phase = Phase::Post;
   bool hasBlock = false;
};
struct SyntaxError : std::runtime_error
{
   size_t line;
   SyntaxError(size_t at, const std::string &message)
       : std::runtime_error("line " + std::to_string(at) + ": " + message), line(at)
   {
   }
};
// Syntax only. Unknown commands and argument semantics require an executor's
// separate validation pass before any engine state may be modified.
std::vector<Statement> Parse(std::string_view source);
}
