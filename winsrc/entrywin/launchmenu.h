#pragma once

enum eDarkLaunchChoice
{
    kDarkLaunchCancel = 0,
    kDarkLaunchConfigured,
    kDarkLaunchVanilla,
    kDarkLaunchFanMission,
    kDarkLaunchChooseGame,
};

// Runs before the Dark application aggregate exists, so this menu must own no
// engine resources.  The selected content is applied before install.cfg and
// the resource manager are initialized.
int DarkShowLaunchMenu(const char* directory, const char* profile,
                       const char* executable, const char* fmRoot,
                       bool collection, char* gameDirectory,
                       int gameDirectorySize, char* fmName, int fmNameSize);
