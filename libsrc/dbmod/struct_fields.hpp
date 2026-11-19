#pragma once
#include <types.h>
#include "fields.hpp"
#include <sdesbase.h>

namespace DarkDbMod
{
// Copies schema metadata into strict staging encoders. The schema's enum/bit
// label tables must outlive the returned encoders, as with the engine registry.
// Unsupported types have no encoder and cannot be staged. Pointer fields are
// never dereferenced or modified by this adapter.
std::vector<FieldSchema> StructFieldSchema(const sStructDesc &description);
} // namespace DarkDbMod
