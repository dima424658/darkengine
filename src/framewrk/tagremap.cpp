/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/tagremap.cpp,v 1.2 1998/10/05 17:28:27 mahk Exp $

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   tagremap.cpp

   miniscule utility classes for remapping between the global cTags
   and a local tokenization stored in cNameMaps

   Both of our main methods here perform idiotic brute-force searches,
   but simple enough and with good enough cache behavior--especially
   if we remap a bunch of tags at a time--that we'll be ok.  But let's
   not use them at frame rates, all right?

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#include <tagremap.h>
#include <ctag.h>

// Must be last header 
#include <dbmem.h>


// We want to ignore the value field if we're remapping an integer tag.
BOOL cTagRemapper::LocalToGlobal(sTagRemap *pRemap, BOOL bTypeOnly)
{
   sTagRemap *p = m_RemapTable.AsPointer();
   cTag TempTag;

   for (int i = m_RemapTable.Size(); i != 0; ++p, --i) {
      if (p->m_LocalType == pRemap->m_LocalType
       && (bTypeOnly || p->m_LocalValue == pRemap->m_LocalValue)) {
         pRemap->m_GlobalType = p->m_GlobalType;
         pRemap->m_GlobalValue = p->m_GlobalValue;
         return TRUE;
      }
   }

   // Looks like we don't have this (tag, value) pair yet.  The act of
   // finding it may force some new global tags into existance.  This
   // is fine with us.
   const char *pszTypeName = m_pTypeMap->NameFromID(pRemap->m_LocalType)->text;
   if (!pszTypeName) {
      mprintf("Tag type not found for remapping.\n");
      pRemap->m_GlobalType = pRemap->m_GlobalValue = 0;
      return FALSE;
   }

   if (bTypeOnly) 
      TempTag.Set(pszTypeName, 0);
   else {
      const char *pszValueName
         = m_pValueMap->NameFromID(pRemap->m_LocalValue)->text;
      if (!pszValueName) {
         mprintf("Tag value not found for remapping.\n");
         pRemap->m_GlobalType = pRemap->m_GlobalValue = 0;
         return FALSE;
      }
      TempTag.Set(pszTypeName, pszValueName);
   }

   pRemap->m_GlobalType = TempTag.GetTypeToken();
   pRemap->m_GlobalValue = TempTag.GetValueToken();

   m_RemapTable.Append(*pRemap);
   return TRUE;
}


BOOL cTagRemapper::GlobalToLocal(sTagRemap *pRemap, BOOL bTypeOnly)
{
   sTagRemap *p = m_RemapTable.AsPointer();
   for (int i = m_RemapTable.Size(); i != 0; ++p, --i) {
      if (p->m_GlobalType == pRemap->m_GlobalType
       && (bTypeOnly || p->m_GlobalValue == pRemap->m_GlobalValue)) {
         pRemap->m_LocalType = p->m_LocalType;
         pRemap->m_LocalValue = p->m_LocalValue;
         return TRUE;
      }
   }

   Warning(("Could not remap tag of type %d\n", pRemap->m_LocalType));
   pRemap->m_LocalType = pRemap->m_LocalValue = 0;
   return FALSE;
}
