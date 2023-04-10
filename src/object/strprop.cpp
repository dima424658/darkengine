/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/strprop.cpp,v 1.6 1998/10/05 17:28:17 mahk Exp $
#include <appagg.h>
#include <string.h>
#include <strprop_.h>


// Must be last header 
#include <dbmem.h>



cStringDataOps cStringDataOps::gOps; 

STDMETHODIMP cStringDataOps::Read(sDatum* pdat, IDataOpsFile* file, int version)
{
   if (!pdat)
      pdat->value = new cStr; 

   cStr& str = *(cStr*)pdat->value; 

   ulong size = version;
   if (version > sizeof(Label))
   {
      file->Read(&size,sizeof(size));
   }
   char* buf = new char[size];
   file->Read(buf,size); 
   buf[size-1] ='\0'; 

   str = buf;
   delete [] buf; 
   return S_OK; 
}

STDMETHODIMP cStringDataOps::Write(sDatum val, IDataOpsFile* file) 
{
   cStr& str = *(cStr*)val.value; 
   ulong size = strlen(str) + 1; 
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



