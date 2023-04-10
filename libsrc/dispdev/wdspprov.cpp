///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/dispdev/RCS/wdspprov.cpp $
// $Author: TOML $
// $Date: 1997/05/23 14:17:45 $
// $Revision: 1.7 $
//

#include <windows.h>

#include <lg.h>
#include <comtools.h>
#include <appagg.h>

#include <wappapi.h>
#include <dispapi.h>

#include <wdisp.h>
#include <wdspprov.h>

#include <dev2d.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cWinDisplayProvider
//
// All defaults
//

cWinDisplayProvider::cWinDisplayProvider(cWinDisplayDevice * pDisplayDevice)
  : m_pDisplayDevice(pDisplayDevice)
{
    m_pWinApp = AppGetObj(IWinApp);
}

///////////////////////////////////////

cWinDisplayProvider::~cWinDisplayProvider()
{
}


///////////////////////////////////////

BOOL cWinDisplayProvider::DoProcessMessage(UINT, WPARAM, LPARAM, long *)
{
    return 0;
}


///////////////////////////////////////

BOOL cWinDisplayProvider::DoGetDirectDraw(IDirectDraw ** ppDD)
{
    *ppDD = NULL;
    return FALSE;
}

///////////////////////////////////////

BOOL cWinDisplayProvider::DoGetBitmapSurface(sGrBitmap * pBm, IDirectDrawSurface ** ppDDS)
{
    *ppDDS = NULL;
    return FALSE;
}

///////////////////////////////////////

HWND cWinDisplayProvider::GetMainWnd()
{
    return m_pWinApp->GetMainWnd();
}

///////////////////////////////////////////////////////////////////////////////
