/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/ctagset.h,v 1.6 2000/01/31 09:48:54 adurant Exp $
#pragma once

/* --<<= --/-/-/-/-/-/-/ <<< ((( ((( /\ ))) ))) >>> \-\-\-\-\-\-\-- =>>-- *\
   ctagset.h

   container for cTag

\* --<<= --\-\-\-\-\-\-\ <<< ((( ((( \/ ))) ))) >>> /-/-/-/-/-/-/-- =>>-- */


#ifndef _CTAGSET_H_
#define _CTAGSET_H_

#include <allocapi.h>

#include <ctag.h>


#define kTagSetStringSeparator ','

class cTagSet
{
public:
   cTagSet()
   {
   }

   // constructing one cTagSet from another
   cTagSet(const cTagSet &Source)
   {
      LGALLOC_PUSH_CREDIT();
      Append(Source);
      LGALLOC_POP_CREDIT();
   }

   // construction from a single tag, potentially already tokenized
   cTagSet(const cTag &Tag)
   {
      LGALLOC_PUSH_CREDIT();
      m_Data.Append(Tag);
      LGALLOC_POP_CREDIT();
   }

   // construction from a formatted string--if the string is not formatted
   // correctly we have a valid, but empty, cTagSet
   cTagSet(const char *pszString)
   {
      if (pszString && *pszString)
         FromString(pszString);
   }

   // assignment copies all data
   cTagSet &operator=(const cTagSet &Source)
   {
      m_Data = Source.m_Data;
      return *this;
   }

   BOOL operator==(const cTagSet &TagSet)
   {
      if (Size() == TagSet.Size())
         return (memcmp(m_Data, TagSet.m_Data, m_Data.Size() * sizeof(cTag)) == 0);
      return FALSE;
   }

   void Clear()
   {
      m_Data.SetSize(0);
   }

   void Add(const cTag &Tag)
   {
      LGALLOC_PUSH_CREDIT();
      m_Data.Append(Tag);
      LGALLOC_POP_CREDIT();
   }

   void Append(const cTagSet &Source);

   // if not in set, return FALSE--deletion may change order of tags
   // in set
   BOOL Remove(const cTag &Tag);

   void ToString(cStr *pDest) const;
   BOOL FromString(const char *pszString);

   const cTag * GetEntry(int iIndex) const
   {
      return &m_Data[iIndex];
   }

   int Size() const
   {
      return m_Data.Size();
   }

private:
   cDynArray<cTag> m_Data;
};

#endif // ~_CTAGSET_H_
