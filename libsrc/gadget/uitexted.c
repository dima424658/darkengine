/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

/*
 * $Source: r:/t2repos/thief2/libsrc/gadget/uitexted.c,v $
 * $Revision: 1.10 $
 * $Author: JAEMZ $
 * $Date: 1998/01/12 20:42:30 $
 *
 */

#include <string.h>

#include <lg.h>
#include <2d.h>
#include <gcompose.h>
#include <uitexted.h>
#include <keydefs.h>
#include <uigadget.h>
#include <mprintf.h>
#include <cfgdbg.h>
#include <gadgint.h>
#include <gadtext.h>




// --------------------------
// TEXT GADGET IMPLEMENTATION
// --------------------------

// ----------------------------------
// Expose function, draws the gadget.

#define ALIGN_DENOM 256
#define XMARGIN 2
#define YMARGIN 1

#define DRAW_STRING gr_string

/* soft characters for line wrapped text. */
#ifndef CHAR_SOFTCR
#define CHAR_SOFTCR  1
#endif
#ifndef CHAR_SOFTSP
#define CHAR_SOFTSP  2
#endif

#pragma off(unreferenced)
void textgadg_draw_call(void* data, LGadBox* box)
{
   short w,h;
   Region* reg = &box->r;
   TextGadg* g = (TextGadg*)box;
   Rect r;
   Rect hide;
   int tcol = guiStyleGetColor(box->style,StyleColorFG);
   GUIcompose compose;


   region_abs_rect(reg,reg->r,&r);
   hide = r;
   uiHideMouse(&hide);

   GUIsetup(&compose,&r,ComposeFlagClear,GUI_CANV_ANY);


   guiStyleSetupFont(box->style,StyleFontNormal);
   gr_font_string_wrap(gr_get_font(),g->text.buf,RectWidth(&r));
   gr_string_size(g->text.buf,&w,&h);

   if (g->edit & TEXTGADG_EDIT_BRANDNEW)
   {
      gr_set_fcolor(guiStyleGetColor(box->style,StyleColorHilite));
      gr_rect(XMARGIN,YMARGIN,w+XMARGIN,h+YMARGIN);
      tcol = guiStyleGetColor(box->style,StyleColorBG);
   }
   

   // compute alignment and draw
   {
      short x,y;
      int xa = ALIGN_DENOM/2,ya = ALIGN_DENOM/2; // alignment

#ifdef REAL_ALIGNMENT
      if (g->flags & TEXTGADG_ALIGN_TOP)
         ya = 0;
      if (g->flags & TEXTGADG_ALIGN_BOTTOM)
         ya = ALIGN_DENOM;
      if (g->flags & TEXTGADG_ALIGN_LEFT)
         xa = 0;
      if (g->flags & TEXTGADG_ALIGN_RIGHT)
         xa = ALIGN_DENOM;
#else
      xa = 0, ya = 0;
#endif

      x = XMARGIN + (RectWidth(&r)  - XMARGIN - w)*xa/ALIGN_DENOM;
      y = YMARGIN + (RectHeight(&r) - YMARGIN - h)*ya/ALIGN_DENOM;

      gr_set_fcolor(tcol);
      DRAW_STRING(g->text.buf,x,y);
   }

   if (g->flags & TEXTGADG_BORDER_FLAG)
   {
      gr_set_fcolor(guiStyleGetColor(box->style,StyleColorBorder));
      gr_box(0,0,RectWidth(&r),RectHeight(&r));
   }


   // draw the cursor
   if (g->edit&TEXTGADG_EDIT_EDITABLE)
   {
      short x,y;
      char stomp;
      char* s;

      stomp = g->text.buf[g->cursor];
      g->text.buf[g->cursor] = '\0';
      y = gr_string_height(g->text.buf)+YMARGIN;
      s = &g->text.buf[g->cursor];
      for (s--; s >= g->text.buf && *s != '\n' && *s != CHAR_SOFTCR; s--)
         ;
      s++;
//      mprintf("s = %s\n",s);
      x = gr_string_width(s)+XMARGIN;
      if (s[strlen(s)-1] == CHAR_SOFTSP)
         x+= gr_char_width(' ');
      gr_set_fcolor(guiStyleGetColor(box->style,StyleColorHilite));
      gr_vline(x,y-gr_string_height(s),y);
      g->text.buf[g->cursor] = stomp;
   }

   // check to see if we have an impending overflow.
   g->at_end = FALSE;
   if (gr_string_height(g->text.buf) + gr_string_height("X") > RectHeight(reg->r))
   {
      char *s;
      for (s = g->text.buf + strlen(g->text.buf) - 1;
           s >= g->text.buf && *s != '\n' && *s != CHAR_SOFTCR; s--)
            ;
      s++;
      if (gr_string_width(s) + gr_char_width('X') > RectWidth(reg->r))
         g->at_end = TRUE;
      ConfigSpew("overtext",("checking line %s for overflow\n",s));
   }
   else if (gr_string_height(g->text.buf) > RectHeight(reg->r))
   {
      g->at_end = TRUE;
   }


      
   gr_font_string_unwrap(g->text.buf);

   guiStyleCleanupFont(box->style,StyleFontNormal);

   GUIdone(&compose);
   uiShowMouse(&hide);

}
#pragma on(unreferenced)

