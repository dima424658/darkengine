#pragma once

#include <storbase.h>
#include <filepath.h>


#pragma pack(1)
struct sDirEndRecord
{
	uint32 Signature;
	uint16 DiskNumber;
	uint16 DirStartDisk;
	uint16 NumEntriesOnDisk;
	uint16 NumEntries;
	uint32 DirSize;
	uint32 DirOffset;
	uint16 CommentLen;
};
#pragma pack(0)

static_assert(sizeof(sDirEndRecord) == 22, "Invalid sDirEndRecord size");

struct sDirFileRecord
{
	uint32 Signature;
	uint16 StoredVersion;
	uint16 ExtractVersion;
	uint16 Flags;
	uint16 CompressionMethod;
	uint16 ModTime;
	uint16 ModDate;
	uint32 CRC32;
	uint32 CompressedSize;
	uint32 UncompressedSize;
	uint16 FilenameLen;
	uint16 ExtraLen;
	uint16 CommentLen;
	uint16 DiskNumberStart;
	uint16 InternalAttributes;
	uint32 ExternalAttributes;
	uint32 HeaderOffset;
};

static_assert(sizeof(sDirFileRecord) == 46, "Invalid sDirFileRecord size");

class cZipStorage;

class cZipSubstorage : public cStorageBase
{
public:
	friend cZipStorage;

	cZipSubstorage(IStore* pParent, const char* pRawName);
	~cZipSubstorage();

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
	void STDMETHODCALLTYPE SetStoreManager(IStoreManager* pMan) override;
	void STDMETHODCALLTYPE SetParent(IStore* pNewParent) override;
	void STDMETHODCALLTYPE RegisterSubstorage(IStore* pSubstore, const char* pName) override;
	void STDMETHODCALLTYPE SetDataStream(IStoreStream*) override;
	void STDMETHODCALLTYPE DeclareContextRoot(int bIsRoot) override;
	void STDMETHODCALLTYPE Close();

	int EnumerateLevelHelper(const char* pSubpath, IStore* pSubstore, tStoreLevelEnumCallback, bool bAbsolute, void* pClientData);
	int EnumerateStreamsHelper(const char* pSubpath, IStore* pSubstore, tStoreLevelEnumCallback, bool bAbsolute, void* pClientData);

protected:
	void SetTopZip(cZipStorage* pStorage);
	cZipSubstorage* MakeSubstorage(const char* pSubPath);
	void AddStream(const char* pName, sDirFileRecord* pDirRecord);

protected:
	cFilePath* m_pFilePath;
	char* m_pName;
	IStore* m_pParent;
	cZipStorage* m_pZipStorage;
	cStorageHashByName* m_pSubstorageTable;
	cStreamHashByName* m_pStreamTable;
	IStoreManager* m_pStoreManager;
	uint m_fFlags;
};

class cZipStorage : public cZipSubstorage
{
public:
	cZipStorage(IStore* pParent, IStoreStream* pZipStream, const char* pName);
	~cZipStorage();

	IStoreStream* ReadyStreamAt(long nPos);
	ulong LastModified();

private:
	IStoreStream* m_pZipStream;
};