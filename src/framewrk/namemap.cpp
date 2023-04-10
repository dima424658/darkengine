/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/namemap.cpp,v 1.4 1998/04/20 17:16:55 kate Exp $

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   namemap.h

   So this here's a weird little setup for persistent two-way mappings
   between strings and integers.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#include <string.h>

#include <namemap.h>

// must be last header
#include <dbmem.h>


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   cNameMapEntry is a dumb little helper structure.  It needs nothing
   but a few constructors.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */

cNameMapEntry::cNameMapEntry(const Label *pName)
   : m_iApparentIndex(0)        // Is there any reason for this?
{
   memcpy(m_Name.text, pName->text, 16);
}


cNameMapEntry::cNameMapEntry()
{
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   cNameMapHash is based on them generic hash tables.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */

tHashSetKey cNameMapHash::GetKey(tHashSetNode p) const
{
   return (tHashSetKey) (((cNameMapEntry *)p)->m_Name.text);
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   cNameMap is the main class.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */

cNameMap::cNameMap()
   : m_iUpperBound(0),
     m_iLowerBound(0)
{
}


cNameMap::~cNameMap()
{
   int iSize = Size();
   for (int i = 0; i < iSize; ++i)
      if (m_IntToName[i])
         delete m_IntToName[i];
}


int cNameMap::AddName(const Label *pName)
{
#ifdef DBG_ON
   AssertMsg1(!(m_NameToInt.Search(pName->text)),
              "cNameMap::AddName: name %s already in database.\n",
              pName->text);
#endif // DBG_ON

   int iNewApparent;
   if (!Size()) {
      iNewApparent = 0;
      m_IntToName.SetSize(1);
   } else {
      iNewApparent = m_iUpperBound + 1;
   }
   UpdateBounds(iNewApparent);

   cNameMapEntry *temp = new cNameMapEntry(pName);
   m_IntToName.SetItem(&temp, RealIndex(iNewApparent));
   CrossIndex(RealIndex(iNewApparent));

   return iNewApparent;
}


void cNameMap::AddNameAtIndex(const Label *pName, int iNewApparent)
{
#ifdef DBG_ON
   if(Size())
   {
      // don't allow collisions of either names or indices
      AssertMsg1(!(m_NameToInt.Search(pName->text)),
                 "cNameMap::AddNameAtIndex: name %s already in database.\n",
                 pName->text);

      AssertMsg1(iNewApparent < m_iLowerBound
              || iNewApparent > m_iUpperBound
              || m_IntToName[RealIndex(iNewApparent)] == 0,
                 "cNameMap::AddNameAtIndex: index %d already used.\n",
                 iNewApparent);
   }
#endif // DBG_ON

   if (!Size()) {
      m_iUpperBound = m_iLowerBound = iNewApparent;
      m_IntToName.SetSize(1);
   }
   UpdateBounds(iNewApparent);

   cNameMapEntry *temp = new cNameMapEntry(pName);
   m_IntToName.SetItem(&temp, RealIndex(iNewApparent));
   CrossIndex(RealIndex(iNewApparent));
}


Label *cNameMap::NameFromID(int iApparentIndex)
{
   if (iApparentIndex < m_iLowerBound
    || iApparentIndex > m_iUpperBound)
      return kNameMapNameNotFound;
   else
      return &m_IntToName[RealIndex(iApparentIndex)]->m_Name;
}


int cNameMap::IDFromName(const Label *pName)
{
   cNameMapEntry *pEntryFound;

   pEntryFound = m_NameToInt.Search(pName->text);
   if (pEntryFound == 0)
      return kNameMapIndexNotFound;
   else
      return pEntryFound->m_iApparentIndex;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   We only load and save the int->string mapping, 'cause it's an array
   so it's nice and simple-like.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */

void cNameMap::Save(ITagFile *pFile)
{
   int iSize = Size();
   char cHasLabel = '+';
   char cHasNoLabel = '-';

   ITagFile_Write(pFile, (char *) &m_iUpperBound, sizeof(m_iUpperBound));
   ITagFile_Write(pFile, (char *) &m_iLowerBound, sizeof(m_iLowerBound));
   ITagFile_Write(pFile, (char *) &iSize, sizeof(iSize));
   for (int i = 0; i < iSize; ++i) {
      if (m_IntToName[i] == 0)
         ITagFile_Write(pFile, (char *) &cHasNoLabel, 1);
      else {
         ITagFile_Write(pFile, (char *) &cHasLabel, 1);
         ITagFile_Write(pFile, (char *) &m_IntToName[i]->m_Name.text, 16);
      }
   }
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   Loading means (1) slurping our names in, then (2) hashing them to
   get our name-to-int mapping.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
void cNameMap::Load(ITagFile *pFile)
{
   int iSize, i;
   char cLabelTest;
   Label TempLabel;

   Clear();

   ITagFile_Read(pFile, (char *) &m_iUpperBound, sizeof(m_iUpperBound));
   ITagFile_Read(pFile, (char *) &m_iLowerBound, sizeof(m_iLowerBound));
   ITagFile_Read(pFile, (char *) &iSize, sizeof(iSize));
   m_IntToName.SetSize(iSize);

   for (i = 0; i < iSize; ++i) {
      ITagFile_Read(pFile, (char *) &cLabelTest, 1);
      if (cLabelTest == '+') {
         ITagFile_Read(pFile, (char *) &TempLabel, 16);
         m_IntToName[i] = new cNameMapEntry(&TempLabel);
         CrossIndex(i);
      } else
         m_IntToName[i] = 0;
   }
}


void cNameMap::Clear()
{
   // Our data is used by both the hash table and the dynamic array,
   // in both cases through a layer of indirection.  The hash table
   // automatically deletes all the entries, so we clear it first.
   m_NameToInt.DestroyAll();
   m_IntToName.SetSize(0);

   m_iLowerBound = 0;
   m_iUpperBound = 0;
}


// When a new index comes along, it may force us to rearrage our array
// and bounds.  The work is done in terms of real indices, naturally.
// If the array has grown then we clear it to, but not including, the
// new element.
void cNameMap::UpdateBounds(int iNewApparentIndex)
{
   int i;

   if (iNewApparentIndex > m_iUpperBound) {
      int iOldUpper = RealIndex(m_iUpperBound);
      int iNewUpper = RealIndex(iNewApparentIndex);

      m_iUpperBound = iNewApparentIndex;

      m_IntToName.SetSize(m_iUpperBound - m_iLowerBound + 1);
      for (i = iOldUpper + 1; i < iNewUpper; ++i)
         m_IntToName[i] = 0;
   }

   if (iNewApparentIndex < m_iLowerBound) {
      int iOldLower = RealIndex(m_iLowerBound);

      m_iLowerBound = iNewApparentIndex;

      int iOldSize = Size();
      int iNewSize = (m_iUpperBound - m_iLowerBound + 1);
      int iSizeDifference = iNewSize - iOldSize;

      m_IntToName.SetSize(iNewSize);

      // Before we clear out the new bottom elements we need to move
      // up the ones we're already storing there.
      for (i = iOldSize - 1; i >= 0; --i)
         m_IntToName[i + iSizeDifference] = m_IntToName[i];

      for (i = 1; i < iSizeDifference; ++i)
         m_IntToName[i] = 0;
   }
}


void cNameMap::CrossIndex(int iRealIndex)
{
   m_NameToInt.Insert(m_IntToName[iRealIndex]);
   m_IntToName[iRealIndex]->m_iApparentIndex = ApparentIndex(iRealIndex);
}
