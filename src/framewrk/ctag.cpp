/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/ctag.cpp,v 1.12 2000/02/14 12:08:32 adurant Exp $

/* --<<= --/-/-/-/-/-/-/ <<< ((( ((( /\ ))) ))) >>> \-\-\-\-\-\-\-- =>>-- *\
   ctag.cpp

   a small, generic, nonpersistent tag class

\* --<<= --\-\-\-\-\-\-\ <<< ((( ((( \/ ))) ))) >>> /-/-/-/-/-/-/-- =>>-- */


#include <mprintf.h>
#include <allocapi.h>

#include <cfgdbg.h>
#include <ctag.h>
#include <hshsttem.h>

#include <ctagnet.h>

// Must be last header 
#include <dbmem.h>


#if defined(_MSC_VER)
#pragma init_seg(lib)
#endif

#ifndef SHIP
// Set this from the debugger if you want mono spew about cTags being
// tokenized.  We can't use config spew for this because some tags
// will be global, constructed before the config system has been
// initialized.
BOOL g_TagDebug = FALSE;
#endif // ~SHIP

// static members of cTag
cDynArray<cTagRecord *> cTag::gm_RecordList;
cTagRecordHash cTag::gm_RecordHash;


void cTag::ToString(cStr *pDest) const
{
   pDest->Empty();
   cTagRecord *pRecord = gm_RecordList[m_iTypeToken];

   if (GetFlags() & kTagFlagInt) 
      pDest->FmtStr("%s%c%d", pRecord->m_TypeName.BufIn(), 
                    kTagStringSeparator, m_iValue);
   else if (pRecord->m_EnumValues.Size() > 0)
      pDest->FmtStr("%s%c%s", pRecord->m_TypeName.BufIn(), 
                    kTagStringSeparator, 
                    pRecord->m_EnumValues[m_iValue].BufIn());
   else
      pDest->FmtStr("%s", pRecord->m_TypeName.BufIn()); 

}


// This parses a C-style null-terminated string and generates tag.
// The type name must be followed by our string separator, and if the
// string following the separator starts with a digit or '-' we treat
// it as an integer.

// This stomps the tag's existing tokenization.
BOOL cTag::FromString(const char *pszString)
{
   int i;
   cStr CopyString = pszString; // copy our input so we can play with it
   CopyString.Trim();           // remove leading & trailing whitespace
   char *pszCopy = CopyString.Detach();

   // find the end of our first token--we interpret a nonempty string
   // with no separator as an integer with a value of 0
   for (i = 0; pszCopy[i] != kTagStringSeparator; ++i) {
      if (pszCopy[i] == '\0') {
#ifndef SHIP
         if (!i) {
            Warning(("cTag string empty.  Bad cTag string.  Bad.\n"));
            Free(pszCopy);
            m_iTypeToken = 0;
            m_iValue = 0;
            return FALSE;
         }
#endif // ~SHIP
         Set(pszCopy, 0);
         Free(pszCopy);
         return TRUE;
      }
   }

   // overwrite separator with 0 to give us our first token
   pszCopy[i] = '\0';

   // We'll allow duplicate separators 'cause we're so stinking nice.
   do {
      ++i;
   } while (pszCopy[i] == kTagStringSeparator);

   // We've found our value.  Is it a number?
   if ((pszCopy[i] >= '0' && pszCopy[i] <= '9') || pszCopy[i] == '-')
      Set(pszCopy, atoi(pszCopy + i));
   else
      Set(pszCopy, pszCopy + i);

   Free(pszCopy);
   return TRUE;
}


// accessors
void cTag::Set(const char *pszTypeName, const char *pszValueName)
{
   cTagRecord *pRecord = GetOrMakeRecord(pszTypeName, kTagFlagEnum);
   m_iTypeToken = pRecord->m_iIndex;
   m_iValue = GetOrMakeEnum(pRecord, pszValueName);
#ifndef SHIP
   if (m_iValue > 254)
      Warning(("cTag %s, enum %s has Value > 254: %d\n",
               pszTypeName, pszValueName, m_iValue));
#endif // ~SHIP
}


void cTag::Set(const char *pszTypeName, int iValue)
{
   cTagRecord *pRecord = GetOrMakeRecord(pszTypeName, kTagFlagInt);
   m_iTypeToken = pRecord->m_iIndex;
   m_iValue = iValue;
}


const char *cTag::GetEnumValue() const
{
   Assert_(GetFlags() & kTagFlagEnum);

   cTagRecord *pRecord = gm_RecordList[m_iTypeToken];
   int iEnumSize = pRecord->m_EnumValues.Size();
   Assert_(m_iValue >= 0 && m_iValue < iEnumSize);

   return (const char *) (pRecord->m_EnumValues)[m_iValue];
}


const char *cTag::GetType() const
{
   cTagRecord *pRecord = gm_RecordList[m_iTypeToken];
   return (const char *) pRecord->m_TypeName;
}


// This is the only place where we create new cTagRecords.

// If there is already a record for this type name, we verify that its
// flags agree with the new ones.  If there isn't already a record we
// make one and give it the requested flags.
cTagRecord *cTag::GetOrMakeRecord(const char *pszTypeName, int iFlags)
{
   LGALLOC_PUSH_CREDIT();
   cTagRecord *pRecord = gm_RecordHash.Search(pszTypeName);

   if (pRecord) {

#ifndef SHIP
      int itype = iFlags & (kTagFlagInt|kTagFlagEnum); 
      int rtype = pRecord->m_iFlags & (kTagFlagInt|kTagFlagEnum); 
      if (itype != rtype 
          && !(rtype == kTagFlagEnum && pRecord->m_EnumValues.Size() <= 1)) 
         mprintf("cTag: enum-int mismatch for %s\n", pszTypeName);
#endif // ~SHIP

   } else {
      pRecord = new cTagRecord(pszTypeName, gm_RecordList.Size(), iFlags);
      Assert_(pRecord);
      gm_RecordList.Append(pRecord);
      gm_RecordHash.Insert(pRecord);
      // Tell any other players about this record
      BroadcastTagRecord(pRecord);

#ifndef SHIP
      if (g_TagDebug)
         mprintf("cTag %d created: %s, type %s\n",
                 pRecord->m_iIndex, pszTypeName,
                 (iFlags & kTagFlagEnum)? "Enum" : "Int");
#endif // ~SHIP
   }

   LGALLOC_POP_CREDIT();
   return pRecord;
}


int cTag::GetOrMakeEnum(cTagRecord *pRecord, const char *pszValueName)
{
   int iNumEnums = pRecord->m_EnumValues.Size();

   for (int i = 0; i < iNumEnums; ++i)
      if ((pRecord->m_EnumValues)[i].CompareNoCase(pszValueName) == 0)
         return i;

   LGALLOC_PUSH_CREDIT();
   // Looks like we don't have this one yet.
   pRecord->m_EnumValues.SetSize(iNumEnums + 1);
   pRecord->m_EnumValues[iNumEnums] = pszValueName;
   // Tell the other players about this new value:
   BroadcastTagEnum(pRecord, iNumEnums, pszValueName);
   LGALLOC_POP_CREDIT();

#ifndef SHIP
      if (g_TagDebug)
         mprintf("cTag %d %s gained enum %d %s\n",
                          pRecord->m_iIndex, pRecord->m_TypeName.BufIn(),
                          iNumEnums, pszValueName);
#endif // ~SHIP

   return iNumEnums;
}
