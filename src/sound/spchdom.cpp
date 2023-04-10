/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/spchdom.cpp,v 1.4 1998/10/05 17:28:05 mahk Exp $

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   spchdom.cpp

   A cSpeechDomain manages the names and values for our speech selection
   database.  It used to be a crude name space, but by this point we
   could easily replace it with a handful of global containers.

   The advantage of keeping it around is that we can provide specific
   validation and mono spew when we find inconsistencies in the scehema
   text files.  Besides which, it's easier than getting rid of it.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#include <spchdom.h>
#include <ctag.h>


// Must be last header 
#include <dbmem.h>


// In debug builds, cNameMap::AddName has an assertion to prevent
// duplicate entries.
int cSpeechDomain::AddConcept(Label *pName, int iPriority)
{
   int iIndex = m_Concept.AddName(pName);

   m_ConceptPriority.InsertAtIndex(iPriority, iIndex);
   return iIndex;
}


// We don't allow no duplicate tags, neither.
int cSpeechDomain::AddTag(Label *pName, int iFlags)
{
   int iIndex = m_Tag.AddName(pName);

   m_TagFlags.InsertAtIndex(iFlags, iIndex);
   return iIndex;
}


// Tag values are the one area in which we tolerate collisions.
int cSpeechDomain::AddValue(Label *pName)
{
   int iNameIndex = m_Value.IDFromName(pName);

   if (iNameIndex == kNameMapIndexNotFound)
      iNameIndex = m_Value.AddName(pName);

   return iNameIndex;
}


int cSpeechDomain::ConceptIndex(const Label *pName)
{
   int rv = m_Concept.IDFromName(pName);

#ifndef SHIP
   if (rv == kNameMapIndexNotFound)
      Warning(("cSpeechDomain::ConceptIndex: Concept not found: %s\n",
              pName->text));
#endif // ~SHIP

   return rv;
}


int cSpeechDomain::TagIndex(const Label *pName)
{
   int rv = m_Tag.IDFromName(pName);

#ifndef SHIP
   if (rv == kNameMapIndexNotFound)
      Warning(("cSpeechDomain::TagIndex: Tag not found: %s\n", pName->text));
#endif // ~SHIP

   return rv;
}


int cSpeechDomain::ValueIndex(const Label *pName)
{
   int rv = m_Value.IDFromName(pName);

#ifndef SHIP
   if (rv == kNameMapIndexNotFound)
      Warning(("cSpeechDomain::ValueIndex: Value not found: %s\n",
               pName->text));
#endif // ~SHIP

   return rv;
}


void cSpeechDomain::Load(ITagFile *pFile)
{
   int iTemp;
   int *piTemp;

   m_Concept.Load(pFile);
   m_Tag.Load(pFile);
   m_Value.Load(pFile);

   ITagFile_Read(pFile, (char *) &iTemp, sizeof(iTemp));
   m_ConceptPriority.SetSize(iTemp);
   piTemp = m_ConceptPriority.AsPointer();
   ITagFile_Read(pFile, (char *) piTemp, iTemp * sizeof(piTemp[0]));

   ITagFile_Read(pFile, (char *) &iTemp, sizeof(iTemp));
   m_TagFlags.SetSize(iTemp);
   piTemp = m_TagFlags.AsPointer();
   ITagFile_Read(pFile, (char *) piTemp, iTemp * sizeof(piTemp[0]));
}


void cSpeechDomain::Save(ITagFile *pFile)
{
   int iTemp;
   int *piTemp;

   m_Concept.Save(pFile);
   m_Tag.Save(pFile);
   m_Value.Save(pFile);

   iTemp = m_ConceptPriority.Size();
   piTemp = m_ConceptPriority.AsPointer();
   ITagFile_Write(pFile, (char *) &iTemp, sizeof(iTemp));
   ITagFile_Write(pFile, (char *) piTemp, iTemp * sizeof(piTemp[0]));

   iTemp = m_TagFlags.Size();
   piTemp = m_TagFlags.AsPointer();
   ITagFile_Write(pFile, (char *) &iTemp, sizeof(iTemp));
   ITagFile_Write(pFile, (char *) piTemp, iTemp * sizeof(piTemp[0]));
}


void cSpeechDomain::Clear()
{
   m_Concept.Clear();
   m_Tag.Clear();
   m_Value.Clear();
   m_Remap.Clear();

   m_ConceptPriority.SetSize(0);
   m_TagFlags.SetSize(0);
}

