#include <search.h>
#include <pathutil.h>
#include <storbase.h>
#include <hshsttem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cSearchPath
//

cSearchPath::cSearchPath(IStoreManager* pStoreMan)
	: m_bPathParsed{ false },
	m_aPathStrings{ new cDynArray<sSearchPathElement>{} },
	m_pContext{ nullptr },
	m_pVariants{ nullptr },
	m_pPathStorages{ nullptr },
	m_pLastNode{ nullptr },
	m_pStoreMan{ pStoreMan }
{
	if (m_pStoreMan)
		m_pStoreMan->AddRef();
}

///////////////////////////////////////

cSearchPath::~cSearchPath()
{
	Clear();

	if (m_pStoreMan)
	{
		m_pStoreMan->Release();
		m_pStoreMan = nullptr;
	}

	if (m_aPathStrings)
	{
		delete m_aPathStrings;
		m_aPathStrings = nullptr;
	}
}

///////////////////////////////////////

void cSearchPath::Clear()
{
	if (m_pContext)
	{
		m_pContext->Release();
		m_pContext = nullptr;
	}

	if (m_pVariants)
	{
		m_pVariants->Release();
		m_pVariants = nullptr;
	}

	for (int i = m_aPathStrings->Size() - 1; i >= 0; --i)
		Free(&(*m_aPathStrings)[i]);

	delete m_aPathStrings;
	m_aPathStrings = new cDynArray<sSearchPathElement>{};

	ClearStorages();
}

///////////////////////////////////////

ISearchPath* cSearchPath::Copy()
{
	auto* pSearchPath = new cSearchPath{ m_pStoreMan };

	if (m_pContext)
		pSearchPath->SetContext(m_pContext);

	if (m_pVariants)
		pSearchPath->SetVariants(m_pVariants);

	for (auto i = 0; i < m_aPathStrings->Size(); ++i)
	{
		switch ((*m_aPathStrings)[i].fRecurse)
		{
		case 0:
			pSearchPath->AddPath((*m_aPathStrings)[i].pPath);
			break;
		case 1:
			pSearchPath->AddPathTrees((*m_aPathStrings)[i].pPath, 0);
			break;
		case 2:
			pSearchPath->AddPathTrees((*m_aPathStrings)[i].pPath, 1);
			break;
		}
	}

	return pSearchPath;
}

///////////////////////////////////////

void cSearchPath::AddPath(const char* pPath)
{
	DoAddPath(pPath, 0);
}

///////////////////////////////////////

void cSearchPath::AddPathTrees(const char* pPath, BOOL bRecurse)
{
	DoAddPath(pPath, (bRecurse != FALSE) + 1);
}

///////////////////////////////////////

void cSearchPath::Ready()
{
	if (!m_bPathParsed)
		SetupStorages();
}

///////////////////////////////////////

void cSearchPath::Refresh()
{
}

///////////////////////////////////////

void cSearchPath::SetContext(ISearchPath* pContext)
{
	if (m_pContext)
	{
		m_pContext->Release();
		m_pContext = nullptr;
	}

	if (pContext)
	{
		m_pContext = pContext;
		m_pContext->AddRef();
	}

	m_bPathParsed = FALSE;
}

///////////////////////////////////////

void cSearchPath::SetVariants(ISearchPath* pVariants)
{
	if (m_pVariants)
	{
		m_pVariants->Release();
		m_pVariants = nullptr;
	}

	if (m_pVariants)
	{
		m_pVariants = pVariants;
		m_pVariants->AddRef();
	}

	m_bPathParsed = FALSE;
}

///////////////////////////////////////

struct FindInfo
{
	IStore* pStore;
	const char* pName;
	uint fFlags;
	IStore* pFound;
};

///////////////////////////////////////

void FindInStorage(ISearchPath* /*pPath*/, const char* pVariant, tSearchPathRecursion /*fRecurse*/, void* pClientData)
{
	auto* pFindInfo = reinterpret_cast<FindInfo*>(pClientData);

	if (pFindInfo->pFound)
		return;

	IStore* pStore = nullptr;
	if (pVariant)
	{
		pStore = pFindInfo->pStore->GetSubstorage(pVariant, FALSE);
		if (!pStore)
			return;
	}
	else
	{
		pStore = pFindInfo->pStore;
		pStore->AddRef();
	}

	if (pFindInfo->fFlags & SEARCH_STORAGES)
	{
		pFindInfo->pFound = pStore->GetSubstorage(pFindInfo->pName, FALSE);
		if (pFindInfo->pFound)
		{
			if (!pFindInfo->pFound->Exists())
			{
				pFindInfo->pFound->Release();
				pFindInfo->pFound = nullptr;
			}
		}
	}
	else if (pStore->StreamExists(pFindInfo->pName))
	{
		pFindInfo->pFound = pStore;
		pFindInfo->pFound->AddRef();
	}

	pStore->Release();
}

///////////////////////////////////////

