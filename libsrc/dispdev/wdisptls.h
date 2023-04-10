///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/dispdev/RCS/wdisptls.h $
// $Author: TOML $
// $Date: 1996/11/18 12:42:15 $
// $Revision: 1.2 $
//

#ifndef __WDISPTLS_H
#define __WDISPTLS_H

// Given a window and a virtual screen size, resizes the window to a perfect
// scale of 2 of the screen size, returning the scale factor.  A negative
// scale indicates the window client area is smaller than desired, a zero
// return means client area is same as display size (toml 11-14-96)
int AdjustWindow(HWND hwnd, int displayWidth, int displayHeight, BOOL fToDisplay = FALSE);

#endif /* !__WDISPTLS_H */
