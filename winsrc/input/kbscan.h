/*
 * $Source: x:/prj/tech/libsrc/input/RCS/kbscan.h $
 * $Revision: 1.6 $
 * $Author: TOML $
 * $Date: 1996/09/23 16:59:01 $
 *
 * Constants for keyboard scan and mesasge codes.
 *
 * This file is part of the input library.
 */

/* scan codes. */

#ifndef __KBSCAN_H
#define __KBSCAN_H

#ifdef __cplusplus
extern "C"  {
#endif  // cplusplus

#define KBC_SHIFT_PREFIX 0xe0
#define KBC_PAUSE_PREFIX 0xe1
#define KBC_PAUSE_DOWN   0xe11d
#define KBC_PAUSE_UP     0xe19d
#define KBC_PRSCR_DOWN   0x2a
#define KBC_PRSCR_UP     0xaa

#define KBC_PAUSE        0x7f
#define KBC_NONE         0xff
#define KBC_LSHIFT       0x2A
#define KBC_RSHIFT       0x36
#define KBC_LCTRL        0x1D
#define KBC_RCTRL        0x9D
#define KBC_LALT         0x38
#define KBC_RALT         0xB8
#define KBC_CAPS         0x3A
#define KBC_NUM          0x45
#define KBC_SCROLL       0x46

/* message codes. */
#define KBC_ACK          0xfa
#define KBC_RESEND       0xfe

#ifdef __cplusplus
}
#endif  // cplusplus

#endif // __KBSCAN_H
