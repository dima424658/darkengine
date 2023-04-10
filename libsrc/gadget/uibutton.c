/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

/*
 * $Source: r:/t2repos/thief2/libsrc/gadget/uibutton.c,v $
 * $Revision: 1.17 $
 * $Author: mahk $
 * $Date: 1998/08/19 13:56:50 $
 *
 */

#include <string.h>

#include <lg.h>
#include <gcompose.h>
#include <uigadget.h>
#include <uibutton.h>
#include <2d.h>
#include <event.h>
#include <2dres.h>
#include <cfgdbg.h>
#include <gadgint.h>
#include <keydefs.h>
//#include <sndshell.h>

#include <mprintf.h>

#define set_region_expose(reg,exp) \
   do { \
      (reg)->status_flags |= EXPOSE_CB; \
      (reg)->expose = (exp);  \
   } while(0)

#define SEQUENCE_START() region_begin_sequence()
#define SEQUENCE_END()   region_end_sequence(TRUE)

// ======================================
// BUTTON GADGET EXPOSE FUNC
#pragma off(unreferenced)
void buttongadg_draw_func(void* data, LGadBox* box)
{
   ButtonGadg* g = (ButtonGadg*)box;
   DrawElemState state = g->state;
   if (g->flags & LGADBUTT_FLAG_STATELESS) state = dsNORMAL;
   ElementDraw(&g->draw,state,0,0,grd_canvas->bm.w,grd_canvas->bm.h);
}

#if 0
static bool buttongadg_expose_func(Region* reg, Rect* rect)
{
   // whoa this is a hack
   Rect* saverect = reg->r;
   Rect r;
   RectSect(rect,saverect,&r);
   reg->r = &r;
   LGadDrawBoxInternal((LGadBox*)reg,NULL,&r);
   reg->r = saverect;
   return TRUE;   
}
#endif

//===================================================
// MOUSE EVENT PROCESSING 
//

// mask to check if any buttons are down
#define ACTION_DOWN_MASK ~(UI_MOUSE_RTIMEOUT|UI_MOUSE_LTIMEOUT|UI_MOUSE_CTIMEOUT)

