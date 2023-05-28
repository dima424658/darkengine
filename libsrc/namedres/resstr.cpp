#include <resstr.h>
#include <resbastm.h>
#include <growblk.h>
#include <allocapi.h>
#include <hashset.h>
#include <hshsttem.h>

#ifndef NO_DB_MEM
// Must be last header
#include <memall.h>
#include <dbmem.h>
#endif

class cStringResIndexHash;

struct sStringTable
{
	cStringResIndexHash* LookupTable;
	char* pStringData;
};

class cStringResEntry
{
public:
	friend cStringResource;
	friend cStringResIndexHash;

	cStringResEntry(const char* pName, int nIndex)
		: m_pName{ nullptr }, m_nIndex{ 0 }
	{
		if (!pName)
		{
			Warning(("cStringResEntry created with no name!\n"));
			return;
		}

		m_pName = static_cast<char*>(malloc(strlen(pName) + 1));
		strcpy(m_pName, pName);
	}

	~cStringResEntry()
	{
		if (m_pName)
		{
			free(m_pName);
			m_pName = nullptr;
		}
	}

private:
	char* m_pName;
	int m_nIndex;
};

class cStringResIndexHash : public cStrIHashSet<cStringResEntry*>
{
public:

	tHashSetKey GetKey(tHashSetNode p) const override
	{
		return p ? reinterpret_cast<tHashSetKey>(reinterpret_cast<cStringResEntry*>(p)->m_pName) : nullptr;
	}
};

cStringResource::cStringResource(IStore* pStore, const char* pName, IResType* pType)
	: cResourceBase{ pStore, pName, pType } {}

void* cStringResource::LoadData(ulong* pSize, ulong* pTimestamp, IResMemOverride* pResMem)
{
	cAutoResThreadLock lock{};
	cGrowingBlock pBlock{};

	if (!pResMem)
		return nullptr;

	auto* pStream = OpenStream();
	if (!pStream)
	{
		CriticalMsg1("Unable to open stream: %s", GetName());
		return nullptr;
	}

	LGALLOC_PUSH_CREDIT();
	auto* pTable = reinterpret_cast<sStringTable*>(pResMem->ResMalloc(sizeof(sStringTable)));
	LGALLOC_POP_CREDIT();

	pTable->LookupTable = new cStringResIndexHash{};

	auto finished = false;
	char name[64]{};
	while (!finished)
	{
		if (!GetStrName(pStream, name))
			break;

		if (!SkipWhitespace(pStream))
			break;

		auto ci = pStream->Getc();
		if (ci == -1)
			break;

		if (ci != '"')
		{
			Warning(("cStringRes: expected quote in %s, near char %ld\n", GetName(), pStream->GetPos())); // TODO ?
			if (!SkipLine(pStream))
				break;
			else
				continue;
		}

		auto c = ci;
		ci = pStream->Getc();
		if (ci == -1)
			break;
		c = ci;

		auto stringStart = pBlock.GetSize();
		while (c != '"')
		{
			if (c == '\\')
			{
				ci = pStream->Getc();
				if (ci == -1)
				{
					finished = true;
					break;
				}

				c = ci;

				switch (ci)
				{
				case '\n':
					break;
				case '\r':
					ci = pStream->Getc();
					if (ci == -1)
						finished = true;
					else
						c = ci;
					break;
				case 'n':
					pBlock.Append('\n');
					break;
				case 't':
					pBlock.Append('\t');
					break;
				default:
					pBlock.Append(c);
				}
			}
			else
			{
				pBlock.Append(c);
			}

			if (finished)
				break;

			ci = pStream->Getc();
			if (ci == -1)
			{
				finished = true;
				break;
			}
			c = ci;
		}

		if (finished)
			break;

		pBlock.Append('\0');

		auto* pEntry = new cStringResEntry{ name, stringStart };
		pTable->LookupTable->Insert(pEntry);
	}

	auto finalSize = pBlock.GetSize();


	LGALLOC_PUSH_CREDIT();
	pTable->pStringData = static_cast<char*>(pResMem->ResMalloc(finalSize));
	LGALLOC_POP_CREDIT();

	memcpy(pTable->pStringData, pBlock.GetBlock(), finalSize);

	if (pTimestamp)
		*pTimestamp = pStream->LastModified();

	pStream->Close();
	pStream->Release();

	if (pSize)
		*pSize = finalSize;

	return pTable;
}

