#pragma once

#include "syntax.hpp"
#include <cstdint>
#include <functional>

namespace DarkDbMod
{
struct FieldEdit
{
   std::string name;
   size_t occurrence = 0;
   std::vector<Argument> values;
   size_t line = 0;
};
struct FieldSchema
{
   std::string name;
   size_t offset = 0;
   size_t size = 0;
   // Engine adapters must validate complete values and return exactly size
   // bytes. Pointer-owning fields require a separate deep-copy implementation.
   std::function<std::vector<uint8_t>(const std::vector<Argument> &)> encode;
};
std::vector<FieldEdit> ReadFieldEdits(const Statement &dataBlock);
// Returns a staged copy only after every edit succeeds. Does not change the
// original record or publish partially parsed fields to the engine.
std::vector<uint8_t> StageFieldEdits(const std::vector<uint8_t> &original,
                                     const std::vector<FieldSchema> &schema,
                                     const std::vector<FieldEdit> &edits);
} // namespace DarkDbMod
