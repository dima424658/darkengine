#pragma once
#include "fingerprint.hpp"

namespace DarkDbMod
{
// Read-only adapters for the loaded database. gamesysName must be the actual
// loaded gamesys identity, not merely a configuration preference.
// Script/module and mod-version adapters are not supplied yet; rules needing
// missing adapters fail explicitly in the evaluator.
FingerprintContext MakeEngineFingerprintContext(const std::string &gamesysName);
} // namespace DarkDbMod
