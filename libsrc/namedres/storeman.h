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
	unsigned ContextVersion() const { return m_ContextVersion; }
	ISearchPath* GlobalContext() const { return m_pGlobalContext; }

private:
	IStore* m_pRootStore;
	IStoreFactory* m_pDefStoreFactory;
	ISearchPath* m_pGlobalContext;
	ISearchPath* m_pDefVariants;
	cFactoryHashByExt* m_pFactoryTable;
	unsigned m_ContextVersion;
};

// Search paths are intentionally handed the interface pointer, but every
// path produced by this implementation belongs to cStorageManager. This
// helper lets them cheaply invalidate their compiled storage list when the
// global content context changes.
unsigned StorageManagerContextVersion(IStoreManager* pStoreMan);
ISearchPath* StorageManagerGlobalContext(IStoreManager* pStoreMan);
