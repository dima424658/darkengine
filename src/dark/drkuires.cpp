/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkuires.cpp,v 1.5 2000/02/19 13:08:56 toml Exp $
#include <drkuires.h>
#include <imgsrc.h>
#include <config.h>
#include <resapi.h>
#include <strrstyp.h>
#include <binrstyp.h>
#include <appagg.h>
#include <filespec.h>
#include <resistr.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

//
// Fetch an image
// 

IImageSource* FetchUIImage(const char* name, const char* path)
{
   return CreateResourceImageSource(path,name); 
}


//
// Fetch a string
// 

cStr FetchUIString(const char* table, const char* name, const char* path)
{
   AutoAppIPtr(ResMan); 

   IRes* pRes = pResMan->Bind(table, RESTYPE_STRING, NULL, path); 
   if (!pRes)
      return ""; 
   IStringRes* pTable;
   Verify(SUCCEEDED(pRes->QueryInterface(IID_IStringRes,(void**)&pTable))); 

   cStr retval = "";
   char* s = pTable->StringLock(name); 
   if (s)
   {
      retval = s;
      pTable->StringUnlock(name); 
   }
   SafeRelease(pTable); 
   SafeRelease(pRes);  
   
   return retval; 
}


extern BOOL FetchUIRects(const char* name, cUIRectArray &array, const char* path)
{
   AutoAppIPtr(ResMan); 

   IRes* pRes = pResMan->Bind(name, RESTYPE_BINARY, NULL, path); 
   if (!pRes)
      return FALSE; 
   Rect* r = (Rect*)pRes->Lock(); 
   int n = pRes->GetSize() / sizeof(*r); 

   array.SetSize(n);
   for (int i = 0; i < n ; i++)
      array[i] = r[i]; 

   pRes->Unlock(); 
   SafeRelease(pRes);
   return TRUE; 
}
