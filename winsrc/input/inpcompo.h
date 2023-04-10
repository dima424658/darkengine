///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/winsrc/input/RCS/inpcompo.h $
// $Author: TOML $
// $Date: 1996/11/05 14:28:19 $
// $Revision: 1.1 $
//
// Central storage for interfaces used by input library
//

#ifndef __INPCOMPO_H
#define __INPCOMPO_H

#include <comtools.h>

F_DECLARE_INTERFACE(IRecorder);
F_DECLARE_INTERFACE(IWinApp);
F_DECLARE_INTERFACE(IInputDevices);
F_DECLARE_INTERFACE(IGameShell);

EXTERN IRecorder *      g_pInputRecorder;
EXTERN IWinApp *        g_pInputWinApp;
EXTERN IInputDevices *  g_pInputDevices;
EXTERN IGameShell *     g_pInputGameShell;

void GetInputComponents();
void ReleaseInputComponents();

#endif /* !__INPCOMPO_H */
