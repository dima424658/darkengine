/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/spchreq.cpp,v 1.3 1998/10/05 17:28:07 mahk Exp $

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   spchreq.cpp

   A request is the thing you hand the speech system to make it play a
   sound.  It is specific to a given domain, and has names which match
   its concept and its (tag, value) pairs.  Before you actually use it
   you need to have spch tokenize it, looking up the indices or
   whatever based on these names.

   The idea is that you keep these around, wherever possible, so that
   you spend as little time tokenizing as possible.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#include <spchreq.h>


// Must be last header 
#include <dbmem.h>


extern cSpeechDomain g_Domain;


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   All cSpeechTagNamePair needs is a couple of constructors.  We assume
   that the labels passed in are already null terminated.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */

cSpeechTagNamePair::cSpeechTagNamePair(Label *pTagName, Label *pValueName)
{
   memcpy(m_TagName.text, pTagName, 16);
   memcpy(m_ValueName.text, pValueName, 16);
}


cSpeechTagNamePair::cSpeechTagNamePair()
{
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   The main event: heeeere's our cSpeechRequest methods.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */

cSpeechRequest::~cSpeechRequest()
{
}


void cSpeechRequest::Clear()
{
   m_TagValuePair.SetSize(0);
   m_TagDBInput.SetSize(0);
}


void cSpeechRequest::ClearTags()
{
   m_TagValuePair.SetSize(0);
}


void cSpeechRequest::AddTag(Label *pTagName, Label *pValueName)
{
   const cSpeechTagNamePair Pair(pTagName, pValueName);
   m_TagValuePair.Append(Pair);

   // invalidate tokenized data
   m_TagDBInput.SetSize(0);
}


void cSpeechRequest::AddTagList(Label *pStartOfList)
{
   while (pStartOfList->text[0] != 0) {
      AddTag(pStartOfList, pStartOfList + 1);
      ++pStartOfList;
   }
}


void cSpeechRequest::AddToTagInput(cTagDBInput *pInput)
{
   int iSize = m_TagDBInput.Size();

   for (int i = 0; i < iSize; ++i)
      pInput->Append(m_TagDBInput[i]);
}


// Our return value indicates whether tokenization was possible.
// (The domain issues monospew if any token wasn't found).
BOOL cSpeechRequest::Tokenize()
{
   // find our concept
   m_ConceptIndex = g_Domain.ConceptIndex(&m_ConceptName);
   if (m_ConceptIndex == kNameMapIndexNotFound) {
      return FALSE;
   }

   int iSize = m_TagValuePair.Size();
   m_TagDBInput.SetSize(iSize);

   for (int i = 0; i < iSize; ++i) {
      tTagDBKeyType TagIndex
         = g_Domain.TagIndex(&m_TagValuePair[i].m_TagName);
      if (m_ConceptIndex == kNameMapIndexNotFound) {
         return FALSE;
      }

      tTagDBKeyValue ValueIndex
         = g_Domain.TagValueIndex(TagIndex, &m_TagValuePair[i].m_ValueName);

      cTagDBInputPair NewPair(TagIndex, ValueIndex);
      m_TagDBInput.SetItem(&NewPair, i);
   }

   return TRUE;
}
