#pragma once
#include <resapi.h>
#include <dynarray.h>
#include <hashset.h>

class cResMan;
class cResourceData;
class cResourceTypeData;
class cHashByResName;

class cResourceName
{
public:
	friend cResMan;
	friend cResourceTypeData;
	friend cHashByResName;

	cResourceName(const char* pResName);
	~cResourceName();

private:
	char* m_pName;
	cResourceData* m_pFirstStream;
	cDynArray<const char*>* m_ppFullNames;
	uint m_fFlags;
};

class cHashByResName : public cStrIHashSet<cResourceName*>
{
public:
	friend cResMan;

	tHashSetKey GetKey(tHashSetNode p) const override
	{
		return p ? (tHashSetKey)((cResourceName*)p)->m_pName : nullptr;
	}

	cResourceData* FindResData(const char* pName, IStore* pStore, BOOL bCreate);
};

class cResARQFulfiller;
class cHashByResourceType;

class cResourceTypeData
{
public:
	friend cResMan;
	friend cResARQFulfiller;
	friend cHashByResourceType;

	cResourceTypeData(IResType* pType, cResourceData* pResData, IRes* pRes);
	~cResourceTypeData();
	const char* GetName();

private:
	void* m_pData;
	ulong m_nSize;
	IResMemOverride* m_pResMem;
	int m_nUserLockCount;
	int m_nInternalLockCount;
	IRes* m_pRes;
	IResType* m_pType;
	cResourceData* m_pResourceData;
	cResourceTypeData* m_pProxiedRes;
	int m_Freshed;
};

class cHashByResourceType : public cStrIHashSet<cResourceTypeData*>
{
public:
	tHashSetKey GetKey(tHashSetNode p) const override
	{
		if (((cResourceTypeData*)p)->m_pType != nullptr)
			return (tHashSetKey)((cResourceTypeData*)p)->m_pType->GetName();
		else
			return nullptr;
	}
};

class cResourceData
{
public:
	friend cResMan;
	friend cResourceName;
	friend cResourceTypeData;
	friend cHashByResName;

	cResourceData(IStore* pStore, cResourceName* pNameData);
	~cResourceData();

private:
	IStore* m_pStore;
	cResourceName* m_pNameData;
	cHashByResourceType m_ResourceTypeHash;
	cResourceData* m_pNext;
};

class cInstalledResTypeHash;

class cResTypeData
{
public:
	friend cResMan;
	friend cInstalledResTypeHash;

	cResTypeData(const char* pName, IResType* pType);
	~cResTypeData();

private:
	enum { kNameSize = 20 };

	IResType** m_pFactories;
	int m_nNumFactories;
	char m_Name[kNameSize];
};

class cInstalledResTypeHash : public cStrIHashSet<cResTypeData*>
{
public:
	tHashSetKey GetKey(tHashSetNode p) const override
	{
		return (tHashSetKey)((cResTypeData*)p)->m_Name;
	}
};

class cInstalledResTypeByName;
class cNamedResType
{
public:
	friend cResMan;
	friend cInstalledResTypeByName;

	cNamedResType(IResType* pType);
	~cNamedResType();

private:
	IResType* m_pType;
};

class cInstalledResTypeByName : public cStrIHashSet<cNamedResType*>
{
public:
	tHashSetKey GetKey(tHashSetNode p) const override
	{
		return (tHashSetKey)((cNamedResType*)p)->m_pType->GetName();
	}
};