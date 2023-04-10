/*
 * $Source: x:/prj/tech/libsrc/ui/RCS/event.c $
 * $Revision: 1.71 $
 * $Author: JAEMZ $
 * $Date: 1997/06/02 16:46:58 $
 *
 */
#include <lg.h>
#include <mouse.h>
#include <kb.h>
#include <kbcook.h>
#include <array.h>
#include <rect.h>
#include <slab.h>
#include <event.h>
#include <stdlib.h>
#include <dbg.h>
#include <_ui.h>
#include <vmouse.h>
#include <mousevel.h>
#include <joystick.h>

#ifdef WIN32

//-----------------------------------------------------------------------------
// Jacobson, 1-30-96
// This include helps the UI library keep track of where it's writing to
//-----------------------------------------------------------------------------
#include <comtools.h>
#include <appagg.h>
#include <dispapi.h>

IDisplayDevice * g_pUiDisplayDevice;

#endif

// ---------------------
// HANDLER CHAIN DEFINES
// ---------------------

typedef struct _ui_event_handler
{
   ulong typemask;  // Which event types does this handle?
     /* handler proc: called when a specific event is received */
   uiHandlerProc proc;
   void *state;  // handler-specific state data
   int next;     // used for chaining handlers.
} uiEventHandler;

typedef struct _handler_chain
{
   Array chain;
   int front;
   ulong opacity;
} handler_chain;


#define INITIAL_CHAINSIZE  4
#define INITIAL_FOCUSES 5
#define CHAIN_END -1

ulong uiGlobalEventMask = ALL_EVENTS;


// ----------------------------
// HANDLER CHAIN IMPLEMENTATION
// ----------------------------

errtype uiInstallRegionHandler(Region* r, ulong evmask, uiHandlerProc callback, void* state, int* id)
{
   handler_chain *ch;
   uiEventHandler* eh;
   int i;
   errtype err;
   Spew(DSRC_UI_Handlers,("uiInstallRegionhandler(%x,%x,%x,%x,%x)\n",r,evmask,callback,state,id));
   if (callback == NULL || r == NULL || evmask == 0) return ERR_NULL;
   ch = (handler_chain*) r->handler;
   if (ch == NULL)
   {
      Spew(DSRC_UI_Handlers,("uiInstallRegionHandler(): creating new handler chain\n"));
      ch = Malloc(sizeof(handler_chain));
      if (ch == NULL)
      {
         Spew(DSRC_UI_Handlers,("uiInstallRegionHandler: out of memory\n"));
         return ERR_NOMEM;
      }
      array_init(&ch->chain,sizeof(uiEventHandler),INITIAL_CHAINSIZE);
      ch->front = CHAIN_END;
      r->handler = (void*)ch;
      ch->opacity = uiDefaultRegionOpacity;
   }
   err = array_newelem(&ch->chain,&i);
   if (err != OK)
   {
      Spew(DSRC_UI_Handlers,("uiInstallRegionHandler(): array_newelem returned %d\n",err));
      return err;
   }
   eh = &((uiEventHandler*)(ch->chain.vec))[i];
   eh->next = ch->front;
   eh->typemask = evmask;
   eh->proc = callback;
   eh->state = state;
   ch->front = i;
   ch->opacity &= ~evmask;
   *id = i;
   Spew(DSRC_UI_Handlers,("exit uiInstallRegionHandler(): *id = %d\n",*id));
   return OK;
}

errtype uiRemoveRegionHandler(Region* r, int id)
{
   errtype err;
   handler_chain* ch;
   uiEventHandler* handlers;
   int i;

   Spew(DSRC_UI_Handlers,("uiRemoveRegionHandler(%x,%d)\n",r,id));
   if (r == NULL) return ERR_NULL;
   ch = (handler_chain*)r->handler;
   if (ch == NULL || id < 0) return ERR_RANGE;
   handlers = (uiEventHandler*)(ch->chain.vec);
   if (id == ch->front)
   {
      int next = handlers[id].next;
      err = array_dropelem(&ch->chain,id);
      if (err != OK) return err;
      ch->front = next;
      return OK;
   }
   for (i = ch->front; handlers[i].next != CHAIN_END; i = handlers[i].next)
   {
      if (handlers[i].next == id)
      {
         errtype err = array_dropelem(&ch->chain,id);
         if (err != OK) return err;
         handlers[i].next = handlers[id].next;
         return OK;
      }
   }
   return ERR_NOEFFECT;
}


