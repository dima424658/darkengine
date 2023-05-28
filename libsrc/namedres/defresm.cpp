#include "defresm.h"

#ifndef NO_DB_MEM
// Must be last header
#include <memall.h>
#include <dbmem.h>
#endif

void* cDefResMem::ResMalloc(ulong nNumBytes)
{
	return Malloc(nNumBytes);
}

void cDefResMem::ResFree(void* pData)
{
	Free(pData);
}

unsigned long cDefResMem::GetSize(void* pData)
{
	return MSize(pData);
}