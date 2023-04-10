// $Header: x:/prj/tech/libsrc/cpptools/RCS/hashpp.h 1.7 1998/04/21 18:57:31 mahk Exp $

#ifndef HASHPP_H
#define HASHPP_H

#include <lg.h>
#include <hashfunc.h>
#include <error.h>

#define DEFAULT_SIZE 10

////////////////////////////////////////////////////////////
// DENSE HASH TABLE CLASS
//
// This class is intended for mapping small keys into small values. 
// It keeps its own private copy of each value, and does not use pointers.
// If you have larger things you want to hash, use cHashSet. 
//

class cHashHelperFunctions;
template <class KEY, class VALUE, class FUNC> class cHashIter;

template <class KEY, class VALUE, class FUNC /* == cHashTableFunctions<KEY> */> 
class cHashTable : private cHashHelperFunctions
{
   struct Elem
   {  
      KEY key;
      VALUE value;
   };

   BOOL Lookup(const KEY&, VALUE&) const;

protected:
   int find_index(const KEY& key) const;
   BOOL find_elem(const KEY& key, int* idx) const;
   errtype Grow(int newsize);
   void Init(); 
   
   static KEY& SomeKey();  // return an arbitrary key
   static VALUE& SomeValue(); // return an arbitrary value
   

   int size; 
   int sizelog2; 
   int fullness;
   ubyte *statvec;
   Elem *vec;
   


public:
   friend class cHashIter<KEY,VALUE,FUNC>;
   typedef cHashIter<KEY,VALUE,FUNC> cIter;
   

   cHashTable(int size = DEFAULT_SIZE);
   cHashTable(const cHashTable& ); 
   virtual ~cHashTable();

   // set value for a key, return true IFF it was already defined
   BOOL Set(const KEY&, const VALUE&);

   // insert a (key,value) pair.  key must not yet be present in the table
   errtype Insert(const KEY&, const VALUE&); 

   // Lookup a value for a key.  
   BOOL Lookup(const KEY& k , VALUE* v) const 
   { Assert_(v != NULL); return Lookup(k,*v);};

   // Like lookup, but undefined if the value is not present
   const VALUE& Search(const KEY&) const; 

   // Search for the copy of the key we're using in the table
   // Really only useful if you want to delete the key after removing 
   // it's entry 
   const KEY& GetKey(const KEY&) const; 

   // Determine whether a key is present in the table
   BOOL HasKey(const KEY& key) const { VALUE val; return Lookup(key,val);}; 
   
   
   // Remove key (and its value) from the table
   errtype Delete(const KEY&);

   // Return an iterator across elements
   cIter Iter() const; 

   // Return the number of elements
   int nElems() const { return fullness; }; 

   // Clear to an empty hash table with the specified default size
   void Clear(int newsize = DEFAULT_SIZE);
};


////////////////////////////////////////////////////////////
// HASH TABLE ITERATOR
// 


template <class KEY, class VALUE, class FUNC>
class cHashIter
{
   int idx;
   int size;
   const cHashTable<KEY,VALUE,FUNC>* table;

public:
   cHashIter(const cHashTable<KEY,VALUE,FUNC>* t);
   cHashIter() :idx(0),size(-1),table(NULL) {}; 
   ~cHashIter();
   const KEY &  Key();   // current key, undefined if done
   const VALUE& Value(); // current value, undefined if done
   BOOL Done();   // true IFF done
   void Next();        // Move on to next value
};

////////////////////////////////////////////////////////////
// USEFUL HASH/EQUALITY FUNCTION CLASSES
//

//
// This uses the common hash functions found in the cHashFunctions class
//

template <class KEY> class cHashTableFunctions
{
public:
   static unsigned Hash(const KEY & k)
   {
      return cHashFunctions::Hash(k);
   }

   static BOOL IsEqual(const KEY& k1, const KEY& k2)
   {
      return cHashFunctions::IsEqual(k1,k2);
   }
};

//
// hash function class type for scalar types
//


template <class KEY> class cScalarHashFunctions
{
public:
   static unsigned Hash(const KEY& k)
   {
      return (unsigned)k;
   }

   static BOOL IsEqual(const KEY& k1, const KEY& k2)
   {
      return k1 == k2;
   }
};

//
// hash function template for classes that have their own Hash and == functions
//

template <class KEY> class cClassHashFunctions
{
public:
   static unsigned Hash(const KEY& k)
   {
      return k.Hash();
   }

   static BOOL IsEqual(const KEY& k1, const KEY& k2)
   {
      return k1 == k2;
   }
};


////////////////////////////////////////////////////////////
// HELPER FUNCTION CLASS
//

class cHashHelperFunctions
{
protected:
   static int hashlog2(int x);
   static int expmod(int b, int e, uint m);
   static bool is_fermat_prime(uint n, uint numtests);
   static bool is_prime(uint n);
};

////////////////////////////////////////////////////////////
// DENSE HASH TABLE CLASS for strings 
//

template <class VALUE>
class cStrHashTable : public cHashTable <const char*, VALUE, cHashTableFunctions<const char*> >
{
};

template <class KEY, class VALUE, class FUNC>
class cStrHashIter : public cHashIter  <const char*, VALUE, cHashTableFunctions<const char *> >
{
};

#endif // HASHPP_H
