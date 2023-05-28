#include <resman.h>

#include <storeapi.h>
#include <resthred.h>
#include <mprintf.h>

void cResMan::InstallResourceType(const char* pExt, IResType* pType)
{
	if (pType && pExt)
	{
		auto* pData = m_ResTypeHash.Search(pExt);

		if (pData)
		{
			++pData->m_nNumFactories;
			pData->m_pFactories = reinterpret_cast<IResType**>(Realloc(pData->m_pFactories, sizeof(IResType*) * pData->m_nNumFactories));
			pData->m_pFactories[pData->m_nNumFactories - 1] = pType;
			pType->AddRef();
		}
		else
		{
			m_ResTypeHash.Insert(new cResTypeData{ pExt, pType });
		}
	}
}

///////////////////////////////////////

void cResMan::RemoveResourceType(const char* pExt, IResType* pType)
{
	if (pType && pExt)
	{
		auto* pData = m_ResTypeHash.Search(pExt);

		if (pData)
		{
			int i;
			for (i = 0; i < pData->m_nNumFactories && pData->m_pFactories[i] != pType; ++i)
				;

			if (i == pData->m_nNumFactories)
				return;

			if (pData->m_nNumFactories == 1)
				m_ResTypeHash.Remove(pData);
			else
			{
				pData->m_pFactories[pData->m_nNumFactories - 1]->Release();
				if (i < pData->m_nNumFactories - 1)
					memmove(&pData->m_pFactories[i], &pData->m_pFactories[i + 1], sizeof(IResType*) * (pData->m_nNumFactories - (i + 1)));
				--pData->m_nNumFactories;
			}
		}
	}
}

///////////////////////////////////////

void cResMan::FreeData(cResourceTypeData* pData, BOOL bTestUser)
{
	cAutoResThreadLock lock;

	if (pData)
	{
		cResourceTypeData* pTestData;
		if (pData->m_pProxiedRes)
			pTestData = pData->m_pProxiedRes;
		else
			pTestData = pData;

		if (bTestUser && pTestData->m_nUserLockCount)
			CriticalMsg2("ResMan FreeData callback ignored. UserLock count = %d for %s\n", pTestData->m_nUserLockCount, pTestData->GetName());

		if (pTestData->m_nInternalLockCount != 1)
			CriticalMsg2("ResMan FreeData callback ignored. Internal Lock count = %d for %s\n", pTestData->m_nInternalLockCount, pTestData->GetName());

		if (bTestUser && pTestData->m_nUserLockCount || pTestData->m_nInternalLockCount != 1)
		{
			CriticalMsg("Not Freeing locked data");
			return;
		}

		pData->m_nInternalLockCount = 0;
		if (pData->m_pProxiedRes)
			pData->m_pProxiedRes = nullptr;
		else
		{
			IResMemOverride* pResMem;
			if (pData->m_pResMem)
				pResMem = pData->m_pResMem;
			else
				pResMem = &m_DefResMem;

			IResControl* pResControl;
			if (SUCCEEDED(pData->m_pRes->QueryInterface(IID_IResControl, (void**)&pResControl)))
			{
				if (g_fResPrintDrops)
				{
					char* pathName;
					pData->m_pRes->GetCanonPathName(&pathName);
					mprintf("cResMan::FreeData(): Freeing %s\n", pathName);

					if (pathName)
						Free(pathName);
				}

				if (!pResControl->FreeData(pData->m_pData, pData->m_nSize, pResMem))
					Warning(("cResMan::FreeData -- couldn't free resource!\n"));

				m_pResStats->LogStatRes(pData->m_pRes, eResourceStats::MemFreed);
				pResControl->Release();
			}
		}

		pData->m_pData = nullptr;
		pData->m_pRes->Release();
	}
}

///////////////////////////////////////

cResourceTypeData* cResMan::GetResourceTypeData(IRes* pRes)
{
	IResControl* pResControl;
	if (SUCCEEDED(pRes->QueryInterface(IID_IResControl, (void**)&pResControl)))
	{
		auto manData = pResControl->GetManData();
		cResourceTypeData* pResData = reinterpret_cast<cResourceTypeData*>(manData);
		pResControl->Release();

		if (manData == NO_RES_APP_DATA) // TODO: nullptr
		{
			DbgReportWarning("Resource without any Manager Data!\n");
			return nullptr;
		}

		return pResData;
	}

	CriticalMsg("Resource without an IResControl!");
	return nullptr;
}

///////////////////////////////////////

bool cResMan::VerifyStorage(IStore* pStorage)
{
	return pStorage && pStorage->Exists();
}

///////////////////////////////////////

