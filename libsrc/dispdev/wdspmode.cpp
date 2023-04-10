///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/dispdev/RCS/wdspmode.cpp $
// $Author: KEVIN $
// $Date: 1997/10/21 16:18:05 $
// $Revision: 1.7 $
//

#include <windows.h>

#include <lg.h>
#include <comtools.h>
#include <appagg.h>

#include <wappapi.h>
#include <dispapi.h>

#include <wdisp.h>
#include <wdspmode.h>

#include <dev2d.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cWinDisplayModeOperations
//
// All defaults
//

///////////////////////////////////////

cWinDisplayModeOperations::~cWinDisplayModeOperations()
{
}

///////////////////////////////////////

BOOL cWinDisplayModeOperations::OnTaskSwitch(BOOL)
{
   return FALSE;
}


///////////////////////////////////////

eGrDispMode cWinDisplayModeOperations::DoGetMode(sGrModeInfoEx * pRetModeInfo)
{
    if (pRetModeInfo)
        *pRetModeInfo = m_ModeInfoEx;

    return m_ModeInfoEx.mode;
}

///////////////////////////////////////

BOOL cWinDisplayModeOperations::DoStatHTrace()
{
    return FALSE;
}

///////////////////////////////////////

BOOL cWinDisplayModeOperations::DoStatVTrace()
{
    return FALSE;
}

///////////////////////////////////////

BOOL cWinDisplayModeOperations::DoSetPalette(LOGPALETTE &, unsigned, unsigned)
{
    return TRUE;
}

///////////////////////////////////////

BOOL cWinDisplayModeOperations::DoPageFlip()
{
    return FALSE;
}

///////////////////////////////////////

void cWinDisplayModeOperations::DoFlush()
{
}

///////////////////////////////////////

void cWinDisplayModeOperations::DoFlushRect(int /*x0*/, int /*y0*/, int /*x1*/, int /*y1*/)
{
    DoFlush();
}

///////////////////////////////////////////////////////////////////////////////
