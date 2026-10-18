///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/cpptools/RCS/pdynarr.cpp $
// $Author: TOML $
// $Date: 1996/10/21 14:14:44 $
// $Revision: 1.5 $
//

#include <lg.h>
#include <pdynarr.h>

///////////////////////////////////////////////////////////////////////////////

tGetPriorityFunc cPriDynArrayCompareHolder::gm_pfnGetPriority;

///////////////////////////////////////

int cPriDynArrayCompareHolder::Compare(const void* pLeft, const void* pRight)
{
    return ComparePriorities((*gm_pfnGetPriority)(pLeft),
                             (*gm_pfnGetPriority)(pRight));
}

///////////////////////////////////////

