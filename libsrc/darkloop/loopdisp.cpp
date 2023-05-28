#include "loopdisp.h"
#include "loopman.h"
#include "loopguid.h"

#include <loopapi.h>
#include <appagg.h>
#include <mprintf.h>

int MessageToIndex(eLoopMessage message)
{
	int i = 0;
	int currentMessage = 1;

	while (currentMessage && !(message & currentMessage))
	{
		++i;
		currentMessage *= 2;
	}

	return i;
}

static const char* ppszLoopMessageNames[] =
{
  "kMsgNull",
  "kMsgStart",
  "kMsgEnd",
  "kMsgExit",
  "kMsgStartBlock",
  "kMsgBlocked",
  "kMsgEndBlock",
  "kMsgBeginFrame",
  "kMsgNormalFrame",
  "kMsgPauseFrame",
  "kMsgEndFrame",
  "kMsgFrameReserved1",
  "kMsgFrameReserved2",
  "kMsgEnterMode",
  "kMsgSuspendMode",
  "kMsgResumeMode",
  "kMsgExitMode",
  "kMsgMinorModeChange",
  "kMsgModeReserved2",
  "kMsgModeReserved3",
  "kMsgLoad",
  "kMsgSave",
  "kMsgUserReserved1",
  "kMsgUserReserved2",
  "kMsgUserReserved3",
  "kMsgApp1",
  "kMsgApp2",
  "kMsgApp3",
  "kMsgApp4",
  "kMsgApp5",
  "kMsgApp6",
  "kMsgApp7",
  "kMsgApp8"
};

const char* LGAPI LoopGetMessageName(eLoopMessage message)
{
	auto iMessage = MessageToIndex(message);
	if (iMessage == 33)
		iMessage = 0;

	return ppszLoopMessageNames[iMessage];
}

eLoopMessageResult LoopProfileSend(ILoopClient* pClient, eLoopMessage message, tLoopMessageData hData)
{
	return pClient->ReceiveMessage(message, hData);
}

char LoopTrack(uint message, const char* pszClient)
{
	static bool fSplitMono = false;
	if (!fSplitMono)
	{
		mono_split(1, 2);
		mono_setwin(2);
		fSplitMono = true;
	}

	int oldx, oldy;
	mono_getxy(&oldx, &oldy);

	mono_setwin(1);

	mono_setxy(0, 0);
	mprintf("%20s", LoopGetMessageName(message));

	mono_setxy(21, 0);
	mprintf("--> %-20s          ", pszClient);

	mono_setwin(2);

	return mono_setxy(oldx, oldy);
}

char LoopTrackClear()
{
	mono_setwin(1u);
	mono_clear();
	return mono_setwin(2u);
}

cLoopDispatch::cLoopDispatch(ILoopMode* loop, sLoopModeInitParmList parmList, tLoopMessageSet msgs)
	: m_msgs{ msgs | 3 },
	m_Queue{},
	m_pLoopMode{ loop },
	m_aClientInfo{},
	m_fDiagnostics{ 0 },
	m_diagnosticSet{ 0 },
	m_ProfileSet{ 0 },
	m_pProfileClientId{ 0 },
	m_AverageFrameTimer{},
	m_TotalModeTime{},
	m_Parms{ parmList }
{
	ConstraintTable constraints{};
	AddClientsFromMode(loop, constraints);

	AutoAppIPtr(LoopManager);

	if (pLoopManager)
	{
		auto pBaseMode = pLoopManager->GetBaseMode();
		AddClientsFromMode(pBaseMode, constraints);
		pBaseMode->Release();
	}

	SortClients(constraints);

	pLoopManager->Release(); // TODO: do we need this???
	pLoopManager = nullptr;

	SendSimpleMessage(kMsgStart);
}

cLoopDispatch::~cLoopDispatch()
{
	SendMessage(kMsgEnd, nullptr, 2);

	for (int i = 0; i < m_aClientInfo.Size(); ++i)
		if (m_aClientInfo[i].priIntInfo.pInterface)
		{
			m_aClientInfo[i].priIntInfo.pInterface->Release();
			m_aClientInfo[i].priIntInfo.pInterface = nullptr;
		}
}

