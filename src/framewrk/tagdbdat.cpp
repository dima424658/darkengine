/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/tagdbdat.cpp,v 1.14 1998/10/03 10:32:27 TOML Exp $

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   tagdbdat.cpp

   The cTagDBDatabase class manages a collection of mappings from
   sets of cTagDBKeys to pairs of integers and floats.

   Each cTagDBDatabase has a set of sTagDBData, and a set of keys.
   For each key there is a child cTagDBDatabase.

   Since the root of the tree is a cTagDBDatabase, and it can have
   data, we can talk about and match against empty key sets; if we are
   matching against all subsets of a given key set, then the ones for
   an empty key set will always be part of the output.  If we are
   matching against an exact set of keys then we will only get the
   data for the empty key set if the query has no keys (duh).

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#include <namemap.h>
#include <tagdbkey.h>
#include <tagdbkst.h>
#include <tagdbdat.h>
#include <tagdbin.h>
#include <tagdbout.h>
#include <mprintf.h>

// must be last header
#include <dbmem.h>


// forward declaration
class cTagDBDatabase;

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   cBranch is a helper class for cTagDBDatabase.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
class cBranch
{
public:
   cBranch()
      : m_Key(),
        m_Child(0)
   {
   }

   cBranch(cTagDBKey Key)
      : m_Key(Key),
        m_Child(0)
   {
   }

   ~cBranch()
   {
      Clear();
   }

   const cBranch & operator=(const cBranch & b)
   {
      m_Key = b.m_Key;
      m_Child = b.m_Child;
      return *this;
   }

   void Load(ITagFile *pFile);
   void Save(ITagFile *pFile);
   void Clear();

   cTagDBKey m_Key;
   cTagDBDatabase *m_Child;
};


class cTagDBDatabase : public ITagDBDatabase
{
public:
   cTagDBDatabase();
   ~cTagDBDatabase();

   void Insert(ITagDBKeySet *pSet, const sTagDBData Data);

   BOOL MatchSubset(cTagDBInput *pInput, cTagDBOutput *pOutput);
   BOOL MatchSet(cTagDBInput *pInput, cTagDBOutput *pOutput);
   BOOL MatchPartialSet(cTagDBInput *Input, cTagDBOutput *Output);

   void TransformData(void Func(sTagDBData *));
   void TransformKeys(void Func(cTagDBKey *));
   ITagDBDatabase *CopyWithFilter(void Func(cTagDBKey *));

   void Load(ITagFile *pFile);
   void Save(ITagFile *pFile);
   void Clear();

#ifndef SHIP
   void Dump();
#endif // ~SHIP

protected:
   // Here are all the nodes below this one and the keys by which we
   // reach them.
   cDynClassArray_<cBranch, 1> m_Branch;

   // This is the data we reach if we search down as far as this node.
   cDynArray_<sTagDBData, 1> m_Data;

   // If m_Branch has this key, this gives us its index.  If not, this
   // gives us the index where it would go.
   int KeyInsertionIndex(cTagDBKey *pKey, int iMinIndex);

   BOOL CopyDataToOutput(cTagDBOutput *pOutput, float fWeight);

   // This is the heart of data insertion.  As such, it's an internal
   // organ.
   void InsertRecursively(ITagDBKeySet *pSet,
                          const sTagDBData Data,
                          tTagDBKeySetIter *pIter);

   BOOL MatchSubsetRecursively(cTagDBInput *pInput,
                               cTagDBOutput *pOutput,
                               int iInputIndex,
                               float fWeight);

   BOOL MatchSetRecursively(cTagDBInput *pInput,
                            cTagDBOutput *pOutput,
                            int iInputIndex,
                            float fWeight);

   BOOL MatchPartialSetRecursively(cTagDBInput *pInput,
                                   cTagDBOutput *pOutput,
                                   int iInputIndex,
                                   float fWeight);

