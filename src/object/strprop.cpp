/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/strprop.cpp,v 1.6 1998/10/05 17:28:17 mahk Exp $
#include <appagg.h>
#include <string.h>
#include <new>
#include <strprop_.h>


// Must be last header 
#include <dbmem.h>



cStringDataOps cStringDataOps::gOps; 

STDMETHODIMP cStringDataOps::Read(sDatum* pdat, IDataOpsFile* file, int version)
{
   if (!pdat->value)
      pdat->value = new cStr; 

   cStr& str = *(cStr*)pdat->value; 

   ulong size = version;
   if (version > sizeof(Label))
   {
      if (file->Read(&size,sizeof(size)) != sizeof(size))
         return E_FAIL;
   }
   if (size == 0 || size > 16 * 1024 * 1024)
      return E_FAIL;
   char* buf = new (std::nothrow) char[size];
   if (!buf)
      return E_OUTOFMEMORY;
   if (file->Read(buf,size) != (long)size)
   {
      delete [] buf;
      return E_FAIL;
   }
   buf[size-1] ='\0'; 

   str = buf;
   delete [] buf; 
   return S_OK; 
}

STDMETHODIMP cStringDataOps::Write(sDatum val, IDataOpsFile* file) 
{
   cStr& str = *(cStr*)val.value; 
   ulong size = (ulong)strlen(str) + 1;
   file->Write(&size,sizeof(size));

   const char* buf = str; 
   file->Write(buf,size); 
   return S_OK; 
}

////////////////////////////////////////

IStringProperty* CreateStringProperty(const sPropertyDesc* desc, ePropertyImpl impl)
{
   cAutoIPtr<IPropertyStore> store ( CreateGenericPropertyStore(impl) );
   return new cGenericStringProperty(desc,store); 
}

IStringProperty* CreateStringPropertyFromStore(const sPropertyDesc* desc, IPropertyStore* store)
{
   return new cGenericStringProperty(desc,store); 
}