HRESULT cLoopDispatch::SendMessage(eLoopMessage message, tLoopMessageData hData, int flags)
{
	if (!(m_msgs & message))
	{
		if (message && !(m_msgs & message))
			Warning(("Message 0x%02X not supported by this dispatch chain\n", message));

		return S_FALSE;
	}

	auto result = S_OK;

	auto fLoopTime = m_fDiagnostics & 0x01;
	auto fFrameHeapchk = m_fDiagnostics & 0x08;
	auto fClientHeapchk = m_fDiagnostics & 0x10;
	auto fLoopTrack = (m_fDiagnostics & 0x01) && (message & m_diagnosticSet);

	auto fPassedSkipTime = false;
	auto fLoopTimeThisMsg = false;
	if (message & (kMsgResumeMode | kMsgStart))
		m_TotalModeTime.Start();

	if (m_TotalModeTime.IsActive())
	{
		m_TotalModeTime.Stop();
		fPassedSkipTime = m_TotalModeTime.GetResult() > 2500.0;
		m_TotalModeTime.Start();
		fLoopTimeThisMsg = fPassedSkipTime && fLoopTime
			&& (message & (kMsgBeginFrame | kMsgNormalFrame | kMsgPauseFrame | kMsgEndFrame));
	}
	else
		fLoopTimeThisMsg = false;

	if (fLoopTimeThisMsg)
		m_AverageFrameTimer.Start();

	auto& targetList = m_DispatchLists[MessageToIndex(message)];

	auto iLast = static_cast<int>(targetList.Size()) - 1;
	for (int i = 0; i <= iLast; ++i)
	{
		auto idx = (flags & 0x01) ? i : iLast - i;
		auto pClient = targetList[idx]->pInterface;
		auto& pszClient = targetList[idx]->nameStr;

		if (fLoopTrack)
			LoopTrack(message, pszClient);

		cAverageTimer* pTimer = nullptr;
		if (fLoopTimeThisMsg)
		{
			pTimer = &reinterpret_cast<sClientInfo*>(targetList[idx]->pData)->timer;
			pTimer->Start();
		}

		eLoopMessageResult clientResult{};
		if (m_ProfileSet & message)
			clientResult = LoopProfileSend(pClient, message, hData);
		else
			clientResult = pClient->ReceiveMessage(message, hData);

		if (fLoopTimeThisMsg)
			pTimer->Stop();

		if (fClientHeapchk)
			_heapchk();

		if (clientResult == kLoopDispatchHalt)
		{
			result = E_FAIL;
			break;
		}
	}

	if (fFrameHeapchk)
		_heapchk();

	if (message & (kMsgSuspendMode || kMsgEnd))
		m_TotalModeTime.Stop();

	if (fLoopTime)
	{
		if (fLoopTimeThisMsg)
			m_AverageFrameTimer.Stop();

		if (fPassedSkipTime && (message & kMsgEndFrame))
		{
			m_AverageFrameTimer.Mark();
			for (int j = 0; j < m_aClientInfo.Size(); ++j)
				m_aClientInfo[j].timer.Mark();
		}

		if (message & (kMsgSuspendMode || kMsgEnd))
		{
			if (m_AverageFrameTimer.IsActive())
			{
				m_AverageFrameTimer.Stop();
				for (int k = 0; k < m_aClientInfo.Size(); ++k)
					m_aClientInfo[k].timer.Stop();
			}

			DumpTimerInfo();
			ClearTimers();
		}
	}

	if (fLoopTrack)
		LoopTrackClear();

	return result;
}

HRESULT cLoopDispatch::SendSimpleMessage(eLoopMessage message)
{
	return SendMessage(message, nullptr, 1);
}

HRESULT cLoopDispatch::PostMessage(eLoopMessage message, tLoopMessageData hData, int flags)
{
	sLoopQueueMessage queueMessage{};
	queueMessage.message = message;
	queueMessage.hData = hData;
	queueMessage.flags = flags;

	m_Queue.Append(queueMessage);

	return S_OK;
}

HRESULT cLoopDispatch::PostSimpleMessage(eLoopMessage message)
{
	return PostMessage(message, nullptr, 0);
}

HRESULT cLoopDispatch::ProcessQueue()
{
	auto result = S_FALSE;

	sLoopQueueMessage message{};
	while (m_Queue.GetMessage(&message))
	{
		result = SendMessage(message.message, message.hData, message.flags);
		if (result != S_OK)
			return result;
	}

	return result;
}

const sLoopModeName* cLoopDispatch::Describe(sLoopModeInitParmList* list)
{
	auto name = m_pLoopMode->GetName();
	if (list)
		*list = m_Parms.List();

	return name;
}

