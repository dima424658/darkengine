#pragma once

#include <resbase.h>
#include <resistr.h>

class cStringResource : public cResourceBase<IStringRes, &IID_IStringRes>
{
public:
	cStringResource(IStore* pStore,
		const char* pName,
		IResType* pType);

	void* STDMETHODCALLTYPE LoadData(ulong* pSize,
		ulong* pTimestamp,
		IResMemOverride* pResMem) override;

	BOOL STDMETHODCALLTYPE FreeData(void* pData,
		ulong nSize,
		IResMemOverride* pResMem) override;

	void STDMETHODCALLTYPE StringPreload(const char*) override;
	char* STDMETHODCALLTYPE StringLock(const char* pStrName) override;
	void STDMETHODCALLTYPE StringUnlock(const char* pStrName) override;
	BOOL STDMETHODCALLTYPE StringExtract(const char* pStrName, char* pBuf, int nSize) override;

private:
	int SkipLine(IStoreStream* pStream);
	int SkipWhitespace(IStoreStream* pStream);
	int GetStrName(IStoreStream* pStream, char* pBuf);
};
