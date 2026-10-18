#include <algorithm>

#include <zipstrm.h>
#include <portableinflate.h>

extern "C"
{
#include <blast.h>
}

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
	unsigned char readBuffer[0x2000];
	char* pDest;
	const char* pDestLimit;
	ulong skip;
	int fComplete;
};

///////////////////////////////////////

unsigned int PkExplodeReader(void* param, unsigned char** buffer)
{
	auto* pInfo = reinterpret_cast<sPkExplodeInfo*>(param);

	if (!param || pInfo->fComplete)
		return 0;

	const auto size = pInfo->pSourceStream->Read(
		static_cast<int>(sizeof(pInfo->readBuffer)),
		reinterpret_cast<char*>(pInfo->readBuffer));
	if (size <= 0)
		return 0;

	*buffer = pInfo->readBuffer;
	return static_cast<unsigned>(size);
}

///////////////////////////////////////

int PkExplodeWriter(void* param, unsigned char* buffer, unsigned size)
{
	auto* pInfo = reinterpret_cast<sPkExplodeInfo*>(param);

	if (pInfo->skip)
	{
		const auto skipped = std::min<unsigned>(pInfo->skip, size);
		pInfo->skip -= skipped;
		buffer += skipped;
		size -= skipped;
	}

	const auto available = static_cast<size_t>(pInfo->pDestLimit - pInfo->pDest);
	const auto copied = std::min<size_t>(available, size);
	if (copied)
	{
		memcpy(pInfo->pDest, buffer, copied);
		pInfo->pDest += copied;
	}
	if (copied != size || pInfo->pDest == pInfo->pDestLimit)
	{
		pInfo->fComplete = 1;
		return 1;
	}
	return 0;
}

///////////////////////////////////////

int PkExplodeStreamToMem(IStoreStream* pSourceStream, void* pDest, int skip, int destMax)
{
	if (!destMax)
		destMax = 134217728;

	sPkExplodeInfo explodeInfo{};
	explodeInfo.pSourceStream = pSourceStream;
	explodeInfo.pDest = static_cast<char*>(pDest);
	explodeInfo.pDestLimit = static_cast<char*>(pDest) + destMax;
	explodeInfo.skip = skip;
	explodeInfo.fComplete = 0;

	auto result = blast(PkExplodeReader, &explodeInfo,
		PkExplodeWriter, &explodeInfo, nullptr, nullptr);
	if ((result != 0 && (result != 1 || !explodeInfo.fComplete)) ||
		explodeInfo.pDest > explodeInfo.pDestLimit)
	{
		CriticalMsg1("Expansion failed (%d)!", result);
		return 0;
	}

	return static_cast<int>(explodeInfo.pDest - static_cast<char*>(pDest));
}

///////////////////////////////////////

int ZInflateStreamToMem(IStoreStream* pStream, int nStreamSize, void* pData, int nSize)
{
	if (nStreamSize <= 0 || nSize < 0)
		return 0;

	auto* inputBuffer = static_cast<unsigned char*>(Malloc(nStreamSize));
	if (!inputBuffer)
		return -1;

	int totalReadSize = 0;
	while (totalReadSize < nStreamSize)
	{
		const auto readSize = pStream->Read(nStreamSize - totalReadSize,
			reinterpret_cast<char*>(inputBuffer + totalReadSize));
		if (readSize <= 0)
		{
			Free(inputBuffer);
			return -1;
		}
		totalReadSize += readSize;
	}

	const auto actualRead = RawDeflateExpand(inputBuffer, nStreamSize, pData, nSize);
	Free(inputBuffer);
	return static_cast<int>(actualRead);
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

	if (nPos < 0 || static_cast<uint32>(nPos) >= m_pInfo->m_nUncompressedSize)
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
	return static_cast<long>(m_pInfo->m_nUncompressedSize);
}

///////////////////////////////////////

long cZipStream::Read(long nNumBytes, char* pBuf)
{
	if (!m_nOpenCount)
		return -1;

	if (nNumBytes < 1)
		return -1;

	auto nBytesLeft = static_cast<long>(m_pInfo->m_nUncompressedSize) - m_nLastPos;
	if (nBytesLeft <= 0)
		return -1;

	if (nNumBytes > nBytesLeft)
		nNumBytes = static_cast<long>(m_pInfo->m_nUncompressedSize) - m_nLastPos;

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
		if (m_pData && m_nOpenCount && m_nLastPos >= 0 &&
			static_cast<uint32>(m_nLastPos) < m_pInfo->m_nUncompressedSize)
			return static_cast<unsigned char>(m_pData[m_nLastPos++]);
		else
			return -1;
	}

	char c{};
	if (m_nOpenCount && Read(1, &c) == 1)
		return static_cast<unsigned char>(c);

	return -1;
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

		nSize = callback(pBuf, static_cast<long>(nRead), nBlockIx, pCallbackData);
		if (nSize < 1)
			break;

		++nBlockIx;
	}

	m_nLastPos = static_cast<long>(p - m_pData);
}

///////////////////////////////////////

ulong cZipStream::LastModified()
{
	return m_pMaster->LastModified();
}

///////////////////////////////////////

