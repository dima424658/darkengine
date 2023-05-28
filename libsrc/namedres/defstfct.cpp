#include <defstfct.h>
#include <storedir.h>
#include <lgassert.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDefaultStorageFactory
//

void cDefaultStorageFactory::EnumerateTypes(tStoreEnumTypeCallback callback, void* pClientData)
{
}

///////////////////////////////////////

IStore* cDefaultStorageFactory::CreateStore(IStore* pParent, const char* pName, const char* pExt)
{
	auto needsData = false;
	IStore* pStore = nullptr;

	if (strcmp(pExt, "zip") == 0)
		needsData = true;
	else
		pStore = new cDirectoryStorage{ pName };

	if (pParent)
	{
		IStoreHierarchy* pParentHier = nullptr;
		if (FAILED(pParent->QueryInterface(IID_IStoreHierarchy, reinterpret_cast<void**>(&pParentHier))))
		{
			CriticalMsg("Couldn't QueryInterface a StoreHierarchy!");
			pStore->Release();
			return nullptr;
		}

		pParentHier->RegisterSubstorage(pStore, pStore->GetName());
		pParentHier->Release();

		if (needsData)
		{
			auto* pDataStream = pParent->OpenStream(pName, 0);
			if (!pDataStream)
			{
				pStore->Release();
				return nullptr;
			}

			IStoreHierarchy* pHier = nullptr;
			if (FAILED(pStore->QueryInterface(IID_IStoreHierarchy, reinterpret_cast<void**>(&pHier))))
			{
				CriticalMsg("Couldn't QI a StoreHierarchy!");
				pStore->Release();
				return nullptr;
			}

			pHier->SetDataStream(pDataStream);
			pDataStream->Release();
			pHier->Release();
		}
	}

	return pStore;
}