IStore* cSearchPath::Find(const char* pName, uint fFlags, IStore** ppCanonStore, const char* pRawRelPath)
{
	if (!pName || pName[0] == '\0')
	{
		Warning(("Find: empty name!\n"));
		return nullptr;
	}

	if (!m_bPathParsed)
		SetupStorages();

	char* pRelPath = nullptr;
	if (pRawRelPath)
		GetNormalizedPath(pRawRelPath, &pRelPath);

	IStore* found = nullptr, * pStore = nullptr;
	for (auto pNode = m_pPathStorages; !found && pNode; pNode = pNode->pNext)
	{
		if (pRelPath)
		{
			pStore = pNode->pStore->GetSubstorage(pRelPath, 0);
		}
		else
		{
			pStore = pNode->pStore;
			pStore->AddRef();
		}

		if (pStore)
		{
			FindInfo info{};
			info.pName = pName;
			info.pStore = pStore;
			info.fFlags = fFlags;
			info.pFound = nullptr;

			if (m_pVariants)
				m_pVariants->Iterate(FindInStorage, FALSE, &info);
			else
				FindInStorage(nullptr, nullptr, 0, &info);

			found = info.pFound;
			if (info.pFound && ppCanonStore)
			{
				*ppCanonStore = pStore;
				pStore->AddRef();
			}

			pStore->Release();
		}
	}

	if (pRelPath)
		Free(pRelPath);

	return found;
}

///////////////////////////////////////

struct SearchCookie
{
	PathStorageNode* pNode;
	void* pData;
	char* pPattern;
	uint fFlags;
	char* pPath;
	cStreamHashByName* pStreamTable;
};

///////////////////////////////////////

void* cSearchPath::BeginContents(const char* pPattern, uint fFlags, const char* pRelPath)
{
	if (!m_bPathParsed)
		SetupStorages();

	if (!m_pPathStorages)
		return nullptr;

	auto* pCookie = reinterpret_cast<SearchCookie*>(Malloc(sizeof(SearchCookie)));

	pCookie->pNode = m_pPathStorages;

	if (pRelPath)
	{
		pCookie->pPath = static_cast<char*>(Malloc(strlen(pRelPath) + 1));
		strcpy(pCookie->pPath, pRelPath);
	}
	else
	{
		pCookie->pPath = nullptr;
	}

	pCookie->pData = nullptr;

	if (pPattern)
	{
		pCookie->pPattern = static_cast<char*>(Malloc(strlen(pPattern) + 1));
		strcpy(pCookie->pPattern, pPattern);
	}
	else
	{
		pCookie->pPattern = nullptr;
	}

	pCookie->fFlags = fFlags;
	pCookie->pStreamTable = new cStreamHashByName{};

	return pCookie;
}

///////////////////////////////////////

BOOL cSearchPath::Next(void* pUntypedCookie, IStore** pFoundStore, char* pFoundName, IStore** ppFoundCanonStore)
{
	auto* pCookie = reinterpret_cast<SearchCookie*>(pUntypedCookie);

	IStore* pStore = nullptr;
	while (1)
	{
		while (1)
		{
		LABEL_1: // TODO: remove labels
			if (!pCookie->pNode)
				return FALSE;

			if (!pCookie->pPath)
				break;

			auto* pStore = pCookie->pNode->pStore->GetSubstorage(pCookie->pPath, FALSE);
			if (pStore)
				goto LABEL_8;

			pCookie = reinterpret_cast<SearchCookie*>(pCookie->pData);
		}

		pStore = pCookie->pNode->pStore;
		pStore->AddRef();
	LABEL_8:
		if (pCookie->pData)
			break;
		pCookie->pData = pStore->BeginContents(pCookie->pPattern, pCookie->fFlags);

		if (pCookie->pData)
			break;

		pStore->Release();

		pCookie = reinterpret_cast<SearchCookie*>(pCookie->pData);
	}

	do
	{
		if (!pStore->Next(pCookie->pData, pFoundName))
		{
			pStore->EndContents(pCookie->pData);
			pStore->Release();
			pCookie->pData = nullptr;

			pCookie = reinterpret_cast<SearchCookie*>(pCookie->pData);
			goto LABEL_1;
		}
	} while (pCookie->pStreamTable->Search(pFoundName));

	*pFoundStore = pStore;

	if (ppFoundCanonStore)
	{
		*ppFoundCanonStore = pStore;
		pStore->AddRef();
	}

	pCookie->pStreamTable->Insert(new cNamedStream{ pFoundName, 1 });

	return TRUE;
}

///////////////////////////////////////

void cSearchPath::EndContents(void* pUntypedCookie)
{
	auto* pCookie = reinterpret_cast<SearchCookie*>(pUntypedCookie);

	if (!pCookie)
		return;

	if (pCookie->pPattern)
		Free(pCookie->pPattern);

	if (pCookie->pPath)
		Free(pCookie->pPath);

	if (pCookie->pStreamTable)
		delete pCookie->pStreamTable;

	Free(pCookie);
}

///////////////////////////////////////

struct PathIterateData
{
	cSearchPath* pPath;
	tSearchPathIterateCallback pRealCallback;
	void* pRealClientData;
};