//--------------------------------------
// Keyboard handler

#define tt_space_for_a_char(t) (!(t)->at_end && ((t)->last_char+2<(t)->text.len))
#define tt_at_last_char(t) ((t)->last_char == (t)->cursor)

// hello mother hello we doing on this fine day?
// is this a problem a one a char

// scan for a space/not space, return -1 if not found, starting at loc n_pos, return new loc
int _texttool_scan_space(TextGadg *t, int dir, bool look_for_empty, int n_pos)
{
   while ((n_pos>=0)&&(n_pos<t->last_char))
      if (isspace(t->text.buf[n_pos]))
         if (look_for_empty) return n_pos; else n_pos+=dir;
      else
         if (look_for_empty) n_pos+=dir; else return n_pos; 
//   if ((n_pos<=0)&&(dir==-1)) return 0;
   return -1;  // hmmm...
}

int _texttool_goto_end(TextGadg *t, int dir)
{
   if (dir>0) return t->last_char; else return 0;
}

static void tt_clear(TextGadg* t)
{
   t->cursor=0;
   t->text.buf[t->cursor]='\0';
   t->last_char=t->cursor;
   t->at_end = FALSE;
}

int _texttool_meta_space(TextGadg *t, int dir)  // spaces, characters, space
{
   int n_pos=t->cursor;
   if ((dir==-1)&&(n_pos==0)) return -1;
   if ((dir== 1)&&(tt_at_last_char(t))) return -1;
   if (dir<0) n_pos--; // ???? hmmm... need to back up past our currect so repeat ^left will work
   if ((n_pos=_texttool_scan_space(t,dir,FALSE,n_pos))!=-1)    // skip the initial space
      if ((n_pos=_texttool_scan_space(t,dir,TRUE,n_pos))!=-1)  // skip characters
		   if (dir>0)
		   {
            if ((n_pos=_texttool_scan_space(t,dir,FALSE,n_pos))!=-1)
               return n_pos;
         }
         else return n_pos+1;       // go back to letter 1 of the last word
   return _texttool_goto_end(t,dir);
}

