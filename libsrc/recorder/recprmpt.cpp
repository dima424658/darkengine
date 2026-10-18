///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/recorder/RCS/recprmpt.cpp $
// $Author: TOML $
// $Date: 1996/11/06 12:12:12 $
// $Revision: 1.2 $
//
// This is in a seperate file to reduce depondence on windows.h (toml 10-29-96)

#include <recprmpt.h>
#include <platform_services.h>

BOOL RecPromptYesNo(const char * pszPrompt)
{
    return PlatformAskYesNo("Recorder", pszPrompt);
}

