///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/comtools/RCS/interset.cpp $
// $Author: TOML $
// $Date: 1998/04/24 13:08:58 $
// $Revision: 1.10 $
//

#include <lg.h>
#include <comtools.h>
#include <interset.h>
#include <dlisttem.h>
#include <hshsttem.h>


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cInterfaceList
//

HRESULT cInterfaceList::ReleaseAll(eISetOrder order)
    {
    cInterfaceInfo * pMemberInfo;

    switch (order)
        {
        case kISetAnyOrder:
        case kISetLIFO:
            {
            while ((pMemberInfo = GetFirst()) != 0)
                {
                SafeRelease(pMemberInfo->pUnknown);
                delete Remove(pMemberInfo);
                }
            break;
            }

        case kISetFIFO:
            {
            while ((pMemberInfo = GetLast()) != 0)
                {
                SafeRelease(pMemberInfo->pUnknown);
                delete Remove(pMemberInfo);
                }
            break;
            }
        }

    return NOERROR;
    }

///////////////////////////////////////

HRESULT cInterfaceList::Empty()
    {
    DestroyAll();
    return NOERROR;
    }

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cInterfaceTable
//
// Table is sized exactly to fit LG UUIDs
//

cInterfaceTable::cInterfaceTable()
    {
    }

///////////////////////////////////////

HRESULT cInterfaceTable::ReleaseAll(BOOL fDeleteInfos)
    {
    tHashSetHandle Handle;
    cInterfaceInfo * pInfo = (cInterfaceInfo *) GetFirst(Handle);
    while (pInfo)
        {
        Remove(pInfo);
        SafeRelease(pInfo->pUnknown);
        if (fDeleteInfos)
            delete pInfo;
        pInfo = (cInterfaceInfo *) GetFirst(Handle);
        }
    return NOERROR;
    }

///////////////////////////////////////

HRESULT cInterfaceTable::Empty(BOOL fDeleteInfos)
    {
    tHashSetHandle Handle;
    cInterfaceInfo * pInfo = (cInterfaceInfo *) GetFirst(Handle);
    while (pInfo)
        {
        Remove(pInfo);
        if (fDeleteInfos)
            delete pInfo;
        pInfo = (cInterfaceInfo *) GetFirst(Handle);
        }
    return NOERROR;
    }

///////////////////////////////////////

tHashSetKey cInterfaceTable::GetKey(tHashSetNode node) const
    {
    return (tHashSetKey)(((cInterfaceInfo *)(node))->pIID);
    }

///////////////////////////////////////////////////////////////////////////////
