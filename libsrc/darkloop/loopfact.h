#pragma once

#include <loopapi.h>
#include <aggmemb.h>
#include <hashpp.h>
#include <hashset.h>
#include <looptype.h>
#include <dynarray.h>

class cLoopClientDescTable : public cGuidHashSet<const sLoopClientDesc*>
{
public:
	tHashSetKey GetKey(tHashSetNode node) const override;
};

class cLoopClientFactory : public cCTUnaggregated<ILoopClientFactory, &IID_ILoopClientFactory, kCTU_Default>
{
	friend class cLoopManager;
public:
	cLoopClientFactory();
	~cLoopClientFactory();

	// DECLARE_UNAGGREGATABLE();

	//
	// Get the version of the loop client interface
	//
	STDMETHOD_(short, GetVersion)(THIS) override;

	//
	// Query what kinds of clients the factory makes as a NULL terminated list
	//
	STDMETHOD_(tLoopClientID**, QuerySupport)(THIS) override;

	STDMETHOD_(BOOL, DoesSupport)(THIS_ tLoopClientID* clientGuid) override;

	//
	// Find/create a client
	//
	STDMETHOD(GetClient)(THIS_ tLoopClientID* pID, tLoopClientData data, ILoopClient** ppResult) override;

	// Inner Factory stuff
	HRESULT AddInnerFactory(ILoopClientFactory* pFactory);

	HRESULT RemoveInnerFactory(ILoopClientFactory* pFactory);

	void ReleaseAll();

	STDMETHOD_(int, AddClient)(const sLoopClientDesc* pClientDesc);
	HRESULT RemoveClient(ulong cookie);

	int AddClients(const sLoopClientDesc** ppClientDesc);

private:
	cLoopClientDescTable m_ClientDescs;
	cDynArray<ILoopClientFactory*> m_InnerFactories;
};