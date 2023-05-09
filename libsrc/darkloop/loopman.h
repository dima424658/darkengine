#pragma once

#include "loopapi.h"
#include <aggmemb.h>

class cLoopManager : public ILoopManager
{
public:
	cLoopManager(IUnknown* pOuterUnknown, unsigned nMaxModes);

	// Add a client
	HRESULT STDMETHODCALLTYPE AddClient(ILoopClient*, ulong* pCookie) override;

	//  Remove a client
	HRESULT STDMETHODCALLTYPE RemoveClient(ulong cookie) override;

	// Add a client factory
	HRESULT STDMETHODCALLTYPE AddClientFactory(ILoopClientFactory*, ulong* pCookie) override;

	// Remove a client factory
	HRESULT STDMETHODCALLTYPE RemoveClientFactory(ulong cookie) override;

	// Find/create a client
	HRESULT STDMETHODCALLTYPE GetClient(tLoopClientID*, tLoopClientData, ILoopClient**) override;

	// Add a mode
	HRESULT STDMETHODCALLTYPE AddMode(const sLoopModeDesc*) override;

	// Get a mode
	ILoopMode* STDMETHODCALLTYPE GetMode(tLoopModeID*) override;

	// Remove a mode
	HRESULT STDMETHODCALLTYPE RemoveMode(tLoopModeID*) override;

	// Set/Get the elements shared by all modes
	HRESULT STDMETHODCALLTYPE SetBaseMode(tLoopModeID*) override;
	ILoopMode* STDMETHODCALLTYPE GetBaseMode() override;

private:

	///////////////////////////////////
	//
	// Aggregate member protocol
	//

	HRESULT Connect();
	HRESULT PostConnect();
	HRESULT Init();
	HRESULT End();
	HRESULT Disconnect();

protected:

	// IUnknown methods
	DECLARE_COMPLEX_AGGREGATION(cLoopManager);
};