/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/prophash.cpp,v 1.12 1997/10/14 11:20:35 TOML Exp $
#include <prophash.h>
#include <hshpptem.h>

// Must be last header
#include <dbmem.h>

#define INITIAL_TABLE_SIZE 3


// explicit template instantiation
// It seems that this syntax is not understood by WATCOM
#ifdef __MSVC
template ObjHashTable;
template ObjHashTable::cIter;
template PropHashTable;
template PropHashTable::cIter;
#endif // __MSVC

////////////////////////////////////////////////////////////
//
// cPImpHashSmallUni
//
////////////////////////////////////////////////////////////

cPImpHashSmallUni::cPImpHashSmallUni (int elem_size) :
   cPropertyImpl (elem_size),table(INITIAL_TABLE_SIZE)
{
   Assrt(elem_size <= MAX_SMALL_PROP_SIZE);
}



////////////////////////////////////////

cPImpHashSmallUni::~cPImpHashSmallUni()
{

}

////////////////////////////////////////

BOOL cPImpHashSmallUni::Get (ObjID obj, PropertyID , void **pptr) const
{   
   // this may prove a problem for complex properties.  
   static tSmallElem value;
   BOOL found = table.Lookup(obj,&value);
   if (found)
   {
      *pptr = &value;
   }
   return found;
}

////////////////////////////////////////

BOOL cPImpHashSmallUni::Set (ObjID obj, PropertyID , void *ptr)
{
   tSmallElem value;
   Copy(obj,&value,ptr);
   return !table.Set(obj,value);
}

////////////////////////////////////////

BOOL cPImpHashSmallUni::Delete(ObjID obj, PropertyID )
{
   tSmallElem value;
   BOOL retval = FALSE;
   
   if (table.Lookup(obj,&value))
   {
      Term(obj,&value);
      retval = TRUE;
   }
   table.Delete(obj);
   return retval;
}

////////////////////////////////////////

void* cPImpHashSmallUni::Create(ObjID obj, PropertyID )
{
   static tSmallElem value = 0;
   Init(obj,&value);
   table.Set(obj,value);   
   return &value;
}

////////////////////////////////////////

BOOL cPImpHashSmallUni::Relevant (ObjID obj, PropertyID ) const
{
   return table.HasKey(obj);
}

////////////////////////////////////////

void cPImpHashSmallUni::Reset(PropertyID )
{  
   for (ObjHashTable::cIter iter = table.Iter(); !iter.Done(); iter.Next())
   {
      Term(iter.Key(),(void*)&iter.Value());
   }
   table.Clear(INITIAL_TABLE_SIZE);
}


////////////////////////////////////////

void cPImpHashSmallUni::IterStart(PropertyID , sPropertyObjIter* iter) const
{
   ObjHashTable::cIter* objiter = new ObjHashTable::cIter(table.Iter());
   iter->state[0] = objiter;
}

////////////////////////////////////////


void cPImpHashSmallUni::IterStop(PropertyID , sPropertyObjIter* iter) const
{
   ObjHashTable::cIter* hashiter = (ObjHashTable::cIter*)iter->state[0];
   if (hashiter != NULL)
   {
      delete hashiter;
      iter->state[0] = NULL;
   }
}

////////////////////////////////////////

BOOL cPImpHashSmallUni::IterNext(PropertyID , sPropertyObjIter* iter, ObjID* next, void** value) const
{
   static tSmallElem tmpvalue;

   ObjHashTable::cIter* hashiter = (ObjHashTable::cIter*)iter->state[0];
   if (hashiter == NULL)
      return FALSE;

   if (!hashiter->Done())
   {
      *next = hashiter->Key();
      tmpvalue = hashiter->Value();
      *value = &tmpvalue;
      hashiter->Next();
      return TRUE;     
   }

   delete hashiter;
   iter->state[0] = NULL;
   return FALSE;
}



////////////////////////////////////////////////////////////
//
// cPImpHashLargeUni
//
// Pretty much like above, except the value lives in an allocated
// block hanging off the table.  
//
////////////////////////////////////////////////////////////

