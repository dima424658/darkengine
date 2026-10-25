///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/input/RCS/inputimp.h $
// $Author: TOML $
// $Date: 1996/10/10 16:50:03 $
// $Revision: 1.2 $
//

#ifndef __INPUTIMP_H
#define __INPUTIMP_H

#include <error.h>

#ifdef __cplusplus
extern "C"  {
#endif  // cplusplus

// Below are the prototypes for all the functions to be recorded.
// If the below functions are called, it will bypass the recording system.

// Mouse functions
typedef struct _lgMouseEvent lgMouseEvent;
errtype _mouse_check_btn(int button, bool* result);
errtype _mouse_extremes( short *xmin, short *ymin, short *xmax, short *ymax);
errtype _mouse_get_rate(short* xr, short* yr, short* thold);
errtype _mouse_get_xy(short* x, short* y);
errtype _mouse_look_next(lgMouseEvent* result);
errtype _mouse_next(lgMouseEvent* result);

// Keyboard functions
uchar     _kb_state(uchar code);
void      _kb_next(kbs_event &);
void      _kb_look_next(kbs_event &);
uchar     _kb_get_state(uchar _kb_code);
int       _kb_get_flags();
bool      _kb_get_cooked(ushort* key);

#ifdef __cplusplus
};
#endif  // cplusplus


#endif /* !__INPUTIMP_H */