IRes* cResMan::GetResource(const char* pName, const char* pTypeName, IStore* pStore)
{
	if (!pStore)
		CriticalMsg("Trying to GetResource without a storage!");

	if (!pTypeName)
		CriticalMsg("Trying to GetResource without a type!");

	auto* pData = m_ResTable.FindResData(pName, pStore, 0);
	if (pData)
	{
		auto* pTypeData = pData->m_ResourceTypeHash.Search(pTypeName);
		if (pTypeData)
		{
			pTypeData->m_pRes->AddRef();
			return pTypeData->m_pRes;
		}
	}

	return nullptr;
}

///////////////////////////////////////

void cResMan::MungePaths(const char* pPathName, const char* pExpRelPath, char** ppRelPath, char** ppName, char* pOldSlash, int* pbCombinedPaths)
{
	if (!pPathName || !ppRelPath || !ppName || !pOldSlash || !pbCombinedPaths)
		CriticalMsg("SplitPath called with missing arguments!");

	const char* pImpRelPath = nullptr;
	*pbCombinedPaths = 0;
	*ppRelPath = nullptr;
	*ppName = nullptr;
	*pOldSlash = '\0';

	auto nNameSize = strlen(pPathName);

	int i;
	for (i = nNameSize - 1; i >= 0 && pPathName[i] != '\\' && pPathName[i] != '/'; --i)
		;

	if ((signed int)(nNameSize - i) <= 32 && (signed int)nNameSize <= 512)
	{
		int nImpLen = 0;

		if (i >= 0)
		{
			*pOldSlash = pPathName[i];
			const_cast<char*>(pPathName)[i] = '\0';
			pImpRelPath = pPathName;
			*ppName = (char*)&pPathName[i + 1];
			nImpLen = i;
		}
		else
		{
			*ppName = (char*)pPathName;
		}

		if (pExpRelPath)
		{
			if (pImpRelPath)
			{
				*pbCombinedPaths = 1;

				auto needSlash = false;
				auto nExpLen = strlen(pExpRelPath);
				if (pExpRelPath[nExpLen - 1] != '/' && pExpRelPath[nExpLen - 1] != '\\')
				{
					needSlash = true;
					++nExpLen;
				}

				*ppRelPath = static_cast<char*>(Malloc(nExpLen + nImpLen + 1));
				strcpy(*ppRelPath, pExpRelPath);

				if (needSlash)
					strcat(*ppRelPath, "\\");
				strcat(*ppRelPath, pImpRelPath);
			}
			else
			{
				*ppRelPath = (char*)pExpRelPath;
			}
		}
		else
		{
			if (pImpRelPath)
				*ppRelPath = (char*)pImpRelPath;
		}
	}
	else
	{
		Warning(("SplitPath: name too long: %s\n", pPathName));
	}
}

///////////////////////////////////////

void cResMan::RestorePath(char* pPath, char* pName, char OldSlash, int bCombinedPaths)
{
	if (OldSlash)
		*(pName - 1) = OldSlash;

	if (bCombinedPaths)
		Free(pPath);
}

///////////////////////////////////////

cResourceTypeData* cResMan::FindResourceTypeData(IStore* pStore, const char* pResName, IResType* pType)
{
	auto* pData = m_ResTable.FindResData(pResName, pStore, 0);
	if (pData)
		return pData->m_ResourceTypeHash.Search(pType->GetName());

	return nullptr;
}

///////////////////////////////////////

IRes* cResMan::CreateResource(IStore* pStorage, const char* pName, const char* pExt, const char* pTypeName, uint)
{
	if (!pTypeName)
		CriticalMsg("CreateResource called without a type!");

	if (!pStorage || !pName)
		return nullptr;

	if (!pExt)
		pExt = "";

	auto* pType = GetResType(pTypeName);
	if (!pType)
	{
		CriticalMsg("Caller asked to create unknown type of resource!");
		return nullptr;
	}

	IResMemOverride* pResMem;
	auto* pResource = pType->CreateRes(pStorage, pName, pExt, &pResMem);

	if (pResource)
	{

		auto* pData = m_ResTable.FindResData(pName, pStorage, 1);
		if (!pData)
			CriticalMsg("Couldn't create entry in global resource table!");

		auto* pTypeData = new cResourceTypeData{ pType, pData, pResource };
		pData->m_ResourceTypeHash.Insert(pTypeData);

		pTypeData->m_pResMem = pResMem;
		pTypeData->m_Freshed = m_FreshStamp;

		IResControl* pResControl;
		if (SUCCEEDED(pResource->QueryInterface(IID_IResControl, (void**)&pResControl)))
		{
			pResControl->SetManData(reinterpret_cast<DWORD>(pTypeData));
			pResControl->Release();
		}
	}

	return pResource;
}

///////////////////////////////////////

IResType* cResMan::GetResType(const char* pTypeName)
{
	auto* pTypeEntry = m_ResTypeByNameHash.Search(pTypeName);
	if (!pTypeEntry)
	{
		Warning(("GetResType called with unknown type %s!\n", pTypeName));
		return nullptr;
	}

	return pTypeEntry->m_pType;
}