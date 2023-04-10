/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/ui/uiloop.cpp,v 1.41 1999/06/09 17:28:25 ccarollo Exp $

#include <stdlib.h>

#include <appagg.h>
#include <config.h>
#include <cfgdbg.h>
#include <res.h>
#include <event.h>
#include <slab.h>
#include <uires.h>
#include <cursors.h>
#include <curdat.h>
#include <2d.h>
#include <2dres.h>

#include <loopapi.h>
#include <gadget.h>
#include <hotkey.h>

#include <resapi.h>
#include <respaths.h>
#include <imgrstyp.h>
#include <palrstyp.h>
#include <imgsrc.h>
#include <scrnmode.h>


#include <reg.h>
#include <resloop.h>
#include <uiloop.h>
#include <joypoll.h>
#include <scrnloop.h>
#include <gen_bind.h>
#include <gcompose.h>
#include <dispatch.h>
#include <dispbase.h>
#include <vismsg.h>
#include <loopmsg.h>
#include <palette.h>

// Must be last header 
#include <dbmem.h>


typedef uiLoopContext Context;


typedef struct _uiStateRecord
{
   Context* context;
   LGadRoot root;
   uiSlab* old_slab;
   IDataSource* cursor_src; 
   
   //   uiSlab slab; 
   //   Region root;
   //   Cursor* cursor;
} uiStateRecord;

////////////////////////////////////////

static IDataSource* load_cursor(const char* resname)
{
   ubyte clut[256];

   AutoAppIPtr(ResMan); 
   IRes* pRes = pResMan->Bind(resname,RESTYPE_IMAGE,gContextPath); 
   if (!pRes) return NULL;  

   grs_bitmap* bm = (grs_bitmap*)pRes->Lock(); 
   IDataSource* pCopyBm = CreateBitmapImageSource(bm,kBitmapSrcCopyBits); 
   pRes->Unlock(); 

   bm = (grs_bitmap*)pCopyBm->Lock(); 
   ubyte* bits = bm->bits; 

   // now view the cursor as a palette
   IRes* pPalRes = pResMan->Retype(pRes,RESTYPE_PALETTE,0);    
   Assert_(pPalRes); 
   ubyte* pal = (ubyte*)pPalRes->Lock(); 

   // build the clut 
   pal_build_clut((ubyte(*)[3])pal,clut);

   pPalRes->Unlock(); 

   // now map the bm through the clut
   // @BUG: this assumes 8-bit
   for (int y = 0; y < bm->h; y++)
      for (int x = 0; x < bm->w; x++)
      {
         uchar* pixel = bits + y*bm->row + x; 
         if (*pixel != 0 || (bm->flags & BMF_TRANS) == 0)
            *pixel = clut[*pixel]; 
      }

   pCopyBm->Unlock(); 
   SafeRelease(pRes);
   SafeRelease(pPalRes); 
   return pCopyBm; 
}

static Point cursor_hotspot(const char* resname)
{
   // @TODO: have a real way of figuring out hotspot
   char cfgvar[64]; 
   sprintf(cfgvar,"hotspot_%s",resname); 
   int pt[2] = { 0, 0};  
   int cnt = 2; 
            
   config_get_value(cfgvar,CONFIG_INT_TYPE,pt,&cnt); 
   return MakePoint(pt[0],pt[1]); 
}

static void setup_cursor(uiStateRecord* state)
{
   if (state->context->cursor)
   {
      const char* name = state->context->cursor; 
      LGadSetRootCursor(&state->root,NULL,MakePoint(0,0));
      SafeRelease(state->cursor_src);  

      AutoAppIPtr(ResMan); 
      IRes* pCurRes = pResMan->Bind(name,RESTYPE_IMAGE,gContextPath);

      pCurRes->Lock(); 
      IDataSource* pSrc = load_cursor(name); 
      Point hotspot = cursor_hotspot(name); 
      LGadSetRootCursor(&state->root,pSrc,hotspot); 
      state->cursor_src = pSrc; 
      pCurRes->Unlock(); 
      SafeRelease(pCurRes); 
   }
}

