/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/cyclegad.c,v 1.21 2000/02/22 22:33:31 bfarquha Exp $

#include <stdio.h>
#include <string.h>

#include <gadget.h>
#include <drawelem.h>
#include <kbcook.h>
#include <keydefs.h>
#include <gadtext.h>

#define _IMPLEMENT_CYCLEGADG
#include <cyclegad.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

//
// Gadget callbacks
//

void UpdateCycleGadg(CycleGadg* gadg,bool redraw)
{
   bool is_f=FALSE;
   char* f = gadg->desc.format_string;
   char* buf = LGadTextBoxText(&gadg->text);

   TriggerCycleGadg(gadg,CYCLE_MAKE_ACTION(kCycleActionUpdate,redraw),kCycleCenterPart);

   switch(f[strlen(f)-1])
   {
      case 'e':
      case 'E':
      case 'f':
      case 'F':
      case 'g':
      case 'G':
         sprintf(buf,f,*(float*)gadg->desc.var);
         break;
      case 's':
         sprintf(buf,f,(char*)gadg->desc.var);
         break;
      default:
         if (strlen(f)>1)
            if (f[strlen(f)-2]=='h')
            {
               is_f=TRUE;
               sprintf(buf,f,*(short*)gadg->desc.var);
            }
         if (!is_f)
            sprintf(buf,f,*(int*)gadg->desc.var);
         break;
   }
   LGadUpdateTextBox(&gadg->text);
   if (redraw)
   {
      //      LGadTextBoxSetFlag(&gadg->text,TEXTBOX_EDIT_BRANDNEW);
      LGadTextBoxSetCursor(&gadg->text,strlen(buf));
      LGadUnfocusTextBox(&gadg->text);
      LGadDrawBox(VB(gadg),NULL);
   }
}

static char format[3] = "%d";

void read_text_buf(CycleGadg* gadg)
{
   char* f = gadg->desc.format_string;
//   char* format = "%d";
   char* buf = LGadTextBoxText(&gadg->text);
   char last = f[strlen(f)-1];
   int delta = 0;

   // if there is no associated text, leave it unchanged
   if (strlen(buf) == 0)
      return;

   format[1] = last;

   if ((strncmp(buf,"++",2) == 0) || (strncmp(buf,"--",2) == 0))
      delta = 1;
   if (delta)
   {
      int i;
      // remove the bogus leading sign
      for (i=0; i < strlen(buf) - 1; i++)
         buf[i] = buf[i+1];
      buf[strlen(buf)-1] = '\0';
   }

   switch(last)
   {
      case 'e':
      case 'E':
      case 'f':
      case 'F':
      case 'g':
      case 'G':
         {
            float var;
            sscanf(buf,format,&var); // (float*)gadg->desc.var);
            if (delta)
               *((float *)gadg->desc.var) = (var) + *((float *)gadg->desc.var);
            else
               *((float *)gadg->desc.var) = var;
         }
         break;
      case 's':
         sscanf(buf,format,(char*)gadg->desc.var);
         break;
      default:
         {
            int var;
            sscanf(buf,format,&var); // (float*)gadg->desc.var);
            if (delta)
               *((int *)gadg->desc.var) = (var) + *((int *)gadg->desc.var);
            else
               *((int *)gadg->desc.var) = var;
            sscanf(buf,format,(int*)gadg->desc.var);
         }
         break;
   }
}


bool cycle_button_list_cb(ushort action, int button, void* data, LGadBox* vb)
{
   bool retval = FALSE;
   CycleGadg* gadg = (CycleGadg*)data;
   if (gadg->desc.cb != NULL)
   {
      ulong act = CYCLE_MAKE_ACTION(kCycleActionMouse,action);
      retval = gadg->desc.cb(gadg,act,button,gadg->desc.var,&gadg->desc);
      if (retval)
         UpdateCycleGadg(gadg,TRUE);
   }
   
   return retval;
}


