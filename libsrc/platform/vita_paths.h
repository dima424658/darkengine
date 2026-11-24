#pragma once

#include <stddef.h>

// Resolves an engine-relative resource through the Vita data roots.
// When for_write is true, a missing path is placed in the preferred root.
// roots may be supplied by DARK_VITA_ASSET_ROOTS (colon-separated).
bool PlatformResolveVitaAssetPath(const char *path, char *resolved,
                                  size_t capacity, bool for_write,
                                  bool *exists);