#pragma off(unreferenced)
bool buttongadg_process_mouse(ButtonGadg* bg, Rect* rect, uiMouseEvent* ev)
{
   int i;
   Rect abs_r;
   Region* reg = &ButtonGadgRegion(bg);
   ulong state = bg->state;
   GadgEvent signal = *(GadgEvent*)ev;
   ushort action = ev->action;
   bool retval = FALSE;
   bool in_rect = FALSE;
   bool redraw = FALSE;
   bool expose = FALSE;

   signal.type = UI_EVENT_USER_DEFINED;
   signal.signaller = BUTTONGADG_SIGNALLER;
   signal.action = 0;


   // If the mouse is outside the button rectangle, pop it up.
   region_abs_rect(reg,rect,&abs_r);
   if (!RECT_TEST_PT(&abs_r,ev->pos))
   {
      // pop the button up while mouse is outside its 
      // region.
      state = dsNORMAL;
      in_rect = FALSE;
      // no new down events
      action &= ~ACTION_DOWN_MASK;
      redraw = TRUE;
      if (bg->focus)
      {
         uiReleaseFocus(reg,UI_EVENT_MOUSE_MOVE);
         bg->focus = FALSE;
      }
   }
   else
   {
      in_rect = TRUE;
      retval = TRUE;
      if (state != dsDEPRESSED)
      {
         action |= bg->last_action & ACTION_DOWN_MASK;
      }
      if (!bg->focus)
      {
         bg->focus = TRUE;
         uiGrabFocus(reg,UI_EVENT_MOUSE_MOVE);
      }   
   }


   // process each mousebutton 
   for (i = 0; i < NUM_MOUSE_BTNS; i++)
   {
      // deal with downs
      ushort mask = action & (MOUSE_BTN2DOWN(i)|UI_MOUSE_BTN2DOUBLE(i));
      if (mask != 0)
      {
         state = dsDEPRESSED;

         bg->last_action |= mask;
      }
      // deal with ups
      if (action & (MOUSE_BTN2UP(i)))
      {
         // set the appropriate event bits in the signal 
         // event.  Note that this code expoits the fact that
         // the button gadget action field is analagous to the 
         // mouse event action field. 

         if (state == dsDEPRESSED)
         {
            mask  = bg->last_action & (MOUSE_BTN2DOWN(i)|UI_MOUSE_BTN2DOUBLE(i));

            // add bits to the signal event, converting UI to GADG bits
            if (mask & UI_MOUSE_BTN2DOUBLE(i))
               signal.action |= BGADG_BTN2DOUBLE(i);
            if (mask & MOUSE_BTN2DOWN(i))
               signal.action |= BGADG_BTN2CLICK(i);

            // permit timeouts to be reported.
            if (bg->last_action & (MOUSE_BTN2DOWN(i)))
               bg->last_action |= UI_MOUSE_BTN2TIMEOUT(i);

            guiStylePlaySound(VB(bg)->style,StyleSoundSelect);
         }
         // clear them from last_action
         bg->last_action &= ~(MOUSE_BTN2DOWN(i)|UI_MOUSE_BTN2DOUBLE(i));
         if ((bg->last_action & ACTION_DOWN_MASK) == 0)
            state = dsNORMAL;
      }
      // deal with timeouts
      if (action & bg->last_action & UI_MOUSE_BTN2TIMEOUT(i))
      {
         signal.action |= UI_MOUSE_BTN2TIMEOUT(i);
         bg->last_action &= ~UI_MOUSE_BTN2TIMEOUT(i);
      }
   }

   // did the buttons come up...
   if (ev->buttons == 0 && !in_rect)
   {
      state = dsNORMAL;
      bg->last_action &= ~ACTION_DOWN_MASK;
   }

   // change state to "attention" if we're in the rect
   if (state != dsDEPRESSED && in_rect)
   {
      if (state != dsATTENTION)
         guiStylePlaySound(VB(bg)->style,StyleSoundTick);
      state = dsATTENTION;
      redraw = TRUE;
   }

   if (bg->flags & LGADBUTT_FLAG_GETDOWNS) // add down events to signal
      signal.action |= ev->action & (MOUSE_LDOWN|MOUSE_RDOWN|MOUSE_CDOWN);

   // decide whether the button should redraw now
   if ((state == dsDEPRESSED && 
       bg->state != dsDEPRESSED )
       || (redraw && state != bg->state))
   {
      if (signal.action != 0)
      {
         SEQUENCE_START();
         expose = TRUE;
      }
      bg->state = state;
      region_expose(reg,rect);
      redraw =TRUE;
   }
   else
      bg->state = state;

   // if the signal event is at all interesting, send it out.
   if (signal.action != 0)
   {
//      spw(("Signalling action %x\n",signal.action));
      uiDispatchEventToRegion((uiEvent*)&signal,reg);
   }


   if (expose)
      SEQUENCE_END();

   return retval;
}


//-------------------------------------------------
// the mouse handler

#pragma off(unreferenced)
bool buttongadg_mouse_handler(uiEvent* ev, Region* reg, void* data)
{
   ButtonGadg* bg = (ButtonGadg*)reg;
   ConfigSpew("uibutton",("Got a mouse event with mask %x\n",ev->subtype));
   return buttongadg_process_mouse(bg,reg->r,(uiMouseEvent*)ev);
}


//==================================
// AT LAST, THE BUTTON API FUNCTIONS

