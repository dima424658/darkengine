#include "struct_fields.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstring>
#include <limits>
#include <locale>
#include <sstream>

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
std::string Text(const std::vector<Argument> &arguments)
{
   std::string text;
   for (size_t index = 0; index < arguments.size(); ++index)
   {
      if (index)
         text += ' ';
      text += arguments[index].text;
   }
   return text;
}
[[noreturn]] void Invalid(const std::string &message)
{
   throw std::runtime_error("DML field encoding: " + message);
}
uint64_t Integer(std::string text, size_t bytes, bool unsignedValue, bool hex, int baseOverride = 0)
{
   if (bytes != 1 && bytes != 2 && bytes != 4 && bytes != 8)
      Invalid("unsupported integer width");
   bool negative = false;
   if (!text.empty() && (text[0] == '-' || text[0] == '+'))
   {
      negative = text[0] == '-';
      text.erase(0, 1);
   }
   int base = hex ? 16 : (baseOverride ? baseOverride : 10);
   if (text.size() > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X'))
   {
      base = 16;
      text.erase(0, 2);
   }
   uint64_t magnitude = 0;
   auto parsed = std::from_chars(text.data(), text.data() + text.size(), magnitude, base);
   if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size())
      Invalid("expected complete integer value");
   const unsigned bits = unsigned(bytes * 8);
   const uint64_t maximum = unsignedValue ? (bits == 64 ? UINT64_MAX : (uint64_t(1) << bits) - 1)
                                          : (uint64_t(1) << (bits - 1)) - 1;
   if ((negative && unsignedValue) || magnitude > maximum + (negative ? 1 : 0))
      Invalid("integer is outside field range");
   return negative ? uint64_t(0) - magnitude : magnitude;
}
template <class T> std::vector<uint8_t> Bytes(T value)
{
   std::vector<uint8_t> output(sizeof(value));
   std::memcpy(output.data(), &value, sizeof(value));
   return output;
}
std::vector<uint8_t> IntegralBytes(uint64_t value, size_t bytes)
{
   switch (bytes)
   {
   case 1:
      return Bytes(uint8_t(value));
   case 2:
      return Bytes(uint16_t(value));
   case 4:
      return Bytes(uint32_t(value));
   case 8:
      return Bytes(value);
   default:
      Invalid("unsupported integer width");
   }
}
std::vector<uint8_t> Encode(const sFieldDesc &field, const std::vector<Argument> &arguments)
{
   const std::string text = Text(arguments);
   if (field.type == kFieldTypeString)
   {
      if (!field.size || text.size() >= field.size || text.find('\0') != std::string::npos)
         Invalid("string exceeds field capacity or contains NUL");
      std::vector<uint8_t> bytes(field.size, 0);
      std::memcpy(bytes.data(), text.data(), text.size());
      return bytes;
   }
   if (field.type == kFieldTypeInt || field.type == kFieldTypeShort)
      return IntegralBytes(
          Integer(text, field.size, field.flags & kFieldFlagUnsigned, field.flags & kFieldFlagHex),
          field.size);
   if (field.type == kFieldTypeBool)
   {
      const auto value = Lower(text);
      if (value == "true" || value == "yes" || value == "on" || value == "1")
         return IntegralBytes(1, field.size);
      if (value == "false" || value == "no" || value == "off" || value == "0")
         return IntegralBytes(0, field.size);
      Invalid("expected boolean value");
   }
   if (field.type == kFieldTypeEnum || field.type == kFieldTypeBits)
   {
      auto labels = static_cast<const char *const *>(field.data);
      if (field.datasize && !labels)
         Invalid("missing enum/bit label table");
      if (field.type == kFieldTypeEnum)
      {
         for (ulong index = 0; index < field.datasize; ++index)
         {
            if (labels[index] && Lower(labels[index]) == Lower(text))
               return IntegralBytes(
                   Integer(std::to_string(int64_t(field.min) + index), field.size, false, false),
                   field.size);
         }
         const uint64_t raw = Integer(text, field.size, false, field.flags & kFieldFlagHex);
         int64_t value = 0;
         // Convert the low-bit representation through the declared signed width.
         if (field.size == 1)
            value = int8_t(raw);
         else if (field.size == 2)
            value = int16_t(raw);
         else if (field.size == 4)
            value = int32_t(raw);
         else
            value = int64_t(raw);
         if (value < field.min || value > field.max)
            Invalid("enum value outside declared range");
         return IntegralBytes(raw, field.size);
      }
      if (Lower(text) == "none" || Lower(text) == "[none]" || text.empty())
         return IntegralBytes(0, field.size);
      if (!text.empty() && text[0] >= '0' && text[0] <= '9')
         return IntegralBytes(
             Integer(text, field.size, true, field.flags & kFieldFlagHex, field.datasize ? 10 : 2),
             field.size);
      uint64_t mask = 0;
      std::istringstream input(text);
      std::string name;
      while (std::getline(input, name, ','))
      {
         const auto first = name.find_first_not_of(" \t");
         const auto last = name.find_last_not_of(" \t");
         if (first == std::string::npos)
            Invalid("empty bit label");
         name = name.substr(first, last - first + 1);
         bool found = false;
         for (ulong index = 0; index < field.datasize; ++index)
         {
            if (labels[index] && Lower(labels[index]) == Lower(name))
            {
               const int64_t bit = int64_t(field.min) + index;
               if (bit < 0 || bit >= int64_t(field.size) * 8 || bit >= 64)
                  Invalid("bit label lies outside field width");
               mask |= uint64_t(1) << bit;
               found = true;
               break;
            }
         }
         if (!found)
            Invalid("unknown bit label: " + name);
      }
      return IntegralBytes(mask, field.size);
   }
   std::string numeric = text;
   for (char &c : numeric)
      if (c == ',')
         c = ' ';
   std::istringstream input(numeric);
   input.imbue(std::locale::classic());
   const bool fixed = field.type == kFieldTypeFix || field.type == kFieldTypeFixVec;
   const bool angle = field.type == kFieldTypeAng || field.type == kFieldTypeAngVec;
   const bool point = field.type == kFieldTypePoint;
   const bool vector = field.type == kFieldTypeVector || field.type == kFieldTypeDoubleVec ||
                       field.type == kFieldTypeFixVec || field.type == kFieldTypeAngVec;
   const bool wide = field.type == kFieldTypeDouble || field.type == kFieldTypeDoubleVec;
   const size_t count = point ? 2 : (vector ? 3 : 1);
   const size_t width = point ? field.size / 2 : (angle ? 2 : (wide ? 8 : 4));
   if (field.size != count * width || (point && width != 1 && width != 2 && width != 4))
      Invalid("numeric field storage size mismatch");
   std::vector<std::vector<uint8_t>> components;
   for (size_t index = 0; index < count; ++index)
   {
      if (point)
      {
         std::string integer;
         if (!(input >> integer))
            Invalid("expected integer point component");
         components.push_back(IntegralBytes(
             Integer(integer, width, field.flags & kFieldFlagUnsigned, field.flags & kFieldFlagHex),
             width));
         continue;
      }
      double value = 0;
      if (!(input >> value) || !std::isfinite(value))
         Invalid("expected finite numeric component");
      if (fixed)
      {
         const double scaled = value * 65536.0;
         if (!std::isfinite(scaled) || scaled < INT32_MIN || scaled > INT32_MAX)
            Invalid("fixed-point overflow");
         components.push_back(Bytes(int32_t(scaled)));
      }
      else if (angle)
      {
         const double units = std::floor(std::fmod(value, 360.0) * (65536.0 / 360.0) + 0.5);
         components.push_back(Bytes(uint16_t(int64_t(units))));
      }
      else
      {
         if (!wide && !std::isfinite(float(value)))
            Invalid("float overflow");
         components.push_back(wide ? Bytes(value) : Bytes(float(value)));
      }
   }
   if (!(input >> std::ws).eof())
      Invalid("unexpected trailing numeric data");
   // Text angles are heading/pitch/bank; mxs_angvec stores tx/ty/tz.
   if (field.type == kFieldTypeAngVec)
      std::reverse(components.begin(), components.end());
   std::vector<uint8_t> bytes;
   for (const auto &component : components)
      bytes.insert(bytes.end(), component.begin(), component.end());
   return bytes;
}
} // namespace
std::vector<FieldSchema> StructFieldSchema(const sStructDesc &description)
{
   if (description.nfields < 0 || (description.nfields && !description.fields))
      Invalid("invalid struct field table");
   std::vector<FieldSchema> result;
   for (int index = 0; index < description.nfields; ++index)
   {
      const auto field = description.fields[index];
      if (field.offset > description.size || field.size > description.size - field.offset)
         Invalid("native field extends beyond struct");
      size_t length = 0;
      while (length < sizeof(field.name) && field.name[length])
         ++length;
      FieldSchema schema{std::string(field.name, length), field.offset, field.size, {}};
      const bool supported = field.type == kFieldTypeInt || field.type == kFieldTypeShort ||
                             field.type == kFieldTypeBool || field.type == kFieldTypeEnum ||
                             field.type == kFieldTypeBits || field.type == kFieldTypeString ||
                             field.type == kFieldTypeFloat || field.type == kFieldTypeDouble ||
                             field.type == kFieldTypeVector || field.type == kFieldTypeDoubleVec ||
                             field.type == kFieldTypeFix || field.type == kFieldTypeFixVec ||
                             field.type == kFieldTypeAng || field.type == kFieldTypeAngVec ||
                             field.type == kFieldTypePoint;
      if (supported)
         schema.encode = [field](const std::vector<Argument> &values)
         { return Encode(field, values); };
      result.push_back(std::move(schema));
   }
   return result;
}
} // namespace DarkDbMod
