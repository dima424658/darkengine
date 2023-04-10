/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/linkdata.cpp,v 1.5 1998/10/08 16:25:04 MROWLEY Exp $
#include <linkdat_.h>
#include <linkdb_.h>
#include <linkstor.h>
#include <hshsttem.h>

#include <string.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////

STDMETHODIMP cLargeLinkDataStore::Set(LinkID id, void* data)
{
   sLinkDataTableElem* elem = Table.Search(id);
   if (elem == NULL)
   {
      elem = (sLinkDataTableElem*)new char[Size+sizeof(LinkID)];
      elem->id = id;
      elem = Table.Insert(elem);
   }
   if (data != elem->data)
      memcpy(elem->data,data,Size);

   return S_OK;
}

STDMETHODIMP_(void*) cLargeLinkDataStore::Get(LinkID id)
{
   sLinkDataTableElem* elem = Table.Search(id);
   if (elem != NULL) 
      return elem->data;

   return NULL;
}

STDMETHODIMP_(ulong) cLargeLinkDataStore::DataSize()
{
   return Size;
}

STDMETHODIMP cLargeLinkDataStore::Remove(LinkID id)
{
   sLinkDataTableElem* elem = Table.Search(id);
   if (elem != NULL) 
   {
      Table.Remove(elem);  
      delete elem;
   }
   return S_OK;
}

ILinkDataStore* CreateManagedLinkDataStore(ulong data_size)
{
   return new cLargeLinkDataStore(data_size);
}


