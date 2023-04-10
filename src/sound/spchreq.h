/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/spchreq.h,v 1.6 2000/01/31 10:02:52 adurant Exp $
#pragma once

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   spchreq.h

   A request is the thing you hand the speech system to make it play a
   sound.  It has names which match its concept and its (tag, value)
   pairs.  Before you actually use it you need to tokenize it.

   The idea is that you keep these around, wherever possible, so that
   you spend as little time tokenizing as you can.

   The Labels you pass in should already be null terminated.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#ifndef _SPCHREQ_H_
#define _SPCHREQ_H_


#include <string.h>

#include <lgassert.h>
#include <dynarray.h>

#include <label.h>
#include <tagfile.h>
#include <tagdbin.h>
#include <spchdom.h>
#include <speech.h>

#include <spcht.h>
#include <tagdbt.h>
#include <namemap.h>



// Internal helper
class cSpeechTagNamePair
{
public:
   Label m_TagName;
   Label m_ValueName;

   cSpeechTagNamePair(Label *TagName, Label *ValueName);
   cSpeechTagNamePair();
};


extern cSpeechDomain g_Domain;


const Label kSpeechRequestEndTag = {""};

class cSpeechRequest
{
public:
   cSpeechRequest(Label *pConceptName)
   {
      m_ConceptName = *pConceptName;
   }

   ~cSpeechRequest();

   void Clear();

   // We expect to be tokenized again after this is called.
   void ClearTags();

   // This, of course, invalidates any tokenization.
   void AddTag(Label *pTagName, Label *pValueName);

   void AddTagList(Label *pStartOfList);

   // This lets us put any number of requests into a single query.
   void AddToTagInput(cTagDBInput *pInput);

   // A request starts off with the names tags and tag values.  This
   // finds the corresponding indices, or returns FALSE if it can't.
   BOOL Tokenize();

   int ConceptIndex()
   {
      return m_ConceptIndex;
   }

   int ConceptPriority()
   {
      return g_Domain.FindConceptPriority(m_ConceptIndex);
   }

   const Label* ConceptName(void) const { return &m_ConceptName; }; 

   // raw names of stuff
   Label m_ConceptName;
   cDynArray<cSpeechTagNamePair> m_TagValuePair;

protected:
   // tokenized data
   int m_ConceptIndex;
   cTagDBInput m_TagDBInput;
};


#endif // ~_SPCHREQ_H_