bool TriggerCycleGadg(CycleGadg* gadg, ulong action, eCyclePart part)
{
   if (gadg->desc.cb != NULL)
   {
      return gadg->desc.cb(gadg,action,part,gadg->desc.var,&gadg->desc);
   }
   return FALSE;
}

static bool cycle_do_key(CycleGadg* gadg, int key)
{
   bool update = TRUE;
   switch(key)
   {
      case KB_FLAG_DOWN|KEY_ENTER:   
         read_text_buf(gadg);
         break;
      case KB_FLAG_DOWN|KEY_ESC:
         break;
      case KB_FLAG_DOWN|KEY_TAB:
      case KB_FLAG_SHIFT|KB_FLAG_DOWN|KEY_TAB:
         // lock in our current data
         read_text_buf(gadg);
         // and then advance to the next
         if (gadg->desc.next != NULL)
         {
            LGadTextBox *tb;
            if (key == (KB_FLAG_DOWN|KEY_TAB))
               tb = &(gadg->desc.next->text);
            else
               tb = &(gadg->desc.prev->text);
            LGadTextBoxSetCursor(tb,0);
            LGadFocusTextBox(tb);
            LGadTextBoxSetFlag(tb,TEXTBOX_EDIT_BRANDNEW);
            LGadDrawBox(VB(tb),NULL);
         }         
         break;
   } 
   return update;
}

bool cycle_textbox_callback(LGadTextBox* box, LGadTextBoxEvent event, int eventdata, void* user_data)
{
   bool update = FALSE;  // whether to reload text from var
   bool signal = FALSE;  // whether to send to client (update implies signal)
   ushort kind;
   CycleGadg* gadg = (CycleGadg*)user_data;

   switch(event)
   {
   case TEXTBOX_BUTTON:
      if (gadg->desc.flags & CYCLE_NONPOTENT_FLAG)
         signal = TRUE;
      kind = kCycleActionMouse;
      break;
   case TEXTBOX_SPECKEY:
      update = cycle_do_key(gadg, eventdata);
      kind = kCycleActionKey;
      break;
   default:
      return FALSE;
   }

   if (update || signal)
   {
      ulong action = CYCLE_MAKE_ACTION(kind,eventdata);
      if (gadg->desc.cb != NULL)
         signal = gadg->desc.cb(gadg,action,kCycleCenterPart,gadg->desc.var,&gadg->desc);
      if (update || signal)
         UpdateCycleGadg(gadg,TRUE);
   }
   return update;
}

//
// Create the gadget
//

static short special_keys[] =
{
   KEY_ESC|KB_FLAG_DOWN,
   KEY_ENTER|KB_FLAG_DOWN,
   KEY_TAB|KB_FLAG_DOWN,
   KEY_TAB|KB_FLAG_DOWN|KB_FLAG_SHIFT,
   0,
};

#define TEXT_BUFLEN 64

