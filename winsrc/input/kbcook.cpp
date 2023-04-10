/*
 * $Source: x:/prj/tech/winsrc/input/RCS/kbcook.cpp $
 * $Revision: 1.20 $
 * $Author: JAEMZ $
 * $Date: 1998/01/08 18:24:13 $
 *
 * Routines to convert raw scan codes to more useful cooked codes.
 *
 * This file is part of the input library.
 */

#ifdef WIN32
#include <windows.h>
#endif

#include <lg.h>
#include <error.h>
#include <kbcook.h>
#include <kbmod.h>
#include <kbscan.h>
#include <inputimp.h>

#include <windows.h>
#include <comtools.h>
#include <appagg.h>
#include <wappapi.h>

//
// The Watcom C++ parser is verbose in warning of possible integral
// truncation, even in cases where for the given native word size
// there is no problem (i.e., assigning longs to  ints).  Here, we
// quiet the warnings, although it wouldn't be bad for someone to
// evaluate them. (toml 09-14-96)
//
#ifdef __WATCOMC__
#pragma warning 389 9
#endif

extern int kb_set_leds(int led_bits);
extern bool _kb_get_cooked(ushort* key);

/* bitfields of shift/alt/control etc keys. */
extern "C" {
int kbd_modifier_state;
};

/* This cooks kbc codes into ui codes which include ascii stuff. */

errtype kb_cook_real(kbs_event ev, ushort *cooked, bool *results, bool all_special)
{
   int old_mods = kbd_modifier_state;
   ushort cnv = 0;
   ushort flags = KB_CNV(ev.code,0);
   bool  right_alt = FALSE;   // set if right_alt is down and key is amenable

   // shifted if either shift is pressed
   bool shifted = 1 & ((kbd_modifier_state >> KBM_SHIFT_SHF)
      | (kbd_modifier_state >> (KBM_SHIFT_SHF+1)));

   // capslock if capslock is down and the key is amenable to it
   // zero entry contains that info
   bool capslock = 1 & (flags >> CNV_CAPS_SHF)
                        & (kbd_modifier_state >> KBM_CAPS_SHF);

   if (!all_special)
   {
      // If right-alt key is down, see if its zero or not
      if ( 1&(kbd_modifier_state >> (KBM_ALT_SHF + 1))) {
         cnv = KB_CNV(ev.code,2);
         right_alt = (cnv != 0);
      }

      // get ascii code if right-alt unsupported, xoring shift and capslock
      if (!right_alt) cnv = KB_CNV(ev.code,shifted ^ capslock);
   }
   else
   {
      cnv = ev.code|KB_FLAG_SPECIAL|KB_FLAG_SHIFT|KB_FLAG_ALT|KB_FLAG_CTRL;
      shifted ^= capslock;
   }

   // keep the full ascii code, whether its special, or 2nd
   *cooked = cnv & (CNV_SPECIAL|CNV_2ND|0xFF)  ;
   *results = FALSE;

   // if an up event, use negative logic.  Wacky
   if (ev.state == KBS_UP) kbd_modifier_state = ~kbd_modifier_state;
   switch(ev.code)  // check for modifiers
   {
   case 0x7a:
      return 0;

   case KBC_LSHIFT:
      kbd_modifier_state |= KBM_LSHIFT;
      break;
   case KBC_RSHIFT:
      kbd_modifier_state |= KBM_RSHIFT;
      break;
   case KBC_LCTRL:

      kbd_modifier_state |= KBM_LCTRL;
      break;
   case KBC_RCTRL:
      kbd_modifier_state |= KBM_RCTRL;
      break;
   case KBC_CAPS:
      // toggle caps lock
      if (ev.state == KBS_DOWN)
         kbd_modifier_state ^= KBM_CAPS;
      break;
   case KBC_NUM:
      // toggle num lock
      if (ev.state == KBS_DOWN)
        kbd_modifier_state ^= KBM_NUM;
      break;
   case KBC_SCROLL:
      // toggle scroll lock
      if (ev.state == KBS_DOWN)
         kbd_modifier_state ^= KBM_SCROLL;
      break;
   case KBC_LALT:
      kbd_modifier_state |= KBM_LALT;
      break;
   case KBC_RALT:
      kbd_modifier_state |= KBM_RALT;
      break;
   default:
      *results = TRUE;  // Not a modifier key, we must translate.
      break;
   }
   if (ev.state == KBS_UP) kbd_modifier_state = ~kbd_modifier_state;
   if ((kbd_modifier_state&KBM_LED_MASK) != (old_mods&KBM_LED_MASK))
      kb_set_leds(kbd_modifier_state&KBM_LED_MASK);
   if (!*results) return OK;

   // Special if amenable to numlock and numlock is not locked
   if ((cnv & CNV_NUM) && !(kbd_modifier_state & KBM_NUM))
      *cooked = ev.code|KB_FLAG_SPECIAL;

   // whether its pressed down or up
   *cooked |= (short)ev.state << KB_DOWN_SHF;

   // The French os creates control and alt key events when you
   // hit the right alt key for that character.  Hence, we set
   // neither control nor alt keys in such a case
   if (!right_alt) {

      // Set control if amenable to control
      *cooked |= (((kbd_modifier_state << (KB_CTRL_SHF - KBM_CTRL_SHF))
         | (kbd_modifier_state << (KB_CTRL_SHF - KBM_CTRL_SHF-1)))
         & KB_FLAG_CTRL) & cnv;

      // Set alt if amenable to alt and alt keys are down
      *cooked |= (((kbd_modifier_state << (KB_ALT_SHF - KBM_ALT_SHF))
         | (kbd_modifier_state << (KB_ALT_SHF - KBM_ALT_SHF-1)))
            & KB_FLAG_ALT) & cnv;
   }

   // if KB_FLAG_SPECIAL is set, then let set the shifted
   // flag according to shifted
   *cooked |= (shifted << KB_SHIFT_SHF) & cnv;

   return OK;
}

bool _kb_get_cooked(ushort *key)
{
   bool res = FALSE;
   kbs_event ev;
   _kb_next(ev);
   if (ev.code != KBC_NONE)
   {
      kb_cook(ev,key,&res);
      res = TRUE;
   }
   return res;
}
