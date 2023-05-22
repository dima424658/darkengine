#include "loopman.h"

#include <lgassert.h>

//
// Pre-fab COM implementations
//

IMPLEMENT_AGGREGATION_SELF_DELETE(cLoopManager);

cLoopManager::cLoopManager(IUnknown* pOuterUnknown, unsigned nMaxModes) : m_Loop{ pOuterUnknown , this }
{
	cLoop::gm_pLoop = &m_Loop;

	// Add internal components to outer aggregate...
	INIT_AGGREGATION_2(pOuterUnknown,
		IID_ILoopManager,
		this,
		IID_ILoop,
		&m_Loop,
		kPriorityLibrary,
		nullptr);
}

HRESULT cLoopManager::AddClient(ILoopClient* pClient, ulong* pCookie)
{
	sLoopClientDesc* pClientDesc = (sLoopClientDesc*)pClient->GetDescription();
	if (m_Factory.m_ClientDescs.Search(pClientDesc->pID))
		CriticalMsg("Double add of loop client");

	*pCookie = reinterpret_cast<ulong>(pClientDesc->pID); // TODO

	return m_Factory.AddClient(pClientDesc);
}

HRESULT cLoopManager::RemoveClient(ulong cookie)
{
	return m_Factory.RemoveClient(cookie);
}

HRESULT cLoopManager::AddClientFactory(ILoopClientFactory* pFactory, ulong* pCookie)
{
	*pCookie = (ulong)pFactory;

	return m_Factory.AddInnerFactory(pFactory);
}

HRESULT cLoopManager::RemoveClientFactory(ulong cookie)
{
	return m_Factory.RemoveInnerFactory((ILoopClientFactory*)cookie);
}

HRESULT cLoopManager::GetClient(tLoopClientID* pID, tLoopClientData data, ILoopClient** ppResult)
{
	return m_Factory.GetClient(pID, data, ppResult);
}

HRESULT cLoopManager::AddMode(const sLoopModeDesc* pDesc)
{
	auto pLoopMode = _LoopModeCreate(pDesc);
	if (!pLoopMode)
	{
		CriticalMsg("Failed to create loop mode");
		return E_FAIL;
	}

	m_nLoopModes.Insert(new cInterfaceInfo{ pDesc->name.pID, pLoopMode, nullptr });

	return S_OK;
}

ILoopMode* cLoopManager::GetMode(tLoopModeID* pID)
{
	ILoopMode* pLoopMode = nullptr;

	auto pLoopModeInfo = m_nLoopModes.Search(pID);
	if (!pLoopModeInfo)
		return nullptr;

	pLoopModeInfo->pUnknown->QueryInterface(IID_ILoopMode, (void**)&pLoopMode);

	return pLoopMode;
}

HRESULT cLoopManager::RemoveMode(tLoopModeID* pID)
{
	auto pLoopModeInfo = m_nLoopModes.Search(pID);
	if (!pLoopModeInfo)
	{
		CriticalMsg("Attempted to remove mode that was never added");
		return E_FAIL;
	}
	m_nLoopModes.Remove(pLoopModeInfo);

	if (pLoopModeInfo->pUnknown)
	{
		pLoopModeInfo->pUnknown->Release();
		pLoopModeInfo->pUnknown = nullptr;
	}

	return S_OK;
}

HRESULT cLoopManager::SetBaseMode(tLoopModeID* pID) // TODO
{
	auto pLoopModeInfo = m_nLoopModes.Search(pID);

	if (!pLoopModeInfo)
	{
		CriticalMsg("Attempted to set a base mode that was never added");
		m_pBaseMode = nullptr;
		return E_FAIL;
	}
	else
	{
		m_pBaseMode = pID;
	}
}

ILoopMode* cLoopManager::GetBaseMode() // TODO
{
	if (m_pBaseMode)
		return GetMode(m_pBaseMode);
	else
		return nullptr;
}

tResult LGAPI _LoopManagerCreate(REFIID, ILoopManager** ppLoopManager, IUnknown* pOuterUnknown, unsigned nMaxModes)
{
	ILoopManager* pLoopManager = new cLoopManager(pOuterUnknown, nMaxModes);
	if (!pLoopManager)
		return E_FAIL;

	if(ppLoopManager)
		*ppLoopManager = pLoopManager;

	return S_OK;
}