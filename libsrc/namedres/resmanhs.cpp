#include <resmanhs.h>
#include <storeapi.h>
#include <hshsttem.h>
#include <filespec.h>
#include <str.h>

#ifndef NO_DB_MEM
// Must be last header
#include <memall.h>
#include <dbmem.h>
#endif

cResourceName::cResourceName(const char* pResName)
	: m_pName{ nullptr }, m_pFirstStream{ nullptr }, m_ppFullNames{ nullptr }, m_fFlags{ 0 }
{
	if (pResName)
	{
		m_pName = static_cast<char*>(Malloc(strlen(pResName) + 1));
		memmove(m_pName, pResName, strlen(pResName) + 1);
		strlwr(m_pName);
	}
}

cResourceName::~cResourceName()
{
	if (m_fFlags & 1)
	{
		for (int i = 0; i < m_ppFullNames->Size(); ++i)
			Free(m_ppFullNames[i]);

		if (m_ppFullNames)
			delete m_ppFullNames;
	}
	else
	{
		cResourceData* pNext = nullptr;
		for (auto pData = m_pFirstStream; pData != nullptr; pData = pNext)
		{
			pNext = pData->m_pNext;
			delete pData;
		}
	}

	m_pFirstStream = nullptr;
	if (m_pName)
	{
		Free(m_pName);
		m_pName = nullptr;
	}
}

cResourceTypeData::cResourceTypeData(IResType* pType, cResourceData* pResData, IRes* pRes)
	:
	m_pData{ nullptr },
	m_nSize{ 0 },
	m_pResMem{ nullptr },
	m_nUserLockCount{ 0 },
	m_nInternalLockCount{ 0 },
	m_pRes{ pRes },
	m_pType{ pType },
	m_pResourceData{ pResData },
	m_pProxiedRes{ nullptr },
	m_Freshed{ 0 }
{
	if (!pResData)
		CriticalMsg("cResourceTypeData created without a pResData!");

	if (pType)
		pType->AddRef();
}

cResourceTypeData::~cResourceTypeData()
{
	if (m_pData)
		CriticalMsg1("Not freeing Data for resource %s\n", GetName());
	if (m_pResMem)
	{
		m_pResMem->Release();
		m_pResMem = nullptr;
	}

	if (m_pType)
	{
		m_pType->Release();
		m_pType = nullptr;
	}

	if (m_pRes)
	{
		// m_pRes->Release(); TODO: do we need release?
		m_pRes = nullptr;
	}
}

const char* cResourceTypeData::GetName()
{
	return m_pResourceData->m_pNameData->m_pName;
}

cResourceData::cResourceData(IStore* pStore, cResourceName* pNameData)
	: m_pStore{ pStore }, m_pNameData{ pNameData }, m_ResourceTypeHash{}, m_pNext{ 0 }
{
	if (pStore)
		pStore->AddRef();
}

cResourceData::~cResourceData()
{
	m_ResourceTypeHash.DestroyAll();

	if (m_pStore)
	{
		m_pStore->Release();
		m_pStore = nullptr;
	}
}

cResTypeData::cResTypeData(const char* pName, IResType* pType)
	: m_Name{ 0 }, m_pFactories{ 0 }, m_nNumFactories{ 0 }
{
	if (pName && pType)
	{
		strncpy(m_Name, pName, kNameSize);
		m_Name[kNameSize - 1] = '\0';
		strlwr(m_Name);

		pType->AddRef();

		m_pFactories = reinterpret_cast<IResType**>(Malloc(sizeof(IResType*)));
		*m_pFactories = pType;
		m_nNumFactories = 1;
	}
}

cResTypeData::~cResTypeData()
{
	if (m_nNumFactories > 0)
	{
		for (int i = 0; i < m_nNumFactories; ++i)
			m_pFactories[i]->Release();

		Free(m_pFactories);
		m_pFactories = nullptr;

		m_nNumFactories = 0;
	}
}

cNamedResType::cNamedResType(IResType* pType)
	: m_pType{ pType }
{
	if (!pType)
		CriticalMsg("Creating a cNamedResType with a NULL type!");
	else
		m_pType->AddRef();
}

cNamedResType::~cNamedResType()
{
	if (m_pType)
	{
		m_pType->Release();
		m_pType = nullptr;
	}
}


cResourceData* cHashByResName::FindResData(const char* pName, IStore* pStore, BOOL bCreate)
{
	if (bCreate && !pStore)
	{
		CriticalMsg("FindResData: Can't create without pStore!");
		return nullptr;
	}

	if (!pName || *pName == '\0')
	{
		Warning(("FindResData: Empty name!"));
		return nullptr;
	}

	auto* pNameData = Search(pName);
	if (pNameData)
	{
		if (pNameData->m_fFlags & 1)
		{
			for (auto i = 0; i < pNameData->m_ppFullNames->Size(); ++i)
			{
				auto* pData = FindResData((*pNameData->m_ppFullNames)[i], pStore, FALSE);
				if (pData)
					return pData;
			}
		}
		else
		{
			auto* pData = pNameData->m_pFirstStream;
			auto* pPrev = pData;
			if (pStore || !pData)
			{
				while (pData)
				{
					if (pData->m_pStore == pStore)
						return pData;
					pPrev = pData;
					pData = pData->m_pNext;
				}

				if (bCreate)
				{
					pData = new cResourceData{ pStore, pNameData };
					if (pPrev)
						pPrev->m_pNext = pData;
					else
						pNameData->m_pFirstStream = pData;
				}
			}

			return pData;
		}
	}
	else if (bCreate)
	{
		pNameData = new cResourceName{ pName };
		auto* pData = new cResourceData{ pStore, pNameData };
		pNameData->m_pFirstStream = pData;

		Insert(pNameData);

		cFileSpec fileSpec{ pName };
		cStr root{};

		fileSpec.GetFileRoot(root);
		if (!strcmpi(pName, root))
			Warning(("FindResData: Created real entry without extension!\n"));

		pNameData = Search(root);
		if (!pNameData)
		{
			pNameData = new cResourceName{ root };
			pNameData->m_fFlags |= 1;

			pNameData->m_ppFullNames = new cDynArray<const char*>{};
			Insert(pNameData);
		}

		auto* pNameCopy = static_cast<char*>(Malloc(strlen(pName) + 1));
		strcpy(pNameCopy, pName);
		pNameData->m_ppFullNames->Append(pNameCopy);

		return pData;
	}
}