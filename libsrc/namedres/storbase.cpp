#include <storbase.h>
#include <hshsttem.h>

#ifndef NO_DB_MEM
// Must be last header
#include <memall.h>
#include <dbmem.h>
#endif

cNamedStorage::cNamedStorage(const char* pName)
	: m_pStore{ nullptr }, m_pName{ nullptr }
{
	if (!pName)
		CriticalMsg("cNamedStorage being created without a name!");

	m_pName = static_cast<char*>(Malloc(strlen(pName) + 1));
	strcpy(m_pName, pName);
}

cNamedStorage::cNamedStorage(IStore* pStore)
	: m_pStore{ pStore }, m_pName{ nullptr }
{
	if (m_pStore)
	{
		m_pStore->AddRef();
		auto* pName = m_pStore->GetName();
		m_pName = static_cast<char*>(Malloc(strlen(pName) + 1));
		strcpy(m_pName, pName);
	}
}

cNamedStorage::~cNamedStorage()
{
	if (this->m_pStore)
	{
		m_pStore->Release();
		m_pStore = nullptr;
	}

	if (m_pName)
	{
		Free(m_pName);
		m_pName = nullptr;
	}
}

tHashSetKey cStorageHashByName::GetKey(tHashSetNode p) const
{
	return reinterpret_cast<tHashSetKey>((reinterpret_cast<cNamedStorage*>(p)->m_pName));
}

cNamedStream::cNamedStream(const char* pName, bool bExists)
	: m_pName{ nullptr }, m_bExists{ bExists }
{
	if (!pName)
		CriticalMsg("cNamedStream being created without a name!");

	m_pName = static_cast<char*>(Malloc(strlen(pName) + 1));
	strcpy(m_pName, pName);
}

cNamedStream::~cNamedStream()
{
	if (m_pName)
	{
		Free(m_pName);
		m_pName = nullptr;
	}
}

tHashSetKey cStreamHashByName::GetKey(tHashSetNode p) const
{
	return reinterpret_cast<tHashSetKey>((reinterpret_cast<cNamedStream*>(p)->m_pName));
}