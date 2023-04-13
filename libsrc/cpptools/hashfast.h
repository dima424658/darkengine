/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: x:/prj/tech/libsrc/cpptools/RCS/hashfast.h 1.7 1999/09/28 18:49:10 JAEMZ Exp $

#ifndef __HASHFAST_H
#define __HASHFAST_H

// For the memset
#include <string.h>

/*
  This hashing function is about 5.5 times faster than the cHashSet
  hash for searches and uses less memory but has several restrictions.
  1) Your structure needs to have an pointer entry called pNext, and an 
  entry called key.  You can inherit your structure from sHashFastEntry, 
  if you'd like.

  2) You need to manage your own memory, since your entry includes all 
  the hashing things.

  3) The hashing function is just a bit mask, so you need an even key 
  distribution for this to work well.

  4) For size, you need to pass in a power of 2.
  
  5) To get optimal efficiency I recommend making it a global, since then the
  address of the pointer list is a constant.
  

  Here's an example of usage:

  #include <hashfast.h>


struct sObject
{
   sObject *pNext;
   ObjID id;
   int num_entries;
   int pad[16];

   // Just return the id...
   inline ObjID GetKey() {return id;}
};

static cHashFast<512,ObjID,sObject> gHashFast;

void foo()
{
   sObject *pObj;

   pObj = new sObject;
   pObj->id = 32;
   gHashFast.Insert(pObj);

   pObj = gHashFast.Search(32);

   pObj = gHashFast.Remove(32);
   delete pObj;
}
  
*/

// You can inherit from this structure if you'd like to make
// your entry.  Really the only thing your structure needs is
// for the first entry to be a pointer called pNext, and you need
// a function called GetKey to return the key.
struct sHashFastEntry {
   sHashFastEntry *pNext;   // the next entry in the hash table
};

// Iteration structure for a hashfast.
// Pass into IterStart to start an iteration
// Then call IterNext to get next one, returns FALSE when done
// 
struct sHashFastIter
{
   int row;  // current row
   void *pNext; // pointer to next to use for next time
};

// The class is the hash table
template<int SIZE,class KEY,class NODE> 
class cHashFast {
private:
   inline int Hash(KEY key) {return key&(SIZE-1);}
protected:   
   // has an array the right size
   // This needs to be cleared on init
   NODE *m_pEntries[SIZE];
public:
   // Constructor, just clears the array so missed searches don't barf
   cHashFast() {memset(m_pEntries,0,sizeof(m_pEntries));}

   // Resets the whole hash table to being empty.  Does not
   // delete all the entries or anything like that though.
   void Reset() {memset(m_pEntries,0,sizeof(m_pEntries));}

   // Resets the thing and DOES delete all the members
   // so beware...
   void Destroy() 
   {
      NODE *pCur,*pNext;
      int i;
      for (i=0;i<SIZE;++i) {
         pCur = m_pEntries[i];
         while (pCur) {
            pNext = pCur->pNext;
            delete pCur;
            pCur = pNext;
         }
      }
      Reset();
   }

   // This inserts an entry into the table
   // no checking is done to see if the same
   // key already exists
   void Insert(NODE *pEntry)
   {
      int index = Hash(pEntry->GetKey());

      pEntry->pNext = m_pEntries[index];
      m_pEntries[index] = pEntry;
   }

   // Search for an entry, returning it if there
   // Finds the first matching key in the table,
   // returns NULL if not there
   NODE *Search(KEY key) 
   {
      NODE *pCur;

      // look in the hash table
      pCur = m_pEntries[Hash(key)];

      while (pCur != NULL) {
         if (pCur->GetKey() == key)
            return pCur;
         pCur = (NODE *)pCur->pNext;
      }
      return NULL;
   }

   // Removes the entry from the table passing it back.
   // Note that it does NOT delete the entry or free the memory 
   // the user is in charge of that.  Returns NULL if key not found
   NODE *Remove(KEY key)
   {
      NODE *pLast;
      NODE *pCur;

      // Pretend the table entry itself is the last element
      pLast = (NODE *)&(m_pEntries[Hash(key)]);
      pCur = (NODE *)pLast->pNext;

      // Find and relink around the old one
      while (pCur != NULL) {
         if (pCur->GetKey() == key) {
            pLast->pNext = pCur->pNext;
            return pCur;
         }
         pLast=pCur;
         pCur=(NODE *)pCur->pNext;
      }

      return NULL;
   }

   // Iterate over all the objects passing in a function and some data.
   // If the function returns TRUE, the iterator ends.
   // Returns the NODE* it was on if exited early, else returns NULL
   typedef bool (* fHashFastCback)(NODE *a,void *b);

   NODE *Iterate(fHashFastCback pFunc,void *data) 
   {
      NODE *pCur,*pNext;
      int i;

      for (i=0;i<SIZE;++i) {
         pCur = m_pEntries[i];
         while (pCur) {
            pNext = pCur->pNext;
            if (pFunc(pCur,data)) return pCur;
            pCur = pNext;
         }
      }
      return NULL;
   }

   // Non callback iteration.  Not as irritating
   // Call this to start the iterator, then keep passing
   // it back in to get the next one
   void IterStart(sHashFastIter *pIter) 
   {
      pIter->row = 0;
      pIter->pNext = (void *)m_pEntries[pIter->row];
   }

   // Call this to get the next NODE*, returns FALSE
   // when done.  NOT Safe to remove or insert anyone
   // except remove the current one while doing it
   // could add someday if you really need it
   bool IterNext(sHashFastIter *pIter,NODE **ppCur) 
   {
      // If next is null and we're still in range
      NODE *pCur = (NODE*)pIter->pNext;
      while (!pCur && pIter->row<SIZE-1) {
         pIter->row++;
         pCur = m_pEntries[pIter->row];
      }
      // Failure!
      if (!pCur) return FALSE;

      // Success!
      *ppCur = pCur;
      pIter->pNext = (void*)pCur->pNext;
      return TRUE;
   }
};

#endif