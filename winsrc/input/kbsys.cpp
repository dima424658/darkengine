/*
 * $Source: x:/prj/tech/winsrc/input/RCS/kbsys.cpp $
 * $Revision: 1.13 $
 * $Author: JAEMZ $
 * $Date: 1998/01/06 18:33:28 $
 *
 */

#include <windows.h>
#include <lg.h>
#include <kb.h>
#include <kbstate.h>
#include <kbarray.h>
#include <kbscan.h>
#include <inputimp.h>
#include <inpcompo.h>
#include <wappapi.h>
#include <gshelapi.h>
#include <thrdtool.h>
#include <fixedque.h>

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

EXTERN int kbd_modifier_state;

///////////////////////////////////////////////////////////////////////////////
//
// The key queue
//

#define kMaxKeyEvents 64
typedef cFixedMTQueue<kbs_event, kMaxKeyEvents> tKeyEventQueue;

tKeyEventQueue g_KeyEventQueue;

char _kbd_array_raw[256];
char *kbd_array=&_kbd_array_raw[0];

kbs_event kb_null_event={0xff,0x00};

// @TBD (toml 06-14-96): Need to clean this up by adding functionality to input devices interface
// @TBD (toml 11-22-96): changed constants -- no L/R distinguising: something in windows to investigate
static int KBCToVK(uchar code)
    {
    switch(code)
        {
        case KBC_PAUSE:     return VK_PAUSE;
        case KBC_LSHIFT:    return VK_SHIFT; //VK_LSHIFT;
        case KBC_RSHIFT:    return VK_SHIFT; //VK_RSHIFT;
        case KBC_LCTRL:     return VK_CONTROL; //VK_LCONTROL;
        case KBC_RCTRL:     return VK_CONTROL; //VK_RCONTROL;
        case KBC_LALT:      return VK_MENU; //VK_LMENU;
        case KBC_RALT:      return VK_MENU; //VK_RMENU;
        case KBC_CAPS:      return VK_CAPITAL;
        case KBC_NUM:       return VK_NUMLOCK;
        case KBC_SCROLL:    return VK_SCROLL;
        }
    return 0;
    }

uchar _kb_state(uchar code)                      { return ((GetAsyncKeyState(KBCToVK(code)) & 0x8000) >> 15); }
uchar _kb_get_state(uchar kb_code)               { return _kb_state(kb_code); }
void   kb_clear_state(uchar kb_code, uchar bits) { return; }
void   kb_set_state(uchar kb_code, uchar bits)   { return; }
void   kb_set_signal(uchar code, uchar int_no)   { /*kbd_array[code]|=KBA_SIGNAL;*/ }
int   _kb_get_flags(void)                        { return 0; }
void   kb_set_flags(int flags)                   { return; }
void   kb_generate(kbs_event e)                  { return; }
void   kb_flush_bios(void)                       { return; }
int    kb_set_leds(int led_bits)                 { return -1; }

// really in kbcook
// bool kb_get_cooked(ushort* key)                 { return FALSE; }

#define kb_queue_advance(kqv) kqv=(kqv+1)&MAX_QUEUE_MASK

void kb_flush(void)
{
    if (g_KeyEventQueue.IsEmpty())
    {
        if (g_pInputGameShell)
            g_pInputGameShell->PumpEvents(kPumpAll);
        else if (g_pInputWinApp)
            g_pInputWinApp->PumpEvents(kPumpAll, kPumpUntilEmpty);
    }

    ushort key;
    while (_kb_get_cooked(&key))
        ;
}

void kb_queue_init(void)
{
   kb_flush();
}

EXTERN
bool kb_add_event(kbs_event new_event)
{
   return g_KeyEventQueue.AddUnlessFull(&new_event);
}

void _kb_look_next(kbs_event & ret)
{
   if (g_KeyEventQueue.IsEmpty())
   {
      if (g_pInputGameShell)
          g_pInputGameShell->PumpEvents(kPumpAll);
      else if (g_pInputWinApp)
          g_pInputWinApp->PumpEvents(kPumpAll, kPumpUntilEmpty);
   }

   if (!g_KeyEventQueue.PeekNext(&ret))
   {
      ret = kb_null_event;
   }
}

void _kb_next(kbs_event & ret)
{
   if (g_KeyEventQueue.IsEmpty())
   {
      if (g_pInputGameShell)
          g_pInputGameShell->PumpEvents(kPumpAll);
      else if (g_pInputWinApp)
          g_pInputWinApp->PumpEvents(kPumpAll, kPumpUntilEmpty);
   }

   if (!g_KeyEventQueue.GetNext(&ret))
   {
      ret = kb_null_event;
   }
}

EXTERN HANDLE kb_get_queue_available_signal(void)
{
    return g_KeyEventQueue.GetAvailabilitySignalHandle();
}