errtype ButtonGadgInit(Region* parent, ButtonGadg* bg, Rect* rect, int z, ButtonDisplay* dp)
{
   errtype err;
   int id;
   LGadCreateBoxInternal(VB(bg),parent,rect->ul.x,rect->ul.y,RectWidth(rect),
      RectHeight(rect),NULL,NULL,buttongadg_draw_func,0);
   //   set_region_expose(&ButtonGadgRegion(bg),buttongadg_expose_func);
   err = uiInstallRegionHandler(&VB(bg)->r,UI_EVENT_MOUSE|UI_EVENT_MOUSE_MOVE,(uiHandlerProc)buttongadg_mouse_handler,NULL,&id);
   if (err != OK) return err;
   if (dp != NULL)
      bg->draw = *dp;
   else
      memset(&bg->draw,0,sizeof(bg->draw));
   bg->last_action = 0;
   bg->state = dsNORMAL;
   bg->focus = FALSE;
   bg->flags = 0;
   return OK;
}

errtype ButtonGadgDestroy(ButtonGadg* bg)
{
   if (bg->focus)
      uiReleaseFocus(&ButtonGadgRegion(bg),UI_EVENT_MOUSE_MOVE);
   return LGadDestroyBox(VB(bg),FALSE);
}

//===============================================
// Toggle button implementation 
//


bool toggle_event_handler(GadgEvent* ev, Region* reg, void* data)
{
   ButtonToggle* bt = (ButtonToggle*)reg;
   int newstate = *bt->statevar;
   if (ev->signaller != BUTTONGADG_SIGNALLER)
      return FALSE;
   if (ev->action & (BUTTONGADG_LCLICK|BUTTONGADG_LDOUBLE))
   {
      newstate++;
      if (newstate >= bt->n) newstate = 0;
   }
   if (ev->action & (BUTTONGADG_RCLICK|BUTTONGADG_RDOUBLE))
   {
      newstate--;
      if (newstate < 0) newstate = bt->n - 1;
   }
   if (newstate != *bt->statevar)
   {
      ToggleEvent tev = *(ToggleEvent*)ev;
      tev.signaller = BUTTONTOGGLE_SIGNALLER;
      tev.new_state = newstate;

      *bt->statevar = newstate;

      SEQUENCE_START();
      uiDispatchEventToRegion((uiEvent*)&tev,reg);
      bt->button.draw = bt->dispvec[*bt->statevar];
      region_expose(reg,reg->r);
      SEQUENCE_END();
   }
   return FALSE;
}


errtype ButtonToggleInit(ButtonToggle* bt, int n, int* statevar, ButtonDisplay* dispvec)
{
   int id;
   if (*statevar >= n || *statevar < 0)
      *statevar = 0;
   bt->button.draw = dispvec[*statevar];
   bt->statevar = statevar;
   bt->dispvec = dispvec;
   bt->n = n;
   uiInstallRegionHandler(&ButtonGadgRegion(&bt->button),UI_EVENT_USER_DEFINED,(uiHandlerProc)toggle_event_handler,NULL,&id);
   return OK;
}

errtype ButtonToggleDestroy(ButtonToggle* bt)
{
   return ButtonGadgDestroy(&bt->button);
}



//********************************************************
// BUTTON LIST IMPLEMENTATION 


#define NO_CURRBUTTON -1

#define DOWN KB_FLAG_DOWN

// @TBD: get this to work 
#define GOOD_KEY_BUTT(list,butt) TRUE

int find_next_button(ButtonList* list, Point dir)
{
   int button = list->key_button;
   int best = button;
   uint bestweight = 0xFFFFFFFF;
   int i;
   Rect *us = &list->rvec[button];
   Point mid = MakePoint((us->ul.x + us->lr.x)/2,
                         (us->ul.y + us->lr.y)/2);

   for (i = 0; i < list->num_buttons; i++)
      if (i != button) // && list->dvec[i].type != BUTTON_NULL_TYPE)
      {
         Rect* them = &list->rvec[i];
         Point tmid = MakePoint((them->ul.x + them->lr.x)/2,
                               (them->ul.y + them->lr.y)/2);
         short dx = tmid.x - mid.x;
         short dy = tmid.y - mid.y;
         int dotprod = dx*dir.x + dy*dir.y;
         int dist = abs(dx) + abs(dy);

         // are we headed in the general direction
         if (2*dotprod > dist)
         {
            int weight = (dx*dx+dy*dy)/dotprod;
            if (weight < bestweight)
            {
               best = i;
//                mprintf("best weight is %d dotprod %d dist %d\n",weight,dotprod,dist);
               bestweight = weight;
            }
         }
      }

   // No good candidates?  pick a nearby button in the list
   if (best == button)
   {
      do
         best = (best + dir.x + list->num_buttons)%list->num_buttons;
      while(!GOOD_KEY_BUTT(list,best) && best != button);
   }
   if (best == button)
   {
      do
         best = (best + dir.y + list->num_buttons)%list->num_buttons;
      while(!GOOD_KEY_BUTT(list,best) && best != button);
   }

   return best;
}

