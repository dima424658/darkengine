#include <algorithm>

#include <zipstrm.h>
#include <zlib.h>
#include <dynfunc.h>

#ifndef EXP_BUFFER_SIZE
#define EXP_BUFFER_SIZE    12596
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Dynamic loading
//

static void PkFail()
{
	CriticalMsg("Failed to locate and load implode.dll!");
	exit(1);
}

typedef unsigned int(*tPkReadFunc)(char* buf, unsigned int* size, void* param);
typedef void(*tPkWriteFunc)(char* buf, unsigned int* size, void* param);

DeclDynFunc(unsigned int, PkImplode, (tPkReadFunc, tPkWriteFunc, char*, void*, unsigned int*, unsigned int*));
DeclDynFunc(unsigned int, PkExplode, (tPkReadFunc, tPkWriteFunc, char*, void*));

ImplDynFunc(PkImplode, "implode.dll", "implode", PkFail);
ImplDynFunc(PkExplode, "implode.dll", "explode", PkFail);

#define DynPkImplode (DynFunc(PkImplode).GetProcAddress())
#define DynPkExplode (DynFunc(PkExplode).GetProcAddress())

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cNamedZipStream
//

cNamedZipStream::cNamedZipStream(const char* pName, uint32 nCompressedSize,
	uint32 nUncompressedSize, uint32 nHeaderOffset, uint16 nCompressionMethod)
	: cNamedStream{ pName, TRUE },
	m_nCompressedSize{ nCompressedSize },
	m_nUncompressedSize{ nUncompressedSize },
	m_nHeaderOffset{ nHeaderOffset },
	m_nCompressionMethod{ nCompressionMethod }
{
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cZipStream
//

cZipStream::cZipStream(IStore* pStore, cZipStorage* pMaster, cNamedZipStream* pInfo)
	: m_pStorage{ pStore },
	m_pMaster{ pMaster },
	m_pInfo{ pInfo },
	m_pFileSpec{ nullptr },
	m_nLastPos{ 0 },
	m_pData{ 0 },
	m_nOpenCount{ 0 }
{
	if (!m_pStorage)
		CriticalMsg("Creating Zip stream without a storage.");

	if (!m_pMaster)
		CriticalMsg("Creating Zip stream without a master.");

	if (!m_pInfo)
		CriticalMsg("Creating Zip stream without info.");

	m_pStorage->AddRef();
	m_pMaster->AddRef();
}

///////////////////////////////////////

cZipStream::~cZipStream()
{
	if (m_nOpenCount > 0)
	{
		char* pName = nullptr;
		GetName(&pName);
		Warning(("cZipStream: %s deleted without being fully Closed!\n", pName));
		Free(pName);
		Free(m_pData);
		m_nOpenCount = 0;
		m_pData = nullptr;
	}

	if (m_pStorage)
	{
		m_pStorage->Release();
		m_pStorage = nullptr;
	}

	if (m_pMaster)
	{
		m_pMaster->Release();
		m_pMaster = nullptr;
	}

	if (m_pFileSpec)
	{
		delete m_pFileSpec;
		m_pFileSpec = nullptr;
	}
}

///////////////////////////////////////

void cZipStream::SetName(const char* pName)
{
	if (!pName)
		CriticalMsg("Setting Zip stream name to null");

	if (m_pFileSpec)
	{
		delete m_pFileSpec;
		m_pFileSpec = nullptr;
	}

	if (m_pStorage)
	{
		cFilePath storePath{ m_pStorage->GetFullPathName() };
		m_pFileSpec = new cFileSpec{ storePath, pName };
	}
	else
	{
		m_pFileSpec = new cFileSpec{ pName };
	}
}

///////////////////////////////////////

struct sPkExplodeInfo
{
	IStoreStream* pSourceStream;
	const char* pSource;
	const char* pSourceLimit;
	char* pDest;
	const char* pDestLimit;
	ulong skip;
	char* pReadBuf;
	int fComplete;
};

///////////////////////////////////////

unsigned int PkExplodeReader(char* buf, unsigned int* size, void* param)
{
	auto* pInfo = reinterpret_cast<sPkExplodeInfo*>(param);

	if (!param || pInfo->fComplete)
		return 0;

	if (pInfo->pSource >= pInfo->pSourceLimit)
	{
		auto targetReadSize = pInfo->skip ? pInfo->skip : pInfo->pDestLimit - pInfo->pDest;
		if (targetReadSize < 1)
			targetReadSize = 1;
		else if (targetReadSize > 0x10000)
			targetReadSize = 0x10000;

		auto a = pInfo->pSourceStream->Read(targetReadSize, pInfo->pReadBuf);
		pInfo->pSource = pInfo->pReadBuf;
		pInfo->pSourceLimit = a + pInfo->pSource;
	}
	if (pInfo->pSourceLimit == pInfo->pSource)
		return 0;

	auto length = std::min(pInfo->pSourceLimit - pInfo->pSource, static_cast<ptrdiff_t>(*size));
	memcpy(buf, pInfo->pSource, length);
	pInfo->pSource += length;
	*size = length;

	return length;
}

///////////////////////////////////////

void PkExplodeWriter(char* buf, unsigned int* size, void* param)
{
	auto* pInfo = reinterpret_cast<sPkExplodeInfo*>(param);

	auto actualSize = *size;

	if (pInfo->skip)
	{
		if (pInfo->skip > actualSize)
		{
			pInfo->skip -= actualSize;
			return;
		}

		actualSize -= pInfo->skip;
		buf += pInfo->skip;
		pInfo->skip = 0;

	}

	if (actualSize + pInfo->pDest > pInfo->pDestLimit)
		actualSize = pInfo->pDestLimit - pInfo->pDest;

	memcpy(pInfo->pDest, buf, actualSize);
	pInfo->pDest += actualSize;
	if (pInfo->pDest > pInfo->pDestLimit)
		pInfo->fComplete = 1;
}

///////////////////////////////////////

int PkExplodeStreamToMem(IStoreStream* pSourceStream, void* pDest, int skip, int destMax)
{
	static char* pPkBuffer = nullptr;

	if (!pPkBuffer)
		pPkBuffer = static_cast<char*>(Malloc(0x13134u));

	auto pWorkBuf = pPkBuffer;
	if (!destMax)
		destMax = 134217728;

	sPkExplodeInfo explodeInfo{};
	explodeInfo.pSourceStream = pSourceStream;
	explodeInfo.pReadBuf = pPkBuffer + EXP_BUFFER_SIZE;
	explodeInfo.pSource = pPkBuffer + EXP_BUFFER_SIZE;
	explodeInfo.pSourceLimit = pPkBuffer + EXP_BUFFER_SIZE;
	explodeInfo.pDest = static_cast<char*>(pDest);
	explodeInfo.pDestLimit = static_cast<char*>(pDest) + destMax;
	explodeInfo.skip = skip;
	explodeInfo.fComplete = 0;

	auto result = DynPkExplode(PkExplodeReader, PkExplodeWriter, (char*)pWorkBuf, &explodeInfo);
	if (result && (result != 4 || !explodeInfo.fComplete) || explodeInfo.pDest > (char*)explodeInfo.pDestLimit)
	{
		CriticalMsg1("Expansion failed (%d)!", result);
		return 0;
	}

	return explodeInfo.pDest - static_cast<char*>(pDest);
}

///////////////////////////////////////

void* ZAllocInterface(void*, unsigned int items, unsigned int size)
{
	return Malloc(size * items);
}

///////////////////////////////////////

void ZFreeInterface(void*, void* addr)
{
	Free(addr);
}

///////////////////////////////////////

int ZInflateStreamToMem(IStoreStream* pStream, int nStreamSize, void* pData, int nSize)
{
	static constexpr auto BufferSize = 0x10000;
	if (!nStreamSize)
		return 0;

	auto inputBuffer = static_cast<char*>(Malloc(BufferSize));
	auto fFlush = 0;
	auto finished = false;

	auto actualReadSize = pStream->Read(std::min(nStreamSize, BufferSize), inputBuffer);
	if (!actualReadSize)
	{
		CriticalMsg("Inflating empty file!");
		Free(inputBuffer);
		return 0;
	}

	auto totalReadSize = actualReadSize;
	if (actualReadSize == nStreamSize)
		fFlush = 4;

	z_stream_s zBlock{};
	zBlock.next_in = reinterpret_cast<Bytef*>(inputBuffer);
	zBlock.avail_in = actualReadSize;
	zBlock.next_out = reinterpret_cast<Bytef*>(pData);
	zBlock.avail_out = nSize;
	zBlock.zalloc = ZAllocInterface;
	zBlock.zfree = ZFreeInterface;
	zBlock.opaque = 0;

	auto ret = inflateInit2(&zBlock, -15);
	if (ret != Z_OK)
		CriticalMsg1("zlib inflateInit failed with %d\n", ret);

	do
	{
		if (!zBlock.avail_in && totalReadSize < nStreamSize)
		{
			pStream->Read(std::min(static_cast<long>(BufferSize), nStreamSize - totalReadSize), inputBuffer);
			zBlock.next_in = reinterpret_cast<Bytef*>(inputBuffer);
			zBlock.avail_in = actualReadSize;
			totalReadSize += actualReadSize;
		}
		ret = inflate(&zBlock, fFlush);
		if (ret == Z_STREAM_END)
		{
			finished = true;
		}
		else
		{
			if (ret)
			{
				CriticalMsg1("zlib inflate returned %d!\n", ret);
				Free(inputBuffer);
				return -1;
			}

			if (!zBlock.avail_out)
				CriticalMsg("zlib inflate: buffer full before end!");
		}
	} while (!finished);

	auto actualRead = nSize - zBlock.avail_out;
	inflateEnd(&zBlock);
	Free(inputBuffer);

	return actualRead;
}

///////////////////////////////////////

BOOL cZipStream::Open()
{
	IStoreStream* pRealStream = nullptr;

	if (m_pInfo->m_nCompressionMethod)
	{
		if (m_pData)
		{
			++m_nOpenCount;
			return TRUE;
		}

		m_pData = static_cast<char*>(Malloc(m_pInfo->m_nUncompressedSize));
		if (!m_pData)
			return FALSE;

		pRealStream = m_pMaster->ReadyStreamAt(m_pInfo->m_nHeaderOffset);
		if (!pRealStream)
			CriticalMsg("Opening zip stream with no real stream!");

		auto nRealSize = 0;
		if (m_pInfo->m_nCompressionMethod == 8)
			nRealSize = ZInflateStreamToMem(
				pRealStream,
				m_pInfo->m_nCompressedSize,
				m_pData,
				m_pInfo->m_nUncompressedSize);
		else if (m_pInfo->m_nCompressionMethod == 10)
			nRealSize = PkExplodeStreamToMem(pRealStream, m_pData, 0, m_pInfo->m_nUncompressedSize);

		if (nRealSize != m_pInfo->m_nUncompressedSize)
			CriticalMsg2("Zip stream: expected size of %ld, got %ld", m_pInfo->m_nUncompressedSize, nRealSize);
	}
	else
	{
		if (m_nOpenCount > 0)
		{
			++m_nOpenCount;
			return TRUE;
		}

		pRealStream = m_pMaster->ReadyStreamAt(m_pInfo->m_nHeaderOffset);
		if (!pRealStream)
			CriticalMsg("Opening zip stream with no real stream!");
	}

	pRealStream->Release();
	m_nLastPos = 0;
	++m_nOpenCount;

	return TRUE;
}

///////////////////////////////////////

void cZipStream::Close()
{
	if (!m_nOpenCount)
		CriticalMsg("cZipStream closed more than opened");

	if (m_pInfo->m_nCompressionMethod)
	{
		--m_nOpenCount;
		if (!m_nOpenCount)
		{
			Free(m_pData);
			m_pData = nullptr;
		}
	}
	else
	{
		--m_nOpenCount;
	}
}

///////////////////////////////////////

void cZipStream::GetName(char** ppName)
{
	cStr name{};
	m_pFileSpec->GetNameString(name);
	*ppName = name.Detach();;
}

///////////////////////////////////////

BOOL cZipStream::SetPos(long nPos)
{
	if (!m_nOpenCount)
		return FALSE;

	if (nPos >= m_pInfo->m_nUncompressedSize)
		return FALSE;

	m_nLastPos = nPos;
	return TRUE;
}

///////////////////////////////////////

long cZipStream::GetPos()
{
	if (m_nOpenCount)
		return m_nLastPos;
	else
		return -1;
}

///////////////////////////////////////

long cZipStream::ReadAbs(long nStartPos, long nEndPos, char* pBuf)
{
	if (m_nOpenCount && SetPos(nStartPos))
		return Read(nEndPos - nStartPos, pBuf);

	return -1;
}

///////////////////////////////////////

long cZipStream::GetSize()
{
	return m_pInfo->m_nUncompressedSize;
}

///////////////////////////////////////

long cZipStream::Read(long nNumBytes, char* pBuf)
{
	if (!m_nOpenCount)
		return -1;

	if (nNumBytes < 1)
		return -1;

	auto nBytesLeft = m_pInfo->m_nUncompressedSize - m_nLastPos;
	if (nBytesLeft <= 0)
		return -1;

	if (nNumBytes > nBytesLeft)
		nNumBytes = m_pInfo->m_nUncompressedSize - m_nLastPos;

	if (m_pInfo->m_nCompressionMethod)
	{
		if (!m_pData)
			return -1;

		memmove(pBuf, &m_pData[m_nLastPos], nNumBytes);
	}
	else
	{
		auto pRealStream = m_pMaster->ReadyStreamAt(m_pInfo->m_nHeaderOffset);
		if (!pRealStream)
			CriticalMsg("Opening zip stream with no real stream!");

		pRealStream->SetPos(m_nLastPos + pRealStream->GetPos());
		auto size = pRealStream->Read(nNumBytes, pBuf);

		pRealStream->Release();
		if (size != nNumBytes)
			CriticalMsg("Read failed to get the right number of bytes.");
	}

	m_nLastPos += nNumBytes;
	return nNumBytes;
}

///////////////////////////////////////

short cZipStream::Getc()
{
	if (m_pInfo->m_nCompressionMethod)
	{
		if (m_pData && m_nOpenCount && m_nLastPos < m_pInfo->m_nUncompressedSize)
			return m_pData[m_nLastPos++];
		else
			return -1;
	}

	char c{};
	if (m_nOpenCount && Read(1, &c) == 1)
		return c;
}

///////////////////////////////////////

void cZipStream::ReadBlocks(void* pBuf, long nSize, tStoreStreamBlockCallback callback, void* pCallbackData)
{
	CriticalMsg("cZipStream::ReadBlocks Not implemented.");
	auto nBlockIx = 0;
	auto* p = &m_pData[m_nLastPos];
	auto* pEnd = &m_pData[m_pInfo->m_nUncompressedSize];

	if (!m_pData || !m_nOpenCount || !callback)
		return;

	while (p < pEnd)
	{
		auto nRead = (p + nSize) < pEnd ? nSize : pEnd - p;
		memmove(pBuf, p, nRead);
		p += nRead;

		nSize = callback(pBuf, nRead, nBlockIx, pCallbackData);
		if (nSize < 1)
			break;

		++nBlockIx;
	}

	m_nLastPos = p - m_pData;
}

///////////////////////////////////////

ulong cZipStream::LastModified()
{
	return m_pMaster->LastModified();
}

///////////////////////////////////////

