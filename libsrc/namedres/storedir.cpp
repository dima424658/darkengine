#include "storedir.h"

#include <pathutil.h>
#include <filespec.h>
#include <filepath.h>
#include <filestrm.h>
#include <hshsttem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDirectoryStorage
//

cDirectoryStorage::cDirectoryStorage(const char* pRawName)
	:m_pFilePath{ nullptr },
	m_pName{ nullptr },
	m_pParent{ nullptr },
	m_pSubstorageTable{ new cStorageHashByName{} },
	m_pStreamTable{ new cStreamHashByName{} },
	m_Exists{ eExistenceState::kExistenceUnchecked },
	m_pStoreManager{ nullptr },
	m_fFlags{ 0 }
{
	if (pRawName)
		GetNormalizedPath(pRawName, &m_pName);
}

///////////////////////////////////////

cDirectoryStorage::~cDirectoryStorage()
{
	if (m_pName)
	{
		Free(m_pName);
		m_pName = nullptr;
	}

	if (m_pFilePath)
	{
		delete m_pFilePath;
		m_pFilePath = nullptr;
	}

	if (m_pParent)
	{
		m_pParent->Release();
		m_pParent = nullptr;
	}

	if (m_pSubstorageTable)
	{
		delete m_pSubstorageTable;
		m_pSubstorageTable = nullptr;
	}

	if (m_pStreamTable)
	{
		tHashSetHandle h;
		for (auto* pEntry = m_pStreamTable->GetFirst(h); pEntry != nullptr; pEntry = m_pStreamTable->GetNext(h))
		{
			m_pStreamTable->Remove(pEntry);
			delete pEntry;
		}

		delete m_pStreamTable;
		m_pStreamTable = nullptr;
	}

	if (m_pStoreManager)
	{
		m_pStoreManager->Release();
		m_pStoreManager = nullptr;
	}
}

///////////////////////////////////////

const char* cDirectoryStorage::GetName()
{
	return m_pName;
}

///////////////////////////////////////

const char* cDirectoryStorage::GetFullPathName()
{
	if (m_pFilePath)
		return m_pFilePath->GetPathName();

	return nullptr;
}

///////////////////////////////////////

void cDirectoryStorage::EnumerateLevel(tStoreLevelEnumCallback callback, BOOL bAbsolute, BOOL bRecurse, void* pClientData)
{
	if (m_pFilePath)
		EnumerateLevelHelper(m_pFilePath, callback, bAbsolute, bRecurse, pClientData);
}

///////////////////////////////////////

void cDirectoryStorage::EnumerateStreams(tStoreLevelEnumCallback callback, BOOL bAbsolute, BOOL bRecurse, void* pClientData)
{
	if (m_pFilePath)
		EnumerateStreamHelper(m_pFilePath, callback, bAbsolute, bRecurse, pClientData);
}

///////////////////////////////////////

