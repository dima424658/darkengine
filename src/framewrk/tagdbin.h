/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/tagdbin.h,v 1.6 2000/01/31 09:48:47 adurant Exp $
#pragma once

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   tagdbin.h

   A cTagDBInput is the thing you use to ask a cTagDBDatabase a
   question.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#ifndef _TAGDBIN_H_
#define _TAGDBIN_H_


#include <dynarray.h>
#include <ctagset.h>
#include <tagdbt.h>


// For a PartialSet match, this flags dictates that its InputTag is
// required.  It's for the flags field in the InputTag, rather than
// the Input struture itself.
#define kTagDBInputFlagRequired 1
#define kTagDBInputFlagEnum     2


class cTagDBInputTag
{
public:
   cTagDBInputTag(tTagDBKeyType KeyType, tTagDBKeyValue KeyValue,
                  int iFlags = 0, float fWeight = 0.0)
      : m_KeyType(KeyType),
        m_KeyValue(KeyValue),
        m_iFlags(iFlags),
        m_fWeight(fWeight)
   {
   }

   cTagDBInputTag(cTag &Tag, int iFlags = 0, float fWeight = 0.0)
      : m_KeyType(Tag.GetTypeToken()),
        m_KeyValue(Tag.GetValueToken()),
        m_iFlags(iFlags
               | ((Tag.GetFlags() & kTagFlagEnum)? kTagDBInputFlagEnum : 0)),
        m_fWeight(fWeight)
   {
   }

   cTagDBInputTag()
   {
   }

   tTagDBKeyType m_KeyType;
   tTagDBKeyValue m_KeyValue;
   int m_iFlags;
   float m_fWeight;
};


class cTagDBInput : public cDynArray<cTagDBInputTag>
{
public:

   void Clear()
   {
      SetSize(0);
   }

   // It's a little odd that all the flags and weights are the same.    
   void AppendTagSet(const cTagSet *pTagSet, int iFlags = 0, float fWeight = 0.0)
   {
      int iSize = pTagSet->Size();
      for (int i = 0; i < iSize; ++i) {
         cTag Tag = *(pTagSet->GetEntry(i));
         Append(cTagDBInputTag(Tag, iFlags, fWeight));
      }
   }

   void Sort();

   // Have we found all the tags required for a partial match,
   // given that we've made it as far as this index?
   BOOL FoundAllRequired(int iStartIndex)
   {
      int iSize = Size();
      cTagDBInputTag *pArray = AsPointer();

      for (int i = iStartIndex; i < iSize; ++i)
         if (pArray[i].m_iFlags & kTagDBInputFlagRequired)
            return FALSE;

      return TRUE;
   }

   // Hey!  This only works if the collection has been sorted!
   void CombineDuplicates();
};

#endif // ~_TAGDBIN_H_
