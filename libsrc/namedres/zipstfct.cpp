#include <zipstfct.h>
#include <storezip.h>
#include <storeapi.h>
#include <resguid.h>
#include <str.h>
#include <filespec.h>
#include <lgassert.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDefaultStorageFactory
//

class cZipStorageFactory : public cCTUnaggregated<IStoreFactory, &IID_IStoreFactory, kCTU_Default>
{
public:
	void STDMETHODCALLTYPE EnumerateTypes(tStoreEnumTypeCallback callback, void* pClientData) override;
	IStore* STDMETHODCALLTYPE CreateStore(IStore* pParent, const char* pName, const char* pExt) override;
};

void cZipStorageFactory::EnumerateTypes(tStoreEnumTypeCallback callback, void* pClientData)
{
	callback(".crf", this, pClientData);
	callback(".zip", this, pClientData);
}

///////////////////////////////////////

IStore* cZipStorageFactory::CreateStore(IStore* pParent, const char* pName, const char* pExt)
{
	if (!pParent)
		CriticalMsg("cZipStorageFactory: no parent!");

	auto* pDataStream = pParent->OpenStream(pName, 0);
	if (!pDataStream)
		return nullptr;

	cStr root{};
	cFileSpec fileSpec{ pName };
	fileSpec.GetFileRoot(root);

	auto* pStore = new cZipStorage{ pParent, pDataStream , root };
	pDataStream->Release();

	IStoreHierarchy* pParentHier = nullptr;
	if (FAILED(pParent->QueryInterface(IID_IStoreHierarchy, reinterpret_cast<void**>(&pParentHier))))
	{
		CriticalMsg("Couldn't QI a StoreHierarchy!");
		pStore->Release();

		return nullptr;
	}

	pParentHier->RegisterSubstorage(pStore, pStore->GetName());
	pParentHier->Release();

	return pStore;
}

///////////////////////////////////////

IStoreFactory* MakeZipStorageFactory()
{
	return new cZipStorageFactory{};
}
