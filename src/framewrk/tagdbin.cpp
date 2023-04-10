/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/tagdbin.cpp,v 1.5 1998/10/05 17:28:19 mahk Exp $

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   tagdbin.cpp

   This is the structure you use to search a TagDBDatabase.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#include <tagdbin.h>


// Must be last header 
#include <dbmem.h>


void cTagDBInput::Sort()
{
   cTagDBInputTag Temp;
   int iInnerLimit = Size();
   int iOuterLimit = iInnerLimit - 1;
   cTagDBInputTag *pArray = AsPointer();
   int i, j, iBestIndex;

   for (i = 0; i < iOuterLimit; ++i) {
      iBestIndex = i;

      for (j = i + 1; j < iInnerLimit; ++j)
         if ((pArray[iBestIndex].m_KeyType > pArray[j].m_KeyType)
          || ((pArray[iBestIndex].m_KeyType == pArray[j].m_KeyType)
           && (pArray[iBestIndex].m_KeyValue > pArray[j].m_KeyValue)))
            iBestIndex = j;

      Temp = pArray[i];
      pArray[i] = pArray[iBestIndex];
      pArray[iBestIndex] = Temp;
   }
}


// Hey!  This only works if the collection has been sorted!
void cTagDBInput::CombineDuplicates()
{
   int iLimit = Size() - 1;
   cTagDBInputTag *pArray = AsPointer();
   for (int i = 0; i < iLimit; ++i) {
      if (pArray[i].m_KeyType == pArray[i + 1].m_KeyType) {
         DeleteItem(i);
         --iLimit;
         --i;
      }
   }
}

