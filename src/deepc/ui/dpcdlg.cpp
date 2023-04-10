/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// Dialog code that needs to get rewritten.
#include <stdlib.h>

#include <config.h>
#include <cfgdbg.h>

#include <loopapi.h>
#include <comtools.h>
#include <appagg.h>

#include <2d.h>
#include <mprintf.h>
extern "C"
{
#include <event.h>
#include <region.h>
}

#include <dpcdlg.h>
#include <gamemode.h>
#include <scrnloop.h>
#include <uiloop.h>
#include <uiapp.h>
#include <rand.h>

// for shutting down sim
#include <simstate.h>

#include <resapi.h>

#include <objsys.h>
#include <osysbase.h>
#include <objquery.h>
#include <objpos.h>

// Must be last header
#include <dbmem.h>

EXTERN bool DPC_mouse;
//extern IRes *LoadArt(char *artname, grs_bitmap **bmp);
int gWhichDialog;

/*
char *gScreens[3] = { "InvScrn.pcx", "Charsht2.pcx", "Skills.pcx" };

static frameCnt = 0;

static void DrawJunk(void)
{
   frameCnt++;

   if (frameCnt % 10 == 0)
   {
      short w,h,x,y,c;
      w = grd_canvas->bm.w;
      h = grd_canvas->bm.h;
      x = Rand()%w;
      y = Rand()%h;
      c = Rand()%256;

      gr_set_fcolor(c);
      gr_box(x,y,x+Rand()%(w-x+1),y+Rand()%(h-y+1));
   }   
}
*/
/* ------------------------------------------------------------ */
void DrawBackground(char *artname)
{
    /*
        grs_bitmap *bmp;
        IRes *hnd;
        hnd = LoadArt(artname,&bmp);
        gr_bitmap(bmp,0,0);
        hnd->Unlock();
        SafeRelease(hnd);
   */
}

// Event handler for dialog loop.
static bool DialogEventHandler(uiEvent *ev, Region *reg, void *data)
{
   uiMouseEvent *me = (uiMouseEvent *)ev;

    switch (me->action)
    {
        case MOUSE_LDOWN:
        {
            break;
        }
        case MOUSE_RDOWN:
        {
            ILoop* looper = AppGetObj(ILoop);
            ILoop_EndMode(looper, 0);  // This is to "pop" the loopmode
            SafeRelease(looper);    
            return TRUE;
            break;
        }

        default:
        {
        }
    }
    return FALSE;
}

typedef struct State
{
   int handler;
} State;


// @NOTE:  This code seems a bit weak.
// When I figure out more of the codeflow, this should get rewritten. - Bodisafa
#pragma off(unreferenced)
eLoopMessageResult LGAPI DialogLoopFunc(void* statedata, eLoopMessage msg, tLoopMessageData hdata)
{
   State* state = (State*)statedata;
   Region* root = GetRootRegion();
   
   switch(msg)
   {
        case kMsgEnterMode:
        {
            if (!DPC_mouse)
            {
               uiShowMouse(NULL);
            }
            //DrawBackground();
            switch (gWhichDialog)
            {
                case 0:
                case 1:
                {
                    /*
                    gCharGenerator->LoadBitmaps();
                    if (gWhichDialog == 0)
                     gCharGenerator->ChangeState(KGenSplash);
                    else
                     gCharGenerator->ChangeState(KGenGameSummary);
                    */
                    uiInstallRegionHandler(root, UI_EVENT_MOUSE, DialogEventHandler, NULL, &state->handler);
                    //uiInstallRegionHandler(root, UI_EVENT_KBD_COOKED, CCharGenerator::KeyHandler, NULL, &state->handler);
                    break;
                }
                case 2:
                {
                    uiInstallRegionHandler(root, UI_EVENT_MOUSE, DialogEventHandler, NULL, &state->handler);
                    DrawBackground("replic8.pcx");
                }
            }
            SimStatePause();
            //frameCnt = 0;
            break;
       }
       case kMsgExitMode:
       {
           if (!DPC_mouse)
           {
               uiHideMouse(NULL);
           }
           //FreeBackground();
            switch (gWhichDialog)
            {
                case 0:
                case 1:
                {
                    //gCharGenerator->DestroyInterface();
                    //gCharGenerator->FreeBitmaps();
                    uiRemoveRegionHandler(root, state->handler);
                    //gCharGenerator->SetState(KGenNone);
                    break;
                }
            }
           SimStateUnpause();

           break;
       }

        case kMsgNormalFrame:
        {
           switch (gWhichDialog)
           {
               case 0:
               {
                   //gCharGenerator->DrawTeletypeText();
                   break;
               }
           }
    //     DrawJunk();
           break;
        }
        case kMsgEnd:
        {
        }
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

