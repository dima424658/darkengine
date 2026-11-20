/*
 * $Source: x:/prj/tech/winsrc/input/RCS/kbdecl.h $
 * $Revision: 1.9 $
 * $Author: TOML $
 * $Date: 1996/11/07 14:39:47 $
 *
 * Declarations for keyboard library.
 *
 * $Log: kbdecl.h $
 * Revision 1.9  1996/11/07  14:39:47  TOML
 * Improved thread support
 * 
 * Revision 1.8  1996/09/23  16:58:24  TOML
 * Made C++ parser friendly
 *
 * Revision 1.7  1996/01/25  14:20:14  DAVET
 * Added cplusplus stuff
 *
 * Revision 1.6  1996/01/25  14:04:47  DAVET
 * Added standard #ifdef __NAME_H stuff
 *
 * Revision 1.5  1996/01/19  11:21:53  JACOBSON
 * Initial revision
 *
 * Revision 1.4  1994/02/12  18:21:29  kaboom
 * Moved event structure.
 *
 * Revision 1.3  1993/04/29  17:19:59  mahk
 * added kb_get_cooked
 *
 * Revision 1.2  1993/04/28  17:01:48  mahk
 * Added kb_flush_bios
 *
 * Revision 1.1  1993/03/10  17:16:41  kaboom
 * Initial revision
 *
 */

#ifndef __KBDECL_H
#define __KBDECL_H

#ifdef __cplusplus
extern "C"  {
#endif  // cplusplus

extern uchar kb_state(uchar code);

extern int kb_startup(void *init_buf);
extern int kb_shutdown(void);

extern kbs_event kb_next(void);
extern kbs_event kb_look_next(void);
extern void kb_flush(void);
extern uchar kb_get_state(uchar kb_code);
extern void kb_clear_state(uchar kb_code, uchar bits);
extern void kb_set_state(uchar kb_code, uchar bits);
extern void kb_set_signal(uchar code, uchar int_no);
extern int kb_get_flags();
extern void kb_set_flags(int flags);
extern void kb_generate(kbs_event e);
extern void kb_flush_bios(void);
extern bool kb_get_cooked(ushort* key);
EXTERN HANDLE kb_get_queue_available_signal(void);

#ifdef __cplusplus
}
#endif  // cplusplus

#endif  // __KBDECL_H