Point warp_mouse_to_button(ButtonList* list, int button)
{
   Region* reg = &ButtonListRegion(list);
   Rect r = list->rvec[button];
   Point pos;

   mouse_get_xy(&pos.x,&pos.y);
   region_abs_rect(reg,&r,&r);
   if (!RECT_TEST_PT(&r,pos))
   {
      pos.x = (r.ul.x + r.lr.x)/2;
      pos.y = (r.ul.y + r.lr.y)/2;
      uiHideMouse(NULL);
      mouse_put_xy(pos.x,pos.y);
      uiShowMouse(NULL);
   }
   return pos;
}

static bool spoof_mouse(ButtonList* list, bool down)
{
   ushort mask = (down) ? MOUSE_LDOWN : MOUSE_LUP;
   uiMouseEvent ev;

   warp_mouse_to_button(list,list->key_button);
   uiMakeMotionEvent(&ev);
   ev.type = UI_EVENT_MOUSE;
   ev.action = mask;
   if (down) ev.buttons |= (1 << MOUSE_LBUTTON);
   else ev.buttons &= ~(1 << MOUSE_LBUTTON);
   return uiDispatchEventToRegion((uiEvent*)&ev,&ButtonListRegion(list));
}

void new_key_button(ButtonList* list, int button)
{
   Region* reg = &ButtonListRegion(list);

   if (list->key_button == button)
      return;


   if (list->key_button >= 0)
   {
//      list->dvec[list->key_button].flags &= ~BUTTON_SELECTED_FLAG;
      region_expose(reg,&list->rvec[list->key_button]);
   }

   list->key_button = button;

   if (list->key_button >= 0)
   {
//      list->dvec[list->key_button].flags |= BUTTON_SELECTED_FLAG;
      region_expose(reg,&list->rvec[list->key_button]);
   }
   warp_mouse_to_button(list,button);
      
}

bool buttonlist_keyboard_handler(uiEvent* _ev, Region* reg, void* data)
{
   bool retval = FALSE;
   uiCookedKeyEvent* ev = (uiCookedKeyEvent*)_ev;
   ButtonList* list = (ButtonList*)reg;
   int button = list->key_button;

   if (button < 0) return FALSE;

   switch(ev->code & ~KB_FLAG_2ND)
   {
      case KEY_PAD_UP|DOWN:
         button = find_next_button(list,MakePoint(0,-1));
         break;
      case KEY_PAD_LEFT|DOWN:
         button = find_next_button(list,MakePoint(-1,0));
         break;
      case KEY_PAD_RIGHT|DOWN:
         button = find_next_button(list,MakePoint(1,0));
         break;
      case KEY_PAD_DOWN|DOWN:
         button = find_next_button(list,MakePoint(0,1));
         break;
      case KEY_TAB|DOWN:
      {
         int newbutt = button;
         do
            newbutt = (newbutt + 1) % list->num_buttons;
         while (!GOOD_KEY_BUTT(list,newbutt) && newbutt != button);
         button = newbutt;
      }
      break;

      case KEY_ENTER|DOWN:
      case KEY_ENTER:
         retval = spoof_mouse(list,(ev->code & DOWN) != 0);
         break;
   }
   if (list->key_button != button)
   {
      new_key_button(list,button);
      retval = TRUE;
   }
   return retval;
}

