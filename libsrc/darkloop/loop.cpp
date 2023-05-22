
#include <appagg.h>
#include <gshelapi.h>
#include <loop.h>
#include <loopman.h>
#include <tmdecl.h>

IMPLEMENT_SIMPLE_AGGREGATION_SELF_DELETE(cLoop);

cLoop* cLoop::gm_pLoop = nullptr;

cLoop::cLoop(IUnknown* pOuterUnknown, cLoopManager* pLoopManager)
	: m_pLoopManager{ pLoopManager }
{
}

static bool DoneAtExit = false;

STDMETHODIMP_(int) cLoop::Go(sLoopInstantiator* loop)
{
    sLoopTransition trans{};
    trans.from.pID = &GUID_NULL;
    trans.to.pID = loop->pID;
    trans.to.minorMode = loop->minorMode;
    trans.to.init = loop->init;

    m_pLoopStack->Push({ m_pCurrentDispatch, m_FrameInfo, 128 });
    auto oldstack = m_pLoopStack;

    m_pLoopStack = new cLoopModeStack{};

    auto mode = m_pLoopManager->GetMode(loop->pID);
    if (!mode)
    {
        CriticalMsg("Attempted to \"go\" on mode that was never added");
        return E_FAIL;
    }

    mode->CreateDispatch(loop->init, &m_pNextDispatch);
    if (mode)
    {
        mode->Release();
        mode = nullptr;
    }

    if (!DoneAtExit)
    {
        atexit(cLoop::OnExit);
        DoneAtExit = true;
    }

    m_fNewMinorMode = loop->minorMode;
    m_fState |= 16;

    AutoAppIPtr(GameShell);

    auto frameMessage = kMsgNormalFrame;

    m_fState |= 0x21u;
    while (m_fState & 1)
    {
        if (m_fState & 0x20)
        {
            auto msgMode = kMsgEnterMode;
            if (m_fState & 0x80)
                msgMode = kMsgResumeMode;
            else
                m_pNextDispatch->ProcessQueue();

            m_pCurrentDispatch = m_pNextDispatch;
            m_pNextDispatch = nullptr;
            m_pCurrentDispatch->SetDiagnostics(m_fTempDiagnostics, m_tempDiagnosticSet);
            m_pCurrentDispatch->SetProfile(m_TempProfileSet, m_pTempProfileClientId);
            m_pCurrentDispatch->SendMessage(msgMode, reinterpret_cast<tLoopMessageData>(&trans), 1);

            m_fState = (m_fState & 0xFFFFFF00) + m_fState & 0x1F;
            m_FrameInfo.nTicks = tm_get_millisec();
            m_FrameInfo.dTicks = 0;
        }

        if (m_fState & 0x10)
        {
            m_FrameInfo.fMinorMode = m_fNewMinorMode;
            m_pCurrentDispatch->SendMessage(kMsgMinorModeChange, reinterpret_cast<tLoopMessageData>(&m_FrameInfo), 1);
            m_fState &= ~0x10u;
        }

        if (pGameShell != nullptr)
            pGameShell->BeginFrame();

        m_pCurrentDispatch->SendMessage(kMsgBeginFrame, reinterpret_cast<tLoopMessageData>(&m_FrameInfo), 1);
        if (pGameShell != nullptr)
            pGameShell->PumpEvents(0);

        m_pCurrentDispatch->SendMessage(frameMessage, reinterpret_cast<tLoopMessageData>(&m_FrameInfo), 1);
        if (pGameShell != nullptr)
            pGameShell->PumpEvents(0);

        m_pCurrentDispatch->ProcessQueue();
        m_pCurrentDispatch->SendMessage(kMsgEndFrame, reinterpret_cast<tLoopMessageData>(&m_FrameInfo), 2);
        if (pGameShell != nullptr)
            pGameShell->EndFrame();

        ++m_FrameInfo.nCount;

        auto millisec = tm_get_millisec();
        m_FrameInfo.dTicks = millisec - m_FrameInfo.nTicks;
        m_FrameInfo.nTicks = millisec;

        if (m_fState & 0xC)
        {
            if (m_fState & 4)
                frameMessage = kMsgPauseFrame;
            else
                frameMessage = kMsgNormalFrame;

            m_fState &= 0xFFFFFFF3;
            m_fState |= 2;
        }

        if ((m_fState & 0x20) != 0 || (m_fState & 1) == 0)
        {
            auto outmsg = kMsgExitMode;
            if (m_fState & 0x40)
                outmsg = kMsgSuspendMode;

            trans.from.pID = m_pCurrentDispatch->Describe(&trans.from.init)->pID;
            trans.from.minorMode = m_FrameInfo.fMinorMode;

            if (m_pNextDispatch)
            {
                trans.to.pID = m_pNextDispatch->Describe(&loop->init)->pID;
                trans.to.minorMode = m_fNewMinorMode;
            }
            else
            {
                trans.to.pID = &GUID_NULL;
            }

            m_pCurrentDispatch->SendMessage(outmsg, reinterpret_cast<tLoopMessageData>(&trans), 2);
            m_pCurrentDispatch->GetDiagnostics(&m_fTempDiagnostics, &m_tempDiagnosticSet);
            m_pCurrentDispatch->GetProfile(&m_TempProfileSet, &m_pTempProfileClientId);
            if ((m_fState & 0x40) == 0)
                m_pCurrentDispatch->Release();
        }
    }

    delete m_pLoopStack;
    m_pLoopStack = oldstack;

    sModeData pop{};
    oldstack->Pop(&pop);

    m_pCurrentDispatch = pop.dispatch;
    m_FrameInfo = pop.frame;

    return m_fGoReturn;
}

