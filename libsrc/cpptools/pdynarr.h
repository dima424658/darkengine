///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/cpptools/RCS/pdynarr.h $
// $Author: JAEMZ $
// $Date: 1997/08/13 19:00:29 $
// $Revision: 1.6 $
//

#ifndef __PDYNARR_H
#define __PDYNARR_H

#include <dynarray.h>
#include <prikind.h>

///////////////////////////////////////////////////////////////////////////////
//
// TEMPLATE: cPriDynArray
//
// Provides convenient prioritized dynamic array.
//

typedef int (LGAPI * tGetPriorityFunc)(const void *);

class cPriDynArrayCompareHolder
    {
protected:
    static int Compare(const tDynArrayItem *, const tDynArrayItem *);
    static tGetPriorityFunc gm_pfnGetPriority;
    };
	
///////////////////////////////////////

template <class T, tGetPriorityFunc D>
class cPriDynArray : public cDynArray<T>, private cPriDynArrayCompareHolder
    {
public:
    cPriDynArray() {}

    //
    // Sort the array
    //
    void Sort()
    {
      AssertMsg(!gm_pfnGetPriority, "Already sorting?");
      cPriDynArrayCompareHolder::gm_pfnGetPriority = D;
      cDynArrayBase::Sort(cPriDynArrayCompareHolder::Compare);
      cPriDynArrayCompareHolder::gm_pfnGetPriority = NULL;
    }

    //
    // Insert an item (insertion sort)
    //
    void Insert(const T &);

    };

///////////////////////////////////////

///////////////////////////////////////

#endif /* !__PDYNARR_H */
