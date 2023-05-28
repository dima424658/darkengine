#pragma once

#include "resapi.h"

#include <reshelp.h>
#include <resstat.h>
#include <resarq.h>
#include <resmanhs.h>
#include <defresm.h>

#include <cacheapi.h>
#include <aggmemb.h>
#include <hashset.h>

class cResMan :
	public cCTDelegating<IResMan>,
	public cCTDelegating<IResStats>,
	public cCTDelegating<IResMem>,
	public cCTDelegating<IResManHelper>,
	public cCTAggregateMemberControl<kCTU_Default>
{
public:
	cResMan(IUnknown* pOuter);
	~cResMan();

	HRESULT STDMETHODCALLTYPE Init() override;
	HRESULT STDMETHODCALLTYPE End() override;

	ISearchPath* STDMETHODCALLTYPE NewSearchPath(const char* pNewPath) override;
	void STDMETHODCALLTYPE SetDefaultPath(ISearchPath* pPath) override;
	void STDMETHODCALLTYPE SetGlobalContext(ISearchPath* pPath) override;
	void STDMETHODCALLTYPE SetDefaultVariants(ISearchPath* pPath) override;

	IRes* STDMETHODCALLTYPE Bind(const char* pRelativePathname, const char* pTypeName, ISearchPath* pPath, const char* pExpRelPath, uint fBindFlags) override;
	void STDMETHODCALLTYPE BindAll(const char* pPattern, const char* pTypeName, ISearchPath* pPath, void(*callback)(IRes*, IStore*, void*), void* pClientData, const char* pRelPath, uint fBindFlags) override;
	IRes* STDMETHODCALLTYPE BindSpecific(const char* pName, const char* pTypeName, IStore* pStore, IStore* pCanonStore, uint fBindFlags) override;
	IRes* STDMETHODCALLTYPE Retype(IRes* pOldRes, const char* pTypeName, uint fBindFlags) override;
	IRes* STDMETHODCALLTYPE Lookup(const char* pRelativePathname, const char* pTypeName, const char* pCanonPath) override;
	
	IStore* STDMETHODCALLTYPE GetStore(const char* pPathName) override;
	BOOL STDMETHODCALLTYPE RegisterResType(IResType* pType) override;
	BOOL STDMETHODCALLTYPE RegisterStoreFactory(IStoreFactory* pStoreFactory) override;
	void STDMETHODCALLTYPE UnregisterResType(IResType* pType) override;
	void STDMETHODCALLTYPE MarkForRefresh(IRes* pRes) override;
	void STDMETHODCALLTYPE GlobalRefresh() override;

	void STDMETHODCALLTYPE SetMode(eResStatMode mode, BOOL bTurnOn) override;
	void STDMETHODCALLTYPE Dump(const char* pFile) override;
	void STDMETHODCALLTYPE DumpSnapshot(const char* pFile) override;
	void STDMETHODCALLTYPE EnablePaging(BOOL bEnable) override;
	void STDMETHODCALLTYPE Compact() override;

	void* STDMETHODCALLTYPE LockResource(IRes* pRes) override;
	void* STDMETHODCALLTYPE ExtractResource(IRes* pRes, void* pBuf) override;
	void* STDMETHODCALLTYPE FindResource(IRes* pRes, long* pSize) override;
	void STDMETHODCALLTYPE UnlockResource(IRes* pData) override;
	uint STDMETHODCALLTYPE GetResourceLockCount(IRes* pRes) override;
	int STDMETHODCALLTYPE DropResource(IRes* pRes) override;
	long STDMETHODCALLTYPE GetResourceSize(IRes* pRes) override;
	void STDMETHODCALLTYPE UnregisterResource(IRes* pRes, ulong ManData) override;

	int STDMETHODCALLTYPE AsyncLock(IRes* pRes, int nPriority) override;
	int STDMETHODCALLTYPE AsyncExtract(IRes* pRes, int nPriority, void* pBuf, long bufSize) override;
	int STDMETHODCALLTYPE AsyncPreload(IRes* pRes) override;
	int STDMETHODCALLTYPE IsAsyncFulfilled(IRes* pRes) override;
	long STDMETHODCALLTYPE AsyncKill(IRes* pRes) override;
	long STDMETHODCALLTYPE GetAsyncResult(IRes* pRes, void** ppResult) override;

	void InstallResourceType(const char* pExt, IResType* pType);
	void RemoveResourceType(const char* pExt, IResType* pType);
	void FreeData(cResourceTypeData* pData, BOOL bTestUser);
	cResourceTypeData* GetResourceTypeData(IRes* pRes);

private:
	void CleanResources();
	IRes* DoBind(const char* pName, const char* pTypeName, ISearchPath* pPath, const char* pRelPath, uint fBindFlags);
	IRes* DoLookup(const char* pName, const char* pTypeName, const char* pRawCanonPath);

	BOOL DropResourceData(cResourceTypeData* pData);

	void CacheAdd(cResourceTypeData* pData);
	BOOL CacheRemove(cResourceTypeData* pData);

	void* DoLockResource(IRes*, cResourceTypeData* pData);
	void DoUnlockResource(cResourceTypeData* pData);

	IRes* WalkNameChain(cResourceData* pData, const char*, const char* pTypeName, const char* pCanonPath);

	void DoDumpSnapshot(FILE* fp);
	bool VerifyStorage(IStore* pStorage);
	IRes* GetResource(const char* pName, const char* pTypeName, IStore* pStore);
	void MungePaths(const char* pPathName, const char* pExpRelPath, char** ppRelPath, char** ppName, char* pOldSlash, int* pbCombinedPaths);
	void RestorePath(char* pPath, char* pName, char OldSlash, int bCombinedPaths);
	cResourceTypeData* FindResourceTypeData(IStore* pStore, const char* pResName, IResType* pType);
	IRes* CreateResource(IStore* pStorage, const char* pName, const char* pExt, const char* pTypeName, uint);
	void UnregisterResourceData(cResourceTypeData*, int);
	IResType* GetResType(const char* pTypeName);

private:
	cResManARQ m_ResManARQ;
	IStoreManager* m_pStoreMan;
	ISearchPath* m_pDefSearchPath;
	ISearchPath* m_pGlobalContext;
	ISearchPath* m_pDefVariants;
	cInstalledResTypeHash m_ResTypeHash;
	cInstalledResTypeByName m_ResTypeByNameHash;
	cHashByResName m_ResTable;
	ISharedCache* m_pSharedCache;
	ICache* m_pCache;
	int m_bPagingEnabled;
	IResType** m_ppResFactories;
	int m_nResFactories;
	int m_FreshStamp;
	cResStats* m_pResStats;
	bool m_fDidInit;
	cDefResMem m_DefResMem;
};