int _texttool_take_inp(TextGadg *t, int icode)
{
   int can_do=1,max_do=0,tmp1,tmp2,lrep=t->rep_count;

//   mprintf("t->%d\n",t->edit);

   while (lrep-- > 0)
   {
      bool playsound = TRUE;
      switch (icode) // &~KB_FLAG_2ND)
      {
         case KEY_ESC:
            break;
         case KEY_ENTER:
         case KEY_GREY_ENTER:   
            guiStylePlaySound(VB(t)->style,StyleSoundSelect);

            playsound = FALSE;
            break;
         case KB_FLAG_CTRL|'u':  case KB_FLAG_CTRL|'U':
            t->rep_count*=4;
            return 0;         // dont allow rep_count to reset
         case KB_FLAG_CTRL|'g':  case KB_FLAG_CTRL|'G':
            t->rep_count=1;
            break;                             // sound
         case KB_FLAG_CTRL|'t':  case KB_FLAG_CTRL|'T':
            if (tt_at_last_char(t)) tmp1=2;    // switch the two previous characters
            else                    tmp1=1;    // switch current and last character
            // tmp1 is now where to start
            if (t->cursor>=tmp1)
            { 
               tmp2=t->text.buf[t->cursor-tmp1];
               t->text.buf[t->cursor-tmp1]=t->text.buf[t->cursor-tmp1+1];
               t->text.buf[t->cursor-tmp1+1]=tmp2;
               if (!tt_at_last_char(t)) t->cursor++;
            }
            else can_do=0;
            break;
         case KEY_TAB:                          case KB_FLAG_CTRL|'i':  case KB_FLAG_CTRL|'I':
            break;
         case KEY_LEFT:    case KEY_PAD_LEFT:   case KB_FLAG_CTRL|'b':  case KB_FLAG_CTRL|'B':
            if (t->cursor>0) t->cursor--; else can_do=0; break;
         case KB_FLAG_ALT|KEY_LEFT:    case KB_FLAG_ALT|KEY_PAD_LEFT:   case KB_FLAG_ALT|'b':  case KB_FLAG_ALT|'B':  case KB_FLAG_CTRL|KEY_LEFT:   case KB_FLAG_CTRL|KEY_PAD_LEFT:  
            if ((tmp1=_texttool_meta_space(t,-1))!=-1) t->cursor=tmp1; else can_do=0; break;
         case KEY_RIGHT:   case KEY_PAD_RIGHT:  case KB_FLAG_CTRL|'f':  case KB_FLAG_CTRL|'F':
            if (!tt_at_last_char(t))
               t->cursor++;
            else if (!tgadg_edit_flg(t,BRANDNEW))
               can_do=0;
            break;
         case KB_FLAG_ALT|KEY_RIGHT:   case KB_FLAG_ALT|KEY_PAD_RIGHT:  case KB_FLAG_ALT|'f':  case KB_FLAG_ALT|'F':  case KB_FLAG_CTRL|KEY_RIGHT:  case KB_FLAG_CTRL|KEY_PAD_RIGHT:  
            if ((tmp1=_texttool_meta_space(t, 1))!=-1) t->cursor=tmp1; else can_do=0; break;
         case KEY_UP:      case KEY_PAD_UP:     case KB_FLAG_CTRL|'p':  case KB_FLAG_CTRL|'P':
            break;
         case KEY_DOWN:    case KEY_PAD_DOWN:   case KB_FLAG_CTRL|'n':  case KB_FLAG_CTRL|'N':
            break;
         case KEY_HOME:    case KEY_PAD_HOME:   case KB_FLAG_CTRL|'a':  case KB_FLAG_CTRL|'A':
            t->cursor=0; break;
         case KEY_END:     case KEY_PAD_END:    case KB_FLAG_CTRL|'e':  case KB_FLAG_CTRL|'E':
            t->cursor=t->last_char; break;
         case KEY_INS:     case KEY_PAD_INS:    case KB_FLAG_ALT|'i':   case KB_FLAG_ALT|'I':
            if (t->edit&TEXTGADG_EDIT_OVERMODE) t->edit&=~TEXTGADG_EDIT_OVERMODE; else t->edit|=TEXTGADG_EDIT_OVERMODE;
            break;
         case KEY_DEL:     case KEY_PAD_DEL:    case KB_FLAG_CTRL|'d':  case KB_FLAG_CTRL|'D':
            if (tt_at_last_char(t))
               can_do=0;
            else
            {
               if (t->cursor==t->last_char-1)
                  t->text.buf[t->cursor]='\0';
               else
                  memmove(t->text.buf+t->cursor,t->text.buf+t->cursor+1,t->last_char-t->cursor);
               t->last_char--;
            }
            break;
         case KB_FLAG_CTRL|KEY_BS:
            if (t->cursor>0)
            {
               tmp1=_texttool_meta_space(t,-1);    // target loc
               tmp2=t->cursor-tmp1;                // punted characters
               memmove(t->text.buf+tmp1,t->text.buf+t->cursor,t->last_char-t->cursor+1);
               t->cursor=tmp1; t->last_char-=tmp2; // these changes had better be the same
            }
            else can_do=0;
            break;
         case KEY_BS:
            if (tgadg_edit_flg(t,BRANDNEW))
            {
               tt_clear(t);
            }
            else if (t->cursor>0)
            {
               if (tt_at_last_char(t))
                  t->text.buf[t->cursor-1]='\0';
               else
                  memmove(t->text.buf+t->cursor-1,t->text.buf+t->cursor,t->last_char-t->cursor+1);
               t->cursor--; t->last_char--;
               t->at_end = FALSE;
            }
            else can_do=0;
            break;
         case KB_FLAG_CTRL|'k':  case KB_FLAG_CTRL|'K':
            if (!tt_at_last_char(t))
            {
               t->text.buf[t->cursor]='\0';
               t->last_char=t->cursor;
            }
            break;
         default:
            if (((icode&(KB_FLAG_CTRL|KB_FLAG_ALT))==0)&&(kb2ascii(icode)))      // was isprint, but rejected foreign
            {
               if (tgadg_edit_flg(t,BRANDNEW))
               {
                  tt_clear(t);
               }
               if ((tgadg_edit_flg(t,NOSPACES))&&(isspace(kb2ascii(icode))))
                  can_do=0;
               else if (tt_space_for_a_char(t))
               {
                  if (!tgadg_edit_flg(t,OVERMODE))
                     if (!tt_at_last_char(t))
                        memmove(t->text.buf+t->cursor+1,t->text.buf+t->cursor,t->last_char-t->cursor+1);
                  t->text.buf[t->cursor]=kb2ascii(icode);
                  t->cursor++; t->last_char++;
                  if (tt_at_last_char(t))
                     t->text.buf[t->cursor]='\0';
               }
               else can_do=0;
            }
            else can_do=0;
            break;
      }
      max_do|=can_do;
      if (max_do)
      {
         t->edit&=~TEXTGADG_EDIT_BRANDNEW;
         if (playsound)
         {
            guiStylePlaySound(VB(t)->style,StyleSoundTick);
         }
      }
   }
   t->rep_count=1;      // look, we reset this to count 0
   return max_do;
}

