#pragma once

#include <comtools.h>
#include <storeapi.h>
#include <resguid.h>

class cDefaultStorageFactory : public cCTUnaggregated<IStoreFactory, &IID_IStoreFactory, kCTU_Default>
{
public:
	void STDMETHODCALLTYPE EnumerateTypes(tStoreEnumTypeCallback callback, void* pClientData) override;
	IStore* STDMETHODCALLTYPE CreateStore(IStore* pParent, const char* pName, const char* pExt) override;
};