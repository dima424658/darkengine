#pragma once

#include <cstdio>

#include <resapi.h>
#include <hashset.h>

enum eResourceStats
{
	LockRequests = 0x0,
	AlreadyLoaded = 0x1,
	LockFailed = 0x2,
	LoadFailed = 0x3,
	NewLoad = 0x4,
	NewLoadLRU = 0x5,
	ResourceBindings = 0x6,
	ProxiedLoad = 0x7,
	MemAlloced = 0x8,
	MemFreed = 0x9,
	MemCurrent = 0xA,
	MemHighwater = 0xB,
	NumStats = 0xC,
};

class cResStats;
class cNamedStatsHash;

class cNamedStatsData
{
public:
	friend cResStats;
	friend cNamedStatsHash;

	cNamedStatsData(const char* pName);
	~cNamedStatsData();

private:
	char* m_pObjName;
	ulong m_Stats[static_cast<int>(eResourceStats::NumStats)];
};

class cNamedStatsHash : public cStrIHashSet<cNamedStatsData*>
{
public:
	tHashSetKey GetKey(tHashSetNode p) const override
	{
		return (tHashSetKey)((cNamedStatsData*)p)->m_pObjName;
	}
};

class cResStats
{
public:
	cResStats();
	~cResStats();

	void SetMode(eResStatMode ResStatMode, int bTurnOn);
	void Dump(const char* pFile);
	void LogStatRes(IRes* pRes, eResourceStats StatType);

private:
	void DumpItem(FILE* fp, ulong* pStats);
	void DumpTable(FILE* fp, cNamedStatsHash* pTable);
	void LogStat(IRes* pRes, ulong* pStats, eResourceStats statType);
	void LogStatTable(IRes* pRes, cNamedStatsHash* pTable, const char* pName, eResourceStats StatType);

private:
	cNamedStatsHash* m_pResStats;
	cNamedStatsHash* m_pTypeStats;
	cNamedStatsHash* m_pPathStats;

	ulong m_Stats[static_cast<int>(eResourceStats::NumStats)];
	int m_StatMode[kResStatMode_NumModes];
};