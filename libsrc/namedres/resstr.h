#pragma once

#include <resbase.h>

class cStringResource : public cResourceBase<IRes, &IID_IRes>
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

	void StringPreload(const char*);
	char* StringLock(const char* pStrName);
	void StringUnlock(const char* pStrName);
	int StringExtract(char* pStrName, char* pBuf, int nSize);

private:
	int SkipLine(IStoreStream* pStream);
	int SkipWhitespace(IStoreStream* pStream);
	int GetStrName(IStoreStream* pStream, char* pBuf);
};