static short special_keys[] =
{
   KEY_ESC|KB_FLAG_DOWN,
   KEY_ENTER|KB_FLAG_DOWN,
   0,
};


#pragma off(unreferenced)
bool textgadg_process_keybd(uiCookedKeyEvent* ev, Region* reg, TextGadg* gadg)
{
   bool retval = FALSE, update = FALSE, send_signal = FALSE;
   TextGadgEvent signal = *(TextGadgEvent*)ev;
   short code = ev->code;

   signal.signaller = TEXTED_SIGNALLER;
   signal.type = UI_EVENT_USER_DEFINED;

   // check for special keys
   if (gadg->speckeys != NULL)
   {
      short* key;
      for (key = gadg->speckeys;  *key; key++)
         if (*key == code)
         {
            signal.action = TEXTGADG_SPECKEY;
            signal.data = code;
            retval = send_signal = TRUE;
            break;
         }      
   }

   if ((gadg->edit&TEXTGADG_EDIT_EDITABLE) == 0)
      return FALSE;

   // Do we allow pre screening?
   // Make callback directly if so
   // let them screen out the keys they want or not...
   if (gadg->flags&TEXTGADG_PRESCREEN_FLAG)
   {
      LGadTextBox *tb = (LGadTextBox *)reg;
      bool eaten;
      void *data_arr[2];

      data_arr[0]=tb->data;
      data_arr[1]=&code;

      eaten = tb->cb(tb,TEXTGADG_SPECKEY_PRE,code,data_arr);
      if (eaten) return TRUE;
   }

   // set "update" for redraw, set "retval" if you did anything.
   if (code & KB_FLAG_DOWN)   // why doesnt this set retval, then
   {
      update=_texttool_take_inp(gadg,(code&~KB_FLAG_DOWN));
      retval=TRUE;
   }

#ifdef SEQUENCE
   region_begin_sequence();
#endif 
   if (send_signal)           // if there's a signal to send, send it.
      uiDispatchEventToRegion((uiEvent*)&signal,reg);
   if (update)
      region_expose(reg,reg->r);
#ifdef SEQUENCE
   region_end_sequence(TRUE);
#endif 
   return retval;
}
#pragma on(unreferenced)




