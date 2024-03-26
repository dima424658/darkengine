/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/ui/panlmode.cpp,v 1.23 1998/11/10 23:53:36 mahk Exp $

#include <loopapi.h>
#include <appagg.h>

#include <panlmode.h>
#include <loopmsg.h>

#include <string.h>
#include <imgsrc.h>

#include <scrnmode.h>
#include <scrnman.h>
#include <scrnloop.h>
#include <dispapi.h>
#include <gcompose.h>

extern "C"
{
#include <hotkey.h>
}
#include <contexts.h>

// To work around ambient sound bug
#include <sndloop.h>



// This is dumb
#include <editor.h>

#include <uiapp.h>
#include <uiloop.h>
#include <kbcook.h>

extern "C"
{
#include <event.h>
#include <keydefs.h>
}

#include <dev2d.h>

#include <simstate.h>


// Include these last
#include <dbmem.h>
#include <initguid.h>
#include <panlguid.h>

////////////////////////////////////////////////////////////
// PANEL MODE IMPLEMENTATION CLASSES
//

//------------------------------------------------------------
// Loop Client
//

GUImethods* panel_gui_methods(IPanelMode* pmode); 

class cPanelLoopClient : public cCTUnaggregated<ILoopClient, &IID_ILoopClient, kCTU_Default>
{
   IPanelMode* mpPanel; 
   GUImethods* mpOldGUImethods; 

public:

   static sLoopClientDesc Descriptor; 


   cPanelLoopClient(IPanelMode* panel) 
      : mpPanel(panel)
   {
      if (mpPanel)
         mpPanel->AddRef(); 
   };  


   ~cPanelLoopClient()
   {
      SafeRelease(mpPanel); 
   }


   STDMETHOD_(short,GetVersion)() { return kVerLoopClient; }; 
   STDMETHOD_(const sLoopClientDesc*, GetDescription)() { return &Descriptor; }; 

   STDMETHOD_(eLoopMessageResult, ReceiveMessage)(eLoopMessage, tLoopMessageData hData);

protected:
   static BOOL PanelEventHandler(uiEvent *ev, Region *reg, void *data);

   void DrawImage(ePanelModeImage which, BOOL center);
};

//
// Panel loop client helpers
//

// Event handler 

BOOL cPanelLoopClient::PanelEventHandler(uiEvent *ev, Region *reg, void *data)
{
   cPanelLoopClient* us = (cPanelLoopClient*)data; 
   const sPanelModeDesc* desc = us->mpPanel->Describe();
   ulong flags = desc->flags; 


   BOOL potent = FALSE;
   if ((flags & kPanelAnyKey) && ev->type == UI_EVENT_MOUSE && ev->subtype & (MOUSE_LUP|MOUSE_RUP))
      potent = TRUE;

   if (ev->type == UI_EVENT_KBD_COOKED && ev->subtype & KB_FLAG_DOWN)
   {
      if (flags & kPanelAnyKey)
         potent = TRUE;
      else if (flags & kPanelESC)
         potent = ev->subtype == (KEY_ESC|KB_FLAG_DOWN); 
   }

   if (potent)
   {
      us->mpPanel->Exit(); 
   }
   return TRUE; 
}

// Image Drawer

void cPanelLoopClient::DrawImage(ePanelModeImage which, BOOL center)
{
   IImageSource* image = mpPanel->GetImage(which);
   if (image)
   {
      int x = 0, y = 0;
      grs_bitmap* bm = (grs_bitmap*)image->Lock(); 

      if (center)
      {
         x = (grd_canvas->bm.w - bm->w)/2;
         y = (grd_canvas->bm.h - bm->h)/2; 
      }
      gr_bitmap(bm,x,y);
      image->Unlock(); 
      SafeRelease(image); 
   }
}

//
// Static: descriptor
//

static ILoopClient* LGAPI PanelClientFactoryFunc(sLoopClientDesc * pDesc, tLoopClientData data);

sLoopClientDesc cPanelLoopClient::Descriptor = 
{
   &LOOPID_Panel,
   "Interface Panel Client", 
   kPriorityNormal, 
   kMsgEnd | kMsgNormalFrame | kMsgPauseFrame | kMsgsMode ,

   kLCF_Callback,
   PanelClientFactoryFunc,
   
   NO_LC_DATA,
   
   {
      { kConstrainAfter, &LOOPID_ScrnMan, kMsgsMode|kMsgsFrame},
      { kConstrainAfter, &LOOPID_UI, kMsgsMode},
      { kNullConstraint },
   }
}; 

