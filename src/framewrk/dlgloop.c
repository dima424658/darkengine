/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <stdlib.h>

#include <config.h>
#include <cfgdbg.h>

#include <loopapi.h>
#include <comtools.h>
#include <appagg.h>

#include <2d.h>
#include <mprintf.h>
#include <event.h>

#include <dlgloop.h>
#include <gamemode.h>
#include <scrnloop.h>
#include <uiloop.h>
#include <uiapp.h>
#include <rand.h>

// for shutting down sim
#include <simstate.h>

#include <resapi.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

#define INTERFACE_PATH "intrface\\"
static IRes *gHndBackBmp = NULL;

char *gScreens[3] = { "InvScrn.pcx", "Charsht2.pcx", "Skills.pcx" };
char gWhichScreen;

static frameCnt = 0;

// Get the background bitmap and draw it.
static void DrawBackground()
{
   // @TBD: We should probably have a proper InterfacePath, instead of using
   // a relative path under gContextPath.
   IResMan *pResMan = AppGetObj(IResMan);

   // Use the default path for this...
   gHndBackBmp = IResMan_Bind(pResMan, 
                              gScreens[gWhichScreen], 
                              RESTYPE_IMAGE, 
                              NULL, 
                              INTERFACE_PATH);
   if (gHndBackBmp)
   {
      grs_bitmap *bm = (grs_bitmap *) IRes_Lock(gHndBackBmp);
//      gr_set_bitmap_format(bm, (ushort)BMF_TRANS);
      gr_bitmap(bm, 0, 0);
   }

   SafeRelease(pResMan);
}

// Free the background bitmap
static void FreeBackground()
{
   if (gHndBackBmp) {
      IRes_Unlock(gHndBackBmp);
      SafeRelease(gHndBackBmp);
   }
}


// Event handler for dialog loop.
static bool DialogEventHandler(uiEvent *ev, Region *reg, void *data)
{
   uiMouseEvent *me = (uiMouseEvent *)ev;

   switch (me->action)
   {
      case MOUSE_LDOWN:
      {
         ILoop* looper = AppGetObj(ILoop);
         ILoop_EndMode(looper, 0);  // This is supposed to "pop" the loopmode,
         SafeRelease(looper);    // but instead it quits the program.
         break;
      }

      default:
         ;
   }
   return TRUE;
}

typedef struct State
{
   int handler;
} State;


#pragma off(unreferenced)
eLoopMessageResult LGAPI DialogLoopFunc(void* statedata, eLoopMessage msg, tLoopMessageData hdata)
{
   State* state = (State*)statedata;

   switch(msg)
   {
      case kMsgEnterMode:
         DrawBackground();
         SimStatePause();
         frameCnt = 0;
         {
            Region* root = GetRootRegion();
            uiInstallRegionHandler(root, UI_EVENT_MOUSE, DialogEventHandler, NULL, &state->handler);
         }
         break;

      case kMsgExitMode:
         FreeBackground();
         {
            Region* root = GetRootRegion();
            uiRemoveRegionHandler(root, state->handler);
         }
         SimStateUnpause();

         break;

      case kMsgNormalFrame:
//         DrawJunk();
         break;
      case kMsgEnd:
         Free(state);
   }
   return kLoopDispatchContinue;
}

#pragma off(unreferenced)
ILoopClient* LGAPI CreateDialogClient(sLoopClientDesc * pDesc, tLoopClientData data)
{
   return CreateSimpleLoopClient(DialogLoopFunc,Malloc(sizeof(State)),&DialogLoopClientDesc);
}
#pragma on(unreferenced)

sLoopClientDesc DialogLoopClientDesc =
{
   &LOOPID_Dialog, 
   "Dialog Client",
   kPriorityNormal, 
   kMsgsMode|kMsgsFrame|kMsgEnd,

   kLCF_Callback,
   CreateDialogClient,
   
   NO_LC_DATA,

   {
      { kConstrainAfter, &LOOPID_ScrnMan, kMsgsMode|kMsgsFrame},
      { kConstrainAfter, &LOOPID_UI, kMsgsMode},
      { kNullConstraint }
   }
};

