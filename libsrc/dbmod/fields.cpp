#include "fields.hpp"
#include <algorithm>
#include <charconv>

namespace DarkDbMod
{
namespace
{
std::string Lower(std::string value)
{
   for (char &c : value)
      if (c >= 'A' && c <= 'Z')
         c += 'a' - 'A';
   return value;
}
} // namespace
std::vector<FieldEdit> ReadFieldEdits(const Statement &dataBlock)
{
   if (!dataBlock.hasBlock)
      throw SyntaxError(dataBlock.line, "expected field data block");
   std::vector<FieldEdit> edits;
   for (const auto &field : dataBlock.fields)
   {
      if (field.hasBlock || field.arguments.size() < 2)
         throw SyntaxError(field.line, "expected field name and value on the same line");
      FieldEdit edit;
      edit.line = field.line;
      edit.name = field.arguments[0].text;
      if (!field.arguments[0].quoted && edit.name.find_first_of("[]") != std::string::npos)
         throw SyntaxError(field.line, "indexed field names must be quoted before [index]");
      size_t firstValue = 1;
      const auto &index = field.arguments[1];
      if (!index.quoted && !index.text.empty() && index.text.front() == '[')
      {
         if (index.text.size() < 3 || index.text.back() != ']')
            throw SyntaxError(field.line, "expected nonnegative field occurrence [index]");
         const char *begin = index.text.data() + 1;
         const char *end = index.text.data() + index.text.size() - 1;
         const auto parsed = std::from_chars(begin, end, edit.occurrence);
         if (parsed.ec != std::errc{} || parsed.ptr != end)
            throw SyntaxError(field.line, "invalid field occurrence index");
         firstValue = 2;
      }
      if (firstValue == field.arguments.size())
         throw SyntaxError(field.line, "missing field value");
      edit.values.assign(field.arguments.begin() + firstValue, field.arguments.end());
      edits.push_back(std::move(edit));
   }
   return edits;
}
std::vector<uint8_t> StageFieldEdits(const std::vector<uint8_t> &original,
                                     const std::vector<FieldSchema> &schema,
                                     const std::vector<FieldEdit> &edits)
{
   auto staged = original;
   for (const auto &edit : edits)
   {
      const FieldSchema *selected = nullptr;
      size_t occurrence = 0;
      for (const auto &field : schema)
      {
         if (Lower(field.name) != Lower(edit.name))
            continue;
         if (occurrence++ == edit.occurrence)
         {
            selected = &field;
            break;
         }
      }
      if (!selected)
         throw SyntaxError(edit.line, "unknown field or occurrence: " + edit.name);
      if (selected->offset > staged.size() || selected->size > staged.size() - selected->offset)
         throw SyntaxError(edit.line, "field storage lies outside the record: " + edit.name);
      if (!selected->encode)
         throw SyntaxError(edit.line, "unsupported field encoding: " + edit.name);
      std::vector<uint8_t> encoded;
      try
      {
         encoded = selected->encode(edit.values);
      }
      catch (const SyntaxError &)
      {
         throw;
      }
      catch (const std::exception &error)
      {
         throw SyntaxError(edit.line, error.what());
      }
      if (encoded.size() != selected->size)
         throw SyntaxError(edit.line, "field encoder returned an invalid byte count: " + edit.name);
      std::copy(encoded.begin(), encoded.end(), staged.begin() + selected->offset);
   }
   return staged;
}
} // namespace DarkDbMod
