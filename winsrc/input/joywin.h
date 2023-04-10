///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/winsrc/input/RCS/joywin.h $
// $Revision: 1.5 $
//

#ifndef __JOYWIN_H
#define __JOYWIN_H

EXTERN void win32joy_read_pots_raw( short *raw );
EXTERN short win32joy_init( void );
EXTERN uchar win32joy_read_buttons( void );
EXTERN uchar fancy_win32( short *raw );
EXTERN void win32joy_set_dead(bool dead[4]);

#endif /* !__JOYWIN_H */
