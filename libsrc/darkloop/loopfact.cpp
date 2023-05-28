#include <loopfact.h>

// implement hash sets
#include <hshsttem.h>

tHashSetKey cLoopClientDescTable::GetKey(tHashSetNode node) const
{
	return (tHashSetKey)(((sLoopClientDesc*)(node))->pID);
}

//
// Pre-fab COM implementations
//
// IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cLoopClientFactory, ILoopClientFactory);

cLoopClientFactory::cLoopClientFactory() 
	: m_ClientDescs{}, m_InnerFactories{} { }

cLoopClientFactory::~cLoopClientFactory()
{
}

short cLoopClientFactory::GetVersion()
{
	return 1;
}

tLoopClientID** cLoopClientFactory::QuerySupport()
{
	cDynArray<tLoopClientID*> clientGuids{};
	tHashSetHandle h{};

	for (auto* pClientDesc = m_ClientDescs.GetFirst(h); pClientDesc; pClientDesc = m_ClientDescs.GetNext(h))
		clientGuids.Append(pClientDesc->pID);

	clientGuids.Append(&GUID_NULL);

	return clientGuids.Detach();
}

BOOL cLoopClientFactory::DoesSupport(tLoopClientID* clientGuid)
{
	return m_ClientDescs.Search(clientGuid) != nullptr;
}

HRESULT cLoopClientFactory::GetClient(tLoopClientID* pID, tLoopClientData data, ILoopClient** ppResult)
{
	*ppResult = nullptr;

	auto pClientDesc = m_ClientDescs.Search(pID);
	if (pClientDesc)
	{
		switch (pClientDesc->factoryType)
		{
		case kLCF_None:
			return ppResult != nullptr ? S_OK : E_FAIL;
		case kLCF_Singleton:
			*ppResult = pClientDesc->pClient;
			(*ppResult)->AddRef();
			break;
		case kLCF_Callback:
			*ppResult = pClientDesc->pfnFactory(pClientDesc, data);
			break;
		case kLCF_FactObj:
			pClientDesc->pFactory->GetClient(pID, data, ppResult);
			break;
		default:
			CriticalMsg("Invalid factory type");
			break;
		}
	}
	else
	{
		for (int i = 0; !*ppResult && i < m_InnerFactories.Size(); ++i)
			m_InnerFactories[i]->GetClient(pID, data, ppResult);
	}

	return ppResult != nullptr ? S_OK : E_FAIL;
}

HRESULT cLoopClientFactory::AddInnerFactory(ILoopClientFactory* pFactory)
{
	m_InnerFactories.Append(pFactory);
	pFactory->AddRef();

	return S_OK;
}

HRESULT cLoopClientFactory::RemoveInnerFactory(ILoopClientFactory* pFactory)
{
	for (int i = 0; i < m_InnerFactories.Size(); ++i)
	{
		if (m_InnerFactories[i] == pFactory)
		{
			m_InnerFactories[i]->Release();
			m_InnerFactories.FastDeleteItem(i);

			return S_OK;
		}
	}

	return E_FAIL;
}

void cLoopClientFactory::ReleaseAll()
{
	for (int i = 0; i < m_InnerFactories.Size(); ++i)
	{
		if (m_InnerFactories[i])
		{
			m_InnerFactories[i]->Release();
			// m_InnerFactories[i] = nullptr;
		}
	}
}

HRESULT cLoopClientFactory::AddClient(const sLoopClientDesc* pClientDesc)
{
	if (m_ClientDescs.Search(pClientDesc->pID))
		CriticalMsg("Double add of loop client");

	m_ClientDescs.Insert(pClientDesc);

	eLoopClientFactoryType loopClientFactoryType = pClientDesc->factoryType;
	if (loopClientFactoryType == kLCF_Singleton || loopClientFactoryType == kLCF_FactObj)
		pClientDesc->pClient->AddRef();

	return S_OK;
}

HRESULT cLoopClientFactory::RemoveClient(ulong cookie)
{
	auto pClientDesc = m_ClientDescs.RemoveByKey(reinterpret_cast<tLoopClientID*>(cookie));
	if (!pClientDesc)
		CriticalMsg("Client to remove from simple factory is not present");
	else if(pClientDesc->factoryType == kLCF_Singleton || pClientDesc->factoryType == kLCF_FactObj)
		pClientDesc->pClient->Release();
	
	return pClientDesc != nullptr ? S_OK : E_FAIL;
}

HRESULT cLoopClientFactory::AddClients(const sLoopClientDesc** ppClientDesc)
{
	int result = S_OK;

	for (;*ppClientDesc; ++ppClientDesc)
	{
		if (AddClient(*ppClientDesc) != S_OK)
			result = E_FAIL;
	}

	return result;
}

ILoopClientFactory* LGAPI CreateLoopFactory(const sLoopClientDesc** descs)
{
	auto* pLoopClientFactory = new cLoopClientFactory{};
	if (pLoopClientFactory)
	{
		pLoopClientFactory->AddClients(descs);
		return pLoopClientFactory;
	}

	return nullptr;
}