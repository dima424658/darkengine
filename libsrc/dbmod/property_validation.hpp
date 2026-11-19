#pragma once
#include "commands.hpp"

namespace DarkDbMod
{
struct PropertyValidation
{
   size_t line;
   std::string property;
   enum class Status
   {
      Valid,
      Unsupported,
      Invalid
   } status;
   std::string diagnostic;
};
// Read-only schema/value validation. Does not resolve object targets, create
// properties, inspect property stores or execute any patch command.
std::vector<PropertyValidation> ValidateNativePropertyFields(const std::vector<Command> &commands);
} // namespace DarkDbMod
