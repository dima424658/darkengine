#pragma once

#include <loopapi.h>
#include <loopque.h>
#include <pintarr.h>
#include <timings.h>

struct sClientInfo
{
	sPriIntInfo<ILoopClient> priIntInfo;
	ulong interests;
	cAverageTimer timer;
};

struct cClientInfoList
{
	~cClientInfoList()
	{
		for (int i = 0; i < list.Size(); ++i)
			if (list[i])
				delete list[i];
	}

	int Append(const struct sClientInfo& o) { return list.Append(new sClientInfo{ o }); }
	sClientInfo& operator[](int i) { return *list[i]; }
	int Size() { return list.Size(); }

	cDynArray<sClientInfo*> list;
};

struct ConstraintTable
{
	enum { kConstraintTableSize = 32 };

	cDynArray<sAbsoluteConstraint> table[kConstraintTableSize]{};
};

class cLoopDispatch : public cCTUnaggregated<ILoopDispatch, &IID_ILoopDispatch, kCTU_Default>
{
public:
	cLoopDispatch(ILoopMode* mode, sLoopModeInitParmList paramList, tLoopMessageSet messageSet);
	~cLoopDispatch();

	// Send a message down/up the dispatch chain
	STDMETHOD(SendMessage)(eLoopMessage message, tLoopMessageData hData, int flags) override;

	// Send a message forward down the dispatch chain with no data
	STDMETHOD(SendSimpleMessage)(eLoopMessage message) override;

	// Post a message to be sent down/up the dispatch chain
	// the next time ProcessQueue() is called
	STDMETHOD(PostMessage)(eLoopMessage message, tLoopMessageData hData, int flags) override;

	// Post a message to be sent forward down the dispatch chain with no data
	// the next time ProcessQueue() is called
	STDMETHOD(PostSimpleMessage)(eLoopMessage message) override;

	// Send all posted messages in queue
	STDMETHOD(ProcessQueue)() override;

	// Describe my instantiaion
	STDMETHOD_(const sLoopModeName*, Describe)(sLoopModeInitParmList* list) override;

#ifndef SHIP
	// Set/Get the diagnostic mode
	STDMETHOD_(void, SetDiagnostics)(unsigned fDiagnostics, tLoopMessageSet messages) override;
	STDMETHOD_(void, GetDiagnostics)(unsigned* pfDiagnostics, tLoopMessageSet* pMessages) override;

	// Set messages and optional client to use the profileable dispatcher
	STDMETHOD_(void, SetProfile)(tLoopMessageSet messages, tLoopClientID*) override;
	STDMETHOD_(void, GetProfile)(tLoopMessageSet* pMessages, tLoopClientID**) override;

	STDMETHOD_(void, ClearTimers)() override;
	STDMETHOD_(void, DumpTimerInfo)() override;
#endif

private:
	bool DispatchNormalFrame(ILoopClient*, tLoopMessageData hData);
	void AddClientsFromMode(ILoopMode*, ConstraintTable& table);
	void SortClients(ConstraintTable& table);

private:
	ulong m_msgs;
	cLoopQueue m_Queue;

	ILoopMode* m_pLoopMode;
	cClientInfoList m_aClientInfo;
	cPriIntArray<ILoopClient> m_DispatchLists[32];

	uint m_fDiagnostics;
	tLoopMessageSet m_diagnosticSet;
	tLoopMessageSet m_ProfileSet;
	tLoopClientID* m_pProfileClientId;
	cAverageTimer m_AverageFrameTimer;

	cSimpleTimer m_TotalModeTime;

	class cInitParmTable
	{
	public:
		cInitParmTable(sLoopModeInitParmList list)
			: m_list{ copy_parm(list) } {}

		~cInitParmTable() { delete[] m_list; m_list = nullptr; }

		sLoopModeInitParm* Search(const GUID* pID)
		{
			for (auto p = &m_list[0]; p->pID != nullptr; ++p)
				if (p->pID == pID)
					return p;

			return nullptr;
		}

		sLoopModeInitParmList List() { return m_list; }

	private:
		sLoopModeInitParmList copy_parm(sLoopModeInitParmList list)
		{
			if (list == nullptr)
				return nullptr;

			int n = 0;
			while (list[n].pID != nullptr)
				++n;

			auto result = new sLoopModeInitParm[n + 1];
			memcpy(result, list, sizeof(sLoopModeInitParm) * (n + 1));

			return result;
		}

		sLoopModeInitParmList m_list;
	};

	cInitParmTable m_Parms;
};