errtype uiSetRegionHandlerMask(Region* r, int id, int evmask)
{
   handler_chain *ch;
   uiEventHandler* handlers;
   Spew(DSRC_UI_Handlers,("uiSetRegionHandlerMask(%x,%d,%x)\n",r,id,evmask));
   if (r == NULL) return ERR_NULL;
   ch = (handler_chain*)r->handler;
   if (ch == NULL || id >= ch->chain.fullness || id < 0) return ERR_RANGE;
   handlers = (uiEventHandler*)(ch->chain.vec);
   handlers[id].typemask = evmask;
   return OK;
}

// -------
// OPACITY
// -------

ulong uiDefaultRegionOpacity = 0;

ulong uiGetRegionOpacity(Region* reg)
{
   handler_chain *ch = (handler_chain*)(reg->handler);
   if (ch == NULL)
   {
      return uiDefaultRegionOpacity;
   }
   else
      return ch->opacity;
}

errtype uiSetRegionOpacity(Region* reg,ulong mask)
{
   handler_chain *ch = (handler_chain*)(reg->handler);
   if (ch == NULL)
   {
      Spew(DSRC_UI_Handlers,("uiSetRegionOpacity(): creating new handler chain\n"));
      ch = Malloc(sizeof(handler_chain));
      if (ch == NULL)
      {
         Spew(DSRC_UI_Handlers,("uiSetRegionOpacity: out of memory\n"));
         return ERR_NOMEM;
      }
      array_init(&ch->chain,sizeof(uiEventHandler),INITIAL_CHAINSIZE);
      ch->front = CHAIN_END;
      reg->handler = (void*)ch;
      ch->opacity = mask;
   }
   else
      ch->opacity = mask;
   return OK;
}

// -------------------
// FOCUS CHAIN DEFINES
// -------------------

typedef struct _focus_link
{
   Region* reg;
   ulong evmask;
   int next;
} focus_link;

extern uiSlab* uiCurrentSlab;
#define FocusChain (uiCurrentSlab->fchain.chain)
#define CurFocus (uiCurrentSlab->fchain.curfocus)
#define FCHAIN ((focus_link*)(uiCurrentSlab->fchain.chain.vec))

// ----------------
// FOCUS CHAIN CODE
// ----------------


errtype uiGrabSlabFocus(uiSlab* slab, Region* r, ulong evmask)
{
   int i;
   errtype err;
   focus_link* fchain; 
   Spew(DSRC_UI_Slab,("uiGrabSlabFocus(%x,%x,%x)\n",slab,r,evmask));
   if (r == NULL) return ERR_NULL;
   if (evmask == 0) return ERR_NOEFFECT;
   err = array_newelem(&slab->fchain.chain,&i);
   if (err != OK) return err;
   fchain = (focus_link*) slab->fchain.chain.vec;
   fchain[i].reg = r;
   fchain[i].evmask = evmask;
   fchain[i].next = slab->fchain.curfocus;
   Spew(DSRC_UI_Slab,("uiGrabSlabFocus(): old focus = %d new focus = %d\n",slab->fchain.curfocus,i));
   slab->fchain.curfocus = i;
   return OK;
}


errtype uiGrabFocus(Region* r, ulong evmask)
{
   return uiGrabSlabFocus(uiCurrentSlab,r,evmask);
}

errtype uiReleaseSlabFocus(uiSlab* slab, Region* r, ulong evmask)
{
   errtype retval = ERR_NOEFFECT;
   focus_link* fchain = (focus_link*)slab->fchain.chain.vec;
   focus_link *l;
   Spew(DSRC_UI_Slab,("uiReleaseSlabFocus(%x,%x,%x)\n",slab,r,evmask));
   if (r == NULL) return ERR_NULL;
   if (CurFocus==CHAIN_END) return retval;
   l = &fchain[CurFocus];
   if (l->reg == r)
   {
      ulong tmpmask = l->evmask & evmask;
      l->evmask &= ~evmask;
      evmask &= ~tmpmask;
      if (l->evmask == 0)
      {
         int tmp = slab->fchain.curfocus;
         slab->fchain.curfocus = l->next;
         Spew(DSRC_UI_Slab,("uiReleaseSlabFocus(): CurFocus = %d\n",slab->fchain.curfocus));
         array_dropelem(&slab->fchain.chain,tmp);
      }
      if (evmask == 0) return OK;
      retval = OK;

   }
   for(; l->next != CHAIN_END;)
   {
      focus_link* thenext = &fchain[l->next];
      if (thenext->reg == r)
      {
         ulong tmpmask = thenext->evmask & evmask;
         thenext->evmask &= ~evmask;
         evmask &= ~tmpmask;
         if (thenext->evmask == 0)
         {
            int tmp = l->next;
            l->next = thenext->next;
            array_dropelem(&slab->fchain.chain,tmp);
            if (evmask == 0)
               return OK;
            retval = OK;
            // we've deleted an element, effectively advancing
            // thenext, so don't advance l
            continue;
         }
         if (evmask == 0)
            return OK;
         retval = OK;
      }
      l = &fchain[l->next];
   }
   return retval;
}

