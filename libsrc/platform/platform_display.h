#pragma once

#include <types.h>

// Returns the platform text scale relative to its baseline desktop density.
// Implementations include the active display scale and, where the platform
// exposes it separately, the user's font-size preference. The shared UI uses
// this value only for text; game geometry and input coordinates are unchanged.
EXTERN float PlatformGetTextScale(void);
