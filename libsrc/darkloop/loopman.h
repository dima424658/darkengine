#pragma once

#include <loopapi.h>
#include <loop.h>
#include <loopfact.h>
#include <aggmemb.h>
#include <interset.h>

class cLoopManager : public ILoopManager
{
public:
	cLoopManager(IUnknown* pOuterUnknown, unsigned nMaxModes);

	// Add a client
	STDMETHOD(AddClient)(THIS_ ILoopClient*, ulong* pCookie) override;

	// Remove a client
	STDMETHOD(RemoveClient)(THIS_ ulong cookie) override;

	// Add a client factory
	STDMETHOD(AddClientFactory)(THIS_ ILoopClientFactory*, ulong* pCookie) override;

	// Remove a client factory
	STDMETHOD(RemoveClientFactory)(THIS_ ulong cookie) override;

	// Find/create a client
	STDMETHOD(GetClient)(THIS_ tLoopClientID*, tLoopClientData, ILoopClient**) override;

	// Add a mode
	STDMETHOD(AddMode)(THIS_ const sLoopModeDesc*) override;

	// Get a mode
	STDMETHOD_(ILoopMode*, GetMode)(THIS_ tLoopModeID*) override;

	// Remove a mode
	STDMETHOD(RemoveMode)(THIS_ tLoopModeID*) override;

	// Set/Get the elements shared by all modes
	STDMETHOD(SetBaseMode)(THIS_ tLoopModeID*) override;
	STDMETHOD_(ILoopMode*, GetBaseMode)(THIS) override;

private:
	DECLARE_AGGREGATION(cLoopManager);

	HRESULT Init() { return S_OK; }
	HRESULT End()
	{
		m_Factory.ReleaseAll();
		m_nLoopModes.ReleaseAll(true);
	
		return S_OK;
	}

private:
	cLoop m_Loop;
	uint m_nMaxModes;
	cLoopClientFactory m_Factory;
	cInterfaceTable m_nLoopModes;
	tLoopModeID* m_pBaseMode;
};