///////////////////////////////////////

void doDispatchIterate(ISearchPath* /*pPath*/, const char* pContextStore, tSearchPathRecursion /*fRecurse*/, void* pClientData)
{
	auto* pPathIterateData = reinterpret_cast<PathIterateData*>(pClientData);
	pPathIterateData->pPath->DoIterate(pContextStore, pPathIterateData->pRealCallback, pPathIterateData->pRealClientData);
}

///////////////////////////////////////

void cSearchPath::Iterate(tSearchPathIterateCallback callback, BOOL bUseContext, void* pClientData)
{
	static char pContext[4]; // TODO ???
	PathIterateData envelope{};

	if (bUseContext && m_pContext)
	{
		envelope.pPath = this;
		envelope.pRealCallback = callback;
		envelope.pRealClientData = pClientData;

		m_pContext->Iterate(doDispatchIterate, TRUE, &envelope);
	}
	else
	{
		DoIterate(pContext, callback, pClientData);
	}
}

///////////////////////////////////////

void cSearchPath::DoIterate(const char* pContext, tSearchPathIterateCallback callback, void* pClientData)
{
	for (auto i = 0; i < m_aPathStrings->Size(); ++i)
	{
		char fullpath[512]{};
		strcpy(fullpath, pContext);
		strcat(fullpath, (*m_aPathStrings)[i].pPath);
		callback(this, fullpath, (*m_aPathStrings)[i].fRecurse, pClientData);
	}
}

///////////////////////////////////////

struct SubpathEnumerateData
{
	cSearchPath* pThisPath;
	const char* pRootPath;
};

///////////////////////////////////////

int StorageSetupCallback(IStore* /*pStore*/, const char* pSubstoreName, void* pClientData)
{
	char FullPath[512]{};

	auto* pData = reinterpret_cast<SubpathEnumerateData*>(pClientData);

	strcpy(FullPath, pData->pRootPath);
	strcat(FullPath, pSubstoreName);

	pData->pThisPath->SetupSingleStore(FullPath, FALSE);

	return TRUE;
}

///////////////////////////////////////

void cSearchPath::SetupSingleStore(const char* pStorePath, int fRecurse)
{
	auto* pStore = m_pStoreMan->GetStore(pStorePath, FALSE);
	if (pStore && pStore->Exists())
	{
		auto* pNode = reinterpret_cast<PathStorageNode*>(Malloc(sizeof(PathStorageNode)));
		pNode->pStore = pStore;
		pNode->pNext = nullptr;
		if (m_pPathStorages)
			m_pLastNode->pNext = pNode;
		else
			m_pPathStorages = pNode;

		m_pLastNode = pNode;

		if (fRecurse)
		{
			SubpathEnumerateData pData{};
			pData.pThisPath = this;
			pData.pRootPath = pStorePath;

			pStore->EnumerateLevel(StorageSetupCallback, FALSE, fRecurse == 2, &pData);
		}
	}
}

///////////////////////////////////////

void cSearchPath::ClearStorages()
{
	PathStorageNode* pNext = nullptr;
	for (auto* pStoreNode = m_pPathStorages; pStoreNode != nullptr; pStoreNode = pNext)
	{
		pStoreNode->pStore->Release();
		pNext = pStoreNode->pNext;
		delete pStoreNode;
	}

	m_pLastNode = nullptr;
	m_pPathStorages = nullptr;
	m_bPathParsed = FALSE;
}

///////////////////////////////////////

void DispatchSetupStorage(ISearchPath* /*pSearchParh*/, const char* pStore, int fRecurse, void* pClientData)
{
	auto* pData = reinterpret_cast<cSearchPath*>(pClientData);
	pData->SetupSingleStore(pStore, fRecurse);
}

///////////////////////////////////////

void cSearchPath::SetupStorages()
{
	ClearStorages();

	if (m_aPathStrings && m_aPathStrings->Size())
	{
		Iterate(DispatchSetupStorage, TRUE, this);
		m_bPathParsed = true;
	}
}

///////////////////////////////////////

void cSearchPath::DoAddStore(const char* pStoreName, int fRecurse)
{
	if (!pStoreName)
		return;

	sSearchPathElement newEntry{};

	GetNormalizedPath(pStoreName, &newEntry.pPath);
	newEntry.fRecurse = fRecurse;

	m_aPathStrings->Append(newEntry);
	m_bPathParsed = FALSE;
}

///////////////////////////////////////

void cSearchPath::DoAddPath(const char* pPath, int fRecurse)
{
	if (!pPath || strlen(pPath) == 0)
		return;

	char buff[512]{};
	const char* pdelim = nullptr;

	for (auto* p = pPath; *p; p = pdelim + 1)
	{
		pdelim = strpbrk(p, ";+");
		if (!pdelim)
		{
			DoAddStore(p, fRecurse);
			return;
		}

		strncpy(buff, p, pdelim - p);
		buff[pdelim - p] = '\0';
		DoAddStore(buff, fRecurse);
	}
}