///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/recorder/RCS/recdata.cpp $
// $Author: TOML $
// $Date: 1996/09/30 11:11:48 $
// $Revision: 1.4 $
//

#if 0

#include <recapi.h>
#include <lg.h>

///////////////////////////////////////////////////////////////////////////////

void *_RecGetData(sRecDataHandle * phData, size_t size, const char *pszTypeTag)
{
    AssertMsg(phData, "Bad pointer to _RecGetData()");
    AssertMsg(size == phData->sizeData, "Expected data size does not match actual value");
#if DEBUG
    AssertMsg((!pszTypeTag && !phData->pszDataType) ||
              strcmp(pszTypeTag, phData->pszDataType) == 0,
              "Expected data type does not match actual type");
#endif

    return phData->pData;
}
///////////////////////////////////////

void _RecSetData(sRecDataHandle * phData, void *pData, size_t size, const char *pszTypeTag)
{
    AssertMsg(phData, "Bad pointer to _RecSetData()");
    phData->pData = pData;
    phData->sizeData = size;
    phData->pszDataType = pszTypeTag;
}
///////////////////////////////////////////////////////////////////////////////

#endif
