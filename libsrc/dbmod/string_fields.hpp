#pragma once
#include "syntax.hpp"
#include <optional>

namespace DarkDbMod
{
// Owns parsed text independently of DML/resource buffers. An empty block means
// no edit; a quoted empty value is a present empty string. Only the singleton
// unnamed field (occurrence zero) is valid for IStringProperty.
std::optional<std::string> ReadStringPropertyValue(const Statement &statement,
                                                   size_t headerArguments);
} // namespace DarkDbMod