   void CopyWithFilterRecursively(cTagDBDatabase *pNewDB,
                                  void Func(cTagDBKey *),
                                  cDynArray<cTagDBKey> *pKeys,
                                  int iDepth);

#ifndef SHIP
   void DumpRecursively(int iLevel);
#endif // ~SHIP
};


void cBranch::Load(ITagFile *pFile)
{
   ITagFile_Read(pFile, (char *) &m_Key, sizeof(m_Key));
   m_Child = new cTagDBDatabase();
   m_Child->Load(pFile);
}


void cBranch::Save(ITagFile *pFile)
{
   ITagFile_Write(pFile, (char *) &m_Key, sizeof(m_Key));
   m_Child->Save(pFile);
}


void cBranch::Clear()
{
   delete m_Child;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   From here on in it's methods for cTagDBDatabase, plus the factory
   function to give the outside world ITagDBDatabases.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
cTagDBDatabase::cTagDBDatabase()
{
}


cTagDBDatabase::~cTagDBDatabase()
{
   Clear();
}


// @TODO: turn this into a binary search?
int cTagDBDatabase::KeyInsertionIndex(cTagDBKey *pKey, int iMinIndex)
{
   int iBranchSize = m_Branch.Size();

#ifdef DBG_ON
   AssertMsg2((iMinIndex >= 0) && (iMinIndex <= iBranchSize),
             "In tag database, limit out of bounds: (%d out of %d)\n",
             iMinIndex, iBranchSize);
#endif // DBG_ON

   if (iBranchSize == 0)
      return 0;

   int i;

   for (i = iMinIndex; i < iBranchSize; ++i)
      if (m_Branch[i].m_Key > *pKey)
         break;

   if ((i > 0 ) && (m_Branch[i - 1].m_Key == *pKey))
      return i - 1;
   else
      return i;
}


BOOL cTagDBDatabase::CopyDataToOutput(cTagDBOutput *pOutput, float fWeight)
{
   int iSize = m_Data.Size();

   if (iSize) {
      for (int i = 0; i < iSize; ++i)
         pOutput->Append(sTagDBData(m_Data[i].m_iData,
                                    m_Data[i].m_fWeight + fWeight));
      return TRUE;
   } else
      return FALSE;
}


// Insertion is a matter of filtering data down into the database.
// The main work is done in the protected InsertRecursively method.
void cTagDBDatabase::Insert(ITagDBKeySet *pSet, const sTagDBData Data)
{
   pSet->Sort();
   tTagDBKeySetIter Iter = pSet->IterStart();
   InsertRecursively(pSet, Data, &Iter);
}

// The data filters down the tree until it reaches its level.
void cTagDBDatabase::InsertRecursively(ITagDBKeySet *pSet,
                                       const sTagDBData Data,
                                       tTagDBKeySetIter *pIter)
{
   cTagDBKey Key;

   // Is there anything left in this key set?  If not then we must be
   // where this data goes.
   if (pSet->IterNext(pIter, &Key)) {

      // This gives us the index into m_Branch where our data wants to
      // go, without telling us whether we already have a node for it.
      int iKeyIndex = KeyInsertionIndex(&Key, 0);

      // If we don't have an exact match for our next key we have to
      // build the appropriate node.
      if ((m_Branch.Size() <= iKeyIndex)
       || !(m_Branch[iKeyIndex].m_Key == Key)) {
         cBranch NewBranch(Key);
         m_Branch.InsertAtIndex(NewBranch, iKeyIndex);
         m_Branch[iKeyIndex].m_Child = new cTagDBDatabase();
      }

      // Now we propagate down the search tree.  The tTAgDBKeySetIter
      // was updated when we called IterNext.
      m_Branch[iKeyIndex].m_Child->InsertRecursively(pSet, Data, pIter);
   } else
      m_Data.Append(Data);
}


// We expect that each type of key will only be represented once in
// the input set; that is, will will never be both green and blue, or
// both ten feet above and thirty feet below.  (Actually, it may work
// right now with duplicate types, but hasn't been tested that way.)
BOOL cTagDBDatabase::MatchSubset(cTagDBInput *pInput,
                                 cTagDBOutput *pOutput)
{
   pInput->Sort();
   pOutput->Clear();

   return MatchSubsetRecursively(pInput, pOutput, 0, 0.0);
}


BOOL cTagDBDatabase::MatchSubsetRecursively(cTagDBInput *pInput,
                                            cTagDBOutput *pOutput,
                                            int iInputIndex,
                                            float fWeight)
{
   BOOL rv = CopyDataToOutput(pOutput, fWeight);

   int iInputSize = pInput->Size();
   if (iInputIndex == iInputSize)
      return rv;

   int iInputTraverse = iInputIndex;
   int iBranchSize = m_Branch.Size();
   int iBranchTraverse = 0;

   while ((iInputTraverse < iInputSize)
       && (iBranchTraverse < iBranchSize)) {

      cTagDBInputTag *InputTag = pInput->AsPointer() + iInputTraverse;
      tTagDBKeyType InputKeyType = InputTag->m_KeyType;

      if (InputKeyType < m_Branch[iBranchTraverse].m_Key.m_KeyType) {
         ++iInputTraverse;
         continue;
      }

      if (m_Branch[iBranchTraverse].m_Key.m_KeyType < InputKeyType) {
         ++iBranchTraverse;
         continue;
      }

      // The types match, so we check our input value against the
      // range specified in the database.  We keep doing this as long
      // as we're finding keys in the database of the same type as the
      // input.  Unless we run out of database, of course.
      while ((iBranchTraverse < iBranchSize)
          && (m_Branch[iBranchTraverse].m_Key.m_KeyType == InputKeyType)) {

         if (m_Branch[iBranchTraverse].m_Key.Match(InputTag))
            rv |= m_Branch[iBranchTraverse].m_Child
               ->MatchSubsetRecursively(pInput, pOutput,
                                iInputTraverse + 1,
                                fWeight + (*pInput)[iInputTraverse].m_fWeight);

         ++iBranchTraverse;
      }
   }

   return rv;
}


// To match against an exact set of tags we have to recursively
// explore all the ranged matches.
BOOL cTagDBDatabase::MatchSet(cTagDBInput *pInput,
                              cTagDBOutput *pOutput)
{
   pInput->Sort();
   pOutput->Clear();

   return MatchSetRecursively(pInput, pOutput, 0, 0.0);
}


// We return TRUE if we reach at least one terminal node.
BOOL cTagDBDatabase::MatchSetRecursively(cTagDBInput *pInput,
                                         cTagDBOutput *pOutput,
                                         int iInputIndex,
                                         float fWeight)
{
   if (iInputIndex == pInput->Size())
      return CopyDataToOutput(pOutput, fWeight);

   BOOL rv = FALSE;
   cTagDBInputTag *InputTag = pInput->AsPointer() + iInputIndex;
   tTagDBKeyType KeyType = (*pInput)[iInputIndex].m_KeyType;

   // Any number of keys may match our input, but they will all have
   // the same key type, and they will be contiguous in m_Branch since
   // it's sorted.  So first we skip over all those which have lower
   // key types.
   int iBranchSize = m_Branch.Size();
   int iBranchTraverse;

   for (iBranchTraverse = 0; iBranchTraverse < iBranchSize; ++iBranchTraverse)
      if (m_Branch[iBranchTraverse].m_Key.m_KeyType == KeyType)
         goto found_type;

   return FALSE;

found_type:
   while ((m_Branch[iBranchTraverse].m_Key.m_KeyType == KeyType)
       && (iBranchTraverse < iBranchSize)) {

      if (m_Branch[iBranchTraverse].m_Key.Match(InputTag))
         rv |= m_Branch[iBranchTraverse].m_Child
             ->MatchSetRecursively(pInput, pOutput, iInputIndex + 1,
                                fWeight + (*pInput)[iInputIndex].m_fWeight);

      ++iBranchTraverse;
   }

   return rv;
}


BOOL cTagDBDatabase::MatchPartialSet(cTagDBInput *pInput,
                                     cTagDBOutput *pOutput)
{
   pInput->Sort();
   pOutput->Clear();

   return MatchPartialSetRecursively(pInput, pOutput, 0, 0.0);
}


BOOL cTagDBDatabase::MatchPartialSetRecursively(cTagDBInput *pInput,
                                                cTagDBOutput *pOutput,
                                                int iInputIndex,
                                                float fWeight)
{
   BOOL rv;

   if (pInput->FoundAllRequired(iInputIndex))
      rv = CopyDataToOutput(pOutput, fWeight);
   else
      rv = FALSE;

   int iInputSize = pInput->Size();
   if (iInputIndex == iInputSize)
      return rv;

   int iInputTraverse = iInputIndex;
   int iBranchSize = m_Branch.Size();
   int iBranchTraverse = 0;

   while ((iInputTraverse < iInputSize)
       && (iBranchTraverse < iBranchSize)) {

      cTagDBInputTag *InputTag = pInput->AsPointer() + iInputTraverse;
      tTagDBKeyType InputKeyType = (*pInput)[iInputTraverse].m_KeyType;

      if (InputKeyType < m_Branch[iBranchTraverse].m_Key.m_KeyType) {
         // Have we failed to find a required member of the input set?
         if ((*pInput)[iInputTraverse].m_iFlags & kTagDBInputFlagRequired)
            return rv;

         ++iInputTraverse;
         continue;
      }

      if (m_Branch[iBranchTraverse].m_Key.m_KeyType < InputKeyType) {
         ++iBranchTraverse;
         continue;
      }

      // The types match, so we check our input value against the
      // range specified in the database.  We keep doing this as long
      // as we're finding keys in the database of the same type as the
      // input.  Unless we run out of database, of course.
      while ((iBranchTraverse < iBranchSize)
          && (m_Branch[iBranchTraverse].m_Key.m_KeyType == InputKeyType)) {

         if (m_Branch[iBranchTraverse].m_Key.Match(InputTag))
            rv |= m_Branch[iBranchTraverse].m_Child
               ->MatchPartialSetRecursively(pInput, pOutput,
                                iInputTraverse + 1,
                                fWeight + (*pInput)[iInputTraverse].m_fWeight);

         ++iBranchTraverse;
      }
   }

   return rv;
}


void cTagDBDatabase::TransformData(void Func(sTagDBData *))
{
   int i, iSize;

   iSize = m_Branch.Size();
   for (i = 0; i < iSize; ++i)
      m_Branch[i].m_Child->TransformData(Func);

   iSize = m_Data.Size();
   for (i = 0; i < iSize; ++i)
      Func(&m_Data[i]);
}


void cTagDBDatabase::TransformKeys(void Func(cTagDBKey *))
{
   int i, iSize;

   iSize = m_Branch.Size();
   for (i = 0; i < iSize; ++i) {
      Func(&m_Branch[i].m_Key);
      m_Branch[i].m_Child->TransformKeys(Func);
   }
}


ITagDBDatabase *cTagDBDatabase::CopyWithFilter(void Func(cTagDBKey *))
{
   cTagDBDatabase *pNewDB = new cTagDBDatabase();
   cDynArray<cTagDBKey> Keys;

   CopyWithFilterRecursively(pNewDB, Func, &Keys, 0);
   return pNewDB;
}


// What we're doing here is developing the path for each piece of data
// in our tree, transforming the keys into some other mapping, and
// entering them into a new tree.  We do this because each tree relies
// on a canonical ordering for its searches.  So we can't just
// transform the keys where they are; they could be out of order once
// they've been filtered.
void cTagDBDatabase::CopyWithFilterRecursively(cTagDBDatabase *pNewDB,
                                               void Func(cTagDBKey *),
                                               cDynArray<cTagDBKey> *pKeys, 
                                               int iDepth)
{
   ITagDBKeySet *const pKeySet = NewITagDBKeySet();
   int i, iSize;

   for (i = 0; i < iDepth; ++i) {
      cTagDBKey NewKey = (*pKeys)[i];
      pKeySet->AddKey(&NewKey);
   }

   iSize = m_Data.Size();
   for (i = 0; i < iSize; ++i)
      pNewDB->Insert(pKeySet, m_Data[i]);

   delete pKeySet;

   iSize = m_Branch.Size();
   for (i = 0; i < iSize; ++i) {
      cTagDBKey NewKey = m_Branch[i].m_Key;
      Func(&NewKey);
      pKeys->SetSize(iDepth + 1);
      (*pKeys)[iDepth] = NewKey;
      m_Branch[i].m_Child->CopyWithFilterRecursively(pNewDB, Func,
                                                     pKeys, iDepth + 1);
   }
}


void cTagDBDatabase::Load(ITagFile *pFile)
{
   int iSize;

   ITagFile_Read(pFile, (char *) &iSize, sizeof(iSize));
   m_Data.SetSize(iSize);
#ifdef DBG_ON
   Assert_(iSize < 10000);
#endif // DBG_ON
   sTagDBData *pData = m_Data.AsPointer();
   ITagFile_Read(pFile, (char *) pData, sizeof(sTagDBData) * iSize);

   ITagFile_Read(pFile, (char *) &iSize, sizeof(iSize));
#ifdef DBG_ON
   Assert_(iSize < 10000);
#endif // DBG_ON
   m_Branch.SetSize(iSize);
   for (int i = 0; i < iSize; ++i)
      m_Branch[i].Load(pFile);
}


void cTagDBDatabase::Save(ITagFile *pFile)
{
   int iSize = m_Data.Size();
   sTagDBData *pData = m_Data.AsPointer();
   ITagFile_Write(pFile, (char *) &iSize, sizeof(iSize));
   ITagFile_Write(pFile, (char *) pData, sizeof(sTagDBData) * iSize);

   iSize = m_Branch.Size();
   ITagFile_Write(pFile, (char *) &iSize, sizeof(iSize));
   for (int i = 0; i < iSize; ++i)
      m_Branch[i].Save(pFile);
}


void cTagDBDatabase::Clear()
{
   // Since m_Branch is a cDynClassArray, this will destroy our tree.
   m_Branch.SetSize(0);
   m_Data.SetSize(0);
}


#ifndef SHIP

// mono spew with a really awful format
void cTagDBDatabase::Dump()
{
   DumpRecursively(0);
}


void cTagDBDatabase::DumpRecursively(int iLevel)
{
   int i, j;

   for (i = 0; i < iLevel; ++i)
      mprintf(".  ");

   mprintf("Data: ");

   // dump data for this node
   for (i = 0; i < m_Data.Size(); ++i)
      mprintf("%d (@%g) ", m_Data[i].m_iData, m_Data[i].m_fWeight);

   mprintf("\n");

   for (i = 0; i < m_Branch.Size(); ++i) {
      for (j = 0; j < iLevel; ++j)
         mprintf(".  ");

      // for now we assume that all keys are integer, not enum
      mprintf("Key: %d (%d - %d) [%d %d %d %d %d %d %d %d]\n",
              m_Branch[i].m_Key.m_KeyType,
              m_Branch[i].m_Key.m_KeyMin, m_Branch[i].m_Key.m_KeyMax,
              m_Branch[i].m_Key.m_aEnum[0], m_Branch[i].m_Key.m_aEnum[1],
              m_Branch[i].m_Key.m_aEnum[2], m_Branch[i].m_Key.m_aEnum[3],
              m_Branch[i].m_Key.m_aEnum[4], m_Branch[i].m_Key.m_aEnum[5],
              m_Branch[i].m_Key.m_aEnum[6], m_Branch[i].m_Key.m_aEnum[7]);

      m_Branch[i].m_Child->DumpRecursively(iLevel + 1);
   }
}

#endif // ~SHIP


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   Our factory function actually gives you a cTagDBDatabase.
   Don't tell!

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
ITagDBDatabase *NewITagDBDatabase()
{
   return new cTagDBDatabase();
}
