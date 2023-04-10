/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: 
#include <tagpatch.h>
#include <tagfile.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

class cPatchTagFile: public cCTUnaggregated<ITagFile,&IID_ITagFile,kCTU_Default>
{
protected:
   ITagFile* mpBase; 
   ITagFile* mpPatch; 
   ITagFile* mpCur; 

public:
   cPatchTagFile(ITagFile* base, ITagFile* patch)
      : mpBase(base), mpPatch(patch),mpCur(NULL)
   {
      Assert_(mpBase && mpPatch); 
      mpBase->AddRef(); 
      mpPatch->AddRef(); 

      Assert_(mpBase->OpenMode() == mpPatch->OpenMode()); 
   }

   ~cPatchTagFile()
   {
      SafeRelease(mpBase); 
      SafeRelease(mpPatch); 
   }

   STDMETHOD_(const TagVersion*, GetVersion)() 
   {
      return mpBase->GetVersion(); 
   }

   STDMETHOD_(TagFileOpenMode,OpenMode)() 
   {
      return mpBase->OpenMode(); 
   }

   STDMETHOD(OpenBlock)(const TagFileTag* tag, TagVersion* version)
   {
      // Try the patch file first 
      HRESULT result = mpPatch->OpenBlock(tag,version); 
      if (SUCCEEDED(result))
      {
         mpCur = mpPatch; 
         return result; 
      }

      result = mpBase->OpenBlock(tag,version); 
      if (SUCCEEDED(result))
         mpCur = mpBase;
      return result; 
   }

   STDMETHOD(CloseBlock)()
   {
      HRESULT retval = E_FAIL; 
      if (mpCur)
      {
         retval = mpCur->CloseBlock(); 
      }
      mpCur = NULL; 
      return retval; 
   }


   STDMETHOD_(const TagFileTag*, CurrentBlock)()
   {
      return (mpCur) ? mpCur->CurrentBlock() : NULL; 
   }

   STDMETHOD_(ulong, BlockSize)(const TagFileTag* tag)
   {
      ulong result = mpPatch->BlockSize(tag); 
      if (result == kTagNoSuchBlock)
         result = mpBase->BlockSize(tag); 
      return result; 
   }

   STDMETHOD_(ITagFileIter*,Iterate)()
   {
      Warning(("cPatchTagFile::Iterate is incorrectly implemented!\n")); 
      return mpBase->Iterate(); 
   }

   STDMETHOD(Seek)(ulong cur, TagFileSeekMode mode) 
   {
      if (mpCur)
         return mpCur->Seek(cur,mode); 
      return E_FAIL; 
   }

   STDMETHOD_(ulong,Tell)()
   {
      if (mpCur)
         return mpCur->Tell();
      return 0;  
   }


   STDMETHOD_(ulong,TellFromEnd)()
   {
      if (mpCur)
         return mpCur->TellFromEnd();
      return 0;  
   }

   STDMETHOD_(long,Read)(char* buf, int buflen)
   {
      if (mpCur)
         return mpCur->Read(buf,buflen); 
      return -1; 
   }

   STDMETHOD_(long,Write)(const char* buf, int buflen)
   {
      if (mpCur)
         return mpCur->Write(buf,buflen); 
      return -1; 
   }

   STDMETHOD_(long,Move)(char* buf, int buflen)
   {
      if (mpCur)
         return mpCur->Move(buf,buflen); 
      return -1; 
   }

}; 


ITagFile* CreatePatchedTagFile(ITagFile* base,ITagFile* patch)
{
   return new cPatchTagFile(base,patch); 
}
