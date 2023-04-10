// $Header: x:/prj/tech/libsrc/cpptools/RCS/hshpptem.h 1.8 1998/08/24 20:55:54 mahk Exp $

#ifndef HSHPPTEM_H
#define HSHPPTEM_H
#include <lg.h>
#include <stdlib.h>
#include <string.h>
#include <_dstruct.h>

#ifndef SHIP
#include <mprintf.h>
#endif // !SHIP

#ifndef NO_DB_MEM
// Must be last header
#include <memall.h>
#include <dbmem.h>
#endif

////////////////////////////////////////////////////////////
// TEMPLATE INSTANTIATION FOR cHashTable
//

#define HASH_EMPTY     0
#define HASH_TOMBSTONE 1
#define HASH_FULL      2

#define INDEX_NOT_FOUND -1

#define FULLNESS_THRESHHOLD_PERCENT 80

#define HASHTABLE_TEMPLATE  template<class KEY, class VALUE, class FUNC>
#define HASHTABLE cHashTable<KEY,VALUE,FUNC>



////////////////////////////////////////////////////////////
// HASH TABLE MEMBER FUNCTIONS
//

HASHTABLE_TEMPLATE void HASHTABLE::Init()
{
   sizelog2 = hashlog2(size);
   statvec = new ubyte[size];
   Assrt(statvec != NULL);
   for (int i = 0; i < size; i++) statvec[i] = HASH_EMPTY;
   vec = new Elem[size];
   Assrt(vec != NULL);
}


HASHTABLE_TEMPLATE  HASHTABLE::cHashTable(int vecsize)
   : fullness(0),tombstones(0)
{
   while(!is_prime(vecsize)) vecsize++;
   size = vecsize;
   Init();
}

HASHTABLE_TEMPLATE  HASHTABLE::cHashTable(const cHashTable& s)
   : size(s.size), sizelog2(s.sizelog2),fullness(s.fullness)
{
   statvec = new ubyte[size];
   vec = new Elem[size];
   Assrt(statvec != NULL);
   Assrt(vec != NULL);
   memcpy(statvec,s.statvec,sizeof(*statvec)*size);
   memcpy(vec,s.vec,sizeof(*vec)*size);
}


HASHTABLE_TEMPLATE int HASHTABLE::find_index(const KEY& key) const
{
   uint hash = FUNC::Hash(key);
   int index = hash%size;

   // Quick accept so we can skip divide
   if (statvec[index] == HASH_FULL)
   {
      int delta = 1 << (hash%sizelog2);

      for (int j = 0; j < size && statvec[index] == HASH_FULL; j++)
      {
         // compute next index
         index += delta;
         // we suspect this can be an IF
         while (index >= size)
            index -= size;
      }
      if (j >= size)
      {
         Warning(("Hash table index not found!\n"));
         index = INDEX_NOT_FOUND;
      }
   }




   return index;
}

HASHTABLE_TEMPLATE BOOL HASHTABLE::find_elem(const KEY& key, int* idx) const
{
   BOOL found = FALSE;
   uint hash = FUNC::Hash(key);
   int index = hash%size;
   int delta = 0;

   for (int j = 0;  j < size && statvec[index] != HASH_EMPTY; j++)
   {
      Elem& myelem = vec[index];

      if (statvec[index] == HASH_FULL && FUNC::IsEqual(key,myelem.key))
      {
         found = TRUE;
         break;
      }

      if (delta == 0)
         delta = 1 << (hash%sizelog2);

      index  += delta;
      // we suspect this can be an IF
      while (index >= size)
         index -= size;
   }
   *idx = index;
   return found;
}

HASHTABLE_TEMPLATE errtype HASHTABLE::Grow(int newsize)
{
   Elem* oldvec = vec;
   ubyte* oldstat = statvec;
   Elem *newvec;
   ubyte *newstat;
   int oldsize = size;
   int i;

   for (;!is_prime(newsize);newsize++);

   newvec = new Elem[newsize];
   if (newvec == NULL) return ERR_NOMEM;

   newstat = new ubyte[newsize];
   if (newstat == NULL)
   {
      delete [] newvec;
      return ERR_NOMEM;
   }
   vec = newvec;
   statvec = newstat;
   size = newsize;
   sizelog2 = hashlog2(newsize);
   fullness = 0;
   tombstones = 0; 
   for (i = 0; i < newsize; i++) newstat[i] = HASH_EMPTY;
   for (i = 0; i < oldsize; i++)
   {
      if (oldstat[i] == HASH_FULL)
      {
         Insert(oldvec[i].key,oldvec[i].value);
      }
   }
   delete [] oldvec;
   delete [] oldstat;
   return OK;
}



