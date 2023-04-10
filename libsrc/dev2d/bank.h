/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/bank.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:41:23 $
 *
 * Declarations for bank-switching.  Lots of gruesome stuff
 * to make things efficient, but still allow drawing in interrupt.
 *
 * This file is part of the dev2d library.
 *
 */

#ifndef __BANK_H
#define __BANK_H

#include <grd.h>
#include <idevice.h>

#ifdef __cplusplus
extern "C" {
#endif

extern volatile int gdd_bank;
extern volatile int gdd_ignore_bank;
extern volatile int gdd_save_bank;

// a pointer to the function that actually sets the bank
#define gd_force_bank \
         ((void (*)(int bank))grd_device_table[GDC_SET_BANK])

// gd_inc_bank sets the semaphore to indicate gdd_bank is unreliable,
// increments and sets the bank, and resets the unreliability semaphore.
#define gd_inc_bank() \
   do {                          \
      gdd_ignore_bank++;         \
      gd_force_bank(++gdd_bank); \
      gdd_ignore_bank--;         \
   } while (0)

// gd_set_bank sets a semaphore to indicate that the bank must be
// restored by any primitive subsequently called (in interrupt, 
// generally).
// use exactly once per primitive.  always use gd_restore_bank
// before exiting.

#define gd_set_bank(b) \
   do {                         \
      gdd_save_bank++;          \
      if (gdd_ignore_bank!=0) { \
         gdd_bank = b;          \
         gd_force_bank(b);      \
         break;                 \
      }                         \
      if (gdd_bank != b) {      \
         gdd_ignore_bank++;     \
         gdd_bank = b;          \
         gd_force_bank(b);      \
         gdd_ignore_bank--;     \
      }                         \
   } while (0)

// gd_restore_bank decrements the save semaphore and resets
// the bank when necessary.

#define gd_restore_bank(sb) \
   do {                             \
      gdd_save_bank--;              \
      if ((gdd_save_bank!=0)&&      \
          (sb!=gdd_bank)) {         \
         gdd_bank = sb;             \
         gd_force_bank(gdd_bank);   \
      }                             \
   } while (0)                      

/* calculates a video memory pointer from offset p. */
#define gd_bank_p(p) \
   ((uchar *) grd_cap->vbase+((p)&0xffff))

#ifdef __cplusplus
};
#endif
#endif /* !__BANK_H */
