#pragma once

#include <storeapi.h>
#include <storbase.h>
#include <filepath.h>

enum class eExistenceState
{
	kExists,
	kDoesntExist,
	kExistenceUnchecked
};

class cDirectoryStorage : public cStorageBase
{
public:
	cDirectoryStorage(const char* pRawName);
	~cDirectoryStorage();

	const char* STDMETHODCALLTYPE GetName() override;
	const char* STDMETHODCALLTYPE GetFullPathName() override;

	void STDMETHODCALLTYPE EnumerateLevel(tStoreLevelEnumCallback callback, BOOL bAbsolute, BOOL bRecurse, void* pClientData) override;
	void STDMETHODCALLTYPE EnumerateStreams(tStoreLevelEnumCallback callback, BOOL bAbsolute, BOOL bRecurse, void* pClientData) override;

	IStore* STDMETHODCALLTYPE GetSubstorage(const char* pSubPath, BOOL bCreate) override;
	IStore* STDMETHODCALLTYPE GetParent() override;

	void STDMETHODCALLTYPE Refresh(BOOL bRecurse) override;
	BOOL STDMETHODCALLTYPE Exists() override;

	BOOL STDMETHODCALLTYPE StreamExists(const char* pName) override;
	IStoreStream* STDMETHODCALLTYPE OpenStream(const char* pName, uint) override;

	void* STDMETHODCALLTYPE BeginContents(const char* pPattern, uint fFlags) override;
	BOOL STDMETHODCALLTYPE Next(void* pCookie, char* foundName) override;
	void STDMETHODCALLTYPE EndContents(void* pCookie) override;

	void STDMETHODCALLTYPE GetCanonPath(char** ppCanonPath) override;
	void STDMETHODCALLTYPE SetStoreManager(IStoreManager *pMan) override;
	void STDMETHODCALLTYPE SetParent(IStore* pNewParent) override;
	void STDMETHODCALLTYPE RegisterSubstorage(IStore* pSubstore, const char* pName) override;
	void STDMETHODCALLTYPE SetDataStream(IStoreStream*) override;
	void STDMETHODCALLTYPE DeclareContextRoot(int bIsRoot) override;
	void STDMETHODCALLTYPE Close();

private:
	void EnumerateLevelHelper(cFilePath* pPath, tStoreLevelEnumCallback callback, bool bAbsolute, bool bRecurse, void* pClientData);
	void EnumerateStreamHelper(cFilePath* pPath, tStoreLevelEnumCallback callback, bool bAbsolute, bool bRecurse, void* pClientData);
	void EnumerateStreamPath(cFilePath* pPath, tStoreLevelEnumCallback callback, bool bAbsolute, void* pClientData);

private:
	cFilePath* m_pFilePath;
	char* m_pName;
	IStore* m_pParent;
	cStorageHashByName* m_pSubstorageTable;
	cStreamHashByName* m_pStreamTable;
	eExistenceState m_Exists;
	IStoreManager* m_pStoreManager;
	unsigned int m_fFlags;
};