errtype uiReleaseFocus(Region* r, ulong evmask)
{
   return uiReleaseSlabFocus(uiCurrentSlab,r,evmask);
}


// -----------------------
// POLLING AND DISPATCHING
// -----------------------

#define INITIAL_QUEUE_SIZE 32
#define DEFAULT_DBLCLICKTIME  0
#define DEFAULT_DBLCLICKDELAY 0

uiEventRecordFunc ui_recording_func = NULL;
ushort uiDoubleClickTime = DEFAULT_DBLCLICKTIME;
ushort uiDoubleClickDelay = DEFAULT_DBLCLICKDELAY;
bool   uiDoubleClicksOn[NUM_MOUSE_BTNS] = { FALSE, FALSE, FALSE } ;
bool   uiAltDoubleClick = FALSE;
ushort uiDoubleClickTolerance = 5;
static bool   poll_mouse_motion = FALSE;
static uiMouseEvent last_down_events[NUM_MOUSE_BTNS];
static uiMouseEvent last_up_events[NUM_MOUSE_BTNS];

static struct _eventqueue
{
   int in, out;
   int size;
   uiEvent* vec;
} EventQueue;

void event_queue_add(uiEvent* e)
{
   if ((EventQueue.in + 1)%EventQueue.size == EventQueue.out)
   {
      // Queue is full, grow it.
      int i;
      int out = EventQueue.out;
      int newsize = EventQueue.size * 2;
      uiEvent *newvec = Malloc(sizeof(uiEvent)*newsize);
      for(i = 0; out != EventQueue.in; i++, out = (out+1)%EventQueue.size)
         newvec[i] = EventQueue.vec[out];
      Free(EventQueue.vec);
      EventQueue.vec = newvec;
      EventQueue.size = newsize;
      EventQueue.in = i;
      EventQueue.out = 0;
   }
   EventQueue.vec[EventQueue.in] = *e;
   EventQueue.in++;
   if (EventQueue.in >= EventQueue.size) EventQueue.in = 0;
}

bool event_queue_next(uiEvent** e)
{
   if (EventQueue.in != EventQueue.out)
   {
      *e = &EventQueue.vec[EventQueue.out++];
      if (EventQueue.out >= EventQueue.size)
         EventQueue.out = 0;
      return TRUE;
   }
   return FALSE;
}


// TRUE we are opaque to this mask.
bool region_check_opacity(Region* reg, ulong evmask)
{
   return (evmask & uiGetRegionOpacity(reg)) != 0;
}

#pragma disable_message(202)
bool event_dispatch_callback(Region* reg, Rect* r, void* v)
{
   uiEvent* ev = (uiEvent*)v;
   handler_chain *ch = (handler_chain*)(reg->handler);
   int i,next;
   uiEventHandler* handlers = (ch == NULL)
         ? NULL
         : (uiEventHandler*)(ch->chain.vec);

   Spew(DSRC_UI_Dispatch,("event_dispatch_callback(%x,%x,%x) event type %x\n",reg,r,v,ev->type));

   if (ch == NULL || handlers == NULL)
   {
      Spew(DSRC_UI_Dispatch,("event_dispatch_callback(): no handler chain ch = %x handlers = %d\n",ch,handlers));
      return FALSE;
   }

   for (i = ch->front; i != CHAIN_END; i = next)
   {
      next = handlers[i].next;
      if ((handlers[i].typemask & ev->type)
            && (handlers[i].proc)(ev,reg,handlers[i].state))
      {
         Spew(DSRC_UI_Dispatch,("Caught by handler %d\n",i));
         return TRUE;
      }
   }
   Spew(DSRC_UI_Dispatch,("Event Rejected\n"));
   return FALSE;
}
#pragma enable_message(202)

// ui_traverse_point return values:
#define TRAVERSE_HIT       0
#define TRAVERSE_MISS      1
#define TRAVERSE_OPAQUE    2
#define TRAVERSE_CONTINUE  3

#define TRAVERSE_FAIL(val) (((val) == TRAVERSE_MISS) || ((val) == TRAVERSE_CONTINUE))

Region* uiLastMouseRegion[NUM_MOUSE_BTNS];


void ui_set_last_mouse_region(Region* reg,uiMouseEvent* ev)
{
   int i;
   if (ev->type != UI_EVENT_MOUSE)
      return;
   for (i = 0; i < NUM_MOUSE_BTNS; i++)
   {
      if ((ev->action & MOUSE_BTN2DOWN(i)) != 0 ||
          (ev->action & UI_MOUSE_BTN2DOUBLE(i)))
            uiLastMouseRegion[i] = reg;
      if (ev->action & MOUSE_BTN2UP(i))
         uiLastMouseRegion[i] = NULL;
   }
}