void cLoopDispatch::SetDiagnostics(unsigned fDiagnostics, tLoopMessageSet messages)
{
	m_fDiagnostics = fDiagnostics;
	m_diagnosticSet = messages;
}

void cLoopDispatch::GetDiagnostics(unsigned* pfDiagnostics, tLoopMessageSet* pMessages)
{
	*pfDiagnostics = m_fDiagnostics;
	*pMessages = m_diagnosticSet;
}

void cLoopDispatch::SetProfile(tLoopMessageSet messages, tLoopClientID* pClientId)
{
	m_ProfileSet = messages;
	m_pProfileClientId = pClientId;
}

void cLoopDispatch::GetProfile(tLoopMessageSet* pMessages, tLoopClientID** ppClientId)
{
	*pMessages = m_ProfileSet;
	*ppClientId = m_pProfileClientId;
}

void cLoopDispatch::ClearTimers()
{
	m_TotalModeTime.Clear();
	m_AverageFrameTimer.Clear();

	for (int i = 0; i < m_aClientInfo.Size(); ++i)
		m_aClientInfo[i].timer.Clear();
}

int TimerSortFunc(cAverageTimer* const* pLeft, cAverageTimer* const* pRight)
{
	auto result = (*pRight)->GetResult() - (*pLeft)->GetResult();

	if (result == 0.0) // TODO: just return result ?
		return 0;
	else if (result > 0.0)
		return 1;
	else
		return -1;
}

void cLoopDispatch::DumpTimerInfo()
{
	cAnsiStr msg{};

	if (m_AverageFrameTimer.IsActive())
	{
		m_AverageFrameTimer.Stop();

		for (int i = 0; i < m_aClientInfo.Size(); ++i)
			m_aClientInfo[i].timer.Stop();
	}

	if (m_AverageFrameTimer.GetIters() < 10)
	{
		msg.FmtStr("+----------------------------------------------------------------------------+\n"
			"[ Insufficient samples to calculate frame timings ]\n");
	}
	else
	{
		msg.FmtStr(
			1280,
			"+----------------------------------------------------------------------------+\n"
			"|                       FRAME TIMINGS: %-16s                      |\n"
			"+----------------------------------------------------------------------------+\n"
			"\n"
			"Frames sampled:     %12d frames\n"
			" \n"
			"In-loop time:       %12.0f ms    Total mode time:    %12.0f ms\n"
			"In-loop frame avg:  %12.2f ms    Total out-of-loop:  %12.0f ms\n"
			"In-loop framerate:  %12.2f fps   Mode framerate:     %12.2f fps\n"
			" \n",
			m_pLoopMode->GetName()->szName,
			m_AverageFrameTimer.GetIters(),
			m_AverageFrameTimer.GetTotalTime(),
			0.0,
			m_AverageFrameTimer.GetResult(),
			m_TotalModeTime.GetResult() - 2500.0 - static_cast<double>(m_AverageFrameTimer.GetTotalTime()),
			1000.0 / m_AverageFrameTimer.GetResult(),
			1000.0 / ((m_TotalModeTime.GetResult() - 2500.0) / static_cast<double>(m_AverageFrameTimer.GetIters())));
	}

	mprint(msg);
	if (m_AverageFrameTimer.GetIters() >= 10)
	{
		mprintf(" Loop Client                Average Time     %% of Frame  Max Time  Min Time\n"
			" ----------------------------------------------------------------------------\n");

		cDynArray<cAverageTimer*> timers{};
		for (int i = 0; i < m_aClientInfo.Size(); ++i)
		{
			timers.Append(&m_aClientInfo[i].timer);
			timers.Sort(TimerSortFunc);
		}

		auto frameAvg = m_AverageFrameTimer.GetResult();
		auto sumAvg = 0.0;

		for (int i = 0; i < timers.Size(); ++i)
		{
			auto clientAvg = timers[i]->GetResult();
			auto clientPctOfFrame = clientAvg / frameAvg * 100.0;
			if (clientAvg > 1.0)
			{
				sumAvg += clientAvg;

				msg.FmtStr(" %-25s %#12.4f %#12.2f%% %#10lu %#10lu\n",
					timers[i]->GetName(), clientAvg, clientPctOfFrame, timers[i]->GetMaxTime(), timers[i]->GetMinTime());

				mprint(msg);
			}
		}

		auto clientAvg = frameAvg - sumAvg;
		if (clientAvg > 0.0)
		{
			msg.FmtStr(" %-25s %#12.4f %#12.2f%%\n", "Other", clientAvg, clientAvg / frameAvg * 100.0);
			mprint(msg);
		}
	}

	mprintf("+----------------------------------------------------------------------------+\n");

	if (m_AverageFrameTimer.IsActive())
	{
		m_AverageFrameTimer.Start();

		for (int i = 0; i < m_aClientInfo.Size(); ++i)
			m_aClientInfo[i].timer.Start();
	}
}