////////////////////////////////////////

static void visual_message(uiStateRecord* state, DispatchData* data)
{
   switch (data->subtype)
   {
      case kPaletteChange:
         setup_cursor(state); 
         break;
   }
}

////////////////////////////////////////

#pragma off(unreferenced)
static bool hotkey_handler(uiEvent* ev, Region* reg, void* data)
{
   uiCookedKeyEvent* kev = (uiCookedKeyEvent*)ev;
   hotkey_dispatch(kev->code);
   return TRUE;
}
#pragma on(unreferenced)

////////////////////////////////////////
//
// LOOP/DISPATCH callback
//

static eLoopMessageResult LGAPI _LoopFunc(void* data, eLoopMessage msg, tLoopMessageData hdata)
{
   eLoopMessageResult result = kLoopDispatchContinue; 
   uiStateRecord* state = (uiStateRecord*)data;
   LoopMsg info;

   info.raw = hdata;

   switch(msg)
   {
      case kMsgEnterMode:
         uiGetCurrentSlab(&state->old_slab);
         memset(&state->root,0,sizeof(state->root));

         LGadSetupRoot(&state->root,
                       grd_visible_canvas->bm.w,
                       grd_visible_canvas->bm.h,
                       NULL,
                       MakePoint(0,0),
                       0);

         setup_cursor(state); 

         int cookie; 
         uiInstallRegionHandler(LGadBoxRegion(&state->root),UI_EVENT_KBD_COOKED,hotkey_handler,NULL,&cookie);
         UiJoyInit();

         // falling through
         //      case kMsgResumeMode: 

      case kMsgResumeMode: 
         // resize the root region, in case resolution changed
         // if I were cooler I'd check the current dims to see
         // whether we need to bother.
         {
            sScrnMode smode;
            ScrnModeGet(&smode);
            region_resize(LGadBoxRegion(&state->root), smode.w, smode.h);
         }

         LGadSwitchRoot(&state->root);
         DefaultGUIcanvas = grd_canvas;
         SetTheGUImethods(&DefaultGUImethods);
         uiFlush();
         break;

      case kMsgExitMode:
      {
         uiSlab* slab; 
         uiGetCurrentSlab(&slab); 
         if (slab == state->root.root_slab)
         {
            uiHideMouse(NULL);
            uiSetCurrentSlab(state->old_slab);
            uiShowMouse(NULL);
         }
         LGadDestroyRoot(&state->root);
         SafeRelease(state->cursor_src); 
      }
      break; 

      case kMsgSuspendMode:
         uiHideMouse(NULL);
         uiSetCurrentSlab(state->old_slab);
         uiShowMouse(NULL);
         break;
      case kMsgNormalFrame:
      case kMsgPauseFrame:   
         uiPoll();
         uiJoystickPoller();
         break;

      case kMsgEnd:
         Free(state);
         break;
      case kMsgVisual:
         visual_message(state,info.dispatch);
         break;
   }
   return result;
}

#pragma off(unreferenced)
static ILoopClient* LGAPI _CreateClient(sLoopClientDesc * pDesc, tLoopClientData data)
{
   uiStateRecord* state;
   state = (uiStateRecord*)Malloc(sizeof(uiStateRecord));
   memset(state,0,sizeof(*state)); 
   state->context = (Context*)data;
   
   return CreateSimpleLoopClient(_LoopFunc,state,pDesc);
}
#pragma on(unreferenced)

sLoopClientDesc uiLoopClientDesc =
{
   &LOOPID_UI, 
   "Low-level UI",
   kPriorityNormal, 
   kMsgsMode|kMsgsFrame|kMsgEnd|kMsgVisual,

   kLCF_Callback,
   _CreateClient,
   
   NO_LC_DATA,

   {
      { kConstrainAfter, &LOOPID_ScrnMan, kMsgsMode|kMsgsFrameMid},
      { kConstrainAfter, &LOOPID_Res, kMsgsMode},
      {kNullConstraint} // terminator
   }
};


