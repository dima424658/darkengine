/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/imgsrc.cpp,v 1.7 1998/09/18 21:34:38 mahk Exp $
#include <imgsrc.h>
#include <dev2d.h>
#include <appagg.h>

#include <resapi.h>
#include <respaths.h>
#include <imgrstyp.h>

#include <res.h>
#include <fdesc.h> 

#include <memall.h>
#include <mprintf.h>

// Include these last!
#include <dbmem.h>


// 
// Image source implementation 
//

class cBaseImageSource : public cCTUnaggregated<IDataSource,&IID_IDataSource,kCTU_Default>
{
public:
   virtual ~cBaseImageSource() {}; 
};

//
// Bitmap image source
//

// bits-per-pixel of a type
static int bpps[] =  { 8, 1, 8, 16, 8, 24, 8, 16, 24, 8 } ;

class cBitmapImageSource : public cBaseImageSource
{
   grs_bitmap Bitmap; 
   ulong flags; 
public:
   cBitmapImageSource(grs_bitmap* bm, ulong f)
      :flags(f)
   {
      uchar* bits = bm->bits;  
      ushort bmflags = bm->flags; 

      if (flags & kBitmapSrcCopyBits)
      {
         ulong size = bm->w*bm->h*bpps[bm->type]/8; 
         bits = new uchar[size];
         memset(bits,0,size); 
#ifndef SAFE_WAY
         memcpy(bits,bm->bits,size); 
#endif 
         bmflags &= ~BMF_DEVICE; 
      }

      gr_init_bitmap(&Bitmap,bits,bm->type,bmflags,bm->w,bm->h);

      if (flags & kBitmapSrcCopyBits)
      {
#ifdef SAFE_WAY
         grs_canvas canv;
         gr_make_canvas(&Bitmap,&canv); 
         gr_push_canvas(&canv);
         gr_bitmap(bm,0,0); 
         gr_pop_canvas(); 
         gr_close_canvas(&canv); 
#endif 

      }


   }

   ~cBitmapImageSource()
   {
      if (flags & kBitmapSrcCopyBits)
         delete [] Bitmap.bits; 
      gr_close_bitmap(&Bitmap); 
   }


   STDMETHOD_(void*,Lock)() 
   {
      return &Bitmap; 
   }

   STDMETHOD_(void,Unlock)()
   {
   }
};

IImageSource* CreateBitmapImageSource(grs_bitmap* bitmap, ulong flags) 
{
   return new cBitmapImageSource(bitmap,flags); 
}

//
// Resname image source
// 


IImageSource* CreateResourceImageSource(const char* path, const char* resname)
{
   AutoAppIPtr(ResMan); 
   return pResMan->Bind(resname,RESTYPE_IMAGE,gContextPath,path); 
}

//------------------------------------------------------------
// REF IMAGE SOURCE
//

class cRefImageSource : public cBaseImageSource
{
   Ref mRef;

public:
   cRefImageSource(Ref ref)
      : mRef(ref)
   {
   }

   ~cRefImageSource()
   {
   }

   STDMETHOD_(void*,Lock)()
   {
      FrameDesc* f = (FrameDesc*)RefLock(mRef);
      f->bm.bits = (uchar*)(f+1); 

      return &f->bm; 
   }

   STDMETHOD_(void,Unlock)()
   {
      RefUnlock(mRef); 
   }

};

IImageSource* CreateRefImageSource(ulong ref)
{
   return new cRefImageSource(ref); 
}