uchar ui_try_region(Region* reg, Point pos, uiEvent* ev)
{
   Rect cbr;
   uchar retval = TRAVERSE_MISS;

   cbr.ul = pos;
   cbr.lr = pos;
   if (region_check_opacity(reg,ev->type)) retval = TRAVERSE_OPAQUE;
   else if (event_dispatch_callback(reg,&cbr,ev)) retval = TRAVERSE_HIT;
   else return retval;
   ui_set_last_mouse_region(reg,(uiMouseEvent*)ev);
   return retval;
}


uchar ui_traverse_point(Region* reg, Point pos, uiEvent* data)
{
   uchar retval = TRAVERSE_MISS;
   Point rel;
   Region* child;

   rel = pos;
   rel.x -= reg->abs_x;
   rel.y -= reg->abs_y;

   if ((reg->status_flags & INVISIBLE_FLAG) != 0)
      return TRAVERSE_CONTINUE;

   if (reg->event_order)
   {
      retval = ui_try_region(reg,pos,data);
      if (!TRAVERSE_FAIL(retval)) return retval;
   }
   for (child = reg->sub_region; child != NULL; child = child->next_region)
      if (region_test_pt(child,rel))
      {
         retval = ui_traverse_point(child,pos,data);
         if (!TRAVERSE_FAIL(retval)) return retval;
         if (retval != TRAVERSE_CONTINUE) break;
      }
   if (!reg->event_order)
   {
      retval = ui_try_region(reg,pos,data);
      if (!TRAVERSE_FAIL(retval)) return retval;
   }
   return TRAVERSE_MISS;
}

bool send_event_to_region(Region* r, uiEvent* ev)
{
   Spew(DSRC_UI_Dispatch,("send_event_to_region(%x,%x)\n",r,ev));
   return ui_traverse_point(r,ev->pos,ev) == TRAVERSE_HIT;
}

bool uiDispatchEventToRegion(uiEvent* ev, Region* reg)
{
   Point pos;
   uiEvent nev = *ev;

   ui_mouse_do_conversion(&(nev.pos.x),&(nev.pos.y),TRUE);
   pos = nev.pos;
   pos.x += reg->r->ul.x - reg->abs_x;
   pos.y += reg->r->ul.y - reg->abs_y;

   if (!RECT_TEST_PT(reg->r,pos))
   {
      Rect r;
      r.ul = nev.pos;
      r.lr.x = nev.pos.x+1;
      r.lr.y = nev.pos.y+1;
      return event_dispatch_callback(reg,&r,&nev);
   }
   return ui_traverse_point(reg,nev.pos,&nev) == TRAVERSE_HIT;
}


bool uiDispatchEvent(uiEvent* ev)
{
   int i;
   Spew(DSRC_UI_Dispatch,("dispatch_event(%x), CurFocus = %d\n",ev,CurFocus));
   if (ui_recording_func != NULL)
      if (ui_recording_func(ev))
         return FALSE; // if the recording func "grabs" the event, don't process
                       // it as normal.
   if (!(ev->type & uiGlobalEventMask)) return FALSE;
   for (i = CurFocus; i != CHAIN_END; i = FCHAIN[i].next)
   {
      Spew(DSRC_UI_Dispatch,("dispatch_event(): checking focus chain element %d\n",i));
      if (FCHAIN[i].evmask & ev->type)
         if (uiDispatchEventToRegion(ev,FCHAIN[i].reg)) return TRUE;
   }
   return FALSE;
}

errtype uiQueueEvent(uiEvent* ev)
{
   // if this is a keyboard event, queue up earlier events.
   if (ev->type == UI_EVENT_KBD_RAW || ev->type == UI_EVENT_KBD_COOKED)
   {
      kbs_event kbe;
      for(kbe = kb_next(); kbe.code != KBC_NONE; kbe = kb_next())
      {
         uiRawKeyEvent out;
         mouse_get_xy(&out.pos.x,&out.pos.y);
         out.scancode = kbe.code;
         out.action = kbe.state;
         out.type = UI_EVENT_KBD_RAW;
         event_queue_add((uiEvent*)&out);
      }
   }
   if (ev->type == UI_EVENT_MOUSE || ev->type == UI_EVENT_MOUSE_MOVE)
   {
      lgMouseEvent mse;
      errtype err = mouse_next(&mse);
      for(;err == OK; err = mouse_next(&mse))
      {
         uiMouseEvent out;
         out.pos.x = mse.x;
         out.pos.y = mse.y;
         out.type = (mse.type == MOUSE_MOTION) ? UI_EVENT_MOUSE_MOVE :  UI_EVENT_MOUSE;
         out.action = mse.type;
         event_queue_add((uiEvent*)&out);
      }
   }
   event_queue_add(ev);
   return OK;
}

