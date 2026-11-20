#pragma once
#include <comtools.h>

// Main-thread native read-only module presence query; no guest vtable slots are added.
// S_OK = loaded, S_FALSE = absent, failure = unavailable/invalid query.
EXTERN_C HRESULT ScriptModulePresence(const char *name);