STDMETHODIMP_(HRESULT) cLoop::EndAllModes(int goRetVal)
{
    if (!m_pCurrentDispatch)
        return 1;

    m_fGoReturn = goRetVal;

    sModeData mode{ nullptr, {}, 128};
    m_pLoopStack->Pop(&mode);

    while (mode.dispatch != nullptr)
    {
        sLoopTransition trans{};

        trans.from.pID = mode.dispatch->Describe(&trans.from.init)->pID;
        trans.from.minorMode = mode.frame.fMinorMode;
        trans.to.pID = &GUID_NULL;

        mode.dispatch->SendMessage(kMsgExitMode, reinterpret_cast<tLoopMessageData>(&trans), 2);
        mode.dispatch->Release();

        m_pLoopStack->Pop(&mode);
    }

    m_fState &= 0xFFFFFFFE;

    return 0;
}

STDMETHODIMP_(HRESULT) cLoop::Terminate()
{
    if (m_pCurrentDispatch != nullptr)
    {
        sLoopTransition trans{};
        trans.from.pID = m_pCurrentDispatch->Describe(&trans.from.init)->pID;
        trans.from.minorMode = this->m_FrameInfo.fMinorMode;
        trans.to.pID = &GUID_NULL;

        m_pCurrentDispatch->SendMessage(kMsgExitMode, reinterpret_cast<tLoopMessageData>(&trans), 2);
        m_pCurrentDispatch->Release();
        m_pCurrentDispatch = nullptr;

        EndAllModes(0);
    }

    return S_OK;
}

STDMETHODIMP_(const sLoopFrameInfo*) cLoop::GetFrameInfo(void)
{
	return &m_FrameInfo;
}

