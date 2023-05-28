#include <storezip.h>
#include <pathutil.h>
#include <filespec.h>
#include <zipstrm.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cZipSubstorage
//

cZipSubstorage::cZipSubstorage(IStore* pParent, const char* pRawName)
	: m_pFilePath{ nullptr },
	m_pName{ nullptr },
	m_pParent{ nullptr },
	m_pSubstorageTable{ new cStorageHashByName{} },
	m_pStreamTable{ new cStreamHashByName{} },
	m_pStoreManager{ nullptr },
	m_fFlags{ 0 }
{
	if (pRawName)
		GetNormalizedPath(pRawName, &m_pName);

	SetParent(pParent);
}

///////////////////////////////////////

cZipSubstorage::~cZipSubstorage()
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
		for (auto* pEntry = m_pStreamTable->GetFirst(h); pEntry != nullptr; m_pStreamTable->GetNext(h))
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

const char* cZipSubstorage::GetName()
{
	return m_pName;
}

///////////////////////////////////////

const char* cZipSubstorage::GetFullPathName()
{
	if (m_pFilePath)
		return m_pFilePath->GetPathName();

	return nullptr;
}

///////////////////////////////////////

struct sEnumLevelEnvelope
{
	void* pRealClientData;
	tStoreLevelEnumCallback pCallback;
	cZipSubstorage* pStore;
	BOOL bAbsolute;
};

///////////////////////////////////////

int EnumerateLevelCallback(IStore* pSubstore, const char* pSubpath, void* pClientData)
{
	auto* envelope = reinterpret_cast<sEnumLevelEnvelope*>(pClientData);
	return envelope->pStore->EnumerateLevelHelper(pSubpath, pSubstore, envelope->pCallback, envelope->bAbsolute, envelope->pRealClientData);
}

///////////////////////////////////////

void cZipSubstorage::EnumerateLevel(tStoreLevelEnumCallback callback, BOOL bAbsolute, BOOL bRecurse, void* pClientData)
{
	tHashSetHandle hsh{};

	for (auto pEntry = m_pSubstorageTable->GetFirst(hsh); pEntry != nullptr; pEntry = m_pSubstorageTable->GetNext(hsh))
	{
		const char* pName = nullptr;
		if (bAbsolute)
		{
			cFilePath fullPath{ *m_pFilePath };
			cFilePath subPath{ m_pName };
			fullPath.AddRelativePath(subPath);
			fullPath.MakeFullPath();

			pName = fullPath.GetPathName();
		}
		else
		{
			pName = pEntry->m_pName;
		}

		if (callback(this, pName, pClientData) && bRecurse)
		{
			sEnumLevelEnvelope envelope{};
			envelope.pRealClientData = pClientData;
			envelope.pCallback = callback;
			envelope.pStore = this;
			envelope.bAbsolute = bAbsolute;

			pEntry->m_pStore->EnumerateLevel(EnumerateLevelCallback, FALSE, TRUE, &envelope);
		}
	}
}

///////////////////////////////////////

int EnumerateStreamsCallback(IStore* pSubstore, const char* pSubpath, void* pClientData)
{
	auto* envelope = reinterpret_cast<sEnumLevelEnvelope*>(pClientData);
	return envelope->pStore->EnumerateStreamsHelper(pSubpath, pSubstore, envelope->pCallback, envelope->bAbsolute, envelope->pRealClientData);
}

///////////////////////////////////////

void cZipSubstorage::EnumerateStreams(tStoreLevelEnumCallback callback, BOOL bAbsolute, BOOL bRecurse, void* pClientData)
{
	tHashSetHandle hsh{};

	for (auto pStreamEntry = m_pStreamTable->GetFirst(hsh); pStreamEntry != nullptr; pStreamEntry = m_pStreamTable->GetNext(hsh))
	{
		cStr name{};
		const char* pName;
		if (bAbsolute)
		{
			cFileSpec fileSpec{ pStreamEntry->m_pName };

			if (!fileSpec.GetFullPath(name, *m_pFilePath))
				CriticalMsg("cZipSubstorage::EnumerateStreams -- no fullpath");

			pName = name;
		}
		else
		{
			pName = pStreamEntry->m_pName;
		}

		callback(this, pName, pClientData);
	}

	if (bRecurse)
	{
		for (auto pStoreEntry = m_pSubstorageTable->GetFirst(hsh); pStoreEntry != nullptr; pStoreEntry = m_pSubstorageTable->GetNext(hsh))
		{
			sEnumLevelEnvelope envelope{};
			envelope.pRealClientData = pClientData;
			envelope.pCallback = callback;
			envelope.pStore = this;
			envelope.bAbsolute = bAbsolute;

			pStoreEntry->m_pStore->EnumerateStreams(EnumerateStreamsCallback, FALSE, TRUE, &envelope);
		}
	}
}

