#include <resstat.h>
#include <mprintf.h>
#include <hshsttem.h>

#ifndef NO_DB_MEM
// Must be last header
#include <memall.h>
#include <dbmem.h>
#endif

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cNamedStatsData
//

cNamedStatsData::cNamedStatsData(const char* pName)
	: m_pObjName{ nullptr }, m_Stats{}
{
	if (pName)
	{
		m_pObjName = static_cast<char*>(Malloc(strlen(pName) + 1));
		strcpy(m_pObjName, pName);
	}
}

///////////////////////////////////////

cNamedStatsData::~cNamedStatsData()
{
	if (m_pObjName)
	{
		Free(m_pObjName);
		m_pObjName = nullptr;
	}
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cResStats
//

cResStats::cResStats()
	: m_pResStats{ nullptr },
	m_pTypeStats{ nullptr },
	m_pPathStats{ nullptr },
	m_Stats{ },
	m_StatMode{ }
{
}

///////////////////////////////////////

cResStats::~cResStats()
{
	if (m_pResStats)
	{
		delete m_pResStats;
		m_pResStats = nullptr;
	}

	if (m_pTypeStats)
	{
		delete m_pTypeStats;
		m_pTypeStats = nullptr;
	}

	if (m_pPathStats)
	{
		delete m_pPathStats;
		m_pPathStats = nullptr;
	}
}

///////////////////////////////////////

void cResStats::SetMode(eResStatMode ResStatMode, int bTurnOn)
{
	if (ResStatMode >= kResStatMode_NumModes || ResStatMode < kResStatMode_All)
	{
		CriticalMsg("Unknown Stat Mode!");
		return;
	}

	if (ResStatMode)
		m_StatMode[ResStatMode] = bTurnOn;
	else
		for (int i = 0; i < 6; ++i)
			m_StatMode[i] = bTurnOn;

	if (m_StatMode[kResStatMode_Types] && !m_pTypeStats)
		m_pTypeStats = new cNamedStatsHash{};

	if (m_StatMode[kResStatMode_CanonPath] && !m_pPathStats)
		m_pPathStats = new cNamedStatsHash{};

	if ((m_StatMode[kResStatMode_Res] || m_StatMode[kResStatMode_TopHits]) && !m_pResStats)
		m_pResStats = new cNamedStatsHash{};
}

///////////////////////////////////////

void cResStats::Dump(const char* pFile)
{
	FILE* fp = nullptr;
	if (pFile)
		fp = fopen(pFile, "a+");

	if (m_StatMode[kResStatMode_Summary])
	{
		if (fp)
			fprintf(fp, "\nResource Manager Stats:\n");
		else
			mprintf("\nResource Manager Stats:\n");

		DumpItem(fp, m_Stats);
	}

	if (m_StatMode[kResStatMode_Types])
	{
		if (fp)
			fprintf(fp, "\nStats Broken down by Type:\n");
		else
			mprintf("\nStats Broken down by Type:\n");

		DumpTable(fp, m_pTypeStats);
	}

	if (m_StatMode[kResStatMode_CanonPath])
	{
		if (fp)
			fprintf(fp, "\nStats Broken down by Canonical Path:\n");
		else
			mprintf("\nStats Broken down by Canonical Path:\n");

		DumpTable(fp, m_pPathStats);
	}

	if (m_StatMode[kResStatMode_Res])
	{
		if (fp)
			fprintf(fp, "\nStats Broken down by Resource:\n");
		else
			mprintf("\nStats Broken down by Resource:\n");

		DumpTable(fp, m_pResStats);
	}

	if (fp)
		fclose(fp);
}

///////////////////////////////////////

void cResStats::LogStatRes(IRes* pRes, eResourceStats StatType)
{
	if (!pRes)
		return;

	if (StatType >= eResourceStats::NumStats)
		CriticalMsg("Unknown Stat type!");

	if (m_StatMode[kResStatMode_Summary])
		LogStat(pRes, m_Stats, StatType);

	if (m_StatMode[kResStatMode_Types])
	{
		auto* pType = pRes->GetType();
		auto* pTypename = pType->GetName();
		LogStatTable(pRes, m_pTypeStats, pTypename, StatType);
		pType->Release();
	}

	if (m_StatMode[kResStatMode_CanonPath])
	{
		char* pCanonPath = nullptr;
		pRes->GetCanonPath(&pCanonPath);
		LogStatTable(pRes, m_pPathStats, pCanonPath, StatType);
		Free(pCanonPath);
	}

	if (m_StatMode[kResStatMode_Res] || m_StatMode[kResStatMode_TopHits])
	{
		char* pFullPathName = nullptr;
		pRes->GetStreamName(TRUE, &pFullPathName);
		LogStatTable(pRes, m_pResStats, pFullPathName, StatType);
		Free(pFullPathName);
	}
}

///////////////////////////////////////

void cResStats::DumpItem(FILE* fp, ulong* pStatNames)
{
	for (auto i = 0; i < static_cast<int>(eResourceStats::NumStats); ++i)
	{
		if (fp)
			fprintf(fp, "    %d: %d\n", pStatNames[i], m_Stats[i]); // TODO ???
		else
			mprintf("    %d: %d\n", pStatNames[i], m_Stats[i]);
	}
}

///////////////////////////////////////

void cResStats::DumpTable(FILE* fp, cNamedStatsHash* pTable)
{
	if (!pTable)
		return;

	tHashSetHandle hs{};
	for (auto* pNode = pTable->GetFirst(hs); pNode != nullptr; pNode = pTable->GetNext(hs))
	{
		if (fp)
			fprintf(fp, "  %s:\n", pNode->m_pObjName);
		else
			mprintf("  %s:\n", pNode->m_pObjName);
		DumpItem(fp, pNode->m_Stats);
	}
}

///////////////////////////////////////

void cResStats::LogStat(IRes* pRes, ulong* pStats, eResourceStats statType)
{
	if (statType == eResourceStats::MemAlloced)
	{
		auto size = pRes->GetSize();
		m_Stats[eResourceStats::MemAlloced] += size;
		m_Stats[eResourceStats::MemCurrent] += size;
		if (m_Stats[eResourceStats::MemCurrent] > m_Stats[eResourceStats::MemHighwater])
			m_Stats[eResourceStats::MemHighwater] = m_Stats[eResourceStats::MemCurrent];
	}
	else if (statType == eResourceStats::MemFreed)
	{
		auto size = pRes->GetSize();
		m_Stats[eResourceStats::MemFreed] += size;
		m_Stats[eResourceStats::MemCurrent] -= size;
	}
	else
	{
		++m_Stats[statType];
	}
}

///////////////////////////////////////

void cResStats::LogStatTable(IRes* pRes, cNamedStatsHash* pTable, const char* pName, eResourceStats StatType)
{
	if (!pTable)
		CriticalMsg("cResStat: LogStatTable called without a table!");

	if (!pName)
		return;

	auto* pData = pTable->Search(pName);
	if (!pData)
	{
		pData = new cNamedStatsData{ pName };
		pTable->Insert(pData);
	}

	LogStat(pRes, pData->m_Stats, StatType);
}