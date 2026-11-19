#include "string_fields.hpp"
#include "fields.hpp"

namespace DarkDbMod
{
namespace
{
std::string Value(const std::vector<Argument> &arguments, size_t line)
{
   if (arguments.empty())
      throw SyntaxError(line, "missing string value");
   std::string text;
   for (size_t index = 0; index < arguments.size(); ++index)
   {
      if (index)
         text += ' ';
      text += arguments[index].text;
   }
   if (text.find('\0') != std::string::npos)
      throw SyntaxError(line, "string property values cannot contain NUL");
   return text;
}
} // namespace
std::optional<std::string> ReadStringPropertyValue(const Statement &statement,
                                                   size_t headerArguments)
{
   if (!statement.hasBlock)
   {
      if (statement.arguments.size() < headerArguments + 2 ||
          statement.arguments[headerArguments].text != "=" ||
          statement.arguments[headerArguments].quoted)
         throw SyntaxError(statement.line, "expected = and a string value");
      return Value(std::vector<Argument>(statement.arguments.begin() + headerArguments + 1,
                                         statement.arguments.end()),
                   statement.line);
   }
   if (statement.arguments.size() != headerArguments)
      throw SyntaxError(statement.line, "unexpected arguments before string property block");
   std::optional<std::string> result;
   for (const auto &edit : ReadFieldEdits(statement))
   {
      if (!edit.name.empty() || edit.occurrence != 0)
         throw SyntaxError(edit.line,
                           "string property requires the unnamed field at occurrence zero");
      result = Value(edit.values, edit.line);
   }
   return result;
}
} // namespace DarkDbMod
