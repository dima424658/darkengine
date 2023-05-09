#include <loopapi.h>
#include <loopman.h>

class cLoopManager;

class cLoopClientDescTable : public cGuidHashSet<sLoopClientDesc*>
{
public:
	void DestroyAll()
	{
		if (m_nItems)
		{
			for (int i = 0; i < m_nPts; ++i)
			{
				for (sHashSetChunk* pChunk = m_Table[i]; pChunk; pChunk = pChunk->pNext)
				{
					delete pChunk;
					pChunk = NULL;
				}
			}
		}
	}

	tHashSetKey GetKey(tHashSetNode p) const
	{
		return (tHashSetKey__*)p->unused;
	}
};

class cLoopClientFactory : public cCTUnaggregated<ILoopClientFactory, &IID_ILoopClientFactory, kCTU_Default>
{
	friend class cLoopManager;
public:
	cLoopClientFactory();
	~cLoopClientFactory();

	DECLARE_UNAGGREGATABLE();

	//
	// Get the version of the loop client interface
	//
	STDMETHOD_(short, GetVersion)(THIS)
	{
		return 1;
	}

	//
	// Query what kinds of clients the factory makes as a NULL terminated list
	//
	STDMETHOD_(tLoopClientID**, QuerySupport)(THIS)
	{
		cDynArray<tLoopClientID*> clientGuids;
		tHashSetHandle handle;

		for (sLoopClientDesc* pClientDesc = m_ClientDescs.GetFirst(handle); pClientDesc; pClientDesc = m_ClientDescs.GetNext(handle))
			clientGuids.Append(pClientDesc->pID);

		return clientGuids.Detach();
	}

	STDMETHOD_(BOOL, DoesSupport)(THIS_ tLoopClientID*);

	//
	// Find/create a client
	//
	STDMETHOD(GetClient)(THIS_ tLoopClientID*, tLoopClientData, ILoopClient**);

	// Inner Factory stuff
	HRESULT AddInnerFactory(ILoopClientFactory* pFactory);
	HRESULT RemoveInnerFactory(ILoopClientFactory* pFactory);

	void ReleaseAll();

	STDMETHOD_(int, AddClient)(sLoopClientDesc* pClientDesc);

	int AddClients(sLoopClientDesc** ppClientDesc);

private:
	cLoopClientDescTable m_ClientDescs;
	cDynArray<ILoopClientFactory*> m_InnerFactories;
};