BOOL cStringResource::FreeData(void* pUntypedData, ulong nSize, IResMemOverride* pResMem)
{
	cAutoResThreadLock lock{};

	if (!pUntypedData)
		CriticalMsg("FreeData: nothing to free!");

	auto* pData = reinterpret_cast<sStringTable*>(pUntypedData);

	pResMem->ResFree(pData->pStringData);
	// if (pData->LookupTable) TODO ???

	pResMem->ResFree(pData);

	return TRUE;
}

void cStringResource::StringPreload(const char*)
{
	cAutoResThreadLock lock{};
	PreLoad();
}

char* cStringResource::StringLock(const char* pStrName)
{
	cAutoResThreadLock lock{};

	auto* pTable = reinterpret_cast<sStringTable*>(Lock());
	if (!pTable)
		return nullptr;

	auto* pEntry = pTable->LookupTable->Search(pStrName);
	if (pEntry)
		return &pTable->pStringData[pEntry->m_nIndex];

	Unlock();
}

void cStringResource::StringUnlock(const char* pStrName)
{
	cAutoResThreadLock lock{};

	Unlock();
}

int cStringResource::StringExtract(char* pStrName, char* pBuf, int nSize)
{
	cAutoResThreadLock lock{};

	
	auto* pStr = StringLock(pStrName);
	if (!pStr)
	{
		*pBuf = '\0';
		return FALSE;
	}

	strncpy(pBuf, pStr, nSize - 1);
	pBuf[nSize - 1] = '\0';

	StringUnlock(pStrName);

	return TRUE;
}

int cStringResource::SkipLine(IStoreStream* pStream)
{
	for (auto c = pStream->Getc(); c != '\n'; c = pStream->Getc())
		if (c == -1)
			return false;

	return true;
}

int cStringResource::SkipWhitespace(IStoreStream* pStream)
{
	for (auto c = pStream->Getc(); c == ' ' || c == 0x9 || c == '\n' || c == '\r' || c == '/'; c = pStream->Getc())
	{
		if (c == -1)
			return false;

		if (c == 47)
		{
			auto cia = pStream->Getc();
			if (cia == -1)
				return 0;

			if ((char)cia == 47)
			{
				while (c != 10)
				{
					auto cib = pStream->Getc();
					if (cib == -1)
						return 0;
					c = cib;
				}
			}
			else
			{
				if ((char)cia != 42)
				{
					pStream->SetPos(pStream->GetPos() - 2);
					return true;
				}
				auto cie = cia;
				while (cie != 47)
				{
					auto cic = pStream->Getc();
					if (cic == -1)
						return 0;

					int16 cid;
					for (auto ca = cic; ca != 42; ca = cid)
					{
						cid = pStream->Getc();
						if (cid == -1)
							return 0;
					}
					cie = pStream->Getc();
					if (cie == -1)
						return 0;
				}
			}
		}
	}

	pStream->SetPos(pStream->GetPos() - 1);
	return true;
}

int cStringResource::GetStrName(IStoreStream* pStream, char* pBuf)
{
	while (true)
	{
		auto idx = 0;
		if (!SkipWhitespace(pStream))
			return false;

		for (auto c = pStream->Getc(); isalnum(c) || c == '_'; c = pStream->Getc())
		{
			if (c == -1)
				return false;

			if (idx >= 64)
			{
				Warning(("cStringRes: name too long in %s, near char %ld\n", GetName(), pStream->GetPos()));
				if (!SkipLine(pStream))
					return false;
				continue;
			}
			
			pBuf[idx++] = c;
		}

		pStream->SetPos(pStream->GetPos() - 1);

		if (!idx)
		{
			Warning(("cStringRes: GetStrName: expected name in %s, near char %ld\n", GetName(), pStream->GetPos()));
			if (SkipLine(pStream))
				continue;
			return false;
		}

		pBuf[idx] = '\0';
		if (!SkipWhitespace(pStream))
			return false;

		auto c = pStream->Getc();
		if (c == -1)
			return false;

		if (c == ':')
			return true;
		else
		{
			Warning(("cStringRes: GetStrName: expected colon in %s, near char %ld\n", GetName(), pStream->GetPos()));
			if (SkipLine(pStream))
				continue;
			return false;
		}
	}
}
