#include <storeman.h>
#include <storedir.h>
#include <search.h>
#include <defstfct.h>
#include <filespec.h>
#include <hshsttem.h>
#include <lgassert.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cFactoryEntry
//

class cFactoryHashByExt;

class cFactoryEntry
{
public:
	friend cStorageManager;
	friend cFactoryHashByExt;

	cFactoryEntry(IStoreFactory* pFactory, const char* pExt)
		: m_pFactory{ nullptr }, m_pExt{ nullptr }
	{
		if (!pExt)
			CriticalMsg("Creating storage factory entry with no extension!");

		if (!pFactory)
			CriticalMsg("Missing storage factory!");

		m_pFactory = pFactory;
		m_pFactory->AddRef();

		m_pExt = static_cast<char*>(Malloc(strlen(pExt) + 1));
		strcpy(m_pExt, pExt);
	}

	~cFactoryEntry()
	{
		if (m_pFactory)
		{
			m_pFactory->Release();
			m_pFactory = nullptr;
		}

		if (m_pExt)
		{
			Free(m_pExt);
			m_pExt = nullptr;
		}
	}

private:
	IStoreFactory* m_pFactory;
	char* m_pExt;
};

///////////////////////////////////////

class cFactoryHashByExt : public cStrHashSet<cFactoryEntry*>
{
public:
	tHashSetKey GetKey(tHashSetNode node) const override
	{
		return reinterpret_cast<tHashSetKey>(reinterpret_cast<cFactoryEntry*>(node)->m_pExt);
	}
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cStorageManager
//

cStorageManager::cStorageManager() :
	m_pRootStore{ nullptr },
	m_pDefStoreFactory{ nullptr },
	m_pGlobalContext{ nullptr },
	m_pDefVariants{ nullptr },
	m_pFactoryTable{ nullptr }
{
	m_pRootStore = new cDirectoryStorage{ nullptr };

	IStoreHierarchy* pStoreHier = nullptr;
	if (FAILED(m_pRootStore->QueryInterface(IID_IStoreHierarchy, reinterpret_cast<void**>(&pStoreHier))))
		CriticalMsg("Root storage has no IStoreHierarchy!");

	pStoreHier->SetStoreManager(this);
	pStoreHier->Release();

	m_pFactoryTable = new cFactoryHashByExt{};
	m_pDefStoreFactory = new cDefaultStorageFactory{};

	RegisterFactory(m_pDefStoreFactory);
}

///////////////////////////////////////

cStorageManager::~cStorageManager()
{
	if (m_pRootStore)
	{
		Warning(("Storage Manager being deleted without being closed!\n"));
		Close();
	}

	if (m_pFactoryTable)
	{
		delete m_pFactoryTable;
		m_pFactoryTable = nullptr;
	}

	if (m_pDefStoreFactory)
	{
		delete m_pDefStoreFactory;
		m_pDefStoreFactory = nullptr;
	}
}

///////////////////////////////////////

void StorageTypeEnumerator(const char* pExt, IStoreFactory* pFactory, void* pClientData)
{
	reinterpret_cast<cStorageManager*>(pClientData)->InstallStorageType(pExt, pFactory);
}

///////////////////////////////////////

void cStorageManager::RegisterFactory(IStoreFactory* pFactory)
{
	pFactory->EnumerateTypes(StorageTypeEnumerator, this);
}

///////////////////////////////////////

IStore* cStorageManager::GetStore(const char* pPathName, int bCreate)
{
	if (!m_pRootStore)
		CriticalMsg("Storage Manager doesn't have a root store!");

	return m_pRootStore->GetSubstorage(pPathName, bCreate);
}

///////////////////////////////////////

IStore* cStorageManager::CreateSubstore(IStore* pParent, const char* pName)
{
	cStr ext{};
	cFileSpec fileSpec{ pName };
	fileSpec.GetFileExtension(ext);

	auto* pFactoryEntry = m_pFactoryTable->Search(ext);

	IStore* pStorage = nullptr;
	if (pFactoryEntry)
		pStorage = pFactoryEntry->m_pFactory->CreateStore(pParent, pName, ext);
	else
		pStorage = m_pDefStoreFactory->CreateStore(pParent, pName, ext);

	if (pStorage)
	{
		IStoreHierarchy* pHier = nullptr;
		if (FAILED(pStorage->QueryInterface(IID_IStoreHierarchy, reinterpret_cast<void**>(&pHier))))
			Warning(("Storage without an IStoreHierarchy!"));
		else
		{
			pHier->SetStoreManager(this);
			pHier->Release();
		}
	}

	return pStorage;
}

///////////////////////////////////////

void SetContextRoot(ISearchPath*, const char* pRoot, tSearchPathRecursion, void* pClientData)
{
	auto* pStoreMan = reinterpret_cast<cStorageManager*>(pClientData);

	auto* pStore = pStoreMan->GetStore(pRoot, FALSE);
	if (!pStore)
		return;

	IStoreHierarchy* pStoreHier = nullptr;
	if (SUCCEEDED(pStore->QueryInterface(IID_IStoreHierarchy, reinterpret_cast<void**>(&pStoreHier))))
	{
		pStoreHier->DeclareContextRoot(TRUE);
		pStoreHier->Release();
	}

	pStore->Release();
}

///////////////////////////////////////

void cStorageManager::SetGlobalContext(ISearchPath* pPath)
{
	if (m_pGlobalContext)
		m_pGlobalContext->Release();

	m_pGlobalContext = pPath;
	if (m_pGlobalContext)
	{
		m_pGlobalContext->AddRef();
		m_pGlobalContext->Iterate(SetContextRoot, TRUE, this);
	}
}

///////////////////////////////////////


void cStorageManager::SetDefaultVariants(ISearchPath* pPath)
{
	if (m_pDefVariants)
		m_pDefVariants->Release();

	m_pDefVariants = pPath;
	if (m_pDefVariants)
		m_pDefVariants->AddRef();
}

///////////////////////////////////////

ISearchPath* cStorageManager::NewSearchPath(const char* pNewPath)
{
	auto* pNewSearchPath = new cSearchPath{ this };

	if (m_pGlobalContext)
		pNewSearchPath->SetContext(m_pGlobalContext);

	if (m_pDefVariants)
		pNewSearchPath->SetVariants(m_pDefVariants);

	if (pNewPath)
		pNewSearchPath->AddPath(pNewPath);

	return pNewSearchPath;
}

///////////////////////////////////////

void cStorageManager::Close()
{
	if (!m_pRootStore)
		CriticalMsg("Trying to Close without a valid Root Storage!");


	IStoreHierarchy* pStoreHier = nullptr;
	if (FAILED(m_pRootStore->QueryInterface(IID_IStoreHierarchy, reinterpret_cast<void**>(&pStoreHier))))
		CriticalMsg("Root storage has no IStoreHierarchy!");

	pStoreHier->Close();
	pStoreHier->Release();

	if (m_pRootStore)
	{
		m_pRootStore->Release();
		m_pRootStore = nullptr;
	}

	if (m_pGlobalContext)
	{
		m_pGlobalContext->Release();
		m_pGlobalContext = nullptr;
	}

	if (m_pDefVariants)
	{
		m_pDefVariants->Release();
		m_pDefVariants = nullptr;
	}
}

///////////////////////////////////////

int cStorageManager::HeteroStoreExists(IStore* pParentStore, const char* pSubStoreName, char* pNameBuffer)
{
	auto nLen = strlen(pSubStoreName);

	tHashSetHandle hsh{};
	for (auto* pEntry = m_pFactoryTable->GetFirst(hsh); pEntry != nullptr; pEntry = m_pFactoryTable->GetNext(hsh))
	{
		strcpy(pNameBuffer, pSubStoreName);
		if (pNameBuffer[nLen - 1] == '\\')
			pNameBuffer[nLen - 1] = '\0';

		strcat(pNameBuffer, pEntry->m_pExt);
		if (pParentStore->StreamExists(pNameBuffer))
			return TRUE;
	}

	return FALSE;
}

///////////////////////////////////////

void cStorageManager::InstallStorageType(const char* pExt, IStoreFactory* pFactory)
{
	auto* pEntry = m_pFactoryTable->Search(pExt);
	if (pEntry)
	{
		m_pFactoryTable->Remove(pEntry);
		delete pEntry;
	}

	m_pFactoryTable->Insert(new cFactoryEntry{ pFactory, pExt });
}