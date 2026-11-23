#pragma once

// Windows SDK structures use the platform ABI, not Dark's packed-data ABI.
#ifdef _MSC_VER
#pragma pack(push, 8)
#endif
#include <windows.h>
#ifdef _MSC_VER
#pragma pack(pop)
#endif
