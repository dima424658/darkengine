/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/ctag.h,v 1.5 2000/01/31 09:48:53 adurant Exp $
#pragma once

/* --<<= --/-/-/-/-/-/-/ <<< ((( ((( /\ ))) ))) >>> \-\-\-\-\-\-\-- =>>-- *\
   ctag.h

   This is a small, generic, nonpersistent tag class.  Any time we
   create a tag or set its state, we first check whether any tags
   already exist with that name.  If so, we already have their tokens
   so life is easy.  If not, we generate new tokens so life will be
   easy from then on.

   All strings are case-insensitive.

\* --<<= --\-\-\-\-\-\-\ <<< ((( ((( \/ ))) ))) >>> /-/-/-/-/-/-/-- =>>-- */


#ifndef _CTAG_H_
#define _CTAG_H_


#include <dynarray.h>
#include <hashset.h>
#include <str.h>


// flags

// these two are actually mutually exclusive
#define kTagFlagInt             1
#define kTagFlagEnum            2


// the character between the type name and the value
#define kTagStringSeparator ' '


/* --<<= --/-/-/-/-/-/-/ <<< ((( ((( /\ ))) ))) >>> \-\-\-\-\-\-\-- =>>-- *\

   helper class for data common to all cTag instances

\* --<<= --\-\-\-\-\-\-\ <<< ((( ((( \/ ))) ))) >>> /-/-/-/-/-/-/-- =>>-- */
class cTagRecord
{
public:
   cTagRecord(const char *pszTypeName, int iIndex, int iFlags)
      : m_TypeName(pszTypeName),
        m_iIndex(iIndex),
        m_iFlags(iFlags)
   {
   }

   cStr m_TypeName;
   int m_iIndex;     // index into gm_Record; used as type token
   int m_iFlags;     // tells us whether we're int or enum, at least
   cDynClassArray<cStr> m_EnumValues;     // unused for int tags
};


/* --<<= --/-/-/-/-/-/-/ <<< ((( ((( /\ ))) ))) >>> \-\-\-\-\-\-\-- =>>-- *\

   another helper class for cTag

\* --<<= --\-\-\-\-\-\-\ <<< ((( ((( \/ ))) ))) >>> /-/-/-/-/-/-/-- =>>-- */
class cTagRecordHash : public cStrIHashSet<cTagRecord *>
{
public:
   ~cTagRecordHash()
   {
      DestroyAll();
   }

   tHashSetKey GetKey(tHashSetNode p) const
   {
      return (tHashSetKey) (((cTagRecord *)p)->m_TypeName.BufIn());
   }
};


/* --<<= --/-/-/-/-/-/-/ <<< ((( ((( /\ ))) ))) >>> \-\-\-\-\-\-\-- =>>-- *\

   cTag: the only class here which should be used by other modules.

\* --<<= --\-\-\-\-\-\-\ <<< ((( ((( \/ ))) ))) >>> /-/-/-/-/-/-/-- =>>-- */
class cTag
{
public:
   // all our loverly constructors
   cTag()
   {
   }

   cTag(const char *pszTypeName, const char *pszValueName)
   {
      Set(pszTypeName, pszValueName);
   }

   cTag(const char *pszTypeName, int iValue)
   {
      Set(pszTypeName, iValue);
   }

   cTag(const cTag &Tag)
    : m_iTypeToken(Tag.m_iTypeToken),
      m_iValue(Tag.m_iValue)
   {
   }

   cTag &operator=(const cTag &Tag)
   {
      m_iTypeToken = Tag.m_iTypeToken;
      m_iValue = Tag.m_iValue;
      return *this;
   }

   BOOL operator==(const cTag &Tag)
   {
      if (m_iTypeToken == Tag.m_iTypeToken && m_iValue == m_iValue)
         return TRUE;
      else
         return FALSE;
   }

   // converting to and from our standard string format
   void ToString(cStr *pDest) const;
   BOOL FromString(const char *pszString);  // stomps existing tokenization

   // accessors
   void Set(const char *pszTypeName, const char *pszValueName);
   void Set(const char *pszTypeName, int iValue);

   int GetFlags() const
   {
      cTagRecord *pRecord = gm_RecordList[m_iTypeToken];
      return pRecord->m_iFlags;
   }

   // One one of these or the other is valid for a given tag.
   const char *GetEnumValue() const;

   int GetIntValue() const
   {
      return m_iValue;
   }

   const char *GetType() const;

   int GetTypeToken() const
   {
      return m_iTypeToken;
   }

   int GetValueToken() const
   {
      return m_iValue;
   }

   //////////
   // These methods are semi-internal; they are visible mainly for the
   // use of tag networking...
   static int GetNumTags()
   {
      return gm_RecordList.Size();
   }
   static cTagRecord *GetTagRecord(int index)
   {
      return gm_RecordList[index];
   }
   static cTagRecord *GetOrMakeRecord(const char *pszTypeName, int iFlags);
   static int GetOrMakeEnum(cTagRecord *pRecord, const char *pszValueName);
   void Set(int type, int value)
   {
      m_iTypeToken = type;
      m_iValue = value;
   }

private:
   // storage
   int m_iTypeToken;
   int m_iValue;        // could be an int, could be named, depending on type

   static cDynArray<cTagRecord *> gm_RecordList;
   static cTagRecordHash gm_RecordHash;
};

#endif // ~_CTAG_H_
