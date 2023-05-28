#pragma once

#include <comtools.h>
#include <storeapi.h>
#include <resguid.h>
#include <hashset.h>

class cStorageBase
	: public cCTUnaggregated2<IStore, &IID_IStore, IStoreHierarchy, &IID_IStoreHierarchy, kCTU_NoSelfDelete>
{
};

class cZipSubstorage;
class cDirectoryStorage;
class cStorageHashByName;
class cNamedStorage
{
public:
	friend cZipSubstorage;
	friend cDirectoryStorage;
	friend cStorageHashByName;

	cNamedStorage(const char* pName);
	cNamedStorage(IStore* pStore);
	~cNamedStorage();

private:
	IStore* m_pStore;
	char* m_pName;
};

class cStorageHashByName : public cStrIHashSet<cNamedStorage*>
{
public:
	tHashSetKey GetKey(tHashSetNode p) const override;
};

class cStreamHashByName;
class cNamedStream
{
public:
	friend cZipSubstorage;
	friend cDirectoryStorage;
	friend cStreamHashByName;

	cNamedStream(const char* pName, bool bExists);
	~cNamedStream();

private:
	char* m_pName;
	bool m_bExists;
};

class cStreamHashByName : public cStrIHashSet<cNamedStream*>
{
public:
	tHashSetKey GetKey(tHashSetNode p) const override;
};