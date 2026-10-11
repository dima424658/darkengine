// Native Dark Engine adapters for portable 32-bit OSM modules.

#ifndef __OSM32MOD_H
#define __OSM32MOD_H

#include <lg.h>
#include <scrptapi.h>

#pragma pack(push, 8)
#include <string>
#pragma pack(pop)

BOOL LoadOsm32Module(const char *path, const char *moduleName,
                     IScriptMan *scriptManager,
                     IScriptModule **module, std::string *error);

// Creates a script when descriptor belongs to a portable OSM. `handled` is
// false for an ordinary native descriptor, allowing the original factory path.
IScript *CreateOsm32Script(const sScrClassDesc *descriptor,
                           const char *className, ObjID objectId,
                           BOOL *handled);

#endif // __OSM32MOD_H
