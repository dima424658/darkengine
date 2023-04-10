///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/comtools/RCS/pguidset.cpp $
// $Author: TOML $
// $Date: 1998/02/19 10:34:32 $
// $Revision: 1.8 $
//

#include <lg.h>
#include <comtools.h>
#include <constrid.h>
#include <pguidset.h>

///////////////////////////////////////

// KLC, 9/3/96, no longer part of class.
int LGAPI GetEntryPtrPriority(const void *p)
{
    return (*((sPriGuidSetEntry **) p))->priority;
}

///////////////////////////////////////
//
// Set the contraints on set members of equal priority
//

void cPriGuidSetBase::ApplyConstraints(const sAbsoluteConstraint * pConstraints, unsigned nNumConstraints)
{
    const unsigned nItems = m_Entries.Size();
    const unsigned kTriesLimit = 1000;

    BOOL fDidSwap = TRUE;
    unsigned nTries = 0;

    const sAbsoluteConstraint *pCurrentConstraint;
    sPriGuidSetEntry **pBeforeEntry;
    sPriGuidSetEntry **pAfterEntry;
    index_t i;
    // Repeatedly apply constraints until no changes, or reach break-out limit
    while (fDidSwap && nTries < kTriesLimit)
    {
        fDidSwap = FALSE;
        nTries++;
        for (int iCurrentConstraint = 0; iCurrentConstraint < nNumConstraints; iCurrentConstraint++)
        {
            pCurrentConstraint = &pConstraints[iCurrentConstraint];
            pBeforeEntry = NULL;
            pAfterEntry = NULL;

            AssertMsg(!(*(pCurrentConstraint->pIDBefore) == *(pCurrentConstraint->pIDAfter)), "Cannot constrain an item against itself!");

            // Find what binds to the requests of the constraint
            for (i = 0; i < nItems; i++)
            {
                if (*(m_Entries[i]->pID) == *(pCurrentConstraint->pIDBefore))
                {
                    pBeforeEntry = &(m_Entries[i]);
                }
            }

            if (!pBeforeEntry)                   // If no match at this point, continue with next constraint
                continue;

            for (i = 0; i < nItems; i++)
            {
                if (*(m_Entries[i]->pID) == *(pCurrentConstraint->pIDAfter))
                {
                    pAfterEntry = &(m_Entries[i]);
                }
            }

            if (!pAfterEntry)                    // If no match at this point, continue with next constraint
                continue;

            // If both entries are of the same priority and are out of order
            if ((*pBeforeEntry)->priority == (*pAfterEntry)->priority)
            {
                if (pAfterEntry < pBeforeEntry)
                {
                    // Swap 'em...
                    sPriGuidSetEntry *temp = *pBeforeEntry;
                    *pBeforeEntry = *pAfterEntry;
                    *pAfterEntry = temp;
                    fDidSwap = TRUE;
                }
            }
            else
            {
                // @TBD (toml 06-25-96): must hook in id to name function when written
                AssertMsg2(pBeforeEntry < pAfterEntry, "Cannot resolve constraint because \"after\" is of higher priority than \"before\" (%s and %s)", "", "");
                DebugMsgTrue(pBeforeEntry > pAfterEntry, "Constraint satisfied, though items of different priority");
            }
        }
    }
    // @TBD (toml 06-25-96): must hook in id to name function when written
    AssertMsg2(!fDidSwap, "Failed to resolve constraints (last swap was %s and %s)", "", "");
}
