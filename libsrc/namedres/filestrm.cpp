#include <filestrm.h>
#include <filepath.h>
#include <lgassert.h>
#include <str.h>

#include <io.h>
#include <cstdio>

#ifndef NO_DB_MEM
// Must be last header
#include <memall.h>
#include <dbmem.h>
#endif

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cFileStream
//

cFileStream::cFileStream(IStore* pStore)
	: m_pStorage{ pStore },
	m_pFile{ 0 },
	m_pFileSpec{ 0 },
	m_nLastPos{ 0 },
	m_nOpenCount{ 0 }
{
	if (m_pStorage)
		m_pStorage->AddRef();
}

///////////////////////////////////////

cFileStream::~cFileStream()
{
	if (m_pFile)
	{
		char* pName;
		GetName(&pName);

		CriticalMsg1("Destroying stream %s that was not fully closed!", pName);
		Free(pName);

		fclose(m_pFile);
		m_pFile = nullptr;
	}

	if (m_pStorage)
	{
		m_pStorage->Release();
		m_pStorage = nullptr;
	}

	if (m_pFileSpec)
	{
		delete m_pFileSpec;
		m_pFileSpec = nullptr;
	}
}

///////////////////////////////////////

void cFileStream::SetName(const char* pName)
{
	if (!pName)
		return;

	if (m_pFileSpec)
	{
		delete m_pFileSpec;
		m_pFileSpec = nullptr;
	}

	if (m_pStorage)
		m_pFileSpec = new cFileSpec{ cFilePath{ m_pStorage->GetFullPathName() }, pName };
	else
		m_pFileSpec = new cFileSpec{ pName };
}

///////////////////////////////////////

BOOL cFileStream::Open()
{
	if (m_pFile)
	{
		++m_nOpenCount;
		return TRUE;
	}

	if (!m_pFileSpec)
		return FALSE;

	char* pFileName;
	GetName(&pFileName);

	m_pFile = fopen(pFileName, "rb");

	Free(pFileName);

	if (!m_pFile)
		return FALSE;

	m_nLastPos = 0;
	++m_nOpenCount;

	return TRUE;
}

///////////////////////////////////////

void cFileStream::Close()
{
	if (!m_pFile)
		return;

	--m_nOpenCount;

	if (m_nOpenCount == 0)
	{
		fclose(m_pFile);
		m_pFile = nullptr;
	}
}

///////////////////////////////////////

void cFileStream::GetName(char** ppName)
{
	cStr name{};

	m_pFileSpec->GetNameString(name, 0);
	*ppName = name.Detach();
}

///////////////////////////////////////

BOOL cFileStream::SetPos(long nPos)
{
	if (m_pFile)
	{
		m_nLastPos = nPos;
		return fseek(m_pFile, nPos, 0) == 0;
	}

	return FALSE;
}

///////////////////////////////////////

long cFileStream::GetPos()
{
	if (m_pFile)
		return m_nLastPos;

	return 0;
}

///////////////////////////////////////

long cFileStream::ReadAbs(long nStartPos, long nEndPos, char* pBuf)
{
	if (!m_pFile)
		return -1;

	if (m_nLastPos != nStartPos && fseek(m_pFile, nStartPos, 0))
		return -1;

	auto pos = fread(pBuf, 1u, nEndPos - nStartPos + 1, m_pFile);
	m_nLastPos = pos + nStartPos;

	return pos;
}

///////////////////////////////////////

long cFileStream::GetSize()
{
	if (m_pFile)
		return filelength(fileno(m_pFile));

	if (!m_pFileSpec)
		return -1;

	char* pFileName;
	GetName(&pFileName);

	m_pFile = fopen(pFileName, "rb");

	Free(pFileName);

	if (!m_pFile)
		return -1;

	auto nSize = filelength(fileno(m_pFile));
	fclose(m_pFile);
	// m_pFile = nullptr;

	return nSize;
}

///////////////////////////////////////

long cFileStream::Read(long nNumBytes, char* pBuf)
{
	if (!m_pFile || nNumBytes < 1)
		return -1;

	auto pos = fread(pBuf, 1, nNumBytes, m_pFile);
	m_nLastPos += pos;

	return pos;
}

///////////////////////////////////////

int16 cFileStream::Getc()
{
	if (!m_pFile)
		return -1;

	++m_nLastPos;

	return fgetc(m_pFile);
}

///////////////////////////////////////

void cFileStream::ReadBlocks(void* pBuf, long nSize, tStoreStreamBlockCallback callback, void* pCallbackData)
{
	if (!m_pFile)
		return;

	auto bDone = false;
	auto nRead = 0;
	auto nBlockIx = 0;

	while (!bDone)
	{
		nRead = fread(pBuf, 1u, nSize, m_pFile);
		if (nRead < nSize)
			bDone = true;

		if (callback)
		{
			nSize = callback(pBuf, nRead, nBlockIx, pCallbackData);
			if (nSize < 1)
				bDone = true;
		}

		++nBlockIx;
	}
	m_nLastPos += nRead;
}

///////////////////////////////////////

ulong cFileStream::LastModified()
{
	return m_pFileSpec->GetModificationTime();
}