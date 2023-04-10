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
#include <mouse.h>

#include <testloop.h>
#include <gamemode.h>
#include <rand.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


void draw_random_stuff(void)
{
   short w,h,x,y,c;
   //   gr_set_canvas(grd_visible_canvas);
   w = grd_canvas->bm.w;
   h = grd_canvas->bm.h;
   x = Rand()%w;
   y = Rand()%h;
   c = Rand()%256;

   //   mprintf("Color %d is %d %d %d\n",c,grd_pal[c*3],grd_pal[c*3+1],grd_pal[c*3+2]);
   //   gr_clear(c);
   gr_set_fcolor(c);
   gr_box(x,y,x+Rand()%(w-x+1),y+Rand()%(h-y+1));
   
}



#pragma off(unreferenced)
eLoopMessageResult LGAPI TestLoopFunc(void* context, eLoopMessage msg, tLoopMessageData hdata)
{
   static bool spew_num = FALSE;
   eLoopMessageResult result = kLoopDispatchContinue; 
   sLoopFrameInfo* info = (msg & kMsgsFrame) ? (sLoopFrameInfo*)hdata : NULL;

   switch(msg)
   {
      case kMsgEnterMode:
         spew_num = config_is_defined("frame_no");
         break;
      case kMsgBeginFrame:
         break;
      case kMsgEndFrame:
         break;
      case kMsgExitMode:
         spew_num = FALSE;
         break;
      case kMsgNormalFrame:
         if (spew_num)
            mprintf("[%d %d %d]",info->nCount,info->nTicks,info->dTicks);
         //         if (config_is_defined("rectangles"))
         //            draw_random_stuff();
         break;
   }
   return result;
}

#pragma off(unreferenced)
ILoopClient* LGAPI CreateTestClient(sLoopClientDesc * pDesc, tLoopClientData data)
{
   return CreateSimpleLoopClient(TestLoopFunc,NULL,&TestLoopClientDesc);
}
#pragma on(unreferenced)

sLoopClientDesc TestLoopClientDesc =
{
   &LOOPID_Test, 
   "Test Client",
   kPriorityNormal, 
   kMsgsMode|kMsgsFrame,

   kLCF_Callback,
   CreateTestClient,
   
   NO_LC_DATA,

   {
      {kNullConstraint} // terminator
   }
};





