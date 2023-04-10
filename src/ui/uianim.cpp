/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/


#include <cursors.h>
#include <dev2d.h>
#include <timer.h>
#include <comtools.h>
#include <appagg.h>

#include <resapi.h>
#include <resistr.h>
#include <strrstyp.h>
#include <imgrstyp.h>

#include <uianim.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

// Construct from string resource and name of bitmap
sUiAnimElem::sUiAnimElem(IStringRes *pStr,const char *pName,const char *pResPath,int flags) : mFlags(flags)
{
   IResMan *pResMan = AppGetObj(IResMan);

   // start at frame 0 at time 0
   mFrame = 0;
   mDt = 0;
   
   // Using the string resource, get the "x,y fps"
   const char *p = pStr->StringLock(pName);
   sscanf(p,"%d,%d %f %c",&mX,&mY,&mSpf,&mType);
   mSpf = 1.0/mSpf;     // convert fps to second/frame
   pStr->StringUnlock(pName);

   // Collecting frames, quit when none left
   while(1) {   
      char name[16];
      sprintf(name,"%s_%d",pName,mFrames.Size()+1);
      IRes *pFrame =  pResMan->Bind(name,RESTYPE_IMAGE,NULL,pResPath);
      if (pFrame == NULL) break;
      if (mFlags&kAnimLock) pFrame->Lock();
      mFrames.Append(pFrame);
   }
   mBlit();      
   pResMan->Release();
}


// Release all the bitmap frames
sUiAnimElem::~sUiAnimElem()
{
   int i;
   for (i=0;i<mFrames.Size();++i) {
      if (mFlags&kAnimLock) mFrames[i]->Unlock();
      mFrames[i]->Release();
   }
}

void sUiAnimElem::mBlit()
{
   int frame;
   if (mType=='r') {
      int len = mFrames.Size()*2-2;
      frame = mFrame%len;
      if (frame >= mFrames.Size()) frame = len - frame;
   } else {
      frame = mFrame%mFrames.Size();
   }

   grs_bitmap *pBm = (grs_bitmap*)mFrames[frame]->Lock();

   // Hide and expose the cursor
   Rect r = {mX,mY,mX+pBm->w,mY+pBm->h};
   uiHideMouse(&r);

   gr_bitmap(pBm,mX,mY);
   
   uiShowMouse(&r);

   mFrames[frame]->Unlock();
}



// update the frame with the dt, shows next frame if must be
void sUiAnimElem::Update(float dt)
{
   int oFrame = mFrame;

   // Increment
   mDt += dt;
   if (mDt>mSpf) {
      mDt -= mSpf;
      mFrame++;
      // If still too much, catch up, but don't ov
      if (mDt>mSpf) mDt=mSpf;
   }
   if (oFrame!=mFrame) {
      mBlit();
   }
}


void cUiAnim::InitUI()
{
   // Get the string resource
   char name[256];
   IResMan *pResMan = AppGetObj(IResMan);
   sprintf(name,"%sm.str",mPanelName);
   // Release all this when done
   IRes *res = pResMan->Bind(name,RESTYPE_STRING,NULL,mUiAnimResPath);
   // If not present, punt
   if (res==NULL) return;
   
   cAutoIPtr<IStringRes> strres(IID_IStringRes,res); 

   const char* s = strres->StringLock("list");

   // Iterate through list and build all the children
   const char *p = s;
   // eliminate leading whitespace
   if (p != NULL)
      while (*p == ' ') 
         p++; 

   while(p!=NULL && *p != '\0') {
      char lname[16];
      char *end;
      end = strchr(p,' ');
      if (end==NULL) {
         strcpy(lname,p);
         p = NULL;
      } else {
         memcpy(lname,p,end-p);
         lname[end-p]=0;
         p=end+1;
      }

      // We have one, create it
      mAnims.Append(new sUiAnimElem(strres,lname,mUiAnimResPath,mFlags));
   }

   strres->Unlock();
   res->Release();
   pResMan->Release();

   mLastClock = tm_get_millisec();
}



void cUiAnim::TermUI()
{
   // Release all everyone
   int i;
   for (i=0;i<mAnims.Size();++i) {
      delete mAnims[i];
   }
   mAnims.SetSize(0);
}

void cUiAnim::OnLoopMsg(eLoopMessage msg, tLoopMessageData data)
{
   // Update all the anims with the new time
   float dt;
   int clock = tm_get_millisec();
   dt = (float)(clock - mLastClock)/1000.0;
   mLastClock = clock;

   int i;
   for (i=0;i<mAnims.Size();++i) {
      mAnims[i]->Update(dt);
   }
}