bool cLoopDispatch::DispatchNormalFrame(ILoopClient* pClient, tLoopMessageData hData)
{
	return pClient->ReceiveMessage(kMsgNormalFrame, hData) != kLoopDispatchHalt;
}

void TableAddClientConstraints(ConstraintTable& table, const sLoopClientDesc* desc)
{
	for (auto pRel = desc->dispatchConstraints; pRel->constraint.kind != kNullConstraint; ++pRel)
	{
		if (pRel->constraint.kind != kConstrainBefore && pRel->constraint.kind != kConstrainAfter)
			CriticalMsg("Bad constraint");

		sAbsoluteConstraint absoluteConstraint{};
		MakeAbsolute(pRel->constraint, desc->pID, absoluteConstraint);

		int currentMessage = 1;
		int i = 0;
		while (currentMessage)
		{
			if (currentMessage & pRel->messages)
				table.table[i].Append(absoluteConstraint);

			++i;
			currentMessage *= 2;
		}
	}
}

void cLoopDispatch::AddClientsFromMode(ILoopMode* pLoop, ConstraintTable& constraints)
{
	if (!pLoop)
		return;

	AutoAppIPtr(LoopManager);

	if (pLoopManager == nullptr)
		CriticalMsg("Failed to locate ILoopManager implementation");


	auto fFirstWarning = true;
	auto desc = pLoop->Describe();

	auto& initParmTable = m_Parms;

	auto ppIDs = desc->ppClientIDs;
	auto ppLimit = &ppIDs[desc->nClients];

	for (; ppIDs < ppLimit; ++ppIDs)
	{
		auto pID = *ppIDs;

		if (*pID == GUID_NULL)
			break;

		auto next_client = false;
		for (auto i = 0; i < m_aClientInfo.Size(); ++i)
			if (m_aClientInfo[i].priIntInfo.pID == pID)
			{
				next_client = true;
				break;
			}

		if (!next_client)
		{
			auto pInitParms = initParmTable.Search(*ppIDs);
			ILoopClient* pClient = nullptr;

			pLoopManager->GetClient(pID, pInitParms != nullptr ? pInitParms->data : nullptr, &pClient);

			if (pClient)
			{
				auto pClientDesc = pClient->GetDescription();
				auto infoidx = m_aClientInfo.Append({});

				auto& info = m_aClientInfo[infoidx];
				info.timer.SetName(pClientDesc->szName);
				info.priIntInfo.pID = pID;
				info.priIntInfo.pInterface = pClient;
				info.priIntInfo.priority = pClientDesc->priority;
				info.priIntInfo.nameStr = pClientDesc->szName;
				info.priIntInfo.pData = &m_aClientInfo[infoidx];
				info.interests = pClientDesc->interests;

				if (m_msgs & info.interests)
				{
					auto msg = 1;
					auto i = 0;
					while (msg)
					{
						if (m_msgs & msg & info.interests)
							m_DispatchLists[i].Append(&info.priIntInfo);
						++i;
						msg *= 2;
					}
				}
				TableAddClientConstraints(constraints, pClientDesc);
			}
			else
			{
				auto mode = pLoopManager->GetMode(pID);
				if (mode)
				{
					AddClientsFromMode(mode, constraints);

					mode->Release();
					mode = nullptr;
				}
				else if (fFirstWarning)
				{
					Warning(("Loop mode \"%s\" did not find all expected clients\n", (const char*)(desc->name.szName)));
					fFirstWarning = false;
				}
			}
		}
	}
}

void cLoopDispatch::SortClients(ConstraintTable& constraints)
{
	for (int i = 0; i < ConstraintTable::kConstraintTableSize; ++i)
		m_DispatchLists[i].Sort(constraints.table[i].AsPointer(), constraints.table[i].Size());
}