IStore* cDirectoryStorage::GetSubstorage(const char* pSubPath, BOOL bCreate)
{
	if (!pSubPath || !strlen(pSubPath))
		return nullptr;

	char topPathLevel[32]{};
	const char* pathRest = nullptr;
	GetPathTop(pSubPath, topPathLevel, &pathRest);

	IStore* pSubstorage = nullptr;
	cFilePath AnchoredSubpath{};

	auto* pSubEntry = m_pSubstorageTable->Search(topPathLevel);
	if (pSubEntry)
	{
		pSubstorage = pSubEntry->m_pStore;
		if (!pSubstorage)
			return nullptr;
		pSubstorage->AddRef();
	}
	else if (!strcmp(topPathLevel, "..\\") && m_pParent)
	{
		pSubstorage = m_pParent;
		pSubstorage->AddRef();
	}
	else if (!strcmp(topPathLevel, ".\\") && m_pParent)
	{
		pSubstorage = this;
		AddRef();
	}
	else
	{
		auto isDriveLetter = false;
		if (m_pFilePath)
		{
			cFilePath SubstoragePath{ topPathLevel };
			AnchoredSubpath = *m_pFilePath;
			if (!AnchoredSubpath.AddRelativePath(SubstoragePath))
				return nullptr;
		}
		else if (topPathLevel[1] == ':')
			isDriveLetter = true;
		else
			AnchoredSubpath.FromText(topPathLevel);

		if (isDriveLetter || AnchoredSubpath.PathExists())
		{
			auto* pNewStore = new cDirectoryStorage{ topPathLevel };
			pNewStore->SetParent(this);
			pNewStore->SetStoreManager(m_pStoreManager);

			pSubstorage = pNewStore;
			pSubEntry = new cNamedStorage{ pSubstorage };

			m_pSubstorageTable->Insert(pSubEntry);
		}
		else
		{
			char pNameBuffer[32]{};
			if (m_pStoreManager->HeteroStoreExists(this, topPathLevel, pNameBuffer))
			{
				pSubstorage = m_pStoreManager->CreateSubstore(this, pNameBuffer);
				if (pSubstorage)
				{
					RegisterSubstorage(pSubstorage, topPathLevel);
				}
				else
				{
					pSubEntry = new cNamedStorage{ topPathLevel };
					m_pSubstorageTable->Insert(pSubEntry);

					return nullptr;
				}
			}
			else
			{
				if (bCreate)
				{
					auto* dirStorage = new cDirectoryStorage{ topPathLevel };
					dirStorage->SetParent(this);
					pSubstorage = dirStorage;

					pSubEntry = new cNamedStorage{ pSubstorage };
					m_pSubstorageTable->Insert(pSubEntry);
				}
				else
				{
					pSubEntry = new cNamedStorage{ topPathLevel };
					m_pSubstorageTable->Insert(pSubEntry);

					return nullptr;
				}
			}
		}
	}

	if (!strlen(pathRest))
		return pSubstorage;

	auto* result = pSubstorage->GetSubstorage(pathRest, bCreate);
	pSubstorage->Release();
	return result;
}

///////////////////////////////////////

IStore* cDirectoryStorage::GetParent()
{
	if (m_pParent)
		m_pParent->AddRef();

	return m_pParent;
}

///////////////////////////////////////

void cDirectoryStorage::Refresh(BOOL bRecurse)
{
	tHashSetHandle hs;

	for (auto* pStream = m_pStreamTable->GetFirst(hs); pStream != nullptr; pStream = m_pStreamTable->GetNext(hs))
	{
		m_pStreamTable->Remove(pStream);
		if (pStream)
			delete pStream;
	}

	for (auto* pSubstorage = m_pSubstorageTable->GetFirst(hs); pSubstorage != nullptr; pSubstorage = m_pSubstorageTable->GetNext(hs))
	{
		if (bRecurse && pSubstorage->m_pStore)
			pSubstorage->m_pStore->Refresh(bRecurse);

		m_pSubstorageTable->Remove(pSubstorage);
		if (pSubstorage)
			delete pSubstorage;
	}

	m_Exists = eExistenceState::kExistenceUnchecked;
}

///////////////////////////////////////

BOOL cDirectoryStorage::Exists()
{
	if (m_Exists == eExistenceState::kExistenceUnchecked)
	{
		if (!m_pFilePath)
			return TRUE;

		if (m_pParent->Exists())
		{
			if (m_pFilePath->PathExists())
				m_Exists = eExistenceState::kExists;
			else
				m_Exists = m_pName[1] == ':' ? eExistenceState::kExists : eExistenceState::kDoesntExist;
		}
		else
		{
			m_Exists = eExistenceState::kDoesntExist;
		}
	}

	return m_Exists == eExistenceState::kExists;
}

///////////////////////////////////////

BOOL cDirectoryStorage::StreamExists(const char* pName)
{
	if (!m_pFilePath || !pName)
		return FALSE;

	auto* pEntry = m_pStreamTable->Search(pName);
	if (pEntry)
		return pEntry->m_bExists;

	auto bExists = false;
	if (m_pFilePath)
	{
		cFileSpec fileSpec{ *m_pFilePath, pName };
		bExists = fileSpec.FileExists();
	}
	else
	{
		cFileSpec fileSpec{ pName };
		bExists = fileSpec.FileExists();
	}

	pEntry = new cNamedStream{ pName, bExists };
	m_pStreamTable->Insert(pEntry);

	return bExists;
}

///////////////////////////////////////