cPImpHashLargeUni::cPImpHashLargeUni (int elem_size) :
   cPropertyImpl (elem_size),table(INITIAL_TABLE_SIZE)
{
#ifdef DBG_ON
   if(elem_size <= MAX_SMALL_PROP_SIZE)
   {
      Warning(("Large Uni hash table used for small property\n"));
   }
#endif // DBG_ON
}

////////////////////////////////////////

void cPImpHashLargeUni::ClearOut()
{
   // clear out all the allocated value blocks

   ObjHashTable::cIter iter;
   for (iter = table.Iter(); !iter.Done(); iter.Next())
   {
      Term(iter.Key(),iter.Value());
      delete (char*)iter.Value();
   }
}

////////////////////////////////////////

cPImpHashLargeUni::~cPImpHashLargeUni()
{
   ClearOut();
}

////////////////////////////////////////

BOOL cPImpHashLargeUni::Get (ObjID obj, PropertyID , void **pptr) const
{   
   tSmallElem value;
   BOOL found = table.Lookup(obj,&value);
   if (found)
   {
      *pptr = (void*)value;
   }
   return found;
}

////////////////////////////////////////

BOOL cPImpHashLargeUni::Set (ObjID obj, PropertyID , void *ptr)
{
   tSmallElem value;
   BOOL found = table.Lookup(obj,&value);
   // if found, reuse block
   if (found)
      Copy(obj,value,ptr);
   else
   {
      // Alloc a new value block
      void* block = new char[elem_size_]; 
      Init(obj,block);
      Copy(obj,block,ptr);
      table.Insert(obj,(tSmallElem)block);
   }
   return !found;
}

////////////////////////////////////////

BOOL cPImpHashLargeUni::Delete(ObjID obj, PropertyID )
{
   tSmallElem val;
   if (table.Lookup(obj,&val))
   {
      Term(obj,val);
      delete (char*)val;
      table.Delete(obj);
      return TRUE;
   }
   return FALSE;
}

////////////////////////////////////////

void* cPImpHashLargeUni::Create(ObjID obj, PropertyID )
{
   tSmallElem value;
   BOOL found = table.Lookup(obj,&value);
   // if found, reuse block
   if (found)
      return (void*)value;
   else
   {
      // Alloc a new value block
      void* block = new char[elem_size_]; 
      Init(obj,block);
      table.Insert(obj,(tSmallElem)block);
      return block;
   }
}

////////////////////////////////////////

BOOL cPImpHashLargeUni::Relevant (ObjID obj, PropertyID ) const
{
   return table.HasKey(obj);
}


////////////////////////////////////////

void cPImpHashLargeUni::Reset(PropertyID )
{  
   ClearOut();

   table.Clear(INITIAL_TABLE_SIZE);
}


////////////////////////////////////////

void cPImpHashLargeUni::IterStart(PropertyID , sPropertyObjIter* iter) const
{
   ObjHashTable::cIter* objiter = new ObjHashTable::cIter(table.Iter());
   iter->state[0] = objiter;
}

////////////////////////////////////////

void cPImpHashLargeUni::IterStop(PropertyID , sPropertyObjIter* iter) const
{
   ObjHashTable::cIter* hashiter = (ObjHashTable::cIter*)iter->state[0];
   if (hashiter != NULL)
   {
      delete hashiter;
      iter->state[0] = NULL;
   }
}

////////////////////////////////////////

BOOL cPImpHashLargeUni::IterNext(PropertyID , sPropertyObjIter* iter, ObjID* next, void** value) const
{
   ObjHashTable::cIter* hashiter = (ObjHashTable::cIter*)iter->state[0];
   if (hashiter == NULL)
      return FALSE;

   if (!hashiter->Done())
   {
      *next = hashiter->Key();
      *value = (void*)hashiter->Value();
      hashiter->Next();
      return TRUE;     
   }

   delete hashiter;
   iter->state[0] = NULL;
   return FALSE;
}


////////////////////////////////////////////////////////////
// CreateUniHashImpl
//

cPropertyImpl* CreateUniHashImpl(int elem_size)
{
   if (elem_size <= MAX_SMALL_PROP_SIZE)
      return new cPImpHashSmallUni(elem_size); 
   else 
      return new cPImpHashLargeUni(elem_size);
}


//============================================================
//============================================================