#pragma off(unreferenced)
bool buttonlist_mask_func(LGadBox* reg, Rect* r, void* data)
{
   ButtonList* list = (ButtonList*)reg;
   int i;
   for (i = 0; i < list->num_buttons; i++)
      if (RectTestSect(&list->rvec[i],r))
         return TRUE;
   return FALSE;
}
#pragma on(unreferenced)

static void send_to_button(ButtonList* list, uiMouseEvent* ev)
{
   int butt = list->curr_button;
   if (butt < 0)
      return;
   list->gadg.state = list->statevec[butt] & ~BLIST_DOWN_BIT;
   buttongadg_process_mouse(&list->gadg,&list->rvec[butt],ev);
   list->statevec[butt] &= BLIST_DOWN_BIT;
   list->statevec[butt] |= list->gadg.state;
}

bool buttonlist_mouse_handler(uiEvent* _ev, Region* reg, void* data)
{
   bool retval = FALSE;
   ButtonList* list = (ButtonList*)reg;

   ConfigSpew("uibutton",("Got a mouse event with mask %x\n",_ev->subtype));
   
   if (_ev->type & (UI_EVENT_MOUSE_MOVE|UI_EVENT_MOUSE))
   {
      uiMouseEvent *ev = (uiMouseEvent*)_ev;
      Point pos = (reg->parent != NULL)
         ? MakePoint(ev->pos.x - reg->parent->abs_x,
                     ev->pos.y - reg->parent->abs_y)
         : ev->pos;
      int i;
      int newbutt = -1;

      for (i = 0; i < list->num_buttons; i++)
      {
         if (RECT_TEST_PT(&list->rvec[i],pos))
         {
            newbutt = i;
            break;
         }
      }

      SEQUENCE_START();
      if (newbutt != list->curr_button)
      {
         send_to_button(list,ev);
         list->curr_button = newbutt;
      }
      send_to_button(list,ev);
      SEQUENCE_END();
   }
   else if (_ev->type == UI_EVENT_USER_DEFINED)
   {
      GadgEvent* ev = (GadgEvent*)_ev;
      if (ev->signaller == BUTTONGADG_SIGNALLER)
      {
         ButtonListEvent signal = *(ButtonListEvent*)ev;
         ushort action = ev->action;
         int button = list->curr_button; 
         bool send = TRUE;

         SEQUENCE_START();
         if (action & (BUTTONGADG_LCLICK|BUTTONGADG_RCLICK|BUTTONGADG_CCLICK))
         {
            if (list->flags & BLIST_RADIO_FLAG)
            {
               if (ButtonListButtonDown(list,button))
                  send = FALSE;  // already down, no send message
               else
               {
                  list->statevec[list->last_button] = dsNORMAL;
                  list->statevec[button] |= BLIST_DOWN_BIT;
                  region_expose(reg,&list->rvec[button]);
                  region_expose(reg,&list->rvec[list->last_button]);
               }
            }
            list->last_button = button;
         }
         
         if (send)
         {
            signal.signaller = BUTTONLIST_SIGNALLER;
            signal.button = button;
            uiDispatchEventToRegion((uiEvent*)&signal,reg);
         }
         SEQUENCE_END();
      }
   }
   return retval;
}


bool buttonlist_draw_func(void* data, LGadBox* box)
{ 
   ButtonList* list = (ButtonList*)box;
   int i;
   for (i = 0; i < list->num_buttons; i++)
   {
      Rect r = list->rvec[i];
      Rect clip;
      Point ul = BOX_RECT(box)->ul;
      DrawElemState state = list->statevec[i] & BLIST_DSTATE_MASK;
      if (list->statevec[i] & BLIST_DOWN_BIT)
         state = dsDEPRESSED;
      RECT_MOVE(&r,MakePoint(-ul.x,-ul.y));
      
      clip.ul = MakePoint(gr_get_clip_l(),gr_get_clip_t());
      clip.lr = MakePoint(gr_get_clip_r(),gr_get_clip_b());
      
      if (RECT_TEST_SECT(&r,&clip))
         ElementDraw(&list->dvec[i],state,
                     r.ul.x, r.ul.y, RectWidth(&r),RectHeight(&r));
   }
   return TRUE;

}



