#pragma once

#include <storeapi.h>
#include <filespec.h>
#include <cstdio>

class cFileStream : public cCTUnaggregated<IStoreStream, &IID_IStoreStream, kCTU_NoSelfDelete>
{
public:
	cFileStream(IStore* pStore);
	~cFileStream();

	// OnFinalRelease

	void STDMETHODCALLTYPE SetName(const char* pName) override;
	BOOL STDMETHODCALLTYPE Open() override;
	void STDMETHODCALLTYPE Close() override;
	void STDMETHODCALLTYPE GetName(char** ppName) override;
	BOOL STDMETHODCALLTYPE SetPos(long nPos) override;
	long STDMETHODCALLTYPE GetPos() override;
	long STDMETHODCALLTYPE ReadAbs(long nStartPos, long nEndPos, char* pBuf) override;
	long STDMETHODCALLTYPE GetSize() override;
	long STDMETHODCALLTYPE Read(long nNumBytes, char* pBuf) override;
	int16 STDMETHODCALLTYPE Getc() override;
	void STDMETHODCALLTYPE ReadBlocks(void* pBuf, long nSize, tStoreStreamBlockCallback callback, void* pData) override;
	ulong STDMETHODCALLTYPE LastModified() override;

private:
	IStore* m_pStorage;
	FILE* m_pFile;
	cFileSpec* m_pFileSpec;
	long m_nLastPos;
	int m_nOpenCount;
};