////////////////////////////////////////////////////////////
//
// cPImpHashSmallMulti
//
////////////////////////////////////////////////////////////

cPImpHashSmallMulti::cPImpHashSmallMulti (int elem_size, PropHashTable& t) :
   cPropertyImpl(elem_size),table(t)
{
   Assrt(elem_size <= MAX_SMALL_PROP_SIZE);
}



////////////////////////////////////////

cPImpHashSmallMulti::~cPImpHashSmallMulti()
{

}

////////////////////////////////////////

BOOL cPImpHashSmallMulti::Get (ObjID obj, PropertyID id , void **pptr) const
{   
   // this may prove a problem for complex properties.  
   static tSmallElem value;

   PropHashKey key(obj,id);

   BOOL found = table.Lookup(key,&value);
   if (found)
   {
      *pptr = &value;
   }
   return found;
}

////////////////////////////////////////

BOOL cPImpHashSmallMulti::Set (ObjID obj, PropertyID id , void *ptr)
{
   tSmallElem value;
   Copy(obj,&value,ptr);
   PropHashKey key(obj,id);

   return !table.Set(key,value);
}

////////////////////////////////////////

BOOL cPImpHashSmallMulti::Delete(ObjID obj, PropertyID id )
{
   PropHashKey key(obj,id);
   tSmallElem value;
   BOOL retval = FALSE;

   if (table.Lookup(key,&value))
   {
      Term(obj,&value);
      retval = TRUE;
   }
   table.Delete(key);
   return retval;
}

////////////////////////////////////////

void* cPImpHashSmallMulti::Create (ObjID obj, PropertyID id )
{
   static tSmallElem value;
   PropHashKey key(obj,id);

   Init(obj,&value);
   table.Set(key,value);
   return &value;
}

////////////////////////////////////////

BOOL cPImpHashSmallMulti::Relevant (ObjID obj, PropertyID id ) const
{
   PropHashKey key(obj,id);
   return table.HasKey(key);
}

////////////////////////////////////////

void cPImpHashSmallMulti::Reset(PropertyID id )
{  
   PropHashTable::cIter iter;
   for (iter = table.Iter(); !iter.Done(); iter.Next())
   {
      const PropHashKey& key = iter.Key();
      if (key.prop == id)
      {
         Term(key.obj,(void*)&iter.Value());
         table.Delete(key);
      }
   }
}


////////////////////////////////////////

void cPImpHashSmallMulti::IterStart(PropertyID , sPropertyObjIter* iter) const
{
   PropHashTable::cIter* objiter = new PropHashTable::cIter(table.Iter());
   iter->state[0] = objiter;
}

////////////////////////////////////////

void cPImpHashSmallMulti::IterStop(PropertyID , sPropertyObjIter* iter) const
{
   PropHashTable::cIter* hashiter = (PropHashTable::cIter*)iter->state[0];
   if (hashiter != NULL)
   {
      delete hashiter;
      iter->state[0] = NULL;
   }
}

////////////////////////////////////////

BOOL cPImpHashSmallMulti::IterNext(PropertyID id , sPropertyObjIter* iter, ObjID* next, void** value) const
{
   static tSmallElem tmpvalue;

   PropHashTable::cIter* hashiter = (PropHashTable::cIter*)iter->state[0];
   if (hashiter == NULL)
      return FALSE;

   for(;!hashiter->Done();hashiter->Next())
   {
      const PropHashKey& key = hashiter->Key(); 
      if (key.prop == id)
      {
         
         *next = key.obj;
         tmpvalue = hashiter->Value();
         *value = &tmpvalue;
         hashiter->Next();
         return TRUE;     
      }
   }

   delete hashiter;
   iter->state[0] = NULL;
   return FALSE;
}



////////////////////////////////////////////////////////////
//
// cPImpHashLargeMulti
//
// Pretty much like above, except the value lives in an allocated
// block hanging off the table.  
//
////////////////////////////////////////////////////////////

cPImpHashLargeMulti::cPImpHashLargeMulti (int elem_size, PropHashTable& t) :
   cPropertyImpl (elem_size),table(t)
{
#ifdef DBG_ON
   if(elem_size <= MAX_SMALL_PROP_SIZE)
   {
      Warning(("Large Multi hash table used for small property\n"));
   }
#endif // DBG_ON
}