#define MOUSE_EVENT_FLUSHED UI_EVENT_MOUSE_MOVE

void ui_purge_mouse_events(void)
{
   int i;
   for (i = 0; i < NUM_MOUSE_BTNS; i++)
   {
      last_down_events[i].type = UI_EVENT_NULL;
      last_down_events[i].tstamp = 0;
      last_up_events[i].type = UI_EVENT_NULL;
      last_up_events[i].tstamp = 0;
   }
}

void ui_flush_mouse_events(ulong timestamp, Point pos)
{
   int i;
   for (i = 0; i < NUM_MOUSE_BTNS; i++)
   {

      if (uiDoubleClicksOn[i] &&
         last_down_events[i].type != UI_EVENT_NULL)
      {
         int crit = uiDoubleClickDelay;
         ulong timediff = timestamp - last_down_events[i].tstamp;
         Point downpos = last_down_events[i].pos;
         bool out = (abs(pos.x - downpos.x) > uiDoubleClickTolerance ||
                     abs(pos.y - downpos.y) > uiDoubleClickTolerance);

         // OK, if we've waited DoubleClickDelay after a down event, send it out.
         if (out || timediff >= crit)
         {
            uiMouseEvent ev;
            Spew(DSRC_UI_Polling,("flushing old clicks: crit = %d timediff = %d\n",crit,timediff));
            if (last_down_events[i].type != MOUSE_EVENT_FLUSHED)
            {
               ev = last_down_events[i];
               last_down_events[i].type = MOUSE_EVENT_FLUSHED;
               uiDispatchEvent((uiEvent*)&ev);
            }
            if (last_up_events[i].type == UI_EVENT_MOUSE)
            {
               ev = last_up_events[i];
               last_up_events[i].type = MOUSE_EVENT_FLUSHED;
               uiDispatchEvent((uiEvent*)&ev);
            }
         }
         // This is where we do our flushing
         if (last_up_events[i].type != UI_EVENT_NULL)
         {
            crit = uiDoubleClickTime;
            timediff = timestamp - last_up_events[i].tstamp;
            if (out || timediff >= crit)
            {
               last_down_events[i].type   = UI_EVENT_NULL;
                 last_up_events[i].type   = UI_EVENT_NULL;

               // send out a "timeout event"
//               if (!out)
               {
                  uiMouseEvent ev = last_up_events[i];
                  ev.type = UI_EVENT_MOUSE;
                  ev.action = UI_MOUSE_BTN2TIMEOUT(i);
                  uiDispatchEvent((uiEvent*)&ev);
               }
            }
         }

      }
   }
}

#define LEFT_ALT_KEY 0x38
#define RIGHT_ALT_KEY 0xB8

#define ALT_DOWN  (kb_state(LEFT_ALT_KEY) == KBS_DOWN || kb_state(RIGHT_ALT_KEY) == KBS_DOWN)


void ui_dispatch_mouse_event(uiMouseEvent* mout)
{
   int i;
   bool eaten = FALSE;
//   ui_mouse_do_conversion(&(mout->pos.x),&(mout->pos.y),TRUE);
   ui_flush_mouse_events(mout->tstamp,mout->pos);
   for (i = 0; i < NUM_MOUSE_BTNS; i++)
   {
      if (!(uiDoubleClicksOn[i]))
      {
         if (mout->action & MOUSE_BTN2UP(i))
            mout->action |= UI_MOUSE_BTN2TIMEOUT(i);
         continue;
      }
      if (uiAltDoubleClick && ALT_DOWN)
      {
         if (mout->action & MOUSE_BTN2DOWN(i))
         {
//            mout->action &= ~MOUSE_BTN2DOWN(i);
            mout->action |= UI_MOUSE_BTN2DOUBLE(i);
            continue;
         }
      }
      if (last_down_events[i].type != UI_EVENT_NULL)
      {

         if (mout->action & MOUSE_BTN2DOWN(i))
         {
            Spew(DSRC_UI_Polling,("double click down\n"));
            // make a double click event.
            mout->action &= ~MOUSE_BTN2DOWN(i);
            mout->action |= UI_MOUSE_BTN2DOUBLE(i);
            last_down_events[i].type = UI_EVENT_NULL;
            last_up_events[i].type = UI_EVENT_NULL;
         }
         if (mout->action & MOUSE_BTN2UP(i))
         {
            Spew(DSRC_UI_Polling,("up in time %d\n",mout->tstamp - last_down_events[i].tstamp));
            last_up_events[i] = *mout;
            eaten = TRUE;
         }
      }
      else if (mout->action & MOUSE_BTN2DOWN(i))
      {
         Spew(DSRC_UI_Polling,("saving the down\n"));
         last_down_events[i] = *mout;
         eaten = TRUE;
      }
   }
   if (!eaten)
      uiDispatchEvent((uiEvent*)mout);
}

