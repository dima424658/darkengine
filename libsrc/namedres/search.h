#pragma once

#include <comtools.h>
#include <dynarray.h>
#include <storeapi.h>
#include <resguid.h>

struct sSearchPathElement
{
	char* pPath;
	int fRecurse;
};

struct PathStorageNode
{
	IStore* pStore;
	PathStorageNode* pNext;
};

class cSearchPath : public cCTUnaggregated<ISearchPath, &IID_ISearchPath, kCTU_NoSelfDelete>
{
public:
	cSearchPath(IStoreManager* pStoreMan);
	~cSearchPath();

	void STDMETHODCALLTYPE Clear() override;
	ISearchPath* STDMETHODCALLTYPE Copy() override;
	void STDMETHODCALLTYPE AddPath(const char* pPath) override;
	void STDMETHODCALLTYPE AddPathTrees(const char* pPath, BOOL bRecurse) override;
	void STDMETHODCALLTYPE Ready() override;
	void STDMETHODCALLTYPE Refresh() override;
	void STDMETHODCALLTYPE SetContext(ISearchPath* pContext) override;
	void STDMETHODCALLTYPE SetVariants(ISearchPath* pVariants) override;
	IStore* STDMETHODCALLTYPE Find(const char* pName, uint fFlags, IStore** ppCanonStore, const char* pRelPath) override;
	void* STDMETHODCALLTYPE BeginContents(const char* pPattern, uint fFlags, const char* pRelPath) override;
	BOOL STDMETHODCALLTYPE Next(void* pCookie, IStore** pFoundStore, char* pFoundName, IStore** ppFoundCanonStore) override;
	void STDMETHODCALLTYPE EndContents(void* pCookie) override;
	void STDMETHODCALLTYPE Iterate(tSearchPathIterateCallback callback, BOOL bUseContext, void* pClientData) override;

	void DoIterate(const char* pContext, tSearchPathIterateCallback callback, void* pClientData);
	void SetupSingleStore(const char* pStorePath, int fRecurse);

private:
	void ClearStorages();
	void SetupStorages();
	void DoAddStore(const char* pStoreName, int fRecurse);
	void DoAddPath(const char* pPath, int fRecurse);

private:
	IStoreManager* m_pStoreMan;
	unsigned char m_bPathParsed;
	cDynArray<sSearchPathElement>* m_aPathStrings;
	ISearchPath* m_pContext;
	ISearchPath* m_pVariants;
	PathStorageNode* m_pPathStorages;
	PathStorageNode* m_pLastNode;
};