//		Keys.H		Keycode defines (cooked keys only)
//		Rex E. Bradford (REX)
/*
* $Header: x:/prj/tech/libsrc/input/RCS/keydefs.h 1.2 1996/01/25 14:20:38 DAVET Exp $
* $Log: keydefs.h $
 * Revision 1.2  1996/01/25  14:20:38  DAVET
 * Added cplusplus stuff
 * 
 * Revision 1.1  1994/12/06  01:33:18  lmfeeney
 * Initial revision
 * 
 * Revision 1.1  1993/04/27  18:10:02  rex
 * Initial revision
 * 
 * Revision 1.1  1993/04/27  18:08:55  rex
 * Initial revision
 * 
 * Revision 1.1  1993/04/27  18:01:15  rex
 * Initial revision
 * 
*/

#ifndef KEYDEFS_H
#define KEYDEFS_H

#include "kbcook.h"

#ifdef __cplusplus
extern "C"  {
#endif  // cplusplus


//	Ascii keys with handy names

#define KEY_BS		0x08
#define KEY_TAB	0x09
#define KEY_ENTER	0x0D
#define KEY_ESC	0x1B
#define KEY_SPACE	0x20

//	Function keys

#define KEY_F1		(0x3B | KB_FLAG_SPECIAL)
#define KEY_F2		(0x3C | KB_FLAG_SPECIAL)
#define KEY_F3		(0x3D | KB_FLAG_SPECIAL)
#define KEY_F4		(0x3E | KB_FLAG_SPECIAL)
#define KEY_F5		(0x3F | KB_FLAG_SPECIAL)
#define KEY_F6		(0x40 | KB_FLAG_SPECIAL)
#define KEY_F7		(0x41 | KB_FLAG_SPECIAL)
#define KEY_F8		(0x42 | KB_FLAG_SPECIAL)
#define KEY_F9		(0x43 | KB_FLAG_SPECIAL)
#define KEY_F10	(0x44 | KB_FLAG_SPECIAL)
#define KEY_F11	(0x57 | KB_FLAG_SPECIAL)
#define KEY_F12	(0x58 | KB_FLAG_SPECIAL)

//	Other special keys on top row

#define KEY_PRNTSCRN (0x37 | KB_FLAG_2ND | KB_FLAG_SPECIAL)
#define KEY_SCRLK	(0x46 | KB_FLAG_SPECIAL)
#define KEY_PAUSE (0x7F | KB_FLAG_SPECIAL)

//	Grey keys between regular keyboard and numeric keypad

#define KEY_INS	(0x52 | KB_FLAG_SPECIAL | KB_FLAG_2ND)
#define KEY_DEL	(0x53 | KB_FLAG_SPECIAL | KB_FLAG_2ND)
#define KEY_HOME	(0x47 | KB_FLAG_SPECIAL | KB_FLAG_2ND)
#define KEY_END	(0x4F | KB_FLAG_SPECIAL | KB_FLAG_2ND)
#define KEY_PGUP	(0x49 | KB_FLAG_SPECIAL | KB_FLAG_2ND)
#define KEY_PGDN	(0x51 | KB_FLAG_SPECIAL | KB_FLAG_2ND)

#define KEY_LEFT	(0x4B | KB_FLAG_SPECIAL | KB_FLAG_2ND)
#define KEY_RIGHT	(0x4D | KB_FLAG_SPECIAL | KB_FLAG_2ND)
#define KEY_UP		(0x48 | KB_FLAG_SPECIAL | KB_FLAG_2ND)
#define KEY_DOWN	(0x50 | KB_FLAG_SPECIAL | KB_FLAG_2ND)

//	Grey keys on numeric keypad

#define KEY_GREY_SLASH	('/' | KB_FLAG_2ND)
#define KEY_GREY_STAR	('*' | KB_FLAG_2ND)
#define KEY_GREY_PLUS	('+' | KB_FLAG_2ND)
#define KEY_GREY_MINUS	('-' | KB_FLAG_2ND)
#define KEY_GREY_ENTER	(KEY_ENTER | KB_FLAG_2ND)

//	Other keys on numeric keypad

#define KEY_PAD_HOME	(0x47 | KB_FLAG_SPECIAL)
#define KEY_PAD_UP	(0x48 | KB_FLAG_SPECIAL)
#define KEY_PAD_PGUP	(0x49 | KB_FLAG_SPECIAL)
#define KEY_PAD_LEFT	(0x4B | KB_FLAG_SPECIAL)
#define KEY_PAD_CENTER (0x4C | KB_FLAG_SPECIAL)
#define KEY_PAD_RIGHT (0x4D | KB_FLAG_SPECIAL)
#define KEY_PAD_END	(0x4F | KB_FLAG_SPECIAL)
#define KEY_PAD_DOWN	(0x50 | KB_FLAG_SPECIAL)
#define KEY_PAD_PGDN	(0x51 | KB_FLAG_SPECIAL)
#define KEY_PAD_INS	(0x52 | KB_FLAG_SPECIAL)
#define KEY_PAD_DEL	(0x53 | KB_FLAG_SPECIAL)


#ifdef __cplusplus
}
#endif  // cplusplus

#endif

