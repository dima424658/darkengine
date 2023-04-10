///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/gameshel/RCS/wgscreat.cpp $
// $Author: TOML $
// $Date: 1996/12/13 16:40:54 $
// $Revision: 1.7 $
//

#include <windows.h>
#include <lg.h>
#include <comtools.h>
#include <gshelapi.h>
#include <wgshell.h>
#include <wgshelmt.h>

///////////////////////////////////////////////////////////////////////////////
//
// Game shell create method
//

tResult LGAPI _GameShellCreate(REFIID, IGameShell ** /*ppGameShell*/, IUnknown * pOuterUnknown, int fFlags, int /*msecBlockSignal*/)
{
    // Create the game shell.  Instance adds self to pOuterUnknown
    if (GetPrivateProfileInt("Threading", "Enabled", TRUE, "lg.ini") && ((fFlags & kMultithreadedShell) || GetPrivateProfileInt("WinShell", "MT", FALSE, "lg.ini")))
        return (new cMTWinGameShell(fFlags, pOuterUnknown) != 0) ? S_OK : E_FAIL;
    else
        return (new cWinGameShell(fFlags, pOuterUnknown) != 0) ? S_OK : E_FAIL;
}

///////////////////////////////////////////////////////////////////////////////