////////////////////////////////////////

cPImpHashLargeMulti::~cPImpHashLargeMulti()
{
}

////////////////////////////////////////

BOOL cPImpHashLargeMulti::Get (ObjID obj, PropertyID id, void **pptr) const
{   
   tSmallElem value;
   PropHashKey key(obj,id);
   BOOL found = table.Lookup(key,&value);
   if (found)
   {
      *pptr = (void*)value;
   }
   return found;
}

////////////////////////////////////////

BOOL cPImpHashLargeMulti::Set (ObjID obj, PropertyID id , void *ptr)
{
   tSmallElem value;
   PropHashKey key(obj,id);
   BOOL found = table.Lookup(key,&value);
   // if found, reuse block
   if (found)
      Copy(obj,value,ptr);
   else
   {
      // Alloc a new value block
      void* block = new char[elem_size_]; 
      Init(obj,block);
      Copy(obj,block,ptr);
      table.Insert(key,(tSmallElem)block);
   }
   return !found;
}

////////////////////////////////////////

BOOL cPImpHashLargeMulti::Delete(ObjID obj, PropertyID id )
{
   tSmallElem val;
   PropHashKey key(obj,id);
   BOOL retval = table.Lookup(key,&val);

   if (retval)
   {
      Term(key.obj,val);
      delete (char*)val;
      table.Delete(key);
   }
   return retval;
}

////////////////////////////////////////

void* cPImpHashLargeMulti::Create (ObjID obj, PropertyID id )
{
   tSmallElem value;
   PropHashKey key(obj,id);
   BOOL found = table.Lookup(key,&value);
   // if found, reuse block
   if (found)
      return (void*)value;
   else
   {
      // Alloc a new value block
      void* block = new char[elem_size_]; 
      Init(obj,block);
      table.Insert(key,(tSmallElem)block);
      return block;
   }
}

////////////////////////////////////////

BOOL cPImpHashLargeMulti::Relevant (ObjID obj, PropertyID id ) const
{
   PropHashKey key(obj,id);
   return table.HasKey(key);
}

////////////////////////////////////////

void cPImpHashLargeMulti::Reset(PropertyID id)
{  
   PropHashTable::cIter iter;
   for (iter = table.Iter(); !iter.Done(); iter.Next())
   {
      const PropHashKey& key = iter.Key();
      if (key.prop == id)
      {
         Term(key.obj,iter.Value());
         delete (char*)iter.Value();
         table.Delete(key);
      }
   }
}


////////////////////////////////////////

void cPImpHashLargeMulti::IterStart(PropertyID , sPropertyObjIter* iter) const
{
   PropHashTable::cIter* objiter = new PropHashTable::cIter(table.Iter());
   iter->state[0] = objiter;
}

////////////////////////////////////////

void cPImpHashLargeMulti::IterStop(PropertyID , sPropertyObjIter* iter) const
{
   PropHashTable::cIter* hashiter = (PropHashTable::cIter*)iter->state[0];
   if (hashiter == NULL)
   {
      iter->state[0] = NULL;
      delete hashiter;
   }
}

////////////////////////////////////////

BOOL cPImpHashLargeMulti::IterNext(PropertyID id , sPropertyObjIter* iter, ObjID* next, void** value) const
{
   PropHashTable::cIter* hashiter = (PropHashTable::cIter*)iter->state[0];
   if (hashiter == NULL)
      return FALSE;

   for (;!hashiter->Done();hashiter->Next())
   {
      const PropHashKey& key = hashiter->Key();
      if (key.prop == id)
      {
         *next = key.obj;
         *value = (void*)hashiter->Value();
         hashiter->Next();
         return TRUE;     
      }
   }
   delete hashiter;
   iter->state[0] = NULL;
   return FALSE;
}


////////////////////////////////////////////////////////////
// CreateMultiHashImpl
//

cPropertyImpl* CreateMultiHashImpl(int elem_size, PropHashTable& table)
{
   if (elem_size <= MAX_SMALL_PROP_SIZE)
      return new cPImpHashSmallMulti(elem_size, table); 
   else 
      return new cPImpHashLargeMulti(elem_size, table);
}






