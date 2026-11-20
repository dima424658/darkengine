#ifdef _WIN32
#include <initguid.h>
#else
/**
 * Adapted from mingw-w64 headers, revision 93f3505a758fe70e56678f00e753af3bc4f640bb.
 * Modified for Dark Engine native POSIX/Windows builds on 2026-11-20.
 * See COPYING.mingw-w64 and DISCLAIMER in this directory.
 */
#define INITGUID
#include "guiddef.h"

#endif
