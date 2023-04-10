/*
 * $Source: x:/prj/tech/libsrc/input/RCS/kbwin.cpp $
 * $Revision: 1.3 $
 * $Author: TOML $
 * $Date: 1996/10/10 16:50:07 $
 */

#include <windows.h>

#include <kbs.h>
#include <kbstate.h>
#include <kbscan.h>

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

EXTERN
bool kb_add_event(kbs_event new_event);

/* message codes. */
#define KBC_ACK          0xfa
#define KBC_RESEND       0xfe

kbs_event new_event;

/* Simple crackers to easily interpret your window message */
bool was_key_down(long msg_long)
{
	return(msg_long & 0x40000000);
}

bool is_key_extended(long msg_long)
{
	return(msg_long & 0x01000000);
}

uchar get_key_state(long msg_long)
{
	if(msg_long & 0x80000000) return(KBS_UP);
		else return(KBS_DOWN);
}

uchar get_key_repeat_count(long msg_long)
{
	return((uchar)msg_long & 0x0000FFFF);
}

bool is_alt_down(long msg_long)
{
	return((bool)msg_long & 0x20000000);
}

uchar get_key_scancode(long msg_long)
{
	return((uchar)((msg_long & 0x00FF0000) >> 16));
}

#define dumpme(str)

/* Example cracking switch */
bool kb_winkeys_to_lgkeys(uint msg_type, long msg_long)
{
	uchar kbstate;
	uchar kbscancode;
	bool kbrepeat;
	bool kbextended;
	bool kbalt;
	unsigned int repeat_count;

	/* This pulls all the cool information out of the long */
	kbextended = is_key_extended(msg_long);
	kbstate = get_key_state(msg_long);
	repeat_count = get_key_repeat_count(msg_long);
	kbalt = is_alt_down(msg_long);
	kbscancode = get_key_scancode(msg_long);

	/* This function needs to be able to translate "kbscancode" to the
	   wierd asctab format */
	//winscancode_to_lgscancode(kbscancode);

	/* This handles repeat key info, and dumps what type of message we got */
	switch(msg_type)
	{
	case WM_SYSKEYDOWN:
		kbrepeat = was_key_down(msg_long);
		dumpme("WM_SYSKEYDOWN");
		break;

	case WM_SYSKEYUP:
		kbrepeat = FALSE;
		dumpme("WM_SYSKEYUP");
		break;

	case WM_KEYDOWN:
		kbrepeat = was_key_down(msg_long);
		dumpme("WM_KEYDOWN");
		break;

	case WM_KEYUP:
		kbrepeat = FALSE;
		dumpme("WM_KEYUP");
		break;

	default:
		return(FALSE);
	}

#ifdef GO_TO_SCR
	/* This will dump all the shit to the screen for easy viewing */
	wsprintf(dumpy, "SCAN CODE: %x", kbscancode);
	dumpme(dumpy);

	if(kbextended) dumpme("  EXTENDED");
	if(kbrepeat)
	{
		wsprintf(dumpy, "  REPEATED: %d", repeat_count);
		dumpme(dumpy);
	}
	if(kbstate == KBS_DOWN) dumpme("  KEYDOWN"); else dumpme("  KEYUP");
	if(kbalt == TRUE) dumpme("  ALTED");
#endif

   new_event.code=kbscancode;
   if (kbextended)
      new_event.code|=0xE0;
   new_event.state=kbstate;
   kb_add_event(new_event);

	return(TRUE);
}

