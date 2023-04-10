///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/winsrc/input/RCS/inpcompo.cpp $
// $Author: TOML $
// $Date: 1996/11/05 14:28:39 $
// $Revision: 1.1 $
//

#include <comtools.h>
#include <appagg.h>
#include <inpcompo.h>

IRecorder *      g_pInputRecorder;
IWinApp *        g_pInputWinApp;
IInputDevices *  g_pInputDevices;
IGameShell *     g_pInputGameShell;

void GetInputComponents()
{
    if (!g_pInputRecorder)
        g_pInputRecorder = AppGetObj(IRecorder);
    if (!g_pInputWinApp)
        g_pInputWinApp = AppGetObj(IWinApp);
    if (!g_pInputDevices)
        g_pInputDevices = AppGetObj(IInputDevices);
    if (!g_pInputGameShell)
        g_pInputGameShell = AppGetObj(IGameShell);
}

void ReleaseInputComponents()
{
    SafeRelease(g_pInputRecorder);
    SafeRelease(g_pInputWinApp);
    SafeRelease(g_pInputDevices);
    SafeRelease(g_pInputGameShell);
}

