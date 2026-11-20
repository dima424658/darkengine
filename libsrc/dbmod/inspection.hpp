#pragma once

namespace DarkDbMod
{
// Developer diagnostic only: resource-backed syntax/fingerprint/command-shape
// checks, with optional read-only matching against the loaded database.
// Property schema and live property-target inspection are also opt-in. Target
// inspection is a snapshot, not a simulation of earlier commands. No patches
// are executed.
bool InspectPatch(const char *filename, bool gamesysPatch, const char *gamesysName,
                  bool matchCurrentDatabase, bool validatePropertyFields, bool inspectPropertyTargets);
} // namespace DarkDbMod
