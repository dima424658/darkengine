#pragma once
#include "fingerprint.hpp"

namespace DarkDbMod
{
// Read-only adapters for the loaded database. gamesysName must be the actual
// loaded gamesys identity, not merely a configuration preference.
// Mod/version adapters are not supplied yet; rules needing missing adapters
// fail explicitly in the evaluator. Script presence queries the loaded-module table.
FingerprintContext MakeEngineFingerprintContext(const std::string &gamesysName);
} // namespace DarkDbMod
