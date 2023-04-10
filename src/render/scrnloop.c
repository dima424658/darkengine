/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/scrnloop.c,v 1.28 2000/02/25 16:28:37 kevin Exp $ 
#include <windows.h>
#include <config.h>
#include <resapi.h>
#include <palrstyp.h>
#include <appagg.h>
#include <appapi.h>

#include <loopapi.h>
#include <dispbase.h>
#include <vismsg.h>

#include <dev2d.h>
#include <portal.h>
#include <scrnman.h>
#include <scrnmode.h>
#include <resloop.h>
#include <uiloop.h>
#include <wrloop.h>    // for constraints
#include <scrnloop.h>
#include <palmgr.h>
#include <palette.h>
#include <gamma.h>
#include <mprintf.h>

// Must be last header
#include <dbmem.h>

BOOL gScrnLoopSetModeFailed = FALSE; 

eLoopMessageResult LGAPI ScrnManLoopFunc(void* context, eLoopMessage msg, tLoopMessageData hdata)
{
   eLoopMessageResult result = kLoopDispatchContinue;

   ScrnManContext* data = (ScrnManContext*)context;
   switch(msg)
   {
      case kMsgEnterMode:
      case kMsgResumeMode:
      {
         BOOL auto_fail = FALSE; 



#ifndef SHIP
         if (config_is_defined("scrn_loop_auto_fail"))
         {
            auto_fail = TRUE; 
            gScrnLoopSetModeFailed = TRUE; 
         }
#endif 

         if (!auto_fail)
            gScrnLoopSetModeFailed = !ScrnModeSet(data->mode_params.min_mode,
                                                  data->mode_params.preferred,
                                                  data->mode_params.max_mode);

         if (gScrnLoopSetModeFailed)
         {
            // OK, try preferred flags and bitdepth, but default dims
            sScrnMode mode;
            ScrnModeCopy(&mode, ScrnModeGetDefault(), kScrnModeAllValid);
            ScrnModeCopy(&mode, data->mode_params.preferred, kScrnModeBitDepthValid | kScrnModeFlagsValid);
            if (auto_fail || (!ScrnModeSet(data->mode_params.min_mode, &mode, data->mode_params.max_mode)))
            {
               BOOL last_resort = auto_fail ? FALSE : ScrnModeSet(NULL, ScrnModeGetDefault() ,NULL);
               if (!last_resort)
               {
                  char buf[80]; 
                  config_get_raw("scrn_loop_fail_msg",buf,sizeof(buf)); 
                  MessageBox ((HWND)NULL, buf, (LPCSTR)NULL, MB_ICONWARNING);
                  exit(0); 
               }
            }
         }

         if (data->pal_res != NULL)
         {
            IResMan* pResMan = AppGetObj(IResMan); 
            IRes* pRes = IResMan_Bind(pResMan,
                                      data->pal_res,
                                      RESTYPE_PALETTE,
                                      NULL, NULL, 0);
            if (pRes)
            {
               ScrnNewPalette(IRes_Lock(pRes),0,256); 
               IRes_Unlock(pRes); 
               SafeRelease(pRes);
            }
            else
            {
#ifndef SHIP   
               mprintf("Failed to load palette %s\n", data->pal_res);
#endif
            }
         }
// This now happens in ScrnSetRes()...
//         ScrnInit3d();
         porthw_init();
         gamma_display();
         portal_set_mem_rect_row((grd_bpp==8) ? 256 : 512);
      }
      break;

      case kMsgSuspendMode:
      case kMsgExitMode:
         porthw_shutdown();
         ScrnTerm3d();
         break;

      case kMsgBeginFrame:
         ScrnStartFrame();
         break;
      case kMsgEndFrame:
         ScrnEndFrame();
         break;

      case kMsgAppInit:
         palmgr_init();
         {  // here, we really should get a default palette from somewhere useful
            uchar *pal=Malloc(768);
            int i;
            for (i=0; i<64; i++)
               pal[3*i]=pal[3*i+1]=pal[3*i+2]=i<<2;
            for (i=0; i<64; i++)
             { pal[192+3*i+0]=i; pal[192+3*i+1]=pal[192+3*i+2]=0; }
            for (i=0; i<64; i++)
             { pal[384+3*i+1]=i; pal[384+3*i+0]=pal[384+3*i+2]=0; }
            for (i=0; i<64; i++)
             { pal[576+3*i+2]=i; pal[576+3*i+0]=pal[576+3*i+1]=0; }
            ScrnNewPalette(pal,0,256);
            Free(pal);
         }
         break;
      case kMsgAppTerm:
         palmgr_shutdown();
         ScrnUnsetRes();
         break;

      case kMsgVisual:
         ScrnForceUpdate();
         break;
   }
   return result;
}

#pragma off(unreferenced)
ILoopClient* LGAPI CreateScrnManClient(sLoopClientDesc * pDesc, tLoopClientData data)
{
   ScrnManContext* pContext = (ScrnManContext*)data;
   return CreateSimpleLoopClient(ScrnManLoopFunc,pContext,&ScrnManClientDesc);
}
#pragma on(unreferenced)

sLoopClientDesc ScrnManClientDesc =
{
   &LOOPID_ScrnMan,
   "Screen Manager",
   kPriorityNormal,
   kMsgsMode|kMsgsFrameEdge|kMsgVisual|kMsgsAppOuter,

   kLCF_Callback,
   CreateScrnManClient,
   
   NO_LC_DATA,

   {
      { kConstrainAfter,  &LOOPID_Res, kMsgsMode},
      { kConstrainAfter,  &LOOPID_UI,  kMsgVisual},
      { kConstrainBefore, &LOOPID_Wr,  kMsgsAppOuter},
      { kNullConstraint }
   }
};
