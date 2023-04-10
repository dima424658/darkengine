///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/recorder/RCS/recprmpt.cpp $
// $Author: TOML $
// $Date: 1996/11/06 12:12:12 $
// $Revision: 1.2 $
//
// This is in a seperate file to reduce depondence on windows.h (toml 10-29-96)

#ifdef _WIN32
#include <windows.h>
#else
#include <stdio.h>
#endif

#include <recprmpt.h>

BOOL RecPromptYesNo(const char * pszPrompt)
{
#ifdef _WIN32
    return (MessageBox(NULL, pszPrompt, "Recorder", MB_YESNO | MB_ICONHAND | MB_SYSTEMMODAL) == IDYES);
#else
// @TBD (toml 10-29-96): do a real prompting here
    printf(pszPrompt);
    return TRUE;
#endif
}