///////////////////////////////////////

IStore* cZipSubstorage::GetSubstorage(const char* pSubPath, BOOL bCreate)
{
	if (!pSubPath || !strlen(pSubPath))
		return nullptr;

	char topPathLevel[32];
	const char* pathRest;
	GetPathTop(pSubPath, topPathLevel, &pathRest);

	IStore* pSubstorage = nullptr;

	auto* pSubEntry = m_pSubstorageTable->Search(topPathLevel);
	if (pSubEntry)
	{
		pSubstorage = pSubEntry->m_pStore;
		if (!pSubstorage)
			CriticalMsg("NULL substorage in Zip file?!?");
		pSubstorage->AddRef();
	}
	else
	{
		if (strcmp(topPathLevel, "..\\") == 0)
		{
			if (!m_pParent)
				return nullptr;

			pSubstorage = m_pParent;
			pSubstorage->AddRef();
		}
		else if (strcmp(topPathLevel, ".\\") == 0)
		{
			pSubstorage = this;
			AddRef();
		}
		else
		{
			char nameBuffer[32];
			if (!m_pStoreManager->HeteroStoreExists(this, topPathLevel, nameBuffer))
				return 0;

			pSubstorage = m_pStoreManager->CreateSubstore(this, nameBuffer);
			if (!pSubstorage)
				return nullptr;

			pSubEntry = new cNamedStorage{ pSubstorage };
			m_pSubstorageTable->Insert(pSubEntry);
		}
	}

	if (!strlen(pathRest))
		return pSubstorage;

	auto* pResult = pSubstorage->GetSubstorage(pathRest, bCreate);
	pSubstorage->Release();
	return pResult;
}

///////////////////////////////////////

IStore* cZipSubstorage::GetParent()
{
	if (m_pParent)
		CriticalMsg("ZipSubstorage with no parent!");

	m_pParent->AddRef();
	return m_pParent;
}

///////////////////////////////////////

void cZipSubstorage::Refresh(BOOL bRecurse)
{
}

///////////////////////////////////////

BOOL cZipSubstorage::Exists()
{
	return TRUE;
}

///////////////////////////////////////

BOOL cZipSubstorage::StreamExists(const char* pName)
{
	if (!pName)
		CriticalMsg("cZipSubstorage::StreamExists -- null name!");

	return m_pStreamTable->Search(pName) ? TRUE : FALSE;
}

///////////////////////////////////////

IStoreStream* cZipSubstorage::OpenStream(const char* pName, uint)
{
	if (!pName)
		CriticalMsg("cZipSubstorage::OpenStream -- null pName!");

	auto* pStreamEntry = m_pStreamTable->Search(pName);
	if (!pStreamEntry)
		return nullptr;

	auto* pStream = new cZipStream{ this, m_pZipStorage , (cNamedZipStream*)(pStreamEntry) }; // TODO
	pStream->SetName(pName);
	if (pStream->Open())
		return pStream;

	return nullptr;
}

///////////////////////////////////////

struct sZipState
{
	bool bStart;
	uint fFlags;
	char* pPattern;
	tHashSetHandle hsh;
};

///////////////////////////////////////

void* cZipSubstorage::BeginContents(const char* pPattern, uint fFlags)
{
	auto* pState = reinterpret_cast<sZipState*>(Malloc(sizeof(sZipState)));
	pState->bStart = true;
	pState->fFlags = fFlags;

	if (pPattern)
	{
		pState->pPattern = static_cast<char*>(Malloc(strlen(pPattern) + 1));
		strcpy(pState->pPattern, pPattern);
	}
	else
	{
		pState->pPattern = nullptr;
	}
	return pState;
}

