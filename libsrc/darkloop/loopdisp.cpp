#include "loopdisp.h"
#include "loopman.h"
#include "loopguid.h"

#include <loopapi.h>
#include <appagg.h>
#include <mprintf.h>

int MessageToIndex(int message)
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
	auto currentMessage = 1;
	auto iMessage = 0;
	while (currentMessage && !(message & currentMessage))
	{
		++iMessage;
		currentMessage *= 2;
	}

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

cLoopDispatch::cLoopDispatch(ILoopMode* mode, sLoopModeInitParmList paramList, tLoopMessageSet messageSet)
	: m_Queue{}, m_aClientInfo{}, m_AverageFrameTimer{}, m_TotalModeTime{}, m_Parms{ paramList }
{
	ConstraintTable table{};
	AddClientsFromMode(mode, table);

	AutoAppIPtr(LoopManager);

	if (pLoopManager)
	{
		auto mode = pLoopManager->GetBaseMode();
		AddClientsFromMode(mode, table);
		mode->Release();
	}

	SortClients(table);

	SendSimpleMessage(kMsgStart);
}

STDMETHODIMP_(HRESULT) cLoopDispatch::SendMessage(eLoopMessage message, tLoopMessageData hData, int flags)
{
	auto result = S_OK;

	if ((m_msgs & message) != 0)
	{
		auto fLoopTimeThisMsg = false;
		auto fPassedSkipTime = false;

		auto fLoopTime = m_fDiagnostics & 2;
		auto fFrameHeapchk = m_fDiagnostics & 8;
		auto fClientHeapchk = m_fDiagnostics & 0x10;
		auto fLoopTrack = (m_fDiagnostics & 1) && (message & m_diagnosticSet);

		if (message & (kMsgResumeMode | kMsgStart))
			m_TotalModeTime.Start();

		if (m_TotalModeTime.IsActive())
		{
			m_TotalModeTime.Stop();
			fPassedSkipTime = m_TotalModeTime.GetResult() > 2500.0;
			m_TotalModeTime.Start();
			fLoopTimeThisMsg = fPassedSkipTime && fLoopTime && (message & 0x3C0) != 0;
		}
		else
		{
			fLoopTimeThisMsg = false;
		}

		if (fLoopTimeThisMsg)
			m_AverageFrameTimer.Start();

		auto& targetList = m_DispatchLists[2 * MessageToIndex(message)];

		for (int i = 0; i <= targetList.Size() - 1; ++i)
		{
			auto idx = (flags & 1) ? i : targetList.Size() - 1 - i;
			auto pClient = targetList[idx]->pInterface;
			auto& pszClient = targetList[idx]->nameStr;

			if (fLoopTrack)
				LoopTrack(message, pszClient);

			cAverageTimer* pTimer;
			if (fLoopTimeThisMsg)
			{
				pTimer = &reinterpret_cast<sClientInfo*>(targetList[idx]->pData)->timer;
				pTimer->Start();
			}

			eLoopMessageResult clientResult;
			if (m_ProfileSet & message)
				clientResult = LoopProfileSend(pClient, message, hData);
			else
				clientResult = pClient->ReceiveMessage(message, hData);

			if (fLoopTimeThisMsg)
				pTimer->Stop();

			if (fClientHeapchk)
				_heapchk();

			if (clientResult == 1)
			{
				result = E_FAIL;
				break;
			}
		}

		if (fFrameHeapchk)
			_heapchk();

		if ((message & 0x2002) != 0)
			m_TotalModeTime.Stop();

		if (fLoopTime)
		{
			if (fLoopTimeThisMsg)
				m_AverageFrameTimer.Stop();
			if (fPassedSkipTime && (message & 0x200) != 0)
			{
				m_AverageFrameTimer.Mark();
				for (int j = 0; j < m_aClientInfo.Size(); ++j)
					m_aClientInfo[j].timer.Mark();

			}
			if ((message & 0x2002) != 0)
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
	else
	{
		if (message && (m_msgs & message) == 0)
			Warning(("Message 0x%02X not supported by this dispatch chain\n", message));
		return 1;
	}
}

STDMETHODIMP_(HRESULT) cLoopDispatch::SendSimpleMessage(eLoopMessage message)
{
	return SendMessage(message, nullptr, 1);
}

STDMETHODIMP_(HRESULT) cLoopDispatch::PostMessage(eLoopMessage message, tLoopMessageData hData, int flags)
{
	sLoopQueueMessage msg{};
	msg.message = message;
	msg.hData = hData;
	msg.flags = flags;

	m_Queue.Append(msg);

	return S_OK;
}

STDMETHODIMP_(HRESULT) cLoopDispatch::PostSimpleMessage(eLoopMessage message)
{
	return PostMessage(message, nullptr, 0);
}

STDMETHODIMP_(HRESULT) cLoopDispatch::ProcessQueue()
{
	auto result = S_FALSE;

	do
	{
		sLoopQueueMessage message{};
		if (!m_Queue.GetMessage(&message))
			break;

		result = SendMessage(message.message, message.hData, message.flags);
	} while (result != S_OK);

	return result;
}

STDMETHODIMP_(const sLoopModeName*) cLoopDispatch::Describe(sLoopModeInitParmList* list)
{
	auto name = m_pLoopMode->GetName();
	if (list)
		*list = m_Parms.List();

	return name;
}

STDMETHODIMP_(void) cLoopDispatch::SetDiagnostics(unsigned fDiagnostics, tLoopMessageSet messages)
{
	m_fDiagnostics = fDiagnostics;
	m_diagnosticSet = messages;
}

STDMETHODIMP_(void) cLoopDispatch::GetDiagnostics(unsigned* pfDiagnostics, tLoopMessageSet* pMessages)
{
	*pfDiagnostics = m_fDiagnostics;
	*pMessages = m_diagnosticSet;
}

STDMETHODIMP_(void) cLoopDispatch::SetProfile(tLoopMessageSet messages, tLoopClientID* pClientId)
{
	m_ProfileSet = messages;
	m_pProfileClientId = pClientId;
}

STDMETHODIMP_(void) cLoopDispatch::GetProfile(tLoopMessageSet* pMessages, tLoopClientID** ppClientId)
{
	*pMessages = m_ProfileSet;
	*ppClientId = m_pProfileClientId;
}

STDMETHODIMP_(void) cLoopDispatch::ClearTimers()
{
	m_TotalModeTime.Clear();
	m_AverageFrameTimer.Clear();

	for (int i = 0; i < m_aClientInfo.Size(); ++i)
		m_aClientInfo[i].timer.Clear();
}

int TimerSortFunc(cAverageTimer* const* pLeft, cAverageTimer* const* pRight)
{
	auto result = (*pRight)->GetResult() - (*pLeft)->GetResult();

	if (result == 0.0)
		return 0;
	else if (result > 0.0)
		return 1;
	else
		return -1;
}

STDMETHODIMP_(void) cLoopDispatch::DumpTimerInfo()
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
			m_AverageFrameTimer.GetTotalTime(), 0.0,
			0.0, 0.0, // TODO
			0.0, 0.0);
	}

	mprint(msg);
	if (m_AverageFrameTimer.GetIters() >= 0xA)
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

