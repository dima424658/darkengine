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

// A no-argument Thief launch shows the engine-owned content panel before an
// FM has been chosen. Reserve fixed-capacity pools for the installed choices;
// the selected FM's complete configuration is still applied by the panel.
EXTERN void DarkContentPreparePickerBootstrap(void);

// Applies a one-time selection made by the in-engine startup picker. This
// reloads FM-local configuration and republishes the resolved content paths
// without replacing the process (and therefore without replacing its window).
EXTERN BOOL DarkContentApplySelection(const char* mode, const char* fmName);

EXTERN const char* DarkContentProfile(void);
EXTERN const char* DarkContentMode(void);
EXTERN const char* DarkContentBasePath(void);
EXTERN const char* DarkContentFMRoot(void);
// A recognized multi-install collection publishes its runnable campaign
// children here. Empty strings mean that the selected folder is a single
// installation rather than a collection.
EXTERN const char* DarkContentCollectionGoldPath(void);
EXTERN const char* DarkContentCollectionTDPPath(void);
EXTERN const char* DarkContentFMPath(void);
EXTERN const char* DarkContentFMName(void);
EXTERN const char* DarkContentSearchPath(eDarkContentPath path);
EXTERN BOOL DarkContentShowPicker(void);

// Savegames use the selected profile directory, except that Linux preserves
// a writable retail saves directory for compatibility with existing installs.
EXTERN const char* DarkContentSaveDirectory(void);
EXTERN BOOL DarkContentMakeSavePath(char* destination, int size,
                                    const char* filename);
// Resolves an existing save from the active save directory, then from the
// compatibility/fallback directory when no explicit override was supplied.
// If the file does not exist, returns the active save path so callers retain
// their normal failure reporting.
EXTERN BOOL DarkContentMakeLoadPath(char* destination, int size,
                                    const char* filename);

#endif
