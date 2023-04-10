/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/tagremap.h,v 1.2 2000/01/31 09:48:55 adurant Exp $
#pragma once

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   tagremap.h

   miniscule utility classes for remapping between the global cTags
   and a local tokenization stored in cNameMaps--we use our own
   tokenization for the disk image, but swizzle to the game-wide tag
   system after loading.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#ifndef _TAGREMAP_H_
#define _TAGREMAP_H_


#include <namemap.h>


struct sTagRemap
{
   sTagRemap() {}
   sTagRemap(uint LocalType, uint LocalValue,
             uint GlobalType, uint GlobalValue)
      : m_LocalType(LocalType),
        m_LocalValue(LocalValue),
        m_GlobalType(GlobalType),
        m_GlobalValue(GlobalValue)
   {
   }

   uint m_LocalType;
   uint m_LocalValue;
   uint m_GlobalType;
   uint m_GlobalValue;
};


class cTagRemapper
{
public:
   cTagRemapper(cNameMap *pTypeMap, cNameMap *pValueMap)
      : m_pTypeMap(pTypeMap),
        m_pValueMap(pValueMap)
   {
   }

   void Clear()
   {
      m_RemapTable.SetSize(0);
   }

   // The return value indicates success.
   BOOL LocalToGlobal(sTagRemap *pRemap, BOOL bTypeOnly);
   BOOL GlobalToLocal(sTagRemap *pRemap, BOOL bTypeOnly);

protected:
   cNameMap *m_pTypeMap;
   cNameMap *m_pValueMap;
   cDynArray<sTagRemap> m_RemapTable;
};

#endif //~ _TAGREMAP_H_
