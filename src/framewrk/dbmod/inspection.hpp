#pragma once

namespace DarkDbMod
{
// Developer diagnostic only: resource-backed syntax/fingerprint/command-shape
// checks, with optional read-only matching against the loaded database.
// Property schema validation is also opt-in; no patches are executed.
bool InspectPatch(const char *filename, bool gamesysPatch, const char *gamesysName,
                  bool matchCurrentDatabase, bool validatePropertyFields);
} // namespace DarkDbMod
