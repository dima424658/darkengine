#include "loopman.h"

#include <lgassert.h>


//
// Pre-fab COM implementations
//
IMPLEMENT_COMPLEX_AGGREGATION_SELF_DELETE(cLoopManager);

cLoopManager::cLoopManager(IUnknown* pOuterUnknown, unsigned nMaxModes)
{
	// Add internal components to outer aggregate...
	INIT_AGGREGATION_1(pOuterUnknown,
		IID_ILoopManager, this,
		kPriorityLibrary,
		NULL);
}

HRESULT cLoopManager::AddClient(ILoopClient* pClient, ulong* pCookie)
{
}

HRESULT cLoopManager::RemoveClient(ulong cookie)
{
	return E_NOTIMPL;
}

HRESULT cLoopManager::AddClientFactory(ILoopClientFactory*, ulong* pCookie)
{
	return E_NOTIMPL;
}

HRESULT cLoopManager::RemoveClientFactory(ulong cookie)
{
	return E_NOTIMPL;
}

HRESULT cLoopManager::GetClient(tLoopClientID*, tLoopClientData, ILoopClient**)
{
	return E_NOTIMPL;
}

HRESULT cLoopManager::AddMode(const sLoopModeDesc*)
{
	return E_NOTIMPL;
}

ILoopMode* cLoopManager::GetMode(tLoopModeID*)
{
	return nullptr;
}

HRESULT cLoopManager::RemoveMode(tLoopModeID*)
{
	return E_NOTIMPL;
}

HRESULT cLoopManager::SetBaseMode(tLoopModeID*)
{
	return E_NOTIMPL;
}

ILoopMode* cLoopManager::GetBaseMode()
{
	return nullptr;
}

tResult LGAPI _LoopManagerCreate(REFIID, ILoopManager** ppLoopManager, IUnknown* pOuter, unsigned nMaxModes)
{
	if (ppLoopManager)
		*ppLoopManager = new cLoopManager(pOuter, nMaxModes);

	if (*ppLoopManager)
		return S_OK;
	else
		return E_FAIL;
}