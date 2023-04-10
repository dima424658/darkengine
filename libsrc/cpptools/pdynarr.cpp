///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/cpptools/RCS/pdynarr.cpp $
// $Author: TOML $
// $Date: 1996/10/21 14:14:44 $
// $Revision: 1.5 $
//

#if 0
#include <windows.h>
#endif
#include <lg.h>
#include <pdynarr.h>

///////////////////////////////////////////////////////////////////////////////

tGetPriorityFunc cPriDynArrayCompareHolder::gm_pfnGetPriority;

///////////////////////////////////////

int cPriDynArrayCompareHolder::Compare(const tDynArrayItem * pLeft, const tDynArrayItem * pRight)
    {
    int retVal = ComparePriorities((*gm_pfnGetPriority)(pLeft), (*gm_pfnGetPriority)(pRight));
#if 0 // def DEBUG
    if (retVal == 0)
        {
        // Shake up to sort order so we'll get different results with every
        // execution by generating a reasonably unique index into an array
        // of random numbers.
        static int randomValues[256];
        const int nRandomValues = sizeof(randomValues) / sizeof(int);

        if (randomValues[0] == randomValues[1])
            {
            srand(GetTickCount());
            for (int i = 0; i < nRandomValues; i++)
                randomValues[i] = rand();
            }

        unsigned iLeft  = (LOWORD(pLeft) *  HIWORD(pLeft) *  int(pLeft) >> 3) %  (nRandomValues - 1);
        unsigned iRight = (LOWORD(pRight) * HIWORD(pRight) * int(pRight) >> 3) % (nRandomValues - 1);
        retVal = randomValues[iLeft] - randomValues[iRight];
        }
#endif

    return retVal;
    }

///////////////////////////////////////

