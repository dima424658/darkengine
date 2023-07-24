
#include <appagg.h>
#include <gshelapi.h>
#include <loop.h>
#include <loopman.h>
#include <tmdecl.h>

IMPLEMENT_DELEGATION(cLoop);

cLoop* cLoop::gm_pLoop = nullptr;

cLoop::cLoop(IUnknown* pOuterUnknown, cLoopManager* pLoopManager)
	: m_pCurrentDispatch{ nullptr },
	m_fState{ 0 },
	m_FrameInfo{},
	m_fGoReturn{ 0 },
	m_pLoopManager{ pLoopManager },
	m_pLoopStack{ new cLoopModeStack{} },
	m_pNextDispatch{ nullptr },
	m_fTempDiagnostics{ 0 },
	m_tempDiagnosticSet{ 0 },
	m_TempProfileSet{ 0 },
	m_pTempProfileClientId{ nullptr }
{
	INIT_DELEGATION(pOuterUnknown);

	gm_pLoop = this;
}

cLoop::~cLoop()
{
	delete m_pLoopStack;
	m_pLoopStack = nullptr;

	if (m_pCurrentDispatch)
		CriticalMsg("Expected exit of loop manager before destruction!");
}

int cLoop::Go(sLoopInstantiator* loop)
{
	sLoopTransition trans{};
	trans.from.pID = &GUID_NULL;
	trans.to = *loop;

	m_pLoopStack->Push({ m_pCurrentDispatch, m_FrameInfo, 128 });
	auto oldstack = m_pLoopStack;

	m_pLoopStack = new cLoopModeStack{};

	auto pMode = m_pLoopManager->GetMode(loop->pID);
	if (!pMode)
	{
		CriticalMsg("Attempted to \"go\" on mode that was never added");
		return E_FAIL;
	}

	pMode->CreateDispatch(loop->init, &m_pNextDispatch);
	if (pMode)
	{
		pMode->Release();
		pMode = nullptr;
	}

	static auto DoneAtExit = false;
	if (!DoneAtExit)
	{
		atexit(cLoop::OnExit);
		DoneAtExit = true;
	}

	m_fNewMinorMode = loop->minorMode;
	m_fState |= 0x10;

	AutoAppIPtr(GameShell);

	auto frameMessage = kMsgNormalFrame;

	m_fState |= 0x20 | 0x01;
	while (m_fState & 0x01)
	{
		if (m_fState & 0x20)
		{
			auto inmsg = kMsgEnterMode;
			if (m_fState & 0x80)
				inmsg = kMsgResumeMode;
			else
				m_pNextDispatch->ProcessQueue();

			m_pCurrentDispatch = m_pNextDispatch;
			m_pNextDispatch = nullptr;
			m_pCurrentDispatch->SetDiagnostics(m_fTempDiagnostics, m_tempDiagnosticSet);
			m_pCurrentDispatch->SetProfile(m_TempProfileSet, m_pTempProfileClientId);
			m_pCurrentDispatch->SendMessage(inmsg, reinterpret_cast<tLoopMessageData>(&trans), 1);
			
			m_fState &= ~(0x80 | 0x20 | 0x10);
			m_FrameInfo.nTicks = tm_get_millisec();
			m_FrameInfo.dTicks = 0;
		}

		if (m_fState & 0x10)
		{
			m_FrameInfo.fMinorMode = m_fNewMinorMode;
			m_pCurrentDispatch->SendMessage(kMsgMinorModeChange, reinterpret_cast<tLoopMessageData>(&m_FrameInfo), kDispatchForward);
			m_fState &= ~0x10;
		}

		if (pGameShell != nullptr)
			pGameShell->BeginFrame();

		m_pCurrentDispatch->SendMessage(kMsgBeginFrame, reinterpret_cast<tLoopMessageData>(&m_FrameInfo), kDispatchForward);
		if (pGameShell != nullptr)
			pGameShell->PumpEvents(0);

		m_pCurrentDispatch->SendMessage(frameMessage, reinterpret_cast<tLoopMessageData>(&m_FrameInfo), kDispatchForward);
		if (pGameShell != nullptr)
			pGameShell->PumpEvents(0);

		m_pCurrentDispatch->ProcessQueue();
		m_pCurrentDispatch->SendMessage(kMsgEndFrame, reinterpret_cast<tLoopMessageData>(&m_FrameInfo), kDispatchReverse);
		if (pGameShell != nullptr)
			pGameShell->EndFrame();

		++m_FrameInfo.nCount;

		auto ticks = tm_get_millisec();
		m_FrameInfo.dTicks = ticks - m_FrameInfo.nTicks;
		m_FrameInfo.nTicks = ticks;

		if (m_fState & 0x0C)
		{
			if (m_fState & 0x04)
				frameMessage = kMsgPauseFrame;
			else
				frameMessage = kMsgNormalFrame;

			m_fState &= ~0x0C;
			m_fState |= 0x02;
		}

		if ((m_fState & 0x20) || !(m_fState & 1))
		{
			auto outmsg = kMsgExitMode;
			if (m_fState & 0x40)
				outmsg = kMsgSuspendMode;

			trans.from.pID = m_pCurrentDispatch->Describe(&trans.from.init)->pID;
			trans.from.minorMode = m_FrameInfo.fMinorMode;

			if (m_pNextDispatch)
			{
				trans.to.pID = m_pNextDispatch->Describe(&trans.to.init)->pID;
				trans.to.minorMode = m_fNewMinorMode;
			}
			else
			{
				trans.to.pID = &GUID_NULL;
			}

			m_pCurrentDispatch->SendMessage(outmsg, reinterpret_cast<tLoopMessageData>(&trans), 2);
			m_pCurrentDispatch->GetDiagnostics(&m_fTempDiagnostics, &m_tempDiagnosticSet);
			m_pCurrentDispatch->GetProfile(&m_TempProfileSet, &m_pTempProfileClientId);
			if (!(m_fState & 0x40))
				m_pCurrentDispatch->Release();
		}
	}

	delete m_pLoopStack;
	m_pLoopStack = oldstack;

	sModeData old{};
	oldstack->Pop(&old);

	m_pCurrentDispatch = old.dispatch;
	m_FrameInfo = old.frame;

	return m_fGoReturn;
}