errtype ButtonListInit(Region* parent, ButtonList* list, int z, int n, Rect rvec[], ButtonDisplay dvec[],ulong flags)
{
   errtype err;
   int id;
   Rect listrect;

   int i;

   list->statevec = Malloc(sizeof(list->statevec[0])*n);

   for (i = 0; i < n; i++)
   {
      list->statevec[i] = dsNORMAL;
      if (i == 0)
         listrect = rvec[i];
      else
         RECT_UNION(&listrect,&rvec[i],&listrect);
   }
 
   LGadCreateBoxInternal(&list->gadg.box,parent,listrect.ul.x,listrect.ul.y,
      RectWidth(&listrect),RectHeight(&listrect),NULL,NULL,buttonlist_draw_func,0);
   
   //   set_region_expose(&ButtonListRegion(list),buttongadg_expose_func);
   err = uiInstallRegionHandler(&ButtonListRegion(list),UI_EVENT_MOUSE|UI_EVENT_MOUSE_MOVE|UI_EVENT_USER_DEFINED,(uiHandlerProc)buttonlist_mouse_handler,NULL,&id);
   if (err != OK) return err;
   err = uiInstallRegionHandler(&ButtonListRegion(list),UI_EVENT_KBD_COOKED,(uiHandlerProc)buttonlist_keyboard_handler,NULL,&id);

   LGadBoxSetMask(VB(list),buttonlist_mask_func,NULL);

   if (err != OK) return err;
   //   uiGrabFocus(&ButtonListRegion(list),UI_EVENT_MOUSE_MOVE);
   list->gadg.last_action = 0;
   list->gadg.state = dsNORMAL;
   list->gadg.focus = FALSE;
   list->num_buttons = n;
   list->focused = FALSE;
   list->dvec = dvec;
   list->rvec = rvec;
   list->curr_button = NO_CURRBUTTON;
   list->key_button = -1;
   list->last_button = 0;
   list->flags = flags;
   if (flags & BLIST_RADIO_FLAG)
      RadioButtonSetSelection(list,0);
   return OK;
}


errtype RadioButtonSetSelection(ButtonList* list, int new_sel)
{
   if (list == NULL) return ERR_NULL;
   if (new_sel < 0  || new_sel >= list->num_buttons) return ERR_RANGE;
   list->statevec[list->last_button] = dsNORMAL;
   list->statevec[new_sel] = BLIST_DOWN_BIT|dsDEPRESSED;
   list->last_button = new_sel;
   return OK;
}

errtype ButtonListDestroy(ButtonList* list)
{
   //   uiReleaseFocus(&ButtonListRegion(list),UI_EVENT_MOUSE_MOVE);
   Free(list->statevec);
   list->statevec = 0;
   return ButtonGadgDestroy(&list->gadg);
}

errtype ButtonListEnableKeyboard(ButtonList* list, uint def, bool warp)
{
   Region* reg = &ButtonListRegion(list);
   list->key_button = def % list->num_buttons;
//   list->dvec[list->key_button].flags |= BUTTON_SELECTED_FLAG;
   if (warp)
   {
      Rect r = list->rvec[list->key_button];
      short x,y;
      region_abs_rect(reg,&r,&r);
      x = (r.ul.x + r.lr.x)/2;
      y = (r.ul.y + r.lr.y)/2;

      uiHideMouse(NULL);
      mouse_put_xy(x,y);
      uiShowMouse(NULL);
   }
   uiGrabFocus(&ButtonListRegion(list),UI_EVENT_KBD_COOKED);
   return OK;
}




////////////////////////////////////////////////////////////


void ButtonDisplayDims(ButtonDisplay* disp, Rect* dims)
{
   sshort x,y,w,h;
   ElementOffset(disp,&x,&y);
   dims->ul = MakePoint(-x,-y);
   ElementSize(disp,&w,&h);
   dims->lr = MakePoint(w-x,w-y);
}