IStoreStream* cDirectoryStorage::OpenStream(const char* pName, uint)
{
	if (!pName)
		return nullptr;

	if (!StreamExists(pName))
		return nullptr;

	auto* fileStream = new cFileStream{ this };
	fileStream->SetName(pName);
	if (fileStream->Open())
		return fileStream;

	fileStream->Release();
	return nullptr;
}

///////////////////////////////////////

struct sDirState
{
	bool bStart;
	uint fFlags;
	char* pPattern;
	sFindContext FC;
};

void* cDirectoryStorage::BeginContents(const char* pPattern, uint fFlags)
{
	auto* pState = new sDirState{};
	pState->bStart = true;
	pState->fFlags = fFlags;

	if (pPattern)
	{
		pState->pPattern = static_cast<char*>(Malloc(strlen(pPattern) + 1));
		strcpy(pState->pPattern, pPattern);
	}

	return pState;
}

///////////////////////////////////////

int cDirectoryStorage::Next(void* pCookie, char* foundName)
{
	auto* pState = reinterpret_cast<sDirState*>(pCookie);
	auto bContinue = false;

	cFilePath FoundPath{};
	cFileSpec FoundFile{};

	if (pState->bStart)
	{
		if (pState->fFlags & 2)
			bContinue = m_pFilePath->FindFirst(FoundPath, pState->FC);
		else
			bContinue = m_pFilePath->FindFirst(FoundFile, pState->FC, pState->pPattern);

		pState->bStart = false;
	}
	else
	{
		if (pState->fFlags & 2)
			bContinue = m_pFilePath->FindNext(FoundPath, pState->FC);
		else
			bContinue = m_pFilePath->FindNext(FoundFile, pState->FC);
	}

	if (bContinue)
	{
		const char* pName = nullptr;
		if (pState->fFlags & 2)
			pName = FoundPath.GetPathName();
		else
			pName = FoundFile.GetFileName();

		strcpy(foundName, pName);
	}

	return bContinue;
}

///////////////////////////////////////

void cDirectoryStorage::EndContents(void* pCookie)
{
	auto* pState = reinterpret_cast<sDirState*>(pCookie);
	if (pState->pPattern)
		Free(pState->pPattern);

	delete pState;
}

///////////////////////////////////////

void cDirectoryStorage::GetCanonPath(char** ppCanonPath)
{
	if (m_pParent && !(m_fFlags & 1))
	{
		char* pCanonPath = nullptr;
		m_pParent->GetCanonPath(&pCanonPath);
		auto* pName = GetName();

		*ppCanonPath = static_cast<char*>(Realloc(pCanonPath, strlen(pCanonPath) + strlen(pName) + 1));
		strcat(*ppCanonPath, pName);
	}
	else
	{
		*ppCanonPath = static_cast<char*>(Malloc(1));
		**ppCanonPath = '\0';
	}
}

///////////////////////////////////////

void cDirectoryStorage::SetStoreManager(IStoreManager* pMan)
{
	if (m_pStoreManager)
		m_pStoreManager->Release();

	m_pStoreManager = pMan;
	if (m_pStoreManager)
		m_pStoreManager->AddRef();
}

///////////////////////////////////////

void cDirectoryStorage::SetParent(IStore* pNewParent)
{
	if (m_pParent)
		m_pParent->Release();

	m_pParent = pNewParent;
	if (!m_pParent)
		return;

	m_pParent->AddRef();

	if (!m_pName)
		return;

	auto* pParentName = m_pParent->GetFullPathName();
	cFilePath localPath{ m_pName };

	if (pParentName)
	{
		m_pFilePath = new cFilePath{ pParentName };
		m_pFilePath->AddRelativePath(localPath);
	}
	else
	{
		m_pFilePath = new cFilePath{ localPath };
	}

	m_pFilePath->MakeFullPath();
}

///////////////////////////////////////

void cDirectoryStorage::RegisterSubstorage(IStore* pSubstore, const char* pName)
{
	auto pOldSubstore = m_pSubstorageTable->Search(pName);
	if (pOldSubstore)
	{
		if (pSubstore == pOldSubstore->m_pStore)
			return;

		m_pSubstorageTable->Remove(pOldSubstore);
		delete pOldSubstore;
	}

	auto* pStoreEntry = new cNamedStorage{ pSubstore };
	m_pSubstorageTable->Insert(pStoreEntry);

	IStoreHierarchy* pSubstoreHier = nullptr;
	if (SUCCEEDED(pSubstore->QueryInterface(IID_IStoreHierarchy, reinterpret_cast<void**>(&pSubstoreHier))))
	{
		pSubstoreHier->SetParent(this);
		pSubstoreHier->Release();
	}
}

