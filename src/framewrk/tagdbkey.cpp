/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/tagdbkey.cpp,v 1.3 1998/10/05 17:28:21 mahk Exp $

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   tagdbkey.cpp

   This is the atomic object for organizing our tree.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */

#include <tagdbkey.h>


// Must be last header 
#include <dbmem.h>



BOOL cTagDBKey::Match(const cTagDBInputTag *pTag) const
{
   tTagDBKeyValue Value = pTag->m_KeyValue;

   if ((pTag->m_iFlags) & kTagDBInputFlagEnum) {

      if (m_aEnum[0] == Value)
         return TRUE;

      if (m_aEnum[1] == kTagDBKeyEnumUnused)
         return FALSE;
      if (m_aEnum[1] == Value)
         return TRUE;

      if (m_aEnum[2] == kTagDBKeyEnumUnused)
         return FALSE;
      if (m_aEnum[2] == Value)
         return TRUE;

      if (m_aEnum[3] == kTagDBKeyEnumUnused)
         return FALSE;
      if (m_aEnum[3] == Value)
         return TRUE;

      if (m_aEnum[4] == kTagDBKeyEnumUnused)
         return FALSE;
      if (m_aEnum[4] == Value)
         return TRUE;

      if (m_aEnum[5] == kTagDBKeyEnumUnused)
         return FALSE;
      if (m_aEnum[5] == Value)
         return TRUE;

      if (m_aEnum[6] == kTagDBKeyEnumUnused)
         return FALSE;
      if (m_aEnum[6] == Value)
         return TRUE;

      if (m_aEnum[7] == kTagDBKeyEnumUnused)
         return FALSE;
      if (m_aEnum[7] == Value)
         return TRUE;

      return FALSE;

   } else {
      // regular old integer case
      if ((Value >= m_KeyMin) && (Value <= m_KeyMax))
         return TRUE;
      else
         return FALSE;
   }
}

