///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/dispdev/RCS/wdisptls.cpp $
// $Author: TOML $
// $Date: 1997/08/12 14:50:47 $
// $Revision: 1.3 $
//

#include <windows.h>

int AdjustWindow(HWND hwnd, int displayWidth, int displayHeight, BOOL fToDisplay)
{
    RECT rect;
    const long ulWindowStyle   = GetWindowLong(hwnd, GWL_STYLE);
    const long ulWindowExStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    int scaleFactor = 0;

    if (fToDisplay)
    {
        const int xOrigin = GetPrivateProfileInt("WinShell", "GDIOriginX", (GetSystemMetrics(SM_CXSCREEN) - displayWidth) / 2, "lg.ini");
        const int yOrigin = GetPrivateProfileInt("WinShell", "GDIOriginY", (GetSystemMetrics(SM_CYSCREEN) - displayHeight) / 2, "lg.ini");

        rect.left   = xOrigin;
        rect.top    = yOrigin;
        rect.right  = rect.left + displayWidth;
        rect.bottom = rect.top + displayHeight;
    }
    else
    {
        POINT originClient = { 0, 0 };

        GetClientRect(hwnd, &rect);

        int widthClient     = rect.right;
        int heightClient    = rect.bottom;
        int targetWidth     = displayWidth;
        int targetHeight    = displayHeight;

        while (targetWidth > widthClient || targetHeight > heightClient)
        {
            scaleFactor--;
            targetHeight >>= 1;
            targetWidth >>= 1;
        }

        ClientToScreen(hwnd, &originClient);

        rect.left   = originClient.x;
        rect.top    = originClient.y;
        rect.right  = rect.left + targetWidth;
        rect.bottom = rect.top + targetHeight;
    }

    AdjustWindowRectEx(&rect, ulWindowStyle, !!GetMenu(hwnd), ulWindowExStyle);

    MoveWindow(hwnd,
               rect.left, rect.top,
               rect.right - rect.left, rect.bottom - rect.top,
               TRUE);

    return scaleFactor;
}