//
// Loop message handler
//

static BOOL gCanExit = TRUE; 

STDMETHODIMP_(eLoopMessageResult) cPanelLoopClient::ReceiveMessage(eLoopMessage msg, tLoopMessageData data) 
{
   LoopMsg info; 
   info.raw = data; 

   const sPanelModeDesc* desc = mpPanel->Describe();

   switch (msg)
   {
      case kMsgEnterMode:
         SimStatePause(); 
         // fall through
      case kMsgResumeMode:
      {
         gCanExit = TRUE; 
         mpOldGUImethods = TheGUImethods(); 
         SetTheGUImethods(panel_gui_methods(mpPanel)); 
         HotkeyContext = HK_PANEL_MODE; 

         // Install region handler 
         if ((desc->flags & (kPanelAnyKey|kPanelESC)) && msg == kMsgEnterMode)
         {
            Region* root = GetRootRegion();
            int handler; 
            uiInstallRegionHandler(root, UI_EVENT_MOUSE|UI_EVENT_KBD_COOKED, PanelEventHandler, this, &handler);
         }

         
       
#ifdef GRABBG_WORKS_IN_HARDWARE  
         if (desc->flags & kPanelClearScreen)
#endif 
            gr_clear(0); 

         DrawImage(kPanelBG, desc->flags & kPanelCenterBG);
         DrawImage(kPanelFG, desc->flags & kPanelCenterFG);
      }
      break; 

      case kMsgNormalFrame:
      case kMsgPauseFrame:
         if (desc->flags & kPanelAutoExit)
         {
            mpPanel->Exit(); 
         }
         break;

      case kMsgExitMode:
         SimStateUnpause(); 
         // fall through
      case kMsgSuspendMode: 
      {
         SetTheGUImethods(mpOldGUImethods); 
      }
      break; 

   } 

   return kLoopDispatchContinue; 
}

//------------------------------------------------------------
// LOOP CLIENT FACTORY 
//
// Let's just use the simple implementation 
//

static ILoopClient* LGAPI PanelClientFactoryFunc(sLoopClientDesc * pDesc, tLoopClientData data)
{
   return new cPanelLoopClient((IPanelMode*)data);    
}

static ulong panel_factory_id = -1; 

static void create_panel_factory()
{
   if (panel_factory_id == -1)
   {
      const sLoopClientDesc* descs[] = { &cPanelLoopClient::Descriptor, NULL}; 
      ILoopClientFactory* fact = CreateLoopFactory(descs);

      AutoAppIPtr_(LoopManager,pLoopMan); 
      pLoopMan->AddClientFactory(fact,&panel_factory_id); 
      SafeRelease(fact); 
   }
}

#ifdef NEED_FACTORY_DESTROY
static void destroy_panel_factory()
{
   AutoAppIPtr_(LoopManager,pLoopMan);
   pLoopMan->RemoveClientFactory(panel_factory_id); 
}
#endif 

//------------------------------------------------------------
// PANEL LOOP MODE 
//

class cPanelMode : public cCTUnaggregated<IPanelMode,&IID_IPanelMode,kCTU_Default>
{
public: 
   cPanelMode(const sPanelModeDesc* desc);
   virtual ~cPanelMode(); 
   
   STDMETHOD_(const sPanelModeDesc*,Describe)() { return &mPanelDesc; }; 
   STDMETHOD(SetDescription)(const sPanelModeDesc* desc) { mPanelDesc = *desc; return S_OK;}; 

   STDMETHOD(SetImage)(ePanelModeImage which, IImageSource* image); 
   STDMETHOD_(IImageSource*,GetImage)(ePanelModeImage which);

   STDMETHOD(SetParams)(sLoopModeInitParmList parms); 
   STDMETHOD_(const sLoopInstantiator*, Instantiator)(); 

   STDMETHOD(Switch)(eLoopModeChangeKind kind, sLoopModeInitParmList parms);  
   STDMETHOD(Exit)(); 
   

protected: 
   void SetupLoopMode();   
   void CleanupLoopMode(); 