///////////////////////////////////////

BOOL cZipSubstorage::Next(void* pUntypedCookie, char* foundName)
{
	const char* pName = nullptr;
	auto* pCookie = reinterpret_cast<sZipState*>(pUntypedCookie);
	if (pCookie->bStart)
	{
		if (pCookie->fFlags & 2)
		{
			auto* pStoreEntry = m_pSubstorageTable->GetFirst(pCookie->hsh);
			if (pStoreEntry)
				pName = pStoreEntry->m_pName;
		}
		else
		{
			auto* pStreamEntry = m_pStreamTable->GetFirst(pCookie->hsh);
			if (pStreamEntry)
				pName = pStreamEntry->m_pName;
		}

		pCookie->bStart = false;
	}
	else
	{
		if (pCookie->fFlags & 2)
		{
			auto* pStoreEntry = m_pSubstorageTable->GetNext(pCookie->hsh);
			if (pStoreEntry)
				pName = pStoreEntry->m_pName;
		}
		else
		{
			auto* pStreamEntry = m_pStreamTable->GetNext(pCookie->hsh);
			if (pStreamEntry)
				pName = pStreamEntry->m_pName;
		}
	}

	if (pName)
	{
		strcpy(foundName, pName);
		return TRUE;
	}

	return FALSE;
}

///////////////////////////////////////

void cZipSubstorage::EndContents(void* pUntypedCookie)
{
	auto* pCookie = reinterpret_cast<sZipState*>(pUntypedCookie);
	if (pCookie->pPattern)
		Free(pCookie->pPattern);

	Free(pCookie);
}

///////////////////////////////////////

void cZipSubstorage::GetCanonPath(char** ppCanonPath)
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

void cZipSubstorage::SetStoreManager(IStoreManager* pMan)
{
	if (m_pStoreManager)
		m_pStoreManager->Release();
	m_pStoreManager = pMan;

	m_pStoreManager = pMan;
	if (m_pStoreManager)
		m_pStoreManager->AddRef();

	if (!m_pSubstorageTable)
		return;

	tHashSetHandle h{};
	for (auto pEntry = m_pSubstorageTable->GetFirst(h); pEntry != nullptr; pEntry = m_pSubstorageTable->GetNext(h))
	{
		IStoreHierarchy* pHier = nullptr;
		if (SUCCEEDED(pEntry->m_pStore->QueryInterface(IID_IStoreHierarchy, reinterpret_cast<void**>(&pHier))))
		{
			pHier->SetStoreManager(pMan);
			pHier->Release();
		}
	}
}

///////////////////////////////////////

