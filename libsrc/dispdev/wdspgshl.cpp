///////////////////////////////////////////////////////////////////////////////
// $Header: x:/prj/tech/libsrc/dispdev/RCS/wdspgshl.cpp 1.1 1997/09/10 14:00:42 TOML Exp $
//
// This file isolates the display device from dependence on gshelapi.h
//

#include <comtools.h>
#include <gshelapi.h>
#include <appagg.h>

///////////////////////////////////////////////////////////////////////////////

void WinDispPumpEvents()
{
    AutoAppIPtr(GameShell);
    pGameShell->PumpEvents(kPumpAll);
}

///////////////////////////////////////

void WinDispBringToFront()
{
    AutoAppIPtr(GameShell);
    pGameShell->BringToFront();
}

///////////////////////////////////////////////////////////////////////////////