// ----------------------
// KEYBOARD POLLING SETUP
// ----------------------

uchar* ui_poll_keys = NULL;

errtype uiSetKeyboardPolling(ubyte* codes)
{
   ui_poll_keys = codes;
   return OK;
}

void ui_poll_keyboard(void)
{
   uchar *key;
   for (key = ui_poll_keys; *key != KBC_NONE; key++)
      if (kb_state(*key) == KBS_DOWN)
      {
         uiPollKeyEvent ev;
         ev.type = UI_EVENT_KBD_POLL;
         ev.action = KBS_DOWN;
         ev.scancode = *key;
         uiDispatchEvent((uiEvent*)&ev);
      }
}

void ui_pop_up_keys(void)
{
   if (ui_poll_keys != NULL)
   {
      uchar* key;
      for (key = ui_poll_keys; *key != KBC_NONE; key++)
      {
         kb_clear_state(*key,KBA_STATE);
      }
   }
}

short ui_buttonstate = 0; // what does the ui think the button
                          // state is.
Point ui_mousestate = { 0 , 0 }; // where do we think the mouse is?

static uchar ui_joy_state = 0;


errtype uiMakeMotionEvent(uiMouseEvent* ev)
{
   // haha, this is the super secret mouse library variable of the
   // current button state.
   mouse_get_xy(&ui_mousestate.x, &ui_mousestate.y);
   ev->pos = ui_mousestate;
   ev->type = UI_EVENT_MOUSE_MOVE; // must get past event mask
   ev->action = MOUSE_MOTION;
   ev->tstamp = mouse_get_time();
   ev->buttons = (ubyte)(ui_buttonstate|ui_joy_state);
   return OK;
}

// Generate a fake mouse motion event and send it along...
Point ui_poll_mouse_position(void)
{
   uiMouseEvent ev;
   uiMakeMotionEvent(&ev);
   uiDispatchEvent((uiEvent *)(&ev));
   return ev.pos;
}


bool uiJoystickMouseEmulation = TRUE;

Point uiJoystickMouseGain = { 128, 128 };

#define MOUSE_GAIN_DENOM 16
Point uiJoystickDeadSpot  = { 16, 16 };


#define JOY_MOUSE_BUTTONS 2


void do_joy_mouse_emul(void)
{
   sbyte joypots[4];
   uchar butts = joy_read_buttons();
   int px,py;
   int x,y;

   joy_read_pots(joypots);
   px = joypots[0];
   py = joypots[1];

   if (abs(px) < uiJoystickDeadSpot.x)
      x = 0;
   else
      x = px*abs(px)*uiJoystickMouseGain.x / MOUSE_GAIN_DENOM ;

   if (abs(py) < uiJoystickDeadSpot.y)
      y = 0;
   else
      y = py*abs(py)*uiJoystickMouseGain.y / MOUSE_GAIN_DENOM;
   DBG(DSRC_UI_Polling,
      {
          if (x != 0 || y != 0)
            Spew(DSRC_UI_Polling,("Mouse vel %d %d \n",x,y));
      }

         );

   mouse_add_velocity(x,y);

   if (butts != ui_joy_state)
   {
      extern short mouseInstantButts; // mouse library back door.
      ushort action = 0;
      int i;
      uiMouseEvent mev;

      Spew (DSRC_UI_Polling,("butts are %d, old %d\n",butts,ui_joy_state));
      for (i = 0; i < JOY_MOUSE_BUTTONS; i++)
      {
         ubyte bit = 1 << i;
         if ( (butts & bit) != (ui_joy_state & bit)    )
         {
            action |= (butts & bit) ? MOUSE_BTN2DOWN(i) : MOUSE_BTN2UP(i);
//            mouseInstantButts &=  ~bit;
//            mouseInstantButts |= butts & bit;
         }
      }
      uiMakeMotionEvent(&mev);
      mev.buttons = mouseInstantButts|butts;
      mev.action = action;
      mev.type = UI_EVENT_MOUSE;
      ui_dispatch_mouse_event(&mev);
   }

   ui_joy_state = butts;
}

void call_deferred(void);

