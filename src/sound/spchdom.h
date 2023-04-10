/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/spchdom.h,v 1.4 2000/01/31 10:02:49 adurant Exp $
#pragma once

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   spchdom.h

   A domain is a collection of concepts plus a collection of tags,
   where each tag has zero or more ordered values.

   Each concept, each tag, and each tag/value pair which goes into
   a domain is assigned an integer which is unique to that domain.
   You can toss these into the domain in any order, and then query
   the domain to get the integers back.  So it's a specialized
   name database.

   For each concept, we also keep track of an integer priority.
   A higher-priority concept may interrupt a lower one.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#ifndef _SPCHDOM_H_
#define _SPCHDOM_H_


#include <lgassert.h>
#include <dynarray.h>

#include <tagfile.h>
#include <spcht.h>
#include <tagremap.h>
#include <namemap.h>


class cSpeechDomain
{
public:
   cSpeechDomain()
      : m_Remap(&m_Tag, &m_Value)
   {
   }

   ~cSpeechDomain()
   {
      Clear();
   }

   // Each of these returns a handle for the thing being added.
   int AddConcept(Label *pName, int iPriority);
   int AddTag(Label *pName, int iFlags);
   int AddValue(Label *pName);

   // Each of these looks up data entered above.
   int ConceptIndex(const Label *pName);
   int TagIndex(const Label *pName);
   int ValueIndex(const Label *pName);

   // swizzling between our local tokenization and cTags
   void TokensLocalToGlobal(sTagRemap *pRemap, BOOL bTypeOnly)
   {
      if (pRemap->m_LocalType == kNameMapIndexNotFound) {
         mprintf("Speech token not found.\n");
         return;
      }

      if (!m_Remap.LocalToGlobal(pRemap, bTypeOnly))
         if (bTypeOnly)
            mprintf("Unable to remap integer tag \n",
                    TagName(pRemap->m_LocalType)->text);
         else
            mprintf("Unable to remap tag  \n",
                    TagName(pRemap->m_LocalType)->text,
                    ValueName(pRemap->m_LocalValue)->text);
   }

   void TokensGlobalToLocal(sTagRemap *pRemap, BOOL bTypeOnly)
   {
      m_Remap.GlobalToLocal(pRemap, bTypeOnly);
   }

   Label *TagName(int iIndex)
   {
      return m_Tag.NameFromID(iIndex);
   }

   Label *ValueName(int iIndex)
   {
      return m_Value.NameFromID(iIndex);
   }

   Label *ConceptName(int iIndex)
   {
      return m_Concept.NameFromID(iIndex);
   }

   int NumConcepts()
   {
      return m_Concept.Size();
   }

   int NumTags()
   {
      return m_Tag.Size();
   }

   int FindConceptPriority(int iConceptIndex)
   {
      return m_ConceptPriority[iConceptIndex];
   }

   int FindTagFlags(int iTagIndex)
   {
      if (iTagIndex == kNameMapIndexNotFound) {
         mprintf("Speech token not found for finding flags.\n");
         return 0;
      }

      return m_TagFlags[iTagIndex];
   }

   void Load(ITagFile *pFile);
   void Save(ITagFile *pFile);
   void Clear();

protected:
   cNameMap m_Concept;
   cNameMap m_Tag;
   cNameMap m_Value;
   cTagRemapper m_Remap;

   cDynArray<int> m_ConceptPriority;
   cDynArray<int> m_TagFlags;
};

#endif // ~_SPCHDOM_H_