   sLoopModeDesc mLoopDesc; 
   sPanelModeDesc mPanelDesc; 
   IImageSource* mImages[kNumPanelImages];    
   ScrnManContext mScrnParm; 
   sLoopInstantiator mInst; 
}; 


static tLoopClientID* Clients[] =
{
   &GUID_NULL, 
   &LOOPID_ScrnMan,
   &LOOPID_UI,
   &LOOPID_Panel,
   &LOOPID_Sound,
}; 

#define NUM_CLIENTS (sizeof(Clients)/sizeof(Clients[0]))

void cPanelMode::SetupLoopMode()
{
   AutoAppIPtr_(LoopManager,pLoopMan);

   ILoopMode* mode = pLoopMan->GetMode(mPanelDesc.id); 
   BOOL mode_exists = mode != NULL; 
   SafeRelease(mode); 

   if (!mode_exists) // create the mode 
   {
      sLoopModeName name = { mPanelDesc.id, "Panel Mode" }; 
      mLoopDesc.name = name; 
      mLoopDesc.nClients = NUM_CLIENTS; 

   // copy the clients array 
      mLoopDesc.ppClientIDs = new tLoopClientID*[NUM_CLIENTS]; 
      memcpy(mLoopDesc.ppClientIDs,Clients, sizeof(Clients));

   // Stuff in the extra client
      mLoopDesc.ppClientIDs[0] = mPanelDesc.client; 

      pLoopMan->AddMode(&mLoopDesc); 
   }
   else
      mLoopDesc.name.pID = &GUID_NULL; 

   // Initialize the instantiator 
   memset(&mInst,0,sizeof(mInst));
   mInst.pID = mPanelDesc.id; 
   SetParams(NULL); 
}

void cPanelMode::CleanupLoopMode()
{
   if (*mLoopDesc.name.pID != GUID_NULL)
   {
      AutoAppIPtr_(LoopManager,pLoopMan);
      pLoopMan->RemoveMode(mLoopDesc.name.pID);
      delete [] mLoopDesc.ppClientIDs; 
   }
}

static sScrnMode min_scrnmode = 
{
   0, // was: kScrnModeFlagsValid,
   0, 0, 
   0, 
   0, // kScrnModeFullScreen|kScrnMode2dDriver,  // fullscreen direct draw 
}; 

static sScrnMode max_scrnmode = 
{
   kScrnModeFlagsValid,
   0, 0, 
   0, 
   ~(kScrnMode3dDriver),  // anything but d3d 
};

static ScrnManContext def_scrnparm = 
{ 
   { NULL, &min_scrnmode, &max_scrnmode}, 
}; 



cPanelMode::cPanelMode(const sPanelModeDesc* desc) 
   : mPanelDesc(*desc), 
     mScrnParm(def_scrnparm)
{
   create_panel_factory(); 
   SetupLoopMode(); 
   memset(mImages,0,sizeof(mImages));
}

cPanelMode::~cPanelMode()
{
   CleanupLoopMode(); 
   for (int i = 0; i < kNumPanelImages; i++)
      SafeRelease(mImages[i]); 
   delete mInst.init; 
}

STDMETHODIMP cPanelMode::SetImage(ePanelModeImage which, IImageSource* image)
{
   if (mImages[which])
      SafeRelease(mImages[which]);
   mImages[which] = image;
   if (image)
      image->AddRef(); 
   return S_OK; 
}

STDMETHODIMP_(IImageSource*) cPanelMode::GetImage(ePanelModeImage which)
{
   IImageSource* retval = mImages[which];
   if (retval) retval->AddRef();
   return retval; 
}



static uiLoopContext uidata = 
{
   // @TODO: get this ref out of here 
   0, // REF_IMG_EditCursor, // cursor id  
};


#define NDEFAULTS (sizeof(default_parms)/sizeof(default_parms[0]))

