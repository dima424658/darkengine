///////////////////////////////////////////////////////////////////////////////
//
// defehand.cpp
//
// Routes events into input library queues
//

#include <windows.h>
#include <kb.h>
#include <mouse.h>
#include <defehand.h>
#include <comtools.h>
#include <indevapi.h>
#include <inpcompo.h>
#include <appagg.h>

extern "C" bool kb_add_event(kbs_event new_event);

///////////////////////////////////////////////////////////////////////////////

class cDefaultInputDevicesSink : public IPrimaryInputDevicesSink
{
    DECLARE_UNAGGREGATABLE();

public:
    cDefaultInputDevicesSink();
    ~cDefaultInputDevicesSink();

    STDMETHOD_(short, GetVersion)();
    STDMETHOD (OnKey)(const sInpKeyEvent * pEvent);
    STDMETHOD (OnMouse)(const sInpMouseEvent * pEvent);

private:
    HANDLE  m_hMainThread;
    DWORD   m_dwMainThreadId;
};

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_UNAGGREGATABLE_NO_FINAL_RELEASE(cDefaultInputDevicesSink, IPrimaryInputDevicesSink);

///////////////////////////////////////

cDefaultInputDevicesSink::cDefaultInputDevicesSink()
{
    m_dwMainThreadId = GetCurrentThreadId();
    Verify(DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &m_hMainThread, 0, FALSE, DUPLICATE_SAME_ACCESS));
}

///////////////////////////////////////

cDefaultInputDevicesSink::~cDefaultInputDevicesSink()
{
    if (m_hMainThread)
        CloseHandle(m_hMainThread);
}

///////////////////////////////////////

STDMETHODIMP_(short) cDefaultInputDevicesSink::GetVersion()
{
    return kVerInputDeviceAdvise;
}

///////////////////////////////////////

STDMETHODIMP cDefaultInputDevicesSink::OnKey(const sInpKeyEvent * pEvent)
{
    kb_add_event(*pEvent);
    return NOERROR;
}

///////////////////////////////////////

STDMETHODIMP cDefaultInputDevicesSink::OnMouse(const sInpMouseEvent * pEvent)
{
    mouse_wait_for_queue_mutex();
    int iPreviousPriority;

    if (GetCurrentThreadId() != m_dwMainThreadId)
    {
        iPreviousPriority = GetThreadPriority(GetCurrentThread());
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
        SuspendThread(m_hMainThread);
    }

    mouse_generate(*pEvent);

    if (GetCurrentThreadId() != m_dwMainThreadId)
    {
        ResumeThread(m_hMainThread);
        SetThreadPriority(GetCurrentThread(), iPreviousPriority);
    }

    mouse_release_queue_mutex();

    return NOERROR;
}

///////////////////////////////////////////////////////////////////////////////

static cDefaultInputDevicesSink g_DefaultInputDevicesSink;
static DWORD                    g_InputDevicesSinkCookie;

///////////////////////////////////////

void LGAPI ConnectInput()
{
    if (!g_InputDevicesSinkCookie && g_pInputDevices)
    {
        g_pInputDevices->Advise(&g_DefaultInputDevicesSink, &g_InputDevicesSinkCookie);
    }
}

///////////////////////////////////////

void LGAPI DisconnectInput()
{
    if (g_InputDevicesSinkCookie && g_pInputDevices)
    {
        g_pInputDevices->Unadvise(g_InputDevicesSinkCookie);
        g_InputDevicesSinkCookie = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////