///////////////////////////////////////

void cDirectoryStorage::SetDataStream(IStoreStream*)
{
	CriticalMsg("cDirectoryStorage::SetDataStream called!");
}

///////////////////////////////////////

void cDirectoryStorage::DeclareContextRoot(int bIsRoot)
{
	if (bIsRoot)
		m_fFlags |= 1;
	else
		m_fFlags &= 0xFFFFFFFE;
}

///////////////////////////////////////

void cDirectoryStorage::Close()
{
	if (!m_pSubstorageTable)
		return;

	tHashSetHandle h;
	for (auto* pEntry = m_pSubstorageTable->GetFirst(h); pEntry != nullptr; pEntry = m_pSubstorageTable->GetNext(h))
	{
		m_pSubstorageTable->Remove(pEntry);

		IStoreHierarchy* pHier = nullptr;
		if (pEntry->m_pStore && SUCCEEDED(pEntry->m_pStore->QueryInterface(IID_IStoreHierarchy, reinterpret_cast<void**>(&pHier))))
		{
			pHier->Close();
			pHier->Release();
		}
		if (pEntry)
			delete pEntry;
	}
}

///////////////////////////////////////

void cDirectoryStorage::EnumerateLevelHelper(cFilePath* pPath, tStoreLevelEnumCallback callback, bool bAbsolute, bool bRecurse, void* pClientData)
{
	sFindContext FC{};
	cFilePath foundPath{};
	cStr relativePath{};
	const char* pName = nullptr;

	for (auto bContinue = pPath->FindFirst(foundPath, FC); bContinue; bContinue = pPath->FindNext(foundPath, FC))
	{
		if (bAbsolute)
		{
			foundPath.MakeFullPath();
			pName = foundPath.GetPathName();
		}
		else
		{
			if (m_pFilePath->ComputeAnchoredPath(foundPath, relativePath))
			{
				pName = relativePath;
			}
			else
			{
				foundPath.MakeFullPath();
				pName = foundPath.GetPathName();
			}
		}

		if (callback(this, pName, pClientData) && bRecurse)
			EnumerateLevelHelper(&foundPath, callback, bAbsolute, bRecurse, pClientData);
	}

	pPath->FindDone(FC);
}

///////////////////////////////////////

void cDirectoryStorage::EnumerateStreamHelper(cFilePath* pPath, tStoreLevelEnumCallback callback, bool bAbsolute, bool bRecurse, void* pClientData)
{
	EnumerateStreamPath(pPath, callback, bAbsolute, pClientData);
	if (!bRecurse)
		return;

	sFindContext FC{};
	cFilePath foundPath{};
	for (auto bContinue = pPath->FindFirst(foundPath, FC); bContinue; bContinue = pPath->FindNext(foundPath, FC))
		EnumerateStreamHelper(&foundPath, callback, bAbsolute, bRecurse, pClientData);

	pPath->FindDone(FC);
}

///////////////////////////////////////

void cDirectoryStorage::EnumerateStreamPath(cFilePath* pPath, tStoreLevelEnumCallback callback, bool bAbsolute, void* pClientData)
{
	sFindContext FC{};
	cFileSpec foundFile{};
	cStr name{};

	for (auto bContinue = pPath->FindFirst(foundFile, FC); bContinue; bContinue = pPath->FindNext(foundFile, FC))
	{
		if (bAbsolute)
		{
			foundFile.MakeFullPath();
			foundFile.GetNameString(name, kFullPathNameStyle);
		}
		else
		{
			if (foundFile.MakeAnchoredPath(*m_pFilePath))
				foundFile.GetNameString(name, kAnchorRelativeNameStyle);
			else
				foundFile.GetNameString(name, kFullPathNameStyle);
		}

		callback(this, name, pClientData);
	}

	pPath->FindDone(FC);
}