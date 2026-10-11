/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/propstor.cpp,v 1.4 1999/01/28 12:37:41 Zarko Exp $


#include <propstor.h>

#include <prophash.h>
#include <propsprs.h>
#include <proplist.h>
#include <proparry.h>
#include <propbool.h>

#include <hshpptem.h>
#include <dlisttem.h>


#include <propdb.h>

#include <dbmem.h>

//#pragma message("hah!")
template class cHashTable<sObjPropPair,sDatum,sObjPropHashFuncs>;
template class cHashTable<ObjID,sDatum,cHashFunctions>;
template class cSimpleDList<sObjDatumPair>;
template class cHashIter<sObjPropPair,sDatum,sObjPropHashFuncs>;
template class cHashIter<ObjID,sDatum,cHashFunctions>;

//template void cDList<class cSimpleDListNode<struct sObjDatumPair>,1>::DestroyAll(void);
template class cDList<cSimpleDListNode<sObjDatumPair>,1>;



IPropertyStore* CreateGenericPropertyStore(ePropertyImpl impl, IDataOps* ops)
{
   // This is obviously not real yet 

   IPropertyStore* store = NULL; 
   switch (impl)
   {
      case kPropertyImplHash:
         store = new cGenericHashPropertyStore(); 
         break; 

      case kPropertyImplSparseHash:
         store = new cGenericSparseHashPropertyStore(); 
         break; 
         
      case kPropertyImplLlist:
         store = new cGenericListPropertyStore(); 
         break; 

      case kPropertyImplBigArray:
         store = new cGenericArrayPropertyStore(); 
         break; 

      case kPropertyImplBoolean:
         store = new cBoolPropertyStore(); 
         break; 

      default: 
         Warning (("CreateGenericProperty: Bad impl %d\n", impl)); 
         return NULL;
   }

   if (ops)
      store->SetOps(ops); 

   return store; 
}

//
// Yield the default sparse hashtable 
//

cObjPropDatumHashTable& cObjPropDatumHashTable::DefaultTable()
{  
   return sPropertyDatabases::DB->mSparseHash; 
}





