// $Header: x:/prj/tech/libsrc/ui/RCS/vmouse.h 1.2 1998/06/18 13:31:33 JAEMZ Exp $

#ifndef __VMOUSE_H
#define __VMOUSE_H
#pragma once

// virtualized mouse support
EXTERN errtype ui_mouse_get_xy(short *pmx, short *pmy);
EXTERN errtype ui_mouse_put_xy(short pmx, short pmy);
EXTERN errtype ui_mouse_constrain_xy(short xl, short yl, short xh, short yh);
EXTERN errtype ui_mouse_do_conversion(short *pmx, short *pmy, bool down);

#endif  // __VMOUSE_H
