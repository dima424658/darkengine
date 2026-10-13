#pragma once

#ifndef __CONTENTCTX_H
#define __CONTENTCTX_H

#include <lg.h>

#define DARK_CONTENT_MAX_PATH 260

enum eDarkContentPath
{
   kDarkContentResources,
   kDarkContentMissions,
   kDarkContentScripts,
   kDarkContentIncludes,
   kDarkContentMovies,
};

// Reads the launcher-selected installation before the game aggregate is made.
// This is intentionally separate from path setup because install.cfg has not
// been included yet at that point.
EXTERN void DarkContentConfigureProfile(void);

// Loads configuration files supplied by the selected FM after the retail
// game configuration, but before resource/UI objects are initialized.
EXTERN void DarkContentLoadFMConfig(int argc, const char* argv[]);

// Applies the active FM to Dark's legacy path variables after install.cfg and
// the game-specific includes have been processed.
EXTERN void DarkContentConfigure(int argc, const char* argv[]);

EXTERN const char* DarkContentProfile(void);
EXTERN const char* DarkContentMode(void);
EXTERN const char* DarkContentBasePath(void);
EXTERN const char* DarkContentFMPath(void);
EXTERN const char* DarkContentSearchPath(eDarkContentPath path);

// Savegames are deliberately outside both the retail install and the FM.
EXTERN const char* DarkContentSaveDirectory(void);
EXTERN BOOL DarkContentMakeSavePath(char* destination, int size,
                                    const char* filename);

#endif
