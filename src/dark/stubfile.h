/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/stubfile.h,v 1.2 1998/11/03 06:38:14 mahk Exp $
#pragma once  
#ifndef __STUBFILE_H
#define __STUBFILE_H

#include <tagfile.h>

class cTagFileProxy: public cCTUnaggregated<ITagFile,&IID_ITagFile,kCTU_Default>
{
protected:
   ITagFile* mpInner; 

public:

   cTagFileProxy(ITagFile* file) 
      : mpInner(file)
   {
      mpInner->AddRef(); 
   }

   ~cTagFileProxy()
   {
      SafeRelease(mpInner); 
   }

   STDMETHOD_(const TagVersion*,GetVersion)()
   {
      return mpInner->GetVersion(); 
   }

   STDMETHOD_(TagFileOpenMode,OpenMode)()
   {
      return mpInner->OpenMode(); 
   }

   STDMETHOD(OpenBlock)(const TagFileTag* tag, TagVersion* version)
   {
      return mpInner->OpenBlock(tag,version); 
   }

   STDMETHOD(CloseBlock)() { return mpInner->CloseBlock(); }
   
   STDMETHOD_(const TagFileTag*, CurrentBlock)() 
   {
      return mpInner->CurrentBlock(); 
   }

   STDMETHOD_(ulong, BlockSize)(const TagFileTag* tag) 
   {
      return mpInner->BlockSize(tag); 
   }

   STDMETHOD_(ITagFileIter*,Iterate)()
   {
      return mpInner->Iterate(); 
   }

   STDMETHOD(Seek)(ulong offset, TagFileSeekMode whence) 
   {
      return mpInner->Seek(offset,whence); 
   }

   STDMETHOD_(ulong,Tell)() {  return mpInner->Tell(); }; 

   STDMETHOD_(ulong,TellFromEnd)() { return mpInner->TellFromEnd(); }; 

   STDMETHOD_(long,Read)(char* buf, int buflen) 
   {
      return mpInner->Read(buf,buflen); 
   }

   STDMETHOD_(long,Write)(const char* buf, int buflen) 
   {
      return mpInner->Write(buf,buflen); 
   }

   STDMETHOD_(long,Move)(char* buf, int buflen) 
   {
      return mpInner->Move(buf,buflen); 
   }


}; 


#endif // __STUBFILE_H