errtype uiPoll(void)
{
   static Point last_mouse = { -1, -1 };
   errtype err = OK;
   uiEvent out,*ev;
   bool kbdone = FALSE;
   bool msdone = FALSE;

    extern Point LastCursorPos;
   extern struct _cursor* LastCursor;
   extern void ui_update_cursor(Point pos);

   call_deferred();

   mouse_set_velocity(0,0);
   if (uiJoystickMouseEmulation)
      do_joy_mouse_emul();

#define BURN_QUEUE
#ifdef BURN_QUEUE
   // burn through queue
   while(event_queue_next(&ev))
   {
      bool result = TRUE;
//      ui_mouse_do_conversion(&(ev->pos.x),&(ev->pos.y),TRUE);
      if (ev->type == UI_EVENT_MOUSE)
         ui_dispatch_mouse_event((uiMouseEvent*)ev);
      else result = uiDispatchEvent(ev);
      if (!result && ev->type == UI_EVENT_KBD_RAW)
      {
         ushort cooked;
         kbs_event kbe;
         kbe.code = ((uiRawKeyEvent*)ev)->scancode;
         kbe.state = ((uiRawKeyEvent*)ev)->action;
         err = kb_cook(kbe,&cooked,&result);
         if (err != OK) goto out;
         if (result)
         {
            out.subtype = cooked;
            out.type = UI_EVENT_KBD_COOKED;
            uiDispatchEvent(ev);
         }
      }
   }
#endif // BURN_QUEUE

//   ui_mouse_get_xy(&ui_mousestate.x,&ui_mousestate.y);
   mouse_get_xy(&ui_mousestate.x,&ui_mousestate.y);


   while(!kbdone || !msdone)
   {
      if (!kbdone)
      {
         kbs_event kbe = kb_next();
         if (kbe.code != KBC_NONE)
         {
            bool eaten;
            uiRawKeyEvent* ev = (uiRawKeyEvent*)&out;
            Spew(DSRC_UI_Polling,("uiPoll(): got a keyboard event: <%d,%x>\n",kbe.state,kbe.code));
            ev->pos = ui_mousestate;
            ev->scancode = kbe.code;
            ev->action = kbe.state;
            ev->type = UI_EVENT_KBD_RAW;
            eaten = uiDispatchEvent((uiEvent*)ev);
            if (!eaten)
            {
              ushort cooked;
              bool result;
              Spew(DSRC_UI_Polling,("uiPoll(): cooking keyboard event: <%d,%x>\n",kbe.state,kbe.code));
              err = kb_cook(kbe,&cooked,&result);
              if (err != OK) return err;
              if (result)
              {
               out.subtype = cooked;
               out.type = UI_EVENT_KBD_COOKED;
               eaten = uiDispatchEvent(&out);
              }
            }
            if (eaten)
            {
//               kb_clear_state(kbe.code,KBA_STATE);
            }
         }
         else kbdone = TRUE;
      }
      if (!msdone)
      {
         lgMouseEvent mse;
         errtype err = mouse_next(&mse);
         if (poll_mouse_motion)
            while (mse.type == MOUSE_MOTION  && err == OK)
            {
               err = mouse_next(&mse);
            }
         if (err == OK)
         {
            uiMouseEvent* mout = (uiMouseEvent*)&out;
            out.pos.x = mse.x;
            out.pos.y = mse.y;
            ui_buttonstate = mse.buttons;
            ui_mousestate = out.pos;
            // note that the equality operator here means that motion-only
            // events are MOUSE_MOVE, and others are MOUSE events.
            out.type = (mse.type == MOUSE_MOTION) ? UI_EVENT_MOUSE_MOVE :  UI_EVENT_MOUSE;
            out.subtype = mse.type;
            mout->tstamp = mse.timestamp;
            mout->buttons = mse.buttons | ui_joy_state;
            ui_dispatch_mouse_event(mout);
//            uiDispatchEvent((uiEvent*)mout);
         }
         else msdone = TRUE;
      }
   }
   if (poll_mouse_motion)
   {
      ui_mousestate = ui_poll_mouse_position();
   }
   if (ui_poll_keys != NULL && (uiGlobalEventMask & UI_EVENT_KBD_POLL))
      ui_poll_keyboard();
   ui_flush_mouse_events(mouse_get_time(),ui_mousestate);
   if (!PointsEqual(ui_mousestate,last_mouse))
   {
      ui_update_cursor(ui_mousestate);
      last_mouse = ui_mousestate;
   }

out:
   call_deferred();

   return err;
}

errtype uiSetMouseMotionPolling(bool poll)
{
   if (poll) mouseMask &= ~MOUSE_MOTION;
   else mouseMask |= MOUSE_MOTION;
   poll_mouse_motion = poll;
   return OK;
}



errtype uiFlush(void)
{
   uiEvent* e;
   kbs_event kbe = kb_next();
   mouse_flush();

   while (kbe.code != KBC_NONE)
   {
      ushort dummy;
      bool result;
      kb_cook(kbe,&dummy,&result);
      kbe = kb_next();
   }
   while(event_queue_next(&e));
//   ui_pop_up_keys();
   ui_purge_mouse_events();
   return OK;
}

