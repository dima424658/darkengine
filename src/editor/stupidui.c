/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/stupidui.c,v 1.5 2000/02/19 13:11:31 toml Exp $

//////////////////////////////////////////////////////////////
// Don't try this at home kids!
//
// This is a stupid alternate interface for the ui that allows you to 
// grab the events one at a time.
//

#include <event.h>
#include <region.h>
#include <reg.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

static uiEvent event_queue[41];  // THIS ABSOLUTELY MUST BE FORTY-ONE! 
static int qin = 0;
static int qout = 0;
static Region stupid_region; // placeholder for our handler
 

#define QUEUE_SIZE (sizeof(event_queue)/sizeof(event_queue[0]))

// 
// stupid event hander that just queues
//

#pragma off(unreferenced)
bool stupid_event_handler(uiEvent* ev, Region* reg, void* data)
{
   if (ev->type == UI_EVENT_KBD_RAW)
      return FALSE;

   if (qin < QUEUE_SIZE)
   {
      event_queue[qin++] = *ev;
   }
   else
      Warning(("Stupid event queue is too small!\n"));
   return TRUE;
}

//
// Get the next event
//



bool get_stupid_ui_event(uiEvent* ev)
{
   static fail_once_per_frame = TRUE;
   if (qout >= qin)
   {
      if (fail_once_per_frame) {
         fail_once_per_frame = FALSE;
         return FALSE;
      } else
         fail_once_per_frame = TRUE;
      qout = qin = 0;
      uiGrabFocus(&stupid_region,ALL_EVENTS);
      region_flush_sequence(TRUE);
      uiPoll();
      uiReleaseFocus(&stupid_region,ALL_EVENTS);
   }
   if (qout < qin)
   {
      *ev = event_queue[qout++];
      return TRUE;
   }
   return FALSE;
}


//
// start queuing going 
//

void stupid_ui_start(void)
{
   Rect stupid_rect = { { 0, 0}, {1,1}};
   int cookie;

   // the root region of sphincter-space!
   make_region(NULL,&stupid_region,&stupid_rect,0,NULL);
   
   uiInstallRegionHandler(&stupid_region,ALL_EVENTS, stupid_event_handler, NULL, &cookie);

   qin = qout = 0;
}

void stupid_ui_end(void)
{
   for (; qout < qin ; qout++)
   {
      uiQueueEvent(&event_queue[qout]);
   }
   region_destroy(&stupid_region,FALSE);

}


