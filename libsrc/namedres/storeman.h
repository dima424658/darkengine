#pragma once

#include <comtools.h>
#include <storeapi.h>
#include <resguid.h>

class cFactoryHashByExt;

class cStorageManager : public cCTUnaggregated<IStoreManager, &IID_IStoreManager, kCTU_NoSelfDelete>
{
public:
	cStorageManager();
	~cStorageManager();

	void STDMETHODCALLTYPE RegisterFactory(IStoreFactory* pFactory) override;
	IStore* STDMETHODCALLTYPE GetStore(const char* pPathName, int bCreate) override;
	IStore* STDMETHODCALLTYPE CreateSubstore(IStore* pParent, const char* pName) override;
	void STDMETHODCALLTYPE SetGlobalContext(ISearchPath* pPath) override;
	void STDMETHODCALLTYPE SetDefaultVariants(ISearchPath* pPath) override;
	ISearchPath* STDMETHODCALLTYPE NewSearchPath(const char* pNewPath) override;
	void STDMETHODCALLTYPE Close() override;
	int STDMETHODCALLTYPE HeteroStoreExists(IStore* pParentStore, const char* pSubStoreName, char* pNameBuffer) override;

	void InstallStorageType(const char* pExt, IStoreFactory* pFactory);

private:
	IStore* m_pRootStore;
	IStoreFactory* m_pDefStoreFactory;
	ISearchPath* m_pGlobalContext;
	ISearchPath* m_pDefVariants;
	cFactoryHashByExt* m_pFactoryTable;
};