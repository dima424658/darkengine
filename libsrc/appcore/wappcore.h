///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/appcore/RCS/wappcore.h $
// $Author: TOML $
// $Date: 1997/08/05 13:13:27 $
// $Revision: 1.18 $
//

#ifndef __WAPPCORE_H
#define __WAPPCORE_H

#include <comtools.h>
#include <comconn.h>
#include <appapi.h>
#include <wappapi.h>
#include <objcoll.h>

#include <str.h>

///////////////////////////////////////////////////////////////////////////////

F_DECLARE_INTERFACE(IAggregate);

///////////////////////////////////////////////////////////////////////////////
//
// class cWindowsApplication
//
// Implements IApplication and IWinApp
//

class cWindowsApplication
{
public:
    cWindowsApplication(int argc, const char *argv[],
                        const char * pszName,
                        IUnknown * pOuterUnknown);
private:
    //
    // Command line values
    //
    int             m_argc;
    const char **   m_argv;
    cStr            m_NameStr;

    ///////////////////////////////////////////////////////////////////////////
    //
    // class cWindowsApplication::cWinAppOperations
    //
    // Implements IWinApp on behalf of outer class
    //
    class cWinAppOperations : public IWinApp
    {
        DECLARE_DELEGATION();
    public:
        cWinAppOperations(cWindowsApplication * pWindowsApplication, IUnknown * pOuterUnknown)
          : m_pWindowsApplication(pWindowsApplication), m_fFlags(0)
        {
            INIT_DELEGATION(pOuterUnknown);
        }

        STDMETHOD_(HWND, GetMainWnd)();
        STDMETHOD_(HWND, SetMainWnd)(HWND);

        STDMETHOD_(HINSTANCE, GetInstance)();
        STDMETHOD_(HINSTANCE, GetResourceInstance)();

        STDMETHOD_(const char *, GetCommandLine)();
        STDMETHOD_(int,          GetCommandShow)();

        STDMETHOD_(void,         SetRegistryKey)(const char *);
        STDMETHOD_(const char *, GetRegistryKey)();

        STDMETHOD_(void,         SetProfileName)(const char *);
        STDMETHOD_(const char *, GetProfileName)();

        STDMETHOD_(void, SetDialogBkColor)(COLORREF clrCtlBk /* = RGB(192, 192, 192)*/,
                                           COLORREF clrCtlText /* = RGB(0, 0, 0)*/ );

        STDMETHOD_(eWinPumpResult, PumpEvents)(int fPumpFlags, eWinPumpDuration fDuration);

        STDMETHOD (ModalDialogBox)(LPCTSTR lpTemplate, DLGPROC lpDialogFunc);

        DECLARE_CONNECTION_POINT(IWinAppAdviseSink);

    private:
        eWinPumpResult DispatchOneMessage(MSG *, unsigned, unsigned);
        int PreTranslateMessage(MSG *);

        enum
        {
            kReceivedQuit = 0x01
        };

        HWND        m_hMainWnd;
        int         m_fFlags;

        cWindowsApplication * m_pWindowsApplication;
    };

    cWinAppOperations m_WinAppOperations;

    ///////////////////////////////////////////////////////////////////////////
    //
    // class cWindowsApplication::cApplicationOperations
    //
    // Implements IApplication on behalf of outer class
    //
    class cApplicationOperations : public IApplication
    {
        DECLARE_DELEGATION();
    public:
        cApplicationOperations(cWindowsApplication * pWindowsApplication, IUnknown * pOuterUnknown)
          : m_pWindowsApplication(pWindowsApplication)
        {
            INIT_DELEGATION(pOuterUnknown);
        }

        STDMETHOD (QueryQuit)();
        STDMETHOD_(void, Quit)();
        STDMETHOD_(void, Abort)(const char * pszReason);

        STDMETHOD_(void,         SetCaption)(const char *);
        STDMETHOD_(const char *, GetCaption)();

        STDMETHOD_(void,         SetDefaultFilePath)(const char *);
        STDMETHOD_(const char *, GetDefaultFilePath)();

        STDMETHOD_(const char *, GetFullName)();
        STDMETHOD_(const char *, GetPath)();

        STDMETHOD_(int, MessageBox)(const char * pszMessage, const char * pszCaption, int fFlags);
        STDMETHOD_(int, CriticalMessageBox)(const char * pszMessage, const char * pszCaption, int errorCode);

        DECLARE_CONNECTION_POINT(IAppAdviseSink);

        STDMETHOD_(void, AppCommand)(unsigned nCmdId);
       
    private:
        cWindowsApplication * m_pWindowsApplication;
    };

    cApplicationOperations m_ApplicationOperations;

    friend class cWinAppOperations;
    friend class cApplicationOperations;

    DECLARE_SIMPLE_AGGREGATE_CONTROL(cWindowsApplication);
};

#endif /* !__WAPPCORE_H */
