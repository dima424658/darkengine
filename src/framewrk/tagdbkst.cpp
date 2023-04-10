/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/tagdbkst.cpp,v 1.6 1998/10/03 10:32:38 TOML Exp $

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   tagdbkst.cpp

   A TagDBKeySet is the set of cTagDBKeys; in the context of the tree,
   it's a search path.  To actually be used by the tree it must be
   sorted.

   If a client wants a cTagDBKeySet with persistence, it should get it
   through NewTagDBKeySetPersistent; without, through
   NewTagDBKeySetTemp.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#include <lgassert.h>
#include <dynarray.h>

#include <tagdbkey.h>
#include <tagdbkst.h>

// must be last header
#include <dbmem.h>


class cTagDBKeySet : public ITagDBKeySet
{
public:
   cTagDBKeySet();
   ~cTagDBKeySet();

   void Clear();
   int Size();
   void AddKey(cTagDBKey *pKey);
   BOOL HasKey(cTagDBKey *pKey);
   void Sort();
   BOOL IsSorted();

   tTagDBKeySetIter IterStart();
   BOOL IterNext(tTagDBKeySetIter *Iter, cTagDBKey *pKey);

protected:
   // See constants below.
   int m_iFlags;

   cDynClassArray_<cTagDBKey, 2> m_Storage;

   // This just compares adjacent elements to make sure everything's
   // in order.  @TODO: Should this exist in opt builds?
   BOOL VerifySorted();
};


// Flags are all 0 on creation.
#define kTagDBKeySetFlagSorted 1


cTagDBKeySet::cTagDBKeySet()
   : m_iFlags(0)
{
}


BOOL cTagDBKeySet::IsSorted()
{
#ifdef DBG_ON
   if (m_iFlags & kTagDBKeySetFlagSorted)
      AssertMsg(VerifySorted(), 
                "TagDBKeySet sort flag set, but it ain\'t so.\n");
#endif // ~DBG_ON

   if (m_iFlags & kTagDBKeySetFlagSorted)
      return TRUE;
   else
      return FALSE;
}


tTagDBKeySetIter cTagDBKeySet::IterStart()
{
   return 0;
}


BOOL cTagDBKeySet::IterNext(tTagDBKeySetIter *Iter, cTagDBKey *pKey)
{
   if (*Iter == m_Storage.Size())
      return FALSE;

   *pKey = m_Storage[*Iter];
   *Iter = *Iter + 1;   

   return TRUE;
}


cTagDBKeySet::~cTagDBKeySet()
{
}


void cTagDBKeySet::Clear()
{
    m_Storage.SetSize(0);
}


int cTagDBKeySet::Size()
{
   return m_Storage.Size();
}


// This does not maintain the sort order, so it invalidates the
// internal flag exposed by IsSorted().
void cTagDBKeySet::AddKey(cTagDBKey *pKey)
{
   m_Storage.Append(*pKey);
}


// Weird, but functional.  But weird.
static int Compare(const void *I1, const cTagDBKey *I2)
{
   return (int)((cTagDBKey *)I1)->m_KeyType - (int)I2->m_KeyType;
}


// We can search faster if our data is sorted.
BOOL cTagDBKeySet::HasKey(cTagDBKey *pKey)
{
   int iIndex;

   if (IsSorted())
      iIndex = m_Storage.BSearch(pKey, Compare);
   else
      iIndex = m_Storage.LSearch(pKey, Compare);

   if (iIndex == BAD_INDEX)
      return FALSE;
   else
      return TRUE;
}


// We expect such small data sets that a selection sort is actually
// the right thing.
void cTagDBKeySet::Sort()
{
   if (IsSorted())
      return;

   int iInnerLimit = m_Storage.Size();
   int iOuterLimit = iInnerLimit - 1;
   cTagDBKey TempKey;
   int i, j, iBestIndex;

   for (i = 0; i < iOuterLimit; ++i) {
      iBestIndex = i;

      for (j = i + 1; j < iInnerLimit; ++j)
         if (m_Storage[iBestIndex] > m_Storage[j])
            iBestIndex = j;

      TempKey = m_Storage[i];
      m_Storage[i] = m_Storage[iBestIndex];
      m_Storage[iBestIndex] = TempKey;
   }
}


BOOL cTagDBKeySet::VerifySorted()
{
   int iLimit = m_Storage.Size() - 1;

   for (int i = 0; i < iLimit; ++i)
      if (m_Storage[i] > m_Storage[i + i])
         return FALSE;

   return TRUE;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   Curiously, nobody wants an instance of an ABC...

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
ITagDBKeySet *NewITagDBKeySet()
{
   return new cTagDBKeySet();
}
