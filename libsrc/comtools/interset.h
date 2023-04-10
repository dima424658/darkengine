///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/comtools/RCS/interset.h $
// $Author: TOML $
// $Date: 1998/04/24 13:08:59 $
// $Revision: 1.7 $
//
// Pre-fab data structures for storing interface pointers
//

#ifndef __INTERSET_H
#define __INTERSET_H

#include <objcoll.h>
#include <str.h>
#include <dlist.h>
#include <hashset.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cInterfaceInfo
//

class cInterfaceInfo;

class cInterfaceInfo : public cDListNode<cInterfaceInfo, 1>
    {
public:
    cInterfaceInfo();
    cInterfaceInfo(const GUID *, IUnknown *, const char * pszName = NULL);

    const GUID *    pIID;
    IUnknown *      pUnknown;
    cStr            nameStr;
    };

///////////////////////////////////////

inline cInterfaceInfo::cInterfaceInfo()
  : pIID(0),
    pUnknown(0)
    {
    }

inline cInterfaceInfo::cInterfaceInfo(const GUID * pIID, IUnknown * pUnknown, const char * pszName)
  : pIID(pIID),
    pUnknown(pUnknown)
    {
    if (pszName)
        nameStr = pszName;
    }


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cInterfaceList
//

class cInterfaceList : public cDList<cInterfaceInfo, 1>
    {
public:
    //
    // Call Release() on all interfaces in the list, deleting cInterfaceInfo's
    //
    HRESULT ReleaseAll(enum eISetOrder);

    //
    // Empty the list without calling release, deleting cInterfaceInfo's
    //
    HRESULT Empty();

    };

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cInterfaceTable
//

//
// Hash table
//

class cInterfaceTable : public cGuidHashSet<cInterfaceInfo *>
    {
public:
    cInterfaceTable();

    //
    // Call Release() on all interfaces in the list, deleting cInterfaceInfo's
    // This is slower than list release.  If infos are in a cInterfaceList,
    // cInterfaceList::ReleaseAll() should be used
    //
    HRESULT ReleaseAll(BOOL fDeleteInfos = TRUE);

    //
    // Empty the list without calling release, deleting cInterfaceInfo's
    //
    HRESULT Empty(BOOL fDeleteInfos = TRUE);


protected:
    virtual tHashSetKey GetKey(tHashSetNode) const;
    };


#endif /* !__INTERSET_H */