void cZipSubstorage::SetParent(IStore* pNewParent)
{
	if (m_pParent)
	{
		if (m_pParent == pNewParent)
			return;

		m_pParent->Release();
		if (m_pFilePath)
		{
			delete m_pFilePath;
			m_pFilePath = nullptr;
		}
	}

	m_pParent = pNewParent;
	if (!m_pParent)
		return;

	m_pParent->AddRef();

	if (m_pName)
	{
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
}

///////////////////////////////////////

void cZipSubstorage::RegisterSubstorage(IStore* pSubstore, const char* pName)
{
	auto* pOldSubstore = m_pSubstorageTable->Search(pName);
	if (pOldSubstore)
		CriticalMsg("cZipSubstorage::RegisterSubstorage -- substituting...");

	auto* pStoreEntry = new cNamedStorage{ pSubstore };
	m_pSubstorageTable->Insert(pStoreEntry);

	IStoreHierarchy* pSubstoreHier = nullptr;
	if (SUCCEEDED(pSubstore->QueryInterface(IID_IStoreHierarchy, reinterpret_cast<void**>(pSubstoreHier))))
	{
		pSubstoreHier->SetParent(this);
		pSubstoreHier->Release();
	}
}

///////////////////////////////////////

void cZipSubstorage::SetDataStream(IStoreStream*)
{
	CriticalMsg("cZipSubstorage::SetDataStream called!");
}

///////////////////////////////////////

void cZipSubstorage::DeclareContextRoot(int bIsRoot)
{
	if (bIsRoot)
		m_fFlags |= 1;
	else
		m_fFlags &= 0xFFFFFFFE;
}

///////////////////////////////////////

void cZipSubstorage::Close()
{
	if (!m_pSubstorageTable)
		return;

	tHashSetHandle h;
	for (auto* pEntry = m_pSubstorageTable->GetFirst(h); pEntry != nullptr; m_pSubstorageTable->GetNext(h))
	{
		m_pSubstorageTable->Remove(pEntry);

		IStoreHierarchy* pHier = nullptr;
		if (SUCCEEDED(pEntry->m_pStore->QueryInterface(IID_IStoreHierarchy, reinterpret_cast<void**>(pHier))))
		{
			pHier->Close();
			pHier->Release();
		}

		delete pEntry;
	}
}

///////////////////////////////////////

int cZipSubstorage::EnumerateLevelHelper(const char* pSubpath, IStore* pSubstore, tStoreLevelEnumCallback callback, bool bAbsolute, void* pClientData)
{
	cFilePath topPath{ pSubstore->GetName() };
	cFilePath subPath{ pSubpath };
	topPath.AddRelativePath(subPath);

	const char* pName = nullptr;
	if (bAbsolute)
	{
		cFilePath myPath{ *m_pFilePath };
		myPath.AddRelativePath(topPath);
		myPath.MakeFullPath();

		pName = myPath.GetPathName(); // TODO dead myPath ???
	}
	else
	{
		pName = topPath.GetPathName();
	}

	return callback(this, pName, pClientData);
}

///////////////////////////////////////

int cZipSubstorage::EnumerateStreamsHelper(const char* pSubpath, IStore* pSubstore, tStoreLevelEnumCallback callback, bool bAbsolute, void* pClientData)
{
	cStr name{};

	cFilePath topPath{ pSubstore->GetName() };
	cFileSpec fileSpec{ topPath,pSubpath };

	if (bAbsolute)
		fileSpec.GetNameString(name, *m_pFilePath);
	else
		fileSpec.GetNameString(name, kAnchorRelativeNameStyle);

	return callback(this, name, pClientData);
}

///////////////////////////////////////

void cZipSubstorage::SetTopZip(cZipStorage* pStorage)
{
	if (!pStorage)
		CriticalMsg("cZipSubstorage::SetTopZip -- empty pStorage!");

	m_pZipStorage = pStorage;
}

///////////////////////////////////////

cZipSubstorage* cZipSubstorage::MakeSubstorage(const char* pSubPath)
{
	if (!pSubPath)
		CriticalMsg("cZipStorage::MakeSubstorage -- null subpath");

	if (strlen(pSubPath) == 0)
		CriticalMsg("cZipStorage::MakeSubstorage -- empty subpath");

	char topPathLevel[32];
	const char* pathRest;
	GetPathTop(pSubPath, topPathLevel, &pathRest);

	cZipSubstorage* pSubstorage = nullptr;

	auto* pSubEntry = m_pSubstorageTable->Search(topPathLevel);
	if (pSubEntry)
	{
		pSubstorage = dynamic_cast<cZipSubstorage*>(pSubEntry->m_pStore); // TODO ???
	}
	else
	{
		pSubstorage = new cZipSubstorage{ this, topPathLevel };
		pSubstorage->SetTopZip(m_pZipStorage);

		pSubstorage->SetStoreManager(m_pStoreManager);

		auto* pNamedStorage = new cNamedStorage{ pSubstorage };

		pSubstorage->Release();

		m_pSubstorageTable->Insert(pNamedStorage);
	}

	if (strlen(pathRest))
		return pSubstorage->MakeSubstorage(pathRest);

	pSubstorage->AddRef();

	return pSubstorage;
}

///////////////////////////////////////

void cZipSubstorage::AddStream(const char* pName, sDirFileRecord* pDirRecord)
{
	if (!pName)
		CriticalMsg("cZipSubstorage::AddStream -- null name!");

	if (m_pStreamTable->Search(pName) != nullptr)
		CriticalMsg("cZipSubstorage::AddStream -- adding duplicate stream!");

	m_pStreamTable->Insert(new cNamedZipStream{ pName,
			pDirRecord->CompressedSize,
			pDirRecord->UncompressedSize,
			pDirRecord->HeaderOffset,
			pDirRecord->CompressionMethod });
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cZipStorage
//

cZipStorage::cZipStorage(IStore* pParent, IStoreStream* pZipStream, const char* pName)
	: cZipSubstorage{ pParent, pName },
	m_pZipStream{ pZipStream }
{
	if (!pParent)
		CriticalMsg("Zip file created without a parent storage!");
	if (!pZipStream)
		CriticalMsg("Zip file created without an input stream!");

	if (!pName)
		CriticalMsg("Zip file created without a name!");

	m_pZipStream->AddRef();
	m_pZipStorage = this;

	auto ZipSize = m_pZipStream->GetSize();
	auto EndRecStart = ZipSize - 22;
	if (ZipSize == 22)
		CriticalMsg("Zip file too small!");

	if (!m_pZipStream->SetPos(EndRecStart))
		CriticalMsg("Couldn't go to the start of zipfile end record");

	sDirEndRecord EndRecord{};

	m_pZipStream->Read(sizeof(sDirEndRecord), reinterpret_cast<char*>(&EndRecord));
	m_pZipStream->SetPos(EndRecord.DirOffset);

	for (auto nRecordsLeft = EndRecord.NumEntries; nRecordsLeft; --nRecordsLeft)
	{
		sDirFileRecord dirRecord{};
		m_pZipStream->Read(sizeof(sDirFileRecord), reinterpret_cast<char*>(&dirRecord));

		if (dirRecord.Signature != 0x02014b50) // Central directory file header signature
			CriticalMsg("Zip directory record with bad signature!");

		auto* pFilename = static_cast<char*>(Malloc(dirRecord.FilenameLen + 1));
		m_pZipStream->Read(dirRecord.FilenameLen, pFilename);
		pFilename[dirRecord.FilenameLen] = '\0';

		m_pZipStream->SetPos(dirRecord.CommentLen + dirRecord.ExtraLen + m_pZipStream->GetPos());

		if (dirRecord.ExternalAttributes & 0x10)
		{
			auto* pSubstore = MakeSubstorage(pFilename);
			pSubstore->Release();
		}
		else
		{
			char pPath[512]{};
			char pFileRoot[32]{};
			if (PathAndName(pFilename, pPath, pFileRoot))
			{
				auto* pSubstore = MakeSubstorage(pPath);
				if (!pSubstore)
					CriticalMsg1("Couldn't create zip substorage %s!", pPath);

				pSubstore->AddStream(pFileRoot, &dirRecord);
				pSubstore->Release();
			}
			else
			{
				AddStream(pFileRoot, &dirRecord);
			}
		}

		Free(pFilename);
	}
}

///////////////////////////////////////

cZipStorage::~cZipStorage()
{
	m_pZipStream->Close();
	m_pZipStream->Release();
	m_pZipStream = nullptr;
}

///////////////////////////////////////

struct sLocalFileHeader
{
	uint32 Signature;
	uint16 ExtractVersion;
	uint16 Flags;
	uint16 CompressionMethod;
	uint16 ModTime;
	uint16 ModDate;
	uint32 CRC32;
	uint32 CompressedSize;
	uint32 UncompressedSize;
	uint16 FilenameLen;
	uint16 ExtraLen;
};

static_assert(sizeof(sLocalFileHeader) == 30, "Invalid sLocalFileHeader size");

IStoreStream* cZipStorage::ReadyStreamAt(long nPos)
{
	sLocalFileHeader header{};

	m_pZipStream->SetPos(nPos);
	m_pZipStream->Read(sizeof(sLocalFileHeader), reinterpret_cast<char*>(&header));

	if (header.Signature != 0x04034b50) // Local file header signature ("PK\3\4") 
		CriticalMsg("Zip file header with bad signature!");

	m_pZipStream->SetPos(header.ExtraLen + header.FilenameLen + m_pZipStream->GetPos());
	m_pZipStream->AddRef();

	return m_pZipStream;
}

///////////////////////////////////////

ulong cZipStorage::LastModified()
{
	return m_pZipStream->LastModified();
}