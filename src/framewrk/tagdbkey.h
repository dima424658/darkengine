/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/tagdbkey.h,v 1.3 2000/01/31 09:48:48 adurant Exp $
#pragma once

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   tagdbkey.h

   This is the atomic object for organizing our tree.  It has two
   forms: it can represent a range of integers, or as many as eight
   enum values.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#ifndef _TAGDBKEY_H_
#define _TAGDBKEY_H_


#include <lgassert.h>

#include <tagdbt.h>
#include <tagdbin.h>


class cTagDBKey
{
public:
   cTagDBKey() {}
   cTagDBKey(tTagDBKeyType Type, tTagDBKeyValue Min, tTagDBKeyValue Max);
   cTagDBKey(tTagDBKeyType Type, tTagDBKeyEnum[8]);
   BOOL operator > (const cTagDBKey &rhs) const;
   BOOL operator == (const cTagDBKey &rhs) const;

   // This is TRUE if the given value is within our key's range,
   // inclusive.
   BOOL Match(const cTagDBInputTag *pTag) const;

   tTagDBKeyType m_KeyType;

   union 
   {
      struct 
      {
         tTagDBKeyValue m_KeyMin;
         tTagDBKeyValue m_KeyMax;
      };
      struct
      {
         // If there are fewer than eight values here, they should be
         // at the start of the array.  If there are fewer than for
         // then the first unused slot should be kTagDBKeyEnumUnused.
         // Each must fit in two bytes.
         tTagDBKeyEnum m_aEnum[8];
      };
   };
};


inline cTagDBKey::cTagDBKey(tTagDBKeyType Type, 
                            tTagDBKeyValue Min, tTagDBKeyValue Max)
   : m_KeyType(Type), 
     m_KeyMin(Min), 
     m_KeyMax(Max)
{
}


// Apparently we can't initialize arrays using =.
inline cTagDBKey::cTagDBKey(tTagDBKeyType Type, tTagDBKeyEnum Enum[8])
   : m_KeyType(Type)
{
   m_aEnum[0] = Enum[0];
   m_aEnum[1] = Enum[1];
   m_aEnum[2] = Enum[2];
   m_aEnum[3] = Enum[3];
   m_aEnum[4] = Enum[4];
   m_aEnum[5] = Enum[5];
   m_aEnum[6] = Enum[6];
   m_aEnum[7] = Enum[7];
}


// The precedence of the keys makes this a surprisingly expensive
// comparison.
inline BOOL cTagDBKey::operator > (const cTagDBKey &rhs) const
{
   if (m_KeyType > rhs.m_KeyType)
      return TRUE;
   if (m_KeyType == rhs.m_KeyType
    && m_KeyMin > rhs.m_KeyMin)
      return TRUE;
   if (m_KeyType == rhs.m_KeyType
    && m_KeyMin == rhs.m_KeyMin
    && m_KeyMax > rhs.m_KeyMax)
      return TRUE;

   return FALSE;
}


inline BOOL cTagDBKey::operator == (const cTagDBKey &rhs) const
{
   if (m_KeyType == rhs.m_KeyType 
    && m_KeyMin  == rhs.m_KeyMin
    && m_KeyMax  == rhs.m_KeyMax)
      return TRUE;

   return FALSE;
}

#endif // ~_TAGDBKEY_H_