bool uiCheckKeys(key_handler handler)
{
   bool down = FALSE;
   kbs_event kbe;
   lgMouseEvent mse;
   kbe = kb_next();
   for (;kbe.code != KBC_NONE;kbe = kb_next())
   {
      ushort cooked;
      bool res;
      kb_cook(kbe,&cooked,&res);
      if (handler != NULL)
         handler(cooked);
      if (kbe.state == KBS_DOWN)
         down = TRUE;
   }
   if (down)
   {
      ui_pop_up_keys();
      return TRUE;
   }
   if (mouse_next(&mse) == OK)
   {
      int i;
      for (i = 0; i < NUM_MOUSE_BTNS; i++)
      {
         if ((mse.type & MOUSE_BTN2DOWN(i) ) != 0)
            return TRUE;
      }
   }
   if (joy_read_buttons() != 0)
      return TRUE;
   return FALSE;
}

bool uiCheckInput(void)
{
   return uiCheckKeys(NULL);
}

// ---------------------------
// INITIALIZATION AND SHUTDOWN
// ---------------------------

char keybuf[512];

errtype uiInit(uiSlab* slab)
{
   int i;
   errtype err;
   extern errtype ui_init_cursors(void);
#ifdef WIN32
   // Jacobson, 2-12-96
   // Get a pointer to the current display interface so we can correctly
   // lock the frame during cursor draws, among other things.
   g_pUiDisplayDevice = AppGetObj(IDisplayDevice);    // Stores the current display
#endif
   Spew(DSRC_UI_Handlers,("entering uiInit(%x)\n",slab));
   uiSetCurrentSlab(slab);
   Spew(DSRC_UI_Handlers,("uiInit(): CurFocus = %d\n",CurFocus));
#ifdef INIT_INPUT_LIBS   
   if (mouse_init(grd_cap->w,grd_cap->h) != OK)
   {
      Warning(("Could not initialize mouse!\n"));
      return(ERR_NOEFFECT);
   }
// @TBD (toml 05-20-96): in shodan\user kb_startup/kb_shutdown had been replaced by kb_init.  Must investigate reason.
   if (kb_startup(keybuf) < 0)
   {
      Warning(("Could not initialize keyboard!\n"));
      return(ERR_NOEFFECT);
   }
#endif // INIT_INPUT_LIBS
   // initialize the event queue;
   EventQueue.in = EventQueue.out = 0;
   EventQueue.size = INITIAL_QUEUE_SIZE;
   EventQueue.vec = Malloc(sizeof(uiEvent)*INITIAL_QUEUE_SIZE);
   for (i = 0; i < NUM_MOUSE_BTNS; i++)
      last_down_events[i].type = UI_EVENT_NULL;
   err = ui_init_cursors();
   if (err != OK) return err;
   return OK;
}

void uiShutdown(void)
{
   extern errtype ui_shutdown_cursors(void);
   ui_shutdown_cursors();


#ifdef INIT_INPUT_LIBS
   mouse_shutdown();
   kb_shutdown();
#endif 
#ifdef WIN32
   SafeRelease(g_pUiDisplayDevice);
#endif    

   Free(EventQueue.vec);
}


errtype uiShutdownRegionHandlers(Region* r)
{
   errtype err = OK;
   handler_chain *ch = (handler_chain*)(r->handler);
   if (ch == NULL) return ERR_NOEFFECT;
   err = array_destroy(&ch->chain);
   Free(ch);
   return err;
}

errtype ui_init_focus_chain(uiSlab* slab)
{
   errtype err = array_init(&slab->fchain.chain,sizeof(focus_link),INITIAL_FOCUSES);
   if (err != OK)  return err;
   slab->fchain.curfocus = CHAIN_END;
   return OK;
}

////////////////////////////////////////////////////////////
// uiDefer
//

static struct _ui_defer_record
{
   void (*func)(void* );
   void* arg;
} defer_list[32];

#define DEFER_LIST_SIZE (sizeof(defer_list)/sizeof(defer_list[0]))

static int defer_count = 0;

errtype uiDefer(void (*func)(void*), void* arg)
{
   if (defer_count >= DEFER_LIST_SIZE)
   {
      Warning(("uiDefer list overflow\n"));
      return ERR_DOVERFLOW;
   }
   defer_list[defer_count].func = func;
   defer_list[defer_count].arg = arg;
   defer_count++;
   return OK;
}

void call_deferred(void)
{
   while (defer_count > 0)
   {
      defer_count--;
      defer_list[defer_count].func(defer_list[defer_count].arg);
   }
}





