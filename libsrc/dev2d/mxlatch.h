// $Header: x:/prj/tech/libsrc/dev2d/RCS/mxlatch.h 1.4 1997/01/29 11:29:10 KEVIN Exp $
//
// Declarations and macros for mode X latch manipulation.

#ifndef __MXLATCH_H
#define __MXLATCH_H
#include <conio.h>
#include <vgareg.h>
#include <nameconv.h>

#ifdef __cplusplus
extern "C" {
#endif

extern volatile uchar modex_wlatch;
extern volatile uchar modex_rlatch;
extern volatile int wlatch_semaphore;
extern volatile int rlatch_semaphore;
extern uchar modex_lmask[];
extern uchar modex_rmask[];

#define modex_force_wlatch(m) \
   outpw ((ushort )SRX_ADR, (ushort )(SR_MAP | ((modex_wlatch=(m))<<8)))

#define modex_set_wlatch(m)  \
do {                         \
   if (modex_wlatch != (m))  \
      modex_force_wlatch(m); \
} while (0)

#define modex_force_rlatch(m) \
   outpw ((ushort )GRX_ADR, (ushort )(GR_MAP | ((modex_rlatch=(m))<<8)))

#define modex_set_rlatch(m)  \
do {                         \
   if (modex_rlatch != (m))  \
      modex_force_rlatch(m); \
} while (0)

#define wlatch_start(ws) \
do {                     \
   ws = modex_wlatch;    \
   wlatch_semaphore++;   \
} while (0)

#define rlatch_start(rs) \
do {                     \
   rs = modex_rlatch;    \
   rlatch_semaphore++;   \
} while (0)

#define wlatch_restore(ws) \
do {                          \
   wlatch_semaphore--;        \
   if (wlatch_semaphore)      \
      modex_force_wlatch(ws); \
} while (0)

#define rlatch_restore(rs) \
do {                          \
   rlatch_semaphore--;        \
   if (rlatch_semaphore)      \
      modex_force_rlatch(rs); \
} while (0)

#ifdef __cplusplus
};
#endif
#endif /* !__MXLATCH */