void BuildButtonRects(ButtonDisplay* dvec, int n, Rect bounds, Rect* rvec)
{
   int i;
   Point spacing = {0,0}; // vertical spacing between buttons
   Point margin = {0,0}; // margin within each button
   Point m_dims =  { 1, 0};  // matrix dimensions.
   Point cursor = bounds.ul;
   Point size = { 0, 0};  // total text size (vertical)
   int colwid = 0;
   int colhgt = 0;
   int maxwid = 0;
   int rest;
   int row;

   //------------------------------
   // COMPUTE SPACING AND MARGIN

   // Compute and store the dims of each display in rvec (temporarily)
   // figuring out Rows and Columns on the way.


   for (i = 0,row = 0; i < n; i++)       
   {
      Rect dims;
      short w,h;

      ButtonDisplayDims(&dvec[i],&dims);
      h = 2*max(abs(dims.ul.y),abs(dims.lr.y));
      w = 2*max(abs(dims.ul.x),abs(dims.lr.x));

      rvec[i].ul = MakePoint(w,h);  // stuff w,h into rvec for later

      maxwid = max(maxwid,w);
      size.y = max(colhgt,size.y);

      if (colhgt*3 > RectHeight(&bounds)) // do we need to start a new column
      {
         m_dims.y = max(m_dims.y,row);

         // Stuff colwid at the end of rvec: 
         rvec[m_dims.x-1].lr.x = colwid;
         m_dims.x++;

         colhgt = h;
         colwid = w;
         row = 0;
         continue;
      }
      row++;
      colhgt += h;
      colwid = max(colwid,w);
   }


   if (m_dims.x == 1)
   {
      size.y = colhgt;
      m_dims.y = n;  // 1xN matrix
      // stuff the column width
   }
   rvec[m_dims.x-1].lr.x = colwid;

   // copy column widths to the tops of their columns
   // note we also move from x to y to avoid stomping
   for (i = 0,size.x = 0; i < m_dims.x; i++)
   {
      rvec[m_dims.y*i].lr.y = rvec[i].lr.x;
      size.x += rvec[i].lr.x;
   }

   // devote half of the space between text to spacing
   // and the other half to ymargin
   rest = RectHeight(&bounds) - size.y;
   margin.y = min(rest/(2*m_dims.y),2*size.y/m_dims.y);
   if (m_dims.y > 1)
      spacing.y = min((rest+1)/(2*(m_dims.y-1)),margin.y/2);
   else
      spacing.y = 0;

   // Now do it in X, with a slightly differen mathematical esthetic
   rest = RectWidth(&bounds) - size.x;
   margin.x = min(rest/(2*m_dims.x),size.x/m_dims.x);
   if (m_dims.x > 1)
      spacing.x = min((rest+1)/(2*(m_dims.x-1)),margin.x/4);
   else
      spacing.x = 0;

   bounds.ul.y += (RectHeight(&bounds) - spacing.y*(m_dims.y-1) - size.y - margin.y*m_dims.y)/2;
   bounds.ul.x += ( RectWidth(&bounds) - spacing.x*(m_dims.x-1) - size.x - margin.x*m_dims.x)/2;

   cursor = bounds.ul;

   colwid = rvec[0].lr.y;

   for (i = 0,row = 0; i < n; i++)
   {
      short w = rvec[i].ul.x;  // Rip out width and height just in time
      short h = rvec[i].ul.y;

      rvec[i].ul = cursor;

      if ((w | h) == 0)
      {
         rvec[i].lr = cursor;
         continue;
      }

      rvec[i].lr = MakePoint(cursor.x + margin.x + colwid, cursor.y + margin.y + h);
      row++;

      if (row >= m_dims.y) // are we done with this column? 
      {
         cursor.y = bounds.ul.y;  // start at the top again                       
         cursor.x += colwid + margin.x + spacing.x;    // move over a column
         row = 0;  // reset the row counter
         colwid = rvec[i+1].lr.y;  // rip out the width of the column;
      }
      else
      {
         cursor.y = rvec[i].lr.y + spacing.y;
      }
   }
      
}
