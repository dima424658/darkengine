/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/scrnloop.c,v 1.28 2000/02/25 16:28:37 kevin Exp $ 
#include <string.h>
#include <config.h>
#include <resapi.h>
#include <palrstyp.h>
#include <appagg.h>
#include <appapi.h>

#include <loopapi.h>
#include <loopmsg.h>
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
#include <platform_services.h>
#include <stdlib.h>
#include <mprintf.h>

// Must be last header
#include <dbmem.h>

BOOL gScrnLoopSetModeFailed = FALSE; 
static BOOL gScreenPreservedForNestedMode = FALSE;

static BOOL InstantiatorPreservesScreen(const sLoopInstantiator* mode)
{
   sLoopModeInitParm* parm;

   if (mode == NULL || mode->init == NULL)
      return FALSE;

   for (parm = mode->init; parm->pID != NULL; ++parm)
   {
      if (IsEqualGUID(parm->pID, &LOOPID_ScrnMan))
      {
         ScrnManContext* context = (ScrnManContext*)parm->data;
         return context != NULL && context->preserve_screen;
      }
   }
   return FALSE;
}

eLoopMessageResult LGAPI ScrnManLoopFunc(void* context, eLoopMessage msg, tLoopMessageData hdata)
{
   eLoopMessageResult result = kLoopDispatchContinue;

   ScrnManContext* data = (ScrnManContext*)context;
   LoopMsg info;
   info.raw = hdata;
   switch(msg)
   {
      case kMsgEnterMode:
      case kMsgResumeMode:
      {
         BOOL auto_fail = FALSE; 

         // Native movie playback is a child window over the current render
         // target.  Tearing down and rebuilding the renderer here causes a
         // visible display-mode flash and is unnecessary.
         if (data->preserve_screen)
            break;
         if (msg == kMsgResumeMode && gScreenPreservedForNestedMode)
         {
            gScreenPreservedForNestedMode = FALSE;
            break;
         }



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
                  char buf[256] =
                     "The requested display mode could not be initialized. "
                     "Check the configured resolution, color depth, and fullscreen settings.";
                  const char *detail = ScrnGetLastSetResError();
                  if (detail != NULL && detail[0] != '\0')
                     strncpy(buf, detail, sizeof(buf) - 1);
                  else
                     config_get_raw("scrn_loop_fail_msg",buf,sizeof(buf) - 1);
                  buf[sizeof(buf) - 1] = '\0';
                  PlatformShowError("Display initialization failed", buf);
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
               void *palette = IRes_Lock(pRes);
               if (palette)
               {
                  ScrnNewPalette(palette,0,256);
                  IRes_Unlock(pRes);
               }
               else
               {
#ifndef SHIP
                  mprintf("Failed to lock palette %s\n", data->pal_res);
#endif
               }
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
         if (data->preserve_screen)
            break;
         if (msg == kMsgSuspendMode && info.mode != NULL &&
             InstantiatorPreservesScreen(&info.mode->to))
         {
            // The transition record's parameter list belongs to the target
            // dispatch and is only guaranteed to be alive during suspend.
            // Remember the decision here instead of dereferencing it later
            // when the previous mode resumes.
            gScreenPreservedForNestedMode = TRUE;
            break;
         }
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

ILoopClient* LGAPI CreateScrnManClient(sLoopClientDesc * pDesc, tLoopClientData data)
{
   ScrnManContext* pContext = (ScrnManContext*)data;
   return CreateSimpleLoopClient(ScrnManLoopFunc,pContext,&ScrnManClientDesc);
}

sLoopClientDesc ScrnManClientDesc =
{
   &LOOPID_ScrnMan,
   "Screen Manager",
   kPriorityNormal,
   (ulong)kMsgsMode|kMsgsFrameEdge|kMsgVisual|kMsgsAppOuter,

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
