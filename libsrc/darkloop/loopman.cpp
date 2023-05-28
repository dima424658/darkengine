#include "loopman.h"

#include <lgassert.h>

//
// Pre-fab COM implementations
//

IMPLEMENT_AGGREGATION_SELF_DELETE(cLoopManager);

cLoopManager::cLoopManager(IUnknown* pOuterUnknown, unsigned nMaxModes)
	: m_Loop{ pOuterUnknown , this },
	m_nMaxModes{ nMaxModes },
	m_Factory{},
	m_nLoopModes{},
	m_pBaseMode{ nullptr }
{
	// Add internal components to outer aggregate...
	INIT_AGGREGATION_2(pOuterUnknown,
		IID_ILoopManager,
		this,
		IID_ILoop,
		&m_Loop,
		kPriorityLibrary - 1,
		nullptr);
}

HRESULT cLoopManager::AddClient(ILoopClient* pClient, ulong* pCookie)
{
	sLoopClientDesc* pClientDesc = (sLoopClientDesc*)pClient->GetDescription();
	if (m_Factory.m_ClientDescs.Search(pClientDesc->pID))
		CriticalMsg("Double add of loop client");

	*pCookie = reinterpret_cast<ulong>(pClientDesc->pID); // TODO: make cookie void**

	return m_Factory.AddClient(pClientDesc);
}

HRESULT cLoopManager::RemoveClient(ulong cookie)
{
	return m_Factory.RemoveClient(cookie);
}

HRESULT cLoopManager::AddClientFactory(ILoopClientFactory* pFactory, ulong* pCookie)
{
	*pCookie = reinterpret_cast<ulong>(pFactory);

	return m_Factory.AddInnerFactory(pFactory);
}

HRESULT cLoopManager::RemoveClientFactory(ulong cookie)
{
	return m_Factory.RemoveInnerFactory(reinterpret_cast<ILoopClientFactory*>(cookie));
}

HRESULT cLoopManager::GetClient(tLoopClientID* pID, tLoopClientData data, ILoopClient** ppResult)
{
	return m_Factory.GetClient(pID, data, ppResult);
}

HRESULT cLoopManager::AddMode(const sLoopModeDesc* pDesc)
{
	auto* pLoopMode = _LoopModeCreate(pDesc);
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
	auto* pLoopModeInfo = m_nLoopModes.Search(pID);
	if (!pLoopModeInfo)
		return nullptr;

	ILoopMode* pLoopMode = nullptr;
	pLoopModeInfo->pUnknown->QueryInterface(IID_ILoopMode, reinterpret_cast<void**>(&pLoopMode));

	return pLoopMode;
}

HRESULT cLoopManager::RemoveMode(tLoopModeID* pID)
{
	auto* pLoopModeInfo = m_nLoopModes.Search(pID);
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

	delete pLoopModeInfo;

	return S_OK;
}

HRESULT cLoopManager::SetBaseMode(tLoopModeID* pID) // TODO
{
	auto* pMode = m_nLoopModes.Search(pID);
	if (!pMode)
	{
		CriticalMsg("Attempted to set a base mode that was never added");
		m_pBaseMode = nullptr;

		return E_FAIL;
	}

	m_pBaseMode = pID;

	return S_OK;
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
	auto* pLoopManager = new cLoopManager{ pOuterUnknown, nMaxModes };
	if (!pLoopManager)
		return E_FAIL;

	if (ppLoopManager)
		*ppLoopManager = pLoopManager;

	return S_OK;
}