STDMETHODIMP cPanelMode::SetParams(sLoopModeInitParmList parms)
{
   sLoopModeInitParm default_parms[] =
   {
      { &LOOPID_ScrnMan, (tLoopClientData)&mScrnParm}, 
      { &LOOPID_UI, (tLoopClientData)&uidata}, 
      { &LOOPID_Panel, (tLoopClientData)this }, 

   };

   BOOL found[NDEFAULTS] ;
   memset(found,0,sizeof(found)); 

   // count how many new parameters we have, allowing the client to override
   // our default data 
   int nparms = 0;
   int ndefaults = NDEFAULTS; 
   sLoopModeInitParm* parm;

   for (parm = parms; parm != NULL && parm->pID != NULL; parm++,nparms++)
   {
      // does it match a default 
      for (int i = 0; i < NDEFAULTS; i++)
         if (parm->pID == default_parms[i].pID)
         {
            found[i] = TRUE;
            ndefaults--;  // don't have to add this one
         }
   }

   delete mInst.init; 

   // Now build the actual parameter list 
   sLoopModeInitParm* init = new sLoopModeInitParm[nparms+ndefaults+1]; 

   // copy over added parms
   memcpy(init,parms,nparms*sizeof(*init)); 

   // now add in defaults
   for (int i = 0, idx = nparms; i < NDEFAULTS; i++)
      if (!found[i])  // parm wasn't passed in
      {
         init[idx++] = default_parms[i]; 
      }

   // Add null terminator 
   init[nparms+ndefaults].pID = NULL;  

   mInst.init = init; 

   return S_OK; 
}

STDMETHODIMP_(const sLoopInstantiator*) cPanelMode::Instantiator()
{
   // set the screen mode 
   mScrnParm.mode_params.preferred = mPanelDesc.screen_mode; 
   if (mPanelDesc.pal)
      mScrnParm.pal_res = mPanelDesc.pal;

#ifdef GRABBG_WORKS_IN_HARDWARE  
   if (mPanelDesc.flags & kPanelGrabBG)
   {
      AutoAppIPtr_(DisplayDevice,pDisp); 
      pDisp->Lock(); 
      IImageSource* bg = CreateBitmapImageSource(&grd_canvas->bm,kBitmapSrcCopyBits); 
      pDisp->Unlock(); 
      SetImage(kPanelBG,bg);
      SafeRelease(bg); 

   }
#endif 

   return &mInst; 
}

STDMETHODIMP cPanelMode::Exit()
{
   if (!gCanExit)
      return S_FALSE;
 
   AutoAppIPtr(Loop);
   if (mPanelDesc.transition.mode != NULL)
      pLoop->ChangeMode(mPanelDesc.transition.change,(sLoopInstantiator*)mPanelDesc.transition.mode); 
   else
      pLoop->EndMode(0); 
   gCanExit = FALSE; 
   return S_OK; 
}

STDMETHODIMP cPanelMode::Switch(eLoopModeChangeKind change, sLoopModeInitParmList parms)
{
   if (parms) 
      SetParams(parms); 

   // Change modes! 
   AutoAppIPtr(Loop);
   pLoop->ChangeMode(change, (sLoopInstantiator*)Instantiator()); 

   return S_OK; 
}


////////////////////////////////////////////////////////////

IPanelMode* CreatePanelMode(const sPanelModeDesc* desc)
{
   return new cPanelMode(desc); 
}

////////////////////////////////////////////////////////////
// PANEL GUI METHODS
//

// bad variable no donut.  This doesn't addref .
static IPanelMode* gpGUImode = NULL; 

static errtype panel_gui_setup(GUIcompose* c, int ctype)
{
   uiHideMouse(&c->area); 
   return DefaultGUImethods.setup(c,ctype); 
}


static void panel_gui_done(GUIcompose* c)
{
   DefaultGUImethods.done(c); 
   uiShowMouse(&c->area); 
}


static void panel_gui_clear(GUIcompose* c, Rect* r)
{
   grs_clip _clip = grd_gc.clip;
   if (r != NULL)
      gr_set_cliprect(r->ul.x,r->ul.y,r->lr.x,r->lr.y);
   else
      gr_set_cliprect(0,0,c->canv.bm.w,c->canv.bm.h);

   IImageSource* img = gpGUImode->GetImage(kPanelBG);
   if (img)
   {
      grs_bitmap* bm = (grs_bitmap*)img->Lock(); 
      gr_bitmap(bm,-c->area.ul.x,-c->area.ul.y); 
      img->Unlock(); 
      SafeRelease(img); 
   }
   else
      gr_clear(0);
   grd_gc.clip = _clip;
}



static GUImethods* panel_gui_methods(IPanelMode* mode)
{
   static GUImethods methods = { panel_gui_setup, panel_gui_clear, panel_gui_done, DefaultGUImethods.setpal }; 
   gpGUImode = mode; 
   return &methods; 
}