STDMETHODIMP_(HRESULT) cLoop::ChangeMode(eLoopModeChangeKind kind, sLoopInstantiator* loop)
{
    if (!m_pCurrentDispatch)
        CriticalMsg("Changing modes outside GO");
    
    auto next = m_pLoopManager->GetMode(loop->pID);
    if (!next)
        CriticalMsg("Change to unknown loopmode");

    if (m_fState & 0x20)
    {
        if (loop->pID == m_pNextDispatch->Describe(nullptr)->pID)
            return S_FALSE;
    }

    if (!kind && (m_fState & 0xA0) == 160)
        return E_FAIL;

    if (m_fState & 0x20)
    {
        if (kind)
        {
            if (m_fState & 0x40)
            {
                sModeData result{};
                m_pLoopStack->Pop(&result);
                m_fState &= 0xFFFFFFBF;
            }

            if (m_pNextDispatch)
                m_pNextDispatch->Release();
            m_pNextDispatch = nullptr;
        }
        else
        {
            sLoopFrameInfo push_info{};
            push_info.fMinorMode = m_FrameInfo.fMinorMode;
            push_info.nCount = m_FrameInfo.nCount;
            push_info.nTicks = m_FrameInfo.nTicks;
            push_info.dTicks = m_FrameInfo.dTicks;
            push_info.fMinorMode = m_fNewMinorMode;

            m_pLoopStack->Push({ m_pNextDispatch, push_info, 0});

            kind = 1;
        }
    }

    ILoopDispatch* dispatch = nullptr;

    if (kind == 2)
    {
        sLoopTransition trans{};
        trans.to.pID = next->GetName()->pID;
        trans.to.minorMode = 0;
        trans.to.init = 0;

        sModeData mode{ nullptr, {}, 128 };
        m_pLoopStack->Pop(&mode);

        while (mode.dispatch != nullptr)
        {
            trans.from.pID = mode.dispatch->Describe(&trans.from.init)->pID;

            if (trans.from.pID == trans.to.pID)
            {
                dispatch = mode.dispatch;

                m_fNewMinorMode = mode.frame.fMinorMode;  
                m_fState |= 0x80;

                break;
            }

            trans.from.minorMode = mode.frame.fMinorMode;
            mode.dispatch->SendMessageA(0x8000, reinterpret_cast<tLoopMessageData>(&trans), 2);
            mode.dispatch->Release();

            m_pLoopStack->Pop(&mode);
        }
    }

    if (!dispatch)
    {
        next->CreateDispatch(loop->init, &dispatch);
        m_fNewMinorMode = loop->minorMode;
    }

    m_pNextDispatch = dispatch;
    m_fState |= 0x38;

    if (!kind)
    {
        m_pLoopStack->Push({ m_pCurrentDispatch, m_FrameInfo, 128 });
        m_fState |= 0x40u;
    }

    return S_OK;
}

STDMETHODIMP_(HRESULT) cLoop::EndMode(int goRetVal)
{
    if(!m_pCurrentDispatch)
        CriticalMsg("No loop mode to end!");

    m_fGoReturn = goRetVal;

    sModeData mode{};
    m_pLoopStack->Pop(&mode);

    if (mode.dispatch != nullptr)
    {
        m_pNextDispatch = mode.dispatch;
        m_fNewMinorMode = mode.frame.fMinorMode;
        m_fState |= 0x30;
        m_fState |= mode.flags;
    }
    else
    {
        m_fState &= ~0x1;
        m_fState |= 0x20u;
        m_pNextDispatch = nullptr;
    }

    return S_OK;
}

STDMETHODIMP_(ILoopMode*) cLoop::GetCurrentMode(void)
{
    if (m_pLoopManager == nullptr || m_pCurrentDispatch == nullptr)
        return nullptr;

    return m_pLoopManager->GetMode(m_pCurrentDispatch->Describe(nullptr)->pID);
}

STDMETHODIMP_(ILoopDispatch*) cLoop::GetCurrentDispatch(void)
{
	return m_pCurrentDispatch;
}

STDMETHODIMP_(void) cLoop::Pause(BOOL fPause)
{

    if (!fPause && IsPaused())
            m_fState |= 0x8;
    else if(fPause && !IsPaused())
        m_fState |= 0x4;
}

STDMETHODIMP_(BOOL) cLoop::IsPaused(void)
{
    return m_fState & 0x2;
}