HRESULT cLoop::EndAllModes(int goRetVal)
{
	if (!m_pCurrentDispatch)
		return S_FALSE;

	m_fGoReturn = goRetVal;

	sModeData mode{ nullptr, {}, 128 };
	m_pLoopStack->Pop(&mode);

	while (mode.dispatch)
	{
		sLoopTransition trans{};

		trans.from.pID = mode.dispatch->Describe(&trans.from.init)->pID;
		trans.from.minorMode = mode.frame.fMinorMode;
		trans.to.pID = &GUID_NULL;

		mode.dispatch->SendMessage(kMsgExitMode, reinterpret_cast<tLoopMessageData>(&trans), 2);
		mode.dispatch->Release();

		m_pLoopStack->Pop(&mode);
	}

	m_fState &= ~0x1;

	return S_OK;
}

HRESULT cLoop::Terminate()
{
	if (m_pCurrentDispatch)
	{
		sLoopTransition trans{};
		trans.from.pID = m_pCurrentDispatch->Describe(&trans.from.init)->pID;
		trans.from.minorMode = m_FrameInfo.fMinorMode;
		trans.to.pID = &GUID_NULL;

		m_pCurrentDispatch->SendMessage(kMsgExitMode, reinterpret_cast<tLoopMessageData>(&trans), 2);
		m_pCurrentDispatch->Release();
		m_pCurrentDispatch = nullptr;

		EndAllModes(0);
	}

	return S_OK;
}

const sLoopFrameInfo* cLoop::GetFrameInfo(void)
{
	return &m_FrameInfo;
}

