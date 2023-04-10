/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

extern "C" int kbd_modifier_state;

extern "C" int hack_for_kbd_state(void)
{
   return kbd_modifier_state;
}