CycleGadg* CreateCycleGadg(CycleGadg* gadg, LGadRoot* root, CycleGadgDesc* desc)
{
   int i;
   Point arrow_dims[kCycleNumArrows]; 
   short totwid;
   Rect editarea;
   short gw = RectWidth(&desc->bounds);
   short gh = RectHeight(&desc->bounds);

   if (gadg == NULL)
   {
      gadg = Malloc(sizeof(CycleGadg));
      AssertMsg(gadg != NULL, "Failed to alloc a CycleGadg");
      memset(gadg,0,sizeof(CycleGadg));
      gadg->free_self = TRUE;
   }
   else 
      gadg->free_self = FALSE;

   if (root == NULL)
      root = LGadCurrentRoot();

   gadg->desc = *desc;

   LGadSetupSubRoot(&gadg->root, root, desc->bounds.ul.x, desc->bounds.ul.y,gw,gh);

   for (i = 0, totwid = 0; i < kCycleNumArrows; i++)
   {
      short w,h;
      DrawElement* elem = &desc->arrow_drawelems[i];
      ElementSize(elem,&w,&h);
      totwid += w;
      arrow_dims[i] = MakePoint(w,h);
   }

   editarea.ul.y = 0;
   editarea.lr.y = gh;
   {
      Point dims = arrow_dims[kCycleLeftArrow];
      Rect* r = &gadg->arrow_rects[kCycleLeftArrow];
      //      r->ul = MakePoint(0,max(0,(gh-dims.y)/2));
      //      r->lr = MakePoint(dims.x,r->ul.y+dims.y);
      r->ul = MakePoint(0,0);
      r->lr = MakePoint(dims.x,gh);
      editarea.ul.x = r->lr.x;
   }

   {
      Point dims = arrow_dims[kCycleRightArrow];
      Rect* r = &gadg->arrow_rects[kCycleRightArrow];
      //      r->ul = MakePoint(gw - dims.x,max(0,(gh-dims.y)/2));
      //      r->lr = MakePoint(gw,r->ul.y+dims.y);
      r->ul = MakePoint(gw - dims.x,0);
      r->lr = MakePoint(gw,gh);
      editarea.lr.x = r->ul.x;
   }

   
   {
      LGadButtonListDesc listdesc = { kCycleNumArrows, };

      listdesc.button_rects = gadg->arrow_rects;
      listdesc.button_elems = gadg->desc.arrow_drawelems;
      listdesc.cb = cycle_button_list_cb;

      LGadCreateButtonListDesc(&gadg->list, &gadg->root, &listdesc);
      LGadBoxSetUserData(VB(&gadg->list),gadg);
   }

   {
      LGadTextBoxDesc tdesc;
      bool edit = desc->flags & CYCLE_EDIT_FLAG;

      memset(&tdesc,0,sizeof(tdesc));
      tdesc.bounds = editarea; 
      tdesc.editbuf = Malloc(TEXT_BUFLEN);
      tdesc.editbuflen = TEXT_BUFLEN;
      tdesc.cb = cycle_textbox_callback;
      tdesc.data = gadg;
      tdesc.flags = TEXTBOX_BORDER_FLAG|((desc->flags >> CYCLE_TEXTBOX_SHF) & CYCLE_TEXTBOX_MASK);
      tdesc.editbuf[0] = '\0';
      LGadCreateTextBoxDesc(&gadg->text,&gadg->root,&tdesc);
      LGadTextBoxSetFlag(&gadg->text,(desc->flags >> CYCLE_EDIT_SHF) & CYCLE_EDIT_MASK);
      LGadTextBoxSetSpecialKeys(&gadg->text,special_keys);

      LGadTextBoxClrFlag(&gadg->text,TEXTBOX_EDIT_BRANDNEW);
      if (edit) 
      {
         if (desc->flags & CYCLE_FOCUS_FLAG)
            LGadTextBoxClrFlag(&gadg->text,TEXTBOX_EDIT_EDITABLE);
         else
            LGadTextBoxSetFlag(&gadg->text,TEXTBOX_EDIT_EDITABLE|TEXTBOX_EDIT_BRANDNEW);
      }
      LGadTextBoxSetCursor(&gadg->text,strlen(tdesc.editbuf));
   }
   UpdateCycleGadg(gadg,FALSE);

   return gadg;
}


void DestroyCycleGadg(CycleGadg* gadg)
{
   AssertMsg(gadg != NULL, "Tried to destroy a NULL cyclegadg");
   LGadDestroyButtonList(&gadg->list);
   LGadDestroyTextBox(&gadg->text);
   Free(LGadTextBoxText(&gadg->text));
   LGadDestroyRoot(&gadg->root);
   if (gadg->free_self)
      Free(gadg);
}


//////////////////////////////////////////////////////////////
// STRING CYCLEGADG
//

