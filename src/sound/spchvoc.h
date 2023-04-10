/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/spchvoc.h,v 1.3 2000/01/31 10:02:56 adurant Exp $
#pragma once

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   spchvoc.h

   A voice maintains a collection of tag databases, one for each
   concept in the speech system.

   The voice has no notion of an individual tag or tag value.  But it
   does pass these things on to the tag databases it manages.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#ifndef _SPCHVOC_H_
#define _SPCHVOC_H_


#include <lgassert.h>
#include <dynarray.h>
#include <objtype.h>
#include <osystype.h>
#include <tagfile.h>
#include <tagdbkst.h>

#include <spcht.h>
#include <tagdbt.h>


class cSpeechVoice
{
public:
   cSpeechVoice();
   ~cSpeechVoice();

   // We expect our schemas and tags to be loaded before we try
   // building any voices.
   void AddDataStart(sTagDBData Data, const Label *pConceptName);
   void AddDataAddTag(const Label *pTagName, int iMin, int iMax);
   void AddDataAddTag(const Label *pTagName, const Label *pEnum);
   void AddDataFinish();

   BOOL SelectSchemas(cTagDBInput *pInput, int iConceptIndex, 
                      cTagDBOutput *pOutput);

   void TransformData(void Func(sTagDBData *));
   void TransformKeys(void Func(cTagDBKey *));

   // Our load and save routines do not open or close their tag
   // blocks, just use them.  So they have no sense of version.
   void Load(ITagFile *pFile);
   void Save(ITagFile *pFile);
   void Clear();

protected:
   cDynArray<ITagDBDatabase *> m_ConceptDatabase;

   // for building new entries
   ITagDBKeySet *m_pCurrentAddition;
   int m_iCurrentConcept;
   sTagDBData m_CurrentData;
};


#endif // ~_SPCHVOC_H_
