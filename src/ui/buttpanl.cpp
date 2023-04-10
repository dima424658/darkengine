/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/ui/buttpanl.cpp,v 1.12 1999/05/27 12:55:54 mahk Exp $
#include <buttpanl.h>
#include <string.h>
#include <uiloop.h>
#include <loopapi.h>
#include <appagg.h>
#include <loopmsg.h>
#include <uigame.h>
#include <scrnmode.h>
#include <palmgr.h>
#include <config.h>
#include <cfgdbg.h>

// must be last header
#include <dbmem.h>


//////////////////////////////////////////////////////////////
//
// IMPLEMENTATION: cModalButtonPanel::cClient
//

cModalButtonPanel::cClient::cClient(const sLoopClientDesc* desc, cModalButtonPanel* panel)
   :mpPanel(panel),
    mActive(FALSE)
{
   // for now, just use the client desc provided
   // @TODO: copy the desc, adding bits & constraints we need
   mpDesc = desc; 
   memset(&mButtonList,0,sizeof(mButtonList)); 
}

cModalButtonPanel::cClient::~cClient()
{
}

////////////////////////////////////////

STDMETHODIMP_(eLoopMessageResult) cModalButtonPanel::cClient::ReceiveMessage(eLoopMessage msg, tLoopMessageData data) 
{
   LoopMsg info; 
   info.raw = data; 

   switch (msg)
   {
      case kMsgEnterMode:
      case kMsgResumeMode:
         mpPanel->mpClient = this; 
         if (!mActive)
            SetupUI(); 
         mActive = TRUE; 
         mpPanel->OnLoopMsg(msg,data); 
         break; 

      case kMsgExitMode:
      case kMsgSuspendMode:
         mpPanel->OnLoopMsg(msg,data); 
         if (mActive)
            CleanupUI(); 
         mActive = FALSE; 
         break; 

      default:
         mpPanel->OnLoopMsg(msg,data); 
         break; 
   }   

   return kLoopDispatchContinue; 
}

////////////////////////////////////////

bool cModalButtonPanel::cClient::BListCB(ushort action, int button, void* data, LGadBox* blist)
{
   cClient* us = (cClient*)data; 

   us->mpPanel->OnButtonList(action,button); 
   return FALSE; 
}

////////////////////////////////////////



void cModalButtonPanel::cClient::SetupUI()
{
   ConfigSpew("panel_spew",("Panel setup\n")); 
   uiGameStyleSetup(); 
   mpPanel->InitUI(); 

   
   mpCursorImg = mpPanel->CursorImage(); 

   grs_bitmap* bm = (mpCursorImg) ? (grs_bitmap*)mpCursorImg->Lock() : NULL;

   if (bm)
   {
      sScrnMode mode;
      ScrnModeGet(&mode); 

      mPaletteIdx = -1; 
      if (mode.bitdepth == 16) // cursor is in its own palette
      {
         ConfigSpew("panel_spew",("We love to palettize \n")); 

         IDataSource* palres = mpPanel->CursorPalette(); 
         if (palres)
         {
            mPaletteIdx = palmgr_alloc_pal((uchar*)palres->Lock()); 
            bm->align = mPaletteIdx; 

            ConfigSpew("panel_spew",("PaletteIdx = %d\n",mPaletteIdx)); 


            palres->Unlock(); 
            palres->Release(); 
         }
      }

   // @TODO: hot spot other than 0,0!
      Point hotspot = MakePoint(0,0); 
      uiMakeBitmapCursor(&mCursor,bm,hotspot); 

      uiSetGlobalDefaultCursor(&mCursor); 
   }
   else SafeRelease(mpCursorImg); 
   
   cRectArray& rects = mpPanel->Rects(); 
   cElemArray& elems = mpPanel->Elems(); 
   
   int n = mpPanel->NumButtons(); 
   Assert_(rects.Size() >= n); 
   Assert_(elems.Size() >= n); 
   
   if (n > 0)
   {
      LGadButtonListDesc desc = { n, rects.AsPointer(), elems.AsPointer(), BListCB, 0, 0 }; 
      memset(&mButtonList,0,sizeof(mButtonList)); 
      LGadCreateButtonListDesc(&mButtonList,LGadCurrentRoot(),&desc); 
      LGadBoxSetUserData(VB(&mButtonList),this); 

      int flags = LGadBoxFlags(&mButtonList); 
      LGadBoxSetFlags(&mButtonList, flags | BOXFLAG_CLEAR); 
      LGadDrawBox(VB(&mButtonList),NULL); 
   }
}

////////////////////////////////////////

void cModalButtonPanel::cClient::CleanupUI()
{
   ConfigSpew("panel_spew",("Panel cleanup\n")); 

   if (mpPanel->NumButtons() > 0)
   {
      LGadDestroyButtonList(&mButtonList);
      memset(&mButtonList,0,sizeof(mButtonList)); 
   }
   uiSetGlobalDefaultCursor(NULL); 
   if (mpCursorImg)
   {
      mpCursorImg->Unlock(); 
      mpCursorImg->Release(); 
   }
   if (mPaletteIdx >= 0)
      palmgr_release_slot(mPaletteIdx); 
   mPaletteIdx = -1; 

   mpPanel->TermUI(); 
   uiGameStyleCleanup(); 

}

////////////////////////////////////////

ILoopClient* cModalButtonPanel::cClient::FactoryFunc(const sLoopClientDesc* desc, tLoopClientData  data)
{
   return new cClient(desc,(cModalButtonPanel*)data); 
}

//////////////////////////////////////////////////////////////
//
// IMPLEMENTATION: cModalButtonPanel
//


cModalButtonPanel::cModalButtonPanel(sPanelModeDesc* pmdesc, sLoopClientDesc* lcdesc)
   :mFactoryHandle(-1)
{
   
   // stuff the relevant fields of the client desc 
   lcdesc->factoryType = kLCF_Callback;
   lcdesc->pfnFactory = cClient::FactoryFunc;

   // we always install the client handler 
   const sLoopClientDesc* descs[] = { lcdesc, NULL}; 
   cAutoIPtr<ILoopClientFactory> factory ( CreateLoopFactory(descs) );
   AutoAppIPtr_(LoopManager,pLoopMan); 
   pLoopMan->AddClientFactory(factory,&mFactoryHandle); 

   // create the panel mode
   mpPanelMode = CreatePanelMode(pmdesc); 

   // set the params for this mode to point at me
   sLoopModeInitParm client_parm[] = 
   {
      { lcdesc->pID, (tLoopClientData)this },
      { NULL }, 
   };

   mpPanelMode->SetParams(client_parm); 
   
}


cModalButtonPanel::~cModalButtonPanel()
{
   if (mFactoryHandle != -1)
   {
      AutoAppIPtr_(LoopManager,pLoopMan); 
      pLoopMan->RemoveClientFactory(mFactoryHandle); 
   }

   SafeRelease(mpPanelMode);
}

