#pragma once

namespace DarkDbMod
{
// Developer diagnostic only: resource-backed syntax/fingerprint/command-shape
// checks. Does not match a mission, validate engine schemas or execute patches.
bool InspectPatch(const char *filename, bool gamesysPatch);
} // namespace DarkDbMod