HRESULT cLoop::ChangeMode(eLoopModeChangeKind kind, sLoopInstantiator* loop)
{
	if (!m_pCurrentDispatch)
		CriticalMsg("Changing modes outside GO");

	auto next = m_pLoopManager->GetMode(loop->pID);
	if (!next)
		CriticalMsg("Change to unknown loopmode");

	if (m_fState & 0x20)
	{
		if (*loop->pID == *m_pNextDispatch->Describe(nullptr)->pID)
			return S_FALSE;
	}

	if (!kind && (m_fState & 0xA0) == 0xA0)
		return E_FAIL;

	if (m_fState & 0x20)
	{
		if (kind)
		{
			if (m_fState & 0x40)
			{
				sModeData result{};
				m_pLoopStack->Pop(&result);
				m_fState &= ~0x40;
			}

			if (m_pNextDispatch)
			{
				m_pNextDispatch->Release();
				m_pNextDispatch = nullptr;
			}
		}
		else
		{
			sLoopFrameInfo push_info = m_FrameInfo;
			push_info.fMinorMode = m_fNewMinorMode;

			m_pLoopStack->Push({ m_pNextDispatch, push_info, 0 });

			kind = kLoopModeSwitch;
		}
	}

	ILoopDispatch* dispatch = nullptr;

	if (kind == kLoopModeUnwindTo)
	{
		sLoopTransition trans{};
		trans.to.pID = next->GetName()->pID;
		trans.to.minorMode = 0;
		trans.to.init = nullptr;

		sModeData mode{ nullptr, {}, 128 };
		m_pLoopStack->Pop(&mode);

		while (mode.dispatch)
		{
			trans.from.pID = mode.dispatch->Describe(&trans.from.init)->pID;

			if (*trans.from.pID == *trans.to.pID)
			{
				dispatch = mode.dispatch;

				m_fNewMinorMode = mode.frame.fMinorMode;
				m_fState |= 0x80;

				break;
			}

			trans.from.minorMode = mode.frame.fMinorMode;
			mode.dispatch->SendMessage(kMsgExitMode, reinterpret_cast<tLoopMessageData>(&trans), 2);
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
		m_fState |= 0x40;
	}

	return S_OK;
}

HRESULT cLoop::EndMode(int goRetVal)
{
	if (!m_pCurrentDispatch)
		CriticalMsg("No loop mode to end!");

	m_fGoReturn = goRetVal;

	sModeData next{};
	m_pLoopStack->Pop(&next);

	if (next.dispatch)
	{
		m_pNextDispatch = next.dispatch;
		m_fNewMinorMode = next.frame.fMinorMode;
		m_fState |= 0x30;
		m_fState |= next.flags;
	}
	else
	{
		m_fState &= ~0x1;
		m_fState |= 0x20u;
		m_pNextDispatch = nullptr;
	}

	return S_OK;
}

ILoopMode* cLoop::GetCurrentMode()
{
	if (!m_pLoopManager || !m_pCurrentDispatch)
		return nullptr;

	return m_pLoopManager->GetMode(m_pCurrentDispatch->Describe(nullptr)->pID);
}

ILoopDispatch* cLoop::GetCurrentDispatch()
{
	return m_pCurrentDispatch;
}

void cLoop::Pause(BOOL fPause)
{
	if (!fPause && IsPaused())
		m_fState |= 0x8;
	else if (fPause && !IsPaused())
		m_fState |= 0x4;
}

BOOL cLoop::IsPaused(void)
{
	return m_fState & 0x2;
}

HRESULT cLoop::ChangeMinorMode(int minorMode)
{
	if (m_FrameInfo.fMinorMode == minorMode)
		return S_FALSE;

	m_fNewMinorMode = minorMode;
	m_fState |= 0x10;

	return S_OK;
}

int cLoop::GetMinorMode(void)
{
	return m_FrameInfo.fMinorMode;
}

HRESULT cLoop::SendMessage(eLoopMessage message, tLoopMessageData hData, int flags)
{
	if (!m_pCurrentDispatch)
	{
		CriticalMsg("Cannot dispatch messages: no loop mode");
		return E_FAIL;
	}

	return m_pCurrentDispatch->SendMessage(message, hData, flags);
}

HRESULT cLoop::SendSimpleMessage(eLoopMessage message)
{
	if (!m_pCurrentDispatch)
	{
		CriticalMsg("Cannot dispatch messages: no loop mode");
		return E_FAIL;
	}

	return m_pCurrentDispatch->SendSimpleMessage(message);
}

HRESULT cLoop::PostMessage(eLoopMessage message, tLoopMessageData hData, int flags)
{
	if (!m_pCurrentDispatch)
	{
		CriticalMsg("Cannot dispatch messages: no loop mode");
		return E_FAIL;
	}

	return m_pCurrentDispatch->PostMessage(message, hData, flags);
}

HRESULT cLoop::PostSimpleMessage(eLoopMessage message)
{
	if (!m_pCurrentDispatch)
	{
		CriticalMsg("Cannot dispatch messages: no loop mode");
		return E_FAIL;
	}

	return m_pCurrentDispatch->PostSimpleMessage(message);
}

HRESULT cLoop::ProcessQueue(void)
{
	if (!m_pCurrentDispatch)
	{
		CriticalMsg("Cannot dispatch messages: no loop mode");
		return E_FAIL;
	}

	return m_pCurrentDispatch->ProcessQueue();
}

void cLoop::SetDiagnostics(uint fDiagnostics, tLoopMessageSet messages)
{
	if (m_pCurrentDispatch)
		m_pCurrentDispatch->SetDiagnostics(fDiagnostics, messages);
	else
	{

		m_fTempDiagnostics = fDiagnostics;
		m_tempDiagnosticSet = messages;
	}
}

void cLoop::GetDiagnostics(uint* pfDiagnostics, tLoopMessageSet* pMessages)
{
	if (m_pCurrentDispatch)
		m_pCurrentDispatch->GetDiagnostics(pfDiagnostics, pMessages);
	else
	{

		*pfDiagnostics = m_fTempDiagnostics;
		*pMessages = m_tempDiagnosticSet;
	}
}

void cLoop::SetProfile(tLoopMessageSet messages, tLoopClientID* pClientId)
{
	if (m_pCurrentDispatch)
		m_pCurrentDispatch->SetProfile(messages, pClientId);
	else
	{
		m_TempProfileSet = messages;
		m_pTempProfileClientId = pClientId;
	}
}

void cLoop::GetProfile(tLoopMessageSet* pMessages, tLoopClientID** ppClientId)
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