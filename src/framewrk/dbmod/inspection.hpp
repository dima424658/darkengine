#pragma once

namespace DarkDbMod
{
// Developer diagnostic only: resource-backed syntax/fingerprint/command-shape
// checks, with optional read-only matching against the loaded database.
// Does not validate engine property schemas or execute patches.
bool InspectPatch(const char *filename, bool gamesysPatch, const char *gamesysName,
                  bool matchCurrentDatabase);
} // namespace DarkDbMod