typedef struct _string_cycle_data
{
   CycleGadgDesc desc;
   StringCycleGadgDesc sdesc;
   char buf[256];
} string_cycle_data;


bool string_cycle_cb(CycleGadg* gadg, ulong _action, eCyclePart part, void* _var, CycleGadgDesc* desc);




static void string_cycle_reparse(CycleGadg* gadg)
{
   string_cycle_data* data = (string_cycle_data*)gadg->desc.user_data;
   StringCycleGadgDesc* sdesc = (StringCycleGadgDesc*)&data->sdesc;
   int* var = (int*)data->desc.var;

   // fill the buffer from the string vector based on the new value
   strncpy(data->buf,sdesc->strings[*var],sizeof(data->buf));
   data->buf[sizeof(data->buf)-1] = '\0';
}

void RedescribeStringCycleGadg(CycleGadg* gadg, char** strings, int num)
{
   string_cycle_data* data = (string_cycle_data*)gadg->desc.user_data;
   StringCycleGadgDesc* sdesc = (StringCycleGadgDesc*)&data->sdesc;
   int* var = (int*)data->desc.var;
   sdesc->strings = strings;
   sdesc->num = num;
   *var %= num;

   // fill the buffer from the string vector based on the new value
   strncpy(data->buf,sdesc->strings[*var],sizeof(data->buf));
   data->buf[sizeof(data->buf)-1] = '\0';
}

void string_cycle_update(CycleGadg* gadg, ulong _action, eCyclePart part)
{
   CycleGadgDesc* desc = &gadg->desc;
   string_cycle_data* data = (string_cycle_data*)gadg->desc.user_data;
   int* var = (int*)data->desc.var;

   string_cycle_reparse(gadg);

   desc->user_data = data->desc.user_data;  // data data, your face is a mess
   desc->cb = data->desc.cb;
   desc->cb(gadg,_action,part,var,desc);
   desc->cb = string_cycle_cb;
   desc->user_data = data;
} 


void string_cycle_popup_menu(CycleGadg* gadg, Point pos, int item);

static bool string_do_key(CycleGadg* gadg, ushort key)
{
   bool update = FALSE;
   switch(key ^ KB_FLAG_DOWN)
   {
      case KEY_ENTER:
         update = TRUE;
         break;
   }
   return update;
}

bool string_cycle_cb(CycleGadg* gadg, ulong _action, eCyclePart part, void* _var, CycleGadgDesc* desc)
{
   ushort kind = CYCLE_ACTION_KIND(_action);
   ushort action = CYCLE_ACTION_DATA(_action);
   string_cycle_data* data = (string_cycle_data*)desc->user_data;
   StringCycleGadgDesc* sdesc = (StringCycleGadgDesc*)&data->sdesc;
   int* var = (int*)data->desc.var;
   bool update = FALSE;

   switch (kind)
   {
      case kCycleActionMouse:
         if (action & BUTTONGADG_LCLICK)
         {
            static int deltas[3] = { -1, 1, 0}; 
            *var += deltas[part]; 
            update = TRUE;
         }
         if (action & MOUSE_RDOWN)
         {
            Point mousepos;
            mouse_get_xy(&mousepos.x,&mousepos.y);
            string_cycle_popup_menu(gadg, mousepos, *var);            
         }
         break;
      case kCycleActionKey:
         update = string_do_key(gadg,action);
         break;
      case kCycleActionUpdate:
         update = TRUE;
         break;
   }
   if (update)
   {
      // wrap or clip
      if (sdesc->flags & STRING_CYCLE_WRAP)
      {
         while (*var < 0) *var += sdesc->num;
         *var %= sdesc->num;
      }
      else
         *var = max(0,min(*var,sdesc->num-1));

      string_cycle_update(gadg,_action, part);
   }
   return update;
}

