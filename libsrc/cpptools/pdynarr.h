///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/cpptools/RCS/pdynarr.h $
// $Author: TOML $
// $Date: 1998/10/03 10:20:43 $
// $Revision: 1.7 $
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
    static int Compare(const void *, const void *);
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
      cDynArray<T>::Sort((tCompareFunc)cPriDynArrayCompareHolder::Compare);
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