int cLoopDispatch::DispatchNormalFrame(ILoopClient* pClient, tLoopMessageData hData)
{
	return 0; // TODO
}

void TableAddClientConstraints(ConstraintTable& table, const sLoopClientDesc* desc)
{
	for (auto pRel = desc + 1; pRel->pID; pRel = (sLoopClientDesc*)((char*)pRel + 12))
	{
		if (pRel->pID != (GUID*)1 && pRel->pID != (GUID*)2)
			CriticalMsg("Bad constraint");

		sAbsoluteConstraint absoluteConstraint{};
		MakeAbsolute(*reinterpret_cast<const sRelativeConstraint*>(pRel), desc->pID, absoluteConstraint);
		int currentMessage = 1;
		int i = 0;
		while (currentMessage)
		{
			if (currentMessage & pRel->szName[4])
				table.table->Append(absoluteConstraint);

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
			if(m_aClientInfo[i].priIntInfo.pID == pID)
			{
				next_client = true;
				break;
			}

		if (!next_client)
		{
			ILoopClient* pClient = nullptr;
			auto pInitParms = initParmTable.Search(*ppIDs);

			pLoopManager->GetClient(pID, pInitParms != nullptr ? pInitParms->data : nullptr, &pClient);

			if (pClient)
			{
				auto pClientDesc = pClient->GetDescription();
				auto infoidx = m_aClientInfo.Append({});

				auto& info = m_aClientInfo[infoidx];
				info.timer.SetName(pClientDesc->szName);
				info.priIntInfo.pID = pID;
				info.priIntInfo.priority = pClientDesc->priority;
				info.priIntInfo.pInterface = pClient;
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