CycleGadg* CreateStringCycleGadg(CycleGadg* gadg, LGadRoot* root, CycleGadgDesc* user_desc, 
                                 StringCycleGadgDesc* sdesc)
{
   CycleGadgDesc desc = *user_desc;
   string_cycle_data* sdata = Malloc(sizeof(*sdata));
   sdata->desc = *user_desc;
   sdata->sdesc = *sdesc;

   sdata->desc.cb = user_desc->cb;

   desc.cb = string_cycle_cb;
   desc.user_data = sdata;
   desc.var = &sdata->buf;

   // fill the buffer from the string vector based on the value
   strncpy(sdata->buf,sdesc->strings[*(int*)user_desc->var],sizeof(sdata->buf));
   sdata->buf[sizeof(sdata->buf)-1] = '\0';   

   if (desc.format_string == NULL)
      desc.format_string = "%s";

   if (sdesc->flags & STRING_CYCLE_MENU)
      desc.flags |= CYCLE_TDOWNS_FLAG;

   gadg = CreateCycleGadg(gadg,root,&desc);
   return gadg;
}

void DestroyStringCycleGadg(CycleGadg* gadg)
{
   void* sdata = gadg->desc.user_data;
   DestroyCycleGadg(gadg);
   Free(sdata);
}

void* StringCycleGadgData(CycleGadg* gadg)
{
   string_cycle_data* data = (string_cycle_data*)gadg->desc.user_data;
   return data->desc.user_data;
}


//----------------------------------------
// String cyclegadg menu
//

static bool string_menu_cb(int item, LGadMenu *menu)
{
   CycleGadg* gadg = (CycleGadg*)LGadBoxGetUserData(VB(menu));
   string_cycle_data* data = (string_cycle_data*)gadg->desc.user_data;
   ulong action = CYCLE_MAKE_ACTION(kCycleActionKey,KEY_ENTER|KB_FLAG_DOWN);
   int* var = (int*)data->desc.var; 

   if (item >= 0)
   {
      *var = item;
      string_cycle_update(gadg,action,kCycleCenterPart); 
   }
   return TRUE;   
}

void string_cycle_popup_menu(CycleGadg* gadg, Point pos, int item)
{
   string_cycle_data* data = (string_cycle_data*)gadg->desc.user_data;
   StringCycleGadgDesc* sdesc = (StringCycleGadgDesc*)&data->sdesc;
   LGadMenu* menu;
   DrawElement* elems = Malloc(sdesc->num*sizeof(*elems));
   int i;
   short mw =0, mh = 0; // menu dims
   short ew, eh;        // elem dims;
   uint drawflags = BORDER(DRAWFLAG_BORDER_OUTLINE);
   uint menuflags = MENU_GRAB_FOCUS | MENU_OUTER_DISMISS | MENU_ALLOC_ELEMS;
   Rect bounds = *LGadBoxRegion(gadg)->parent->r;
   region_abs_rect(LGadBoxRegion(gadg)->parent,&bounds,&bounds);
   
   for (i = 0; i < sdesc->num; i++)
   {
      ElementClear(&elems[i]);
      elems[i].draw_type = DRAWTYPE_TEXT;
      elems[i].draw_data = sdesc->strings[i]; 
   }

   ElementSize(&elems[0],&ew,&eh);   
   LGadMenuComputeSize(&mw,&mh,sdesc->num,sdesc->num,menuflags,elems,drawflags,0,0);
   // translate the menu down to be over item.
   pos.x -= ew/2;
   pos.y -= item*eh + eh/2;
   
   // clip the menu against the bounds.  
   pos.x = max(bounds.ul.x,min(pos.x,bounds.lr.x - mw));
   pos.y = max(bounds.ul.y,min(pos.y,bounds.lr.y - mh));

   menu = LGadCreateMenuArgs(NULL,NULL, pos.x, pos.y, mw, mh, sdesc->num,sdesc->num, 
            elems, string_menu_cb, 0, menuflags, drawflags, NULL, 0); 

   LGadBoxSetUserData(VB(menu),gadg);

   Free(elems);
}

