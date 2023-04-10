///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/gameshel/RCS/winput.h $
// $Author: DAVET $
// $Date: 1997/10/29 09:46:20 $
// $Revision: 1.10 $
//

#ifndef __WINPUT_H
#define __WINPUT_H

#include <indevapi.h>
#include <comconn.h>


///////////////////////////////////////////////////////////////////////////////

class cWinGameShell;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cWinInputDevices
//
// This class is always and only instantiated by cWinGameShell
//

class cWinInputDevices : public IInputDevices
{
public:
    cWinInputDevices(cWinGameShell * pOuter, IUnknown * pOuterUnknown);

    //
    // Handle a message from owning cWinGameShell instance
    //
    BOOL ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam, long & RetVal);

private:
    // IUnknown methods
    DECLARE_DELEGATION();

    // Holder/Sink methods
    DECLARE_CONNECTION_POINT(IPrimaryInputDevicesSink);

    // Query/Set mouse position, relative to game display surface
    STDMETHOD_(BOOL, GetMouseState)(int * x, int * y, int * pButtons);
    STDMETHOD_(BOOL, SetMousePos)(int x, int y);

    ///////////////////////////////////
    
    BOOL DispatchMouseEvent(UINT msg, WPARAM wParam, LPARAM lParam);

    BOOL WinKeyToLGKey(UINT msg, WPARAM wParam, LPARAM lParam, sInpKeyEvent & Result);
    BOOL WinMouseToLGMouse(UINT msg, WPARAM wParam, LPARAM lParam, sInpMouseEvent & Result);
    int  GetAsyncMouseButtonState();
    
    BOOL IsJapaneseNec98();

    ///////////////////////////////////

    cWinGameShell * m_pOuter;
    BOOL            m_fNEC98KeyBoard;

};

///////////////////////////////////////////////////////////////////////////////

#endif /* !__WINPUT_H */

