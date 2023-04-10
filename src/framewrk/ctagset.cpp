/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/ctagset.cpp,v 1.6 1998/10/18 16:43:53 TOML Exp $

/* --<<= --/-/-/-/-/-/-/ <<< ((( ((( /\ ))) ))) >>> \-\-\-\-\-\-\-- =>>-- *\
   ctag.cpp

   container for cTag

\* --<<= --\-\-\-\-\-\-\ <<< ((( ((( \/ ))) ))) >>> /-/-/-/-/-/-/-- =>>-- */

#include <allocapi.h>

#include <ctagset.h>

// Must be last header 
#include <dbmem.h>


void cTagSet::Append(const cTagSet &Source)
{
   LGALLOC_PUSH_CREDIT();
   
   int iSize = Source.Size();
   const cTag *pTag = Source.m_Data.AsPointer();

   for (int i = 0; i < iSize; ++pTag, ++i)
      m_Data.Append(*pTag);

   LGALLOC_POP_CREDIT();
}


BOOL cTagSet::Remove(const cTag &Tag)
{
   int iSize = m_Data.Size();
   cTag *pTag = m_Data.AsPointer();

   for (int i = 0; i < iSize; ++pTag, ++i)
      if (pTag[i] == Tag) {
         m_Data.FastDeleteItem(i);
         return TRUE;
      }

   return FALSE;
}


void cTagSet::ToString(cStr *pDest) const
{
   LGALLOC_PUSH_CREDIT();
   cStr TagString;
   static char Separator[] = { kTagSetStringSeparator, 0 };
   pDest->Empty();
   int iSize = m_Data.Size();
   const cTag *pTag = m_Data.AsPointer();

   for (int i = 0; i < iSize; ++pTag, ++i) {
      pTag->ToString(&TagString);
      pDest->Append(TagString.GetLength(), TagString.BufIn());
      if (i != (iSize - 1))
         pDest->Append(1, Separator);
   }
   LGALLOC_POP_CREDIT();
}


// The string should, of course, be null-terminated.
BOOL cTagSet::FromString(const char *pszString)
{
   Clear();

   cStr CopyString = pszString; // copy our input so we can play with it
   CopyString.Trim();
   char *pszCopy = CopyString.Detach();
   int iLen = strlen(pszCopy);

   // This makes us robust for trailing separators.
   if (pszCopy[iLen - 1] == kTagSetStringSeparator) {
      pszCopy[iLen - 1] = '\0';
      --iLen;
   }
   
   if (!iLen)
   {
      Free(pszCopy);
      return TRUE;
   }

   char *pszTagString = pszCopy;
   for (int i = 0; i <= iLen; ++i) {
      if (pszCopy[i] == kTagSetStringSeparator || pszCopy[i] == '\0') {
         pszCopy[i] = '\0';

         cTag TempTag;
         if (TempTag.FromString(pszTagString) == FALSE) {
            Free(pszCopy);
            return FALSE;
         }
         Add(TempTag);

         pszTagString = pszCopy + i + 1;
      }
   }

   Free(pszCopy);
   return TRUE;
}
