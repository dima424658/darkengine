/*
 * $Source: x:/prj/tech/libsrc/input/RCS/kbmod.h $
 * $Revision: 1.4 $
 * $Author: TOML $
 * $Date: 1996/09/23 16:59:00 $
 *
 * Constants for keyboard state modifiers.
 *
 * This file is part of the input library.
 */

#ifndef __KBMOD_H
#define __KBMOD_H

#ifdef __cplusplus
extern "C"  {
#endif  // cplusplus

#define KBM_SCROLL   0x0001
#define KBM_NUM      0x0002
#define KBM_CAPS     0x0004
#define KBM_LSHIFT   0x0008
#define KBM_RSHIFT   0x0010
#define KBM_LCTRL    0x0020
#define KBM_RCTRL    0x0040
#define KBM_LALT     0x0080
#define KBM_RALT     0x0100
#define KBM_LED_MASK 7

#define KBM_CAPS_SHF  2
#define KBM_SHIFT_SHF 3
#define KBM_CTRL_SHF  5
#define KBM_ALT_SHF   7


#ifdef __cplusplus
}
#endif  // cplusplus

#endif  // __KBMOD_H