// set value for a key
HASHTABLE_TEMPLATE BOOL HASHTABLE::Set(const KEY& key,const VALUE& value)
{
   int i;
   if (!find_elem(key,&i))
   {
      Insert(key,value);
      return FALSE; 
   }
   vec[i].key = key;
   vec[i].value = value;
   statvec[i] = HASH_FULL;

   return TRUE;
}

   // insert a (key,value) pair.  key must not yet be present in the table
HASHTABLE_TEMPLATE errtype HASHTABLE::Insert(const KEY& key, const VALUE& value)
{
   int i;
   
   if ((fullness+tombstones)*100 > size*FULLNESS_THRESHHOLD_PERCENT)
   {
      
      // If we have as many tombstones as entries, then we can just
      // rehash without changing size.  This works because it takes
      // twice as much work to make a tombstone as it does to make an
      // entry.  (insert + delete vs only insert)

      if (tombstones >= fullness)
         Grow(size); // don't actually change size
      else
         Grow(size + fullness*100/FULLNESS_THRESHHOLD_PERCENT); // grow in proportion to fullness 
   }

   i = find_index(key);
   vec[i].key = key;
   vec[i].value = value;
   if (statvec[i] == HASH_TOMBSTONE)
      tombstones--; 
   statvec[i] = HASH_FULL;
   fullness++;
   return OK;
}

   // Lookup a value for a key.
HASHTABLE_TEMPLATE BOOL HASHTABLE::Lookup(const KEY& key, VALUE& value) const
{
   int i;
   if (find_elem(key,&i))
   {
      value = vec[i].value;
      return TRUE;
   }
   return FALSE;
}

   // Lookup a value for a key.
HASHTABLE_TEMPLATE const KEY& HASHTABLE::GetKey(const KEY& key) const
{
   int i;
   if (find_elem(key,&i))
   {
      return vec[i].key; 
   }
   return SomeKey();
}


   // Like lookup, but undefined if the value is not present
HASHTABLE_TEMPLATE const VALUE& HASHTABLE::Search(const KEY& key) const
{
   int i;
   if (find_elem(key,&i))
   {
      return vec[i].value;
   }
   else
      return SomeValue();
}

   // Remove key (and its value) from the table
HASHTABLE_TEMPLATE errtype HASHTABLE::Delete(const KEY& key)
{
   int i;
   if (find_elem(key,&i))
   {
      statvec[i] = HASH_TOMBSTONE;
      fullness--;
      tombstones++; 
      return OK;
   }
   return ERR_NOEFFECT;
}

HASHTABLE_TEMPLATE  HASHTABLE::~cHashTable()
{
   size = 0;
   fullness = 0;
   delete [] statvec;
   delete [] vec;
}

// Clear the table to empty
HASHTABLE_TEMPLATE void HASHTABLE::Clear(int newsize)
{
   while(!is_prime(newsize)) newsize++;
   delete [] statvec;
   delete []  vec;
   size = newsize;
   fullness = 0;
   Init();
}



// Return an iterator across elements
HASHTABLE_TEMPLATE HASHTABLE::cIter HASHTABLE::Iter() const
{
   HASHTABLE::cIter iter(this);
   return iter;
}

HASHTABLE_TEMPLATE KEY& HASHTABLE::SomeKey()
{
   static KEY key;
   return key;
}

HASHTABLE_TEMPLATE VALUE& HASHTABLE::SomeValue()
{
   static VALUE value;
   return value;
}

////////////////////////////////////////////////////////////
// ITERATOR IMPLEMENTATION
//

#define ITER_TEMPLATE template <class KEY, class VALUE, class FUNC>
#define HASHITER  cHashIter<KEY,VALUE,FUNC>

ITER_TEMPLATE HASHITER::cHashIter(const HASHTABLE* t)
   : table(t), idx(-1), size(t->size)
{
   Next();
}

ITER_TEMPLATE  HASHITER::~cHashIter()
{
}

ITER_TEMPLATE const KEY& HASHITER::Key()
{
   if (idx < size)
   {
      return table->vec[idx].key;
   }
   else return HASHTABLE::SomeKey();
}

ITER_TEMPLATE const VALUE& HASHITER::Value()
{
   if (idx < size)
   {
      return table->vec[idx].value;
   }
   else
      return HASHTABLE::SomeValue();
}

ITER_TEMPLATE BOOL HASHITER::Done()
{
   return idx >= size;
}

ITER_TEMPLATE void HASHITER::Next()
{
   for (idx++;idx < size; idx++)
   {
      if (table->statvec[idx] == HASH_FULL)
         break;
   }
}

#undef new
#undef malloc
#undef realloc

#endif // HSHPPTEM_H

