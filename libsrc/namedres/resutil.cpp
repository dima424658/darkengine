#include "resutil.h"

EXTERN long ResBaseExtractCallback(void* pBuf, long nNumBytes, long nIx, void* pUntypedData)
{
	auto* pData = reinterpret_cast<sExtractData*>(pUntypedData);

	return pData->Callback(pData->pResource, pBuf, nNumBytes, nIx, pData->pCallbackData);
}