STDMETHODIMP_(HRESULT) cLoop::ChangeMinorMode(int minorMode)
{
    if (m_FrameInfo.fMinorMode == minorMode)
        return 1;

    m_fNewMinorMode = minorMode;
    m_fState |= 0x10;
}

STDMETHODIMP_(int) cLoop::GetMinorMode(void)
{
    return m_FrameInfo.fMinorMode;
}

STDMETHODIMP_(HRESULT) cLoop::SendMessage(eLoopMessage message, tLoopMessageData hData, int flags)
{
    if (!m_pCurrentDispatch)
    {
        CriticalMsg("Cannot dispatch messages: no loop mode");
        return E_FAIL;
    }

    return m_pCurrentDispatch->SendMessage(message, hData, flags);
}

STDMETHODIMP_(HRESULT) cLoop::SendSimpleMessage(eLoopMessage message)
{
    if (!m_pCurrentDispatch)
    {
        CriticalMsg("Cannot dispatch messages: no loop mode");
        return E_FAIL;
    }

    return m_pCurrentDispatch->SendSimpleMessage(message);
}

STDMETHODIMP_(HRESULT) cLoop::PostMessage(eLoopMessage message, tLoopMessageData hData, int flags)
{
    if (!m_pCurrentDispatch)
    {
        CriticalMsg("Cannot dispatch messages: no loop mode");
        return E_FAIL;
    }

    return m_pCurrentDispatch->PostMessage(message, hData, flags);
}

STDMETHODIMP_(HRESULT) cLoop::PostSimpleMessage(eLoopMessage message)
{
    if (!m_pCurrentDispatch)
    {
        CriticalMsg("Cannot dispatch messages: no loop mode");
        return E_FAIL;
    }

    return m_pCurrentDispatch->PostSimpleMessage(message);
}

STDMETHODIMP_(HRESULT) cLoop::ProcessQueue(void)
{
    if (!m_pCurrentDispatch)
    {
        CriticalMsg("Cannot dispatch messages: no loop mode");
        return E_FAIL;
    }

    return m_pCurrentDispatch->ProcessQueue();
}

STDMETHODIMP_(void) cLoop::SetDiagnostics(uint fDiagnostics, tLoopMessageSet messages)
{
    if (m_pCurrentDispatch)
        m_pCurrentDispatch->SetDiagnostics(fDiagnostics, messages);
    else
    {

        m_fTempDiagnostics = fDiagnostics;
        m_tempDiagnosticSet = messages;
    }
}

STDMETHODIMP_(void) cLoop::GetDiagnostics(uint* pfDiagnostics, tLoopMessageSet* pMessages)
{
    if (m_pCurrentDispatch)
        m_pCurrentDispatch->GetDiagnostics(pfDiagnostics, pMessages);
    else
    {

        *pfDiagnostics = m_fTempDiagnostics;
        *pMessages = m_tempDiagnosticSet;
    }
}

STDMETHODIMP_(void) cLoop::SetProfile(tLoopMessageSet messages, tLoopClientID* pClientId)
{
    if (m_pCurrentDispatch)
        m_pCurrentDispatch->SetProfile(messages, pClientId);
    else
    {
        m_TempProfileSet = messages;
        m_pTempProfileClientId = pClientId;
    }
}

STDMETHODIMP_(void) cLoop::GetProfile(tLoopMessageSet* pMessages, tLoopClientID** ppClientId)
{
    if (m_pCurrentDispatch)
        m_pCurrentDispatch->GetProfile(pMessages, ppClientId);
    else
    {
        *pMessages = m_TempProfileSet;
        *ppClientId = m_pTempProfileClientId;
    }
}

void cLoop::OnExit()
{
    if (gm_pLoop && gm_pLoop->m_pCurrentDispatch)
    {
        gm_pLoop->m_pCurrentDispatch->Release();
        gm_pLoop->m_pCurrentDispatch = nullptr;
    }
}