bool textgadg_button_handler(uiEvent* ev, Region* reg, void* data)
{
   TextGadg* gadg = (TextGadg*)reg;
   GadgEvent* bge = (GadgEvent*)ev;
   TextGadgEvent signal = *(TextGadgEvent*)ev;
   
   if (bge->signaller != BUTTONGADG_SIGNALLER)
      return FALSE;
   if (bge->action & BUTTONGADG_LCLICK)
   {
      TextGadgClrFlag(gadg,TEXTGADG_EDIT_BRANDNEW);
      if (TextGadgFocus(gadg) == ERR_NOEFFECT)
         LGadDrawBox(VB(gadg),NULL);
   }
   signal.action = TEXTGADG_BUTTON;
   signal.signaller = TEXTED_SIGNALLER;
   signal.type = UI_EVENT_USER_DEFINED;
   signal.data = bge->action;

   uiDispatchEventToRegion((uiEvent*)&signal,reg);
   return FALSE;
}



//----------------------------------------
// TEXT GADGET API FUNCTIONS
errtype TextGadgInit(Region* parent, TextGadg* g, Rect* r, int z, char* buf, int buflen, ulong flags)
{
   errtype err;
   int id, tmp;
   DrawElement dummy;

   ElementClear(&dummy);
   // make the region

   ButtonGadgInit(parent,&g->butt,r,0,&dummy);
   // stuff the draw call.
   VB(g)->drawcall = textgadg_draw_call;


   // install the keyboard handler
   err = uiInstallRegionHandler(&VB(g)->r,UI_EVENT_KBD_COOKED,
      (uiHandlerProc)textgadg_process_keybd,g,&id);
   Assrt(err == OK);
   if (err != OK) return err;

   if (flags & TEXTGADG_FOCUS_FLAG)
   {
      err = uiInstallRegionHandler(&VB(g)->r,UI_EVENT_USER_DEFINED,
                                   textgadg_button_handler,NULL,&id);
      Assrt(err == OK);
      if (err != OK) return err;
   }
   if (flags & TEXTGADG_MOUSE_DOWNS)
      g->butt.flags = LGADBUTT_FLAG_GETDOWNS;
      
   // initialize state vars
   g->edit = TEXTGADG_EDIT_BRANDNEW;
   g->flags = flags;
   g->text.buf = buf;
   g->text.len = buflen;
   tmp=strlen(buf);
   g->cursor = g->last_char = tmp;
   g->rep_count = 1;
   g->speckeys = special_keys;
   return OK;
}

errtype TextGadgDestroy(TextGadg* g)
{
   TextGadgUnfocus(g);
   return ButtonGadgDestroy(&g->butt);
}

errtype TextGadgUpdate(TextGadg* gadg)
{
   gadg->last_char = strlen(gadg->text.buf);
   if (gadg->cursor > gadg->last_char)
      gadg->cursor = gadg->last_char;
   return OK;
}

static TextGadg* focused_gadg = NULL; 

errtype TextGadgFocus(TextGadg* gadg)
{
   if (gadg == focused_gadg) 
      return ERR_NOEFFECT;
   if (focused_gadg != NULL)
      TextGadgUnfocus(focused_gadg);
   uiGrabFocus(&VB(gadg)->r,UI_EVENT_KBD_COOKED);
   TextGadgSetFlag(gadg,TEXTGADG_EDIT_EDITABLE|TEXTGADG_EDIT_BRANDNEW);
   LGadDrawBox(VB(gadg),NULL);
   focused_gadg = gadg;
   return OK;
}

errtype TextGadgUnfocus(TextGadg* gadg)
{
   TextGadgEvent signal;
   if (gadg == NULL)
      gadg = focused_gadg;
   if (gadg != focused_gadg || gadg == NULL)
      return ERR_NOEFFECT;
   uiReleaseFocus(&VB(gadg)->r,UI_EVENT_KBD_COOKED);
   TextGadgClrFlag(gadg,TEXTGADG_EDIT_EDITABLE|TEXTGADG_EDIT_BRANDNEW);
   focused_gadg = NULL;

   // send an escape key.
   uiMakeMotionEvent((uiMouseEvent*)&signal);
   signal.signaller = TEXTED_SIGNALLER;
   signal.type = UI_EVENT_USER_DEFINED;
   signal.action = TEXTGADG_SPECKEY;
   signal.data = KB_FLAG_DOWN|KEY_ESC;

   uiDispatchEventToRegion((uiEvent*)&signal,&VB(gadg)->r);
   LGadDrawBox(VB(gadg),NULL);
   return OK;
}
