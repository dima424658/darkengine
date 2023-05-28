#pragma once

#include <comtools.h>
#include <storeapi.h>
#include <resguid.h>
#include <filespec.h>
#include <storezip.h>
#include <storbase.h>

class cZipStream;

class cNamedZipStream : public cNamedStream
{
public:
	friend cZipStream;

	cNamedZipStream(const char* pName, uint32 nCompressedSize, uint32 nUncompressedSize, uint32 nHeaderOffset, uint16 nCompressionMethod);

private:
	uint32 m_nCompressedSize;
	uint32 m_nUncompressedSize;
	uint32 m_nHeaderOffset;
	uint16 m_nCompressionMethod;
};

class cZipStream : public cCTUnaggregated<IStoreStream, &IID_IStoreStream, kCTU_NoSelfDelete>
{
public:
	cZipStream(IStore* pStore, cZipStorage* pMaster, cNamedZipStream* pInfo);
	~cZipStream();

	void STDMETHODCALLTYPE SetName(const char* pName) override;
	BOOL STDMETHODCALLTYPE Open() override;
	void STDMETHODCALLTYPE Close() override;
	void STDMETHODCALLTYPE GetName(char** pName) override;
	BOOL STDMETHODCALLTYPE SetPos(long nPos) override;
	long STDMETHODCALLTYPE GetPos() override;
	long STDMETHODCALLTYPE ReadAbs(long nStartPos, long nEndPos, char* pBuf) override;
	long STDMETHODCALLTYPE GetSize() override;
	long STDMETHODCALLTYPE Read(long nNumByes, char* pBuf) override;
	short STDMETHODCALLTYPE Getc() override;
	void STDMETHODCALLTYPE ReadBlocks(void* pBuf, long nSize, tStoreStreamBlockCallback, void* pData) override;
	ulong STDMETHODCALLTYPE LastModified() override;

private:
	 IStore* m_pStorage;
	 cZipStorage* m_pMaster;
	 cNamedZipStream* m_pInfo;
	 cFileSpec* m_pFileSpec;
	 long m_nLastPos;
	 char* m_pData;
	 int m_nOpenCount;
};