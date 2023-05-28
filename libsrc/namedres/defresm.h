#pragma once

#include <resapi.h>
#include <resguid.h>

#include <comtools.h>

class cDefResMem : public cCTUnaggregated<IResMemOverride, &IID_IResMemOverride, kCTU_NoSelfDelete>
{
public:
	void* STDMETHODCALLTYPE ResMalloc(ulong nNumBytes);
	void STDMETHODCALLTYPE ResFree(void* pData);
	unsigned long STDMETHODCALLTYPE GetSize(void* pData);
};