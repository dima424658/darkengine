/*
 * $Source: x:/prj/tech/libsrc/mprintf/RCS/mprintf.h $
 * $Revision: 1.10 $
 * $Author: TOML $
 * $Date: 1997/07/28 18:44:39 $
 * 
 * Defines and prototypes for monochrome print stuff.
 *
 * $Log: mprintf.h $
 * Revision 1.10  1997/07/28  18:44:39  TOML
 * output debug string support
 * 
 * Revision 1.9  1997/06/24  18:12:24  TOML
 * made mprintf take const char *
 * 
 * Revision 1.8  1996/08/03  16:32:39  dc
 * mono_win_init
 * 
 * Revision 1.7  1996/02/14  14:49:39  JACOBSON
 * Cplusplus support
 * 
 * Revision 1.6  1994/10/20  02:00:29  dc
 * special callback power for serial
 * 
 * Revision 1.5  1993/09/13  23:34:05  dc
 * multi-page, set_flags, ^L clear in strings
 * 
 * Revision 1.4  1993/01/27  16:09:48  dc
 * New stoked mprintf, take 1, windows/set/get/so on
 * 
 * Revision 1.3  1992/12/15  13:38:01  kaboom
 * Fixed a few missing ; errors.
 * 
 * Revision 1.2  1992/10/23  12:12:38  kaboom
 * Put in defines for mono mode and mono text attributes.  Also added some
 * prototypes for other mono functions.
 * 
 * Revision 1.1  1992/10/15  20:22:32  kaboom
 * Initial revision
 */

#ifndef __MPRINTF_H
#define __MPRINTF_H

#ifdef __cplusplus
extern "C" {
#endif

/* turn all range checking and parameter checking on */
#define FULL_CHECKING                 

/* relase version number */
#define __MPRINTF_LIB_VER  "2.0"

/* monochrome screen modes. */
#define MONO_OFF            0
#define MONO_ON             1
#define MONO_TOG            2

/* monochrome screen text attributes. */
#define MA_NORMAL           0x07
#define MA_UNDER            0x01
#define MA_REV              0x70
#define MA_BOLD             0x40
#define MA_BLINK            0x80

/* split axis */
#define MONO_AXIS_X         0x00
#define MONO_AXIS_Y         0x01
#define MONO_AXIS_R         0x02                  /* repeat last split */

/* mono log how */
#define MONO_LOG_DEF        0x01                  /* use default log file */
#define MONO_LOG_NEW        0x02						 /* delete any existing log info */
#define MONO_LOG_CON        0x08                  /* continue last log - bitfield */

/* for which */
#define MONO_LOG_ALLWIN     0xFF
#define MONO_LOG_FULLSC     0x80

/* these are dumb, but i am psyched */
#define MONO_WIN_ONE        0x01
#define MONO_WIN_TWO        0x02
#define MONO_WIN_MAX       (MONO_WIN_TWO)
#define MONO_WIN_NXT       (MONO_WIN_MAX+1)

/* for page flipping */
#define MONO_NUM_PAGES      8
#define MONO_CGA_PAGE1     (MONO_NUM_PAGES)
#define MONO_CGA_PAGES      4
#define MONO_MAX_PAGES     (MONO_NUM_PAGES+MONO_CGA_PAGES)

#define MONO_PFLIP_CLR      0              /* punt the split */
#define MONO_PFLIP_KEEP     1              /* keep the split */

/* flags */
#define MONO_FLG_WRAP      (1<<0)
#define MONO_FLG_CLEAR     (1<<1)
#define MONO_FLG_MSG       (1<<2)

#define MONO_FLG_WRAPCLEAR (MONO_FLG_WRAP|MONO_FLG_CLEAR)

#define MONO_WRAP_MESSAGE "-----\n"

/* for mput and mget */
#define MONO_CUR_POS (-1)

// this brutal hack is to support the idea of adding functions to the low level write chain in _mprint
extern void (*mono_spc_func)(char *,int);

extern bool mono_to_debugger;

extern bool mono_detect (void);            
extern bool mono_init (void);
extern bool mono_win_init (bool have_screen);
extern int  mono_setmode (int mode);        /* set the mode */
extern int  mprint(const char *s);                /* print a string */
extern int  mprintf (const char *f, ...);         /* print a formatted string */
/* notice mget and mput do not reset cursor positon... ret -1 if off screen or no mono screen */
extern int  mget(uchar *s, int x, int y);   /* get current character (at x,y specified w/o moving cursor ) */
extern int  mput(uchar s, int x, int y);    /* put single character  ( or a -1 for x means at current loc) */
extern void mono_clear (void);             
extern void mono_scroll (int n);            /* scroll n lines */
extern int  mono_logon (char *fn, int how, int which);/* log mono prints to file fn, which is which wins/0 all 128 only fullscr */
extern void mono_logoff (void);             /* turn off logging */
/* kill_last deletes last used/current, else del default, always means kill even if open */
extern bool mono_logdel (bool kill_last, bool always);
extern void mono_setattr (uchar attrib);    /* set mono character attrib bits */
extern bool mono_setxy (int x, int y);
extern void mono_getxy (int *x, int *y);
extern bool mono_setpage(int pageid, bool focus); /* hard set/get page, whether to focus too */
extern int  mono_getpage(void);
extern bool mono_flip (int pageid);         /* flip to new page */
extern bool mono_unflip (void);             /* unflip back to last source page */
extern int  mono_setfocus(int pageid);      /* set/get the focus, ie. what we are displaying */
extern int  mono_getfocus(void);
extern void mono_scr_disable(void);         /* causes the display to stop retracing */
extern void mono_scr_enable(void);          /* reenables the video hardware */
extern void mono_set_flags(int flags, char *msg);   /* flags is random stuff, message is next line for wrap mode */
/* cur_mon says current or mono_only, set top and bottom of mono cursor */
extern void mono_cursor(bool cur_mon, int start, int stop);
extern int  mono_split (int axe, int loc);  /* axe is which axis */
extern bool mono_unsplit (void);
extern bool mono_setwin (int which);	     /* win1 is top or left */
extern int  mono_getwin (void);     	     /* returns -1 if no window */
/* dump screen to fn, erase if set, readable is whether to put \n's at the end of each line */
extern bool mono_dump(char *fn, bool erase_it,bool readable);

#ifdef __cplusplus
};
#endif

#endif /* !__MPRINTF_H */
