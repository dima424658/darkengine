/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/spchvoc.cpp,v 1.5 1998/10/05 17:28:09 mahk Exp $

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   spchvoc.cpp

   A cSpeechVoice depends on the global domain to be constant--in
   particular, in its number of concepts--over the life of the
   cSpeechVoice.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#include <limits.h>

#include <tagdbdat.h>
#include <tagdbkst.h>
#include <tagdbout.h>
#include <spchdom.h>

#include <spchvoc.h>


// Must be last header 
#include <dbmem.h>


extern cSpeechDomain g_Domain;


// We depend on the appropriate domain to already have been loaded, so
// that we know how many concepts we've got.
cSpeechVoice::cSpeechVoice()
{
   m_pCurrentAddition = NewITagDBKeySet();
   int iNumConcepts = g_Domain.NumConcepts();
   m_ConceptDatabase.SetSize(iNumConcepts);

   for (int i = 0; i < iNumConcepts; ++i)
      m_ConceptDatabase[i] = NewITagDBDatabase();
}


cSpeechVoice::~cSpeechVoice()
{
   int iSize = m_ConceptDatabase.Size();
   for (int i = 0; i < iSize; ++i)
      delete m_ConceptDatabase[i];

   delete m_pCurrentAddition;
}


void cSpeechVoice::AddDataStart(sTagDBData Data, const Label *pConceptName)
{
   m_iCurrentConcept = g_Domain.ConceptIndex(pConceptName);
   m_CurrentData = Data;
   m_pCurrentAddition->Clear();
}


void cSpeechVoice::AddDataAddTag(const Label *pTagName, int iMin, int iMax)
{
   tTagDBKeyType iTagIndex = g_Domain.TagIndex(pTagName);
   cTagDBKey NewKey(iTagIndex, iMin, iMax);

   m_pCurrentAddition->AddKey(&NewKey);
}


// pEnum is expected to be an array of eight const labels.
void cSpeechVoice::AddDataAddTag(const Label *pTagName, const Label *pEnum)
{
   tTagDBKeyEnum aEnum[8];
   tTagDBKeyType iTagIndex = g_Domain.TagIndex(pTagName);

   for (int i = 0; i < 8; ++pEnum, ++i)
      if (!pEnum->text[0])
         aEnum[i] = kTagDBKeyEnumUnused;
      else
         aEnum[i] = g_Domain.ValueIndex(pEnum);

   cTagDBKey NewKey(iTagIndex, aEnum);
   m_pCurrentAddition->AddKey(&NewKey);
}


void cSpeechVoice::AddDataFinish()
{
   if (m_iCurrentConcept == kNameMapIndexNotFound)
      return;

   ITagDBDatabase *pCurrent = m_ConceptDatabase[m_iCurrentConcept];

   pCurrent->Insert(m_pCurrentAddition, m_CurrentData);
}


BOOL cSpeechVoice::SelectSchemas(cTagDBInput *pInput, int iConceptIndex,
                                 cTagDBOutput *pOutput)
{
   m_ConceptDatabase[iConceptIndex]->MatchSubset(pInput, pOutput);

   if (pOutput->Size() > 0)
      return TRUE;
   else
      return FALSE;
}


void cSpeechVoice::TransformData(void Func(sTagDBData *))
{
   int iNumConcepts = g_Domain.NumConcepts();

   for (int i = 0; i < iNumConcepts; ++i)
      m_ConceptDatabase[i]->TransformData(Func);
}


void cSpeechVoice::TransformKeys(void Func(cTagDBKey *))
{
   int iNumConcepts = g_Domain.NumConcepts();

   for (int i = 0; i < iNumConcepts; ++i) {
      ITagDBDatabase *pTemp = m_ConceptDatabase[i]->CopyWithFilter(Func);
      delete m_ConceptDatabase[i];
      m_ConceptDatabase[i] = pTemp;
   }
}


void cSpeechVoice::Load(ITagFile *pFile)
{
   for (int i = 0; i < g_Domain.NumConcepts(); ++i)
      m_ConceptDatabase[i]->Load(pFile);
}


void cSpeechVoice::Save(ITagFile *pFile)
{
   for (int i = 0; i < g_Domain.NumConcepts(); ++i)
      m_ConceptDatabase[i]->Save(pFile);
}


void cSpeechVoice::Clear()
{
   for (int i = 0; i < m_ConceptDatabase.Size(); ++i)
      delete m_ConceptDatabase[i];
}

