/*
 * $Source: x:/prj/tech/winsrc/input/RCS/kbs.h $
 * $Revision: 1.4 $
 * $Author: TOML $
 * $Date: 1996/11/05 12:33:21 $
 *
 * Types for keyboard system.
 *
 * This file is part of the input library.
 */

#ifndef __KBS_H
#define __KBS_H

#ifdef __cplusplus
extern "C"  {
#endif  // cplusplus


typedef struct kbs_event {
   uchar code;
   uchar state;
} kbs_event;

EXTERN kbs_event kb_null_event;

#ifdef __cplusplus
}
#endif  // cplusplus

#endif /* !__KBS_H */

