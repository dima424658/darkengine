/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/tagdbout.cpp,v 1.4 1998/10/05 17:28:24 mahk Exp $

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   tagdbout.cpp

   This is really just a collection of integers which lets you iterate
   over it.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#include <rand.h>
#include <mprintf.h>
#include <osysbase.h>
#include <tagdbout.h>


// Must be last header 
#include <dbmem.h>


// If we were interested in pulling a number of selections out of a
// single output we could move some of this work into a separate
// function and only do it once.
int cTagDBOutput::Choose()
{
   int i, iSize = Size();
   sTagDBData *pData = AsPointer();

   if (iSize == 0)
      return OBJ_NULL;

   if (iSize == 1)
      return pData[0].m_iData;

   // Rand() returns in the range 0..32767, so we scale our
   // selection value to match.
   float fTotalWeight = 0.0;
   for (i = 0; i < iSize; ++i)
      fTotalWeight += pData[i].m_fWeight;

   // We scale the selection down a tiny bit here to make sure we'll
   // fall within the range, even with roundoff error.
   float fSelection = ((float) Rand()) * (fTotalWeight / 32768.0);

   i = -1;
   float fAccum = 0;
   do {
      i++;
      fAccum += pData[i].m_fWeight;
   } while (fAccum < fSelection);

   return pData[i].m_iData;
}


// This incremental selection sort finds the element with the highest
// weighting for the array, from iIndex to the end, and swaps that
// element with the one at iIndex.
void cTagDBOutput::NextByWeight(int iIndex)
{
   sTagDBData Temp;
   int iLimit = Size();
#ifndef SHIP
   AssertMsg1(iIndex < iLimit, "cTagDBOutput::NextByWeight: no element %d",
              iIndex);
#endif
   sTagDBData *pArray = AsPointer();
   int iBestIndex = iIndex;
   float fBestWeight = pArray[iIndex].m_fWeight;

   for (int i = iIndex + 1; i < iLimit; ++i)
      if (pArray[i].m_fWeight > fBestWeight) {
         fBestWeight = pArray[i].m_fWeight;
         iBestIndex = i;
      }

   if (iBestIndex != iIndex) {
      Temp = pArray[iBestIndex];
      pArray[iBestIndex] = pArray[iIndex];
      pArray[iIndex] = Temp;
   }
}
