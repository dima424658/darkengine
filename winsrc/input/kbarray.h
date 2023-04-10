/*
 * $Source: x:/prj/tech/libsrc/input/RCS/kbarray.h $
 * $Revision: 1.2 $
 * $Author: DAVET $
 * $Date: 1996/01/25 14:20:11 $
 *
 * Definitions of bits in keyboard state array.
 */

#ifndef __KBARRAY_H
#define __KBARRAY_H

#ifdef __cplusplus
extern "C"  {
#endif  // cplusplus

extern char *kbd_array;

#define KBA_STATE       1
#define KBA_REPEAT      2
#define KBA_SIGNAL      4
#ifdef __cplusplus
}
#endif  // cplusplus

#endif /* !__KBARRAY_H */

