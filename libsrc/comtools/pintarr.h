///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/comtools/RCS/pintarr.h $
// $Author: TOML $
// $Date: 1997/07/03 13:25:11 $
// $Revision: 1.3 $
//

#ifndef __PINTARR_H
#define __PINTARR_H

#include <str.h>
#include <pguidset.h>

///////////////////////////////////////////////////////////////////////////////
//
// STRUCT: sPriIntInfo
//
// Prioritized interface information
//

template <class InterfaceType>
struct sPriIntInfo : public sPriGuidSetEntry
    {
public:
    sPriIntInfo()
      : pInterface(NULL),
        pData(NULL)
        {
        }

    sPriIntInfo(const GUID * pID, InterfaceType * pInterface,
                const char * pszName = NULL,
                int priority = kPriorityNormal,
                void * pData = NULL)
      : sPriGuidSetEntry(pID, priority),
        pInterface(pInterface),
        nameStr(pszName),
        pData(pData)
        {
        }

    // The interface
    InterfaceType * pInterface;

    // The human-readable name of the component (optional)
    cStr            nameStr;

    // Client defined value
    void *          pData;

    };


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPriIntArray
//
// Stores pointers to IUnknown, sorted by a priority (0-255), optionally
// constrained to resolve equal priority items
//

template <class InterfaceType>
class cPriIntArray : public cPriGuidSet< sPriIntInfo<InterfaceType> >
    {
public:

    cPriIntArray();

    };


///////////////////////////////////////

template <class InterfaceType>
inline cPriIntArray<InterfaceType>::cPriIntArray()
    {
    }

///////////////////////////////////////////////////////////////////////////////

#endif /* !__PINTARR_H */
