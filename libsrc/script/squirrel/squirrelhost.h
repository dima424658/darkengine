#pragma once

#ifndef __SQUIRRELHOST_H
#define __SQUIRRELHOST_H

#include <lg.h>
#include <scrptapi.h>

// Creates the engine-owned Squirrel 3.2 provider used in place of an external
// squirrel.osm.  The returned object implements the normal IScriptModule ABI,
// so the rest of the Dark script manager does not need a second lifecycle.
BOOL CreateSquirrelScriptModule(const char* moduleName,
                                IScriptMan* scriptManager,
                                tScriptPrintFunc print,
                                IScriptModule** module);

// Called from the game overlay pass while the software draw canvas is locked.
// It is a no-op when the native Squirrel provider is not loaded.
void SquirrelRenderOverlays(void);

#endif
