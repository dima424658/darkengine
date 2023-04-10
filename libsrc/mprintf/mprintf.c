/*
 * $Source: x:/prj/tech/libsrc/mprintf/RCS/mprintf.c $
 * $Revision: 1.23 $
 * $Author: TOML $
 * $Date: 1997/07/28 19:25:35 $
 *
 * Monochrome printf.
 */

/* things to do - logon/logoff w/o destroying fname */

#include <conio.h>
#include <io.h>
#include <fcntl.h>
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys\stat.h>
#ifdef _MSC_VER
#include <msconv.h>
#endif
#include <lgsprntf.h>
#include <mprintf.h>
#include <coremutx.h>

#define MONO_BASE    0xb0000
#define CGA_BASE     0xb8000
#define MONO_PAGE_SZ 0x01000

#define MONO_CARD    0
#define CGA_CARD     1

/* video bios constants. */
#define VBIOS_INT    0x10
#define VB_GET_COMBO 0x1a00
#define VB_SET_XY    0x0200

/* monochrome register ports. */
#define M_CNTRL      0x3b8             /* mono controller register */
#define M_SRX_ADR    0x3b4             /* mono sequencer address port */
#define M_SRX_DATA   0x3b5             /* mono sequencer data port */
#define SR_CTP       0x0a              /* cursor top and bottom */
#define SR_CBT       0x0b
#define SR_AHI       0x0c              /* page address hi and low */
#define SR_ALO       0x0d
#define SR_CLH       0x0e              /* seq cursor location high */
#define SR_CLL       0x0f              /* low */

#define C_SRX_ADR    0x3d4             /* cga sequencer address port */
#define C_SRX_DATA   0x3d5             /* cga sequencer data port */

#define COMBO_MONO   1
#define MONO_WID     80
#define MONO_HGT     25
#define MONO_ROW     (2*MONO_WID)
#define MONO_OFF     0
#define MONO_ON      1
#define MONO_TOG     2
#define MONO_FILE    "mprintf.log"
#define MONO_DUMP    "mono.dmp"

#ifdef _WIN32
__declspec(dllimport) 
void 
__stdcall
OutputDebugStringA(
    const char * lpOutputString
    );
#define OutputDebugString(s) OutputDebugStringA(s)
#endif

bool mono_to_debugger = FALSE;

/* have we yet called mono_init()? */
static bool mono_done_init = FALSE;

/* TRUE if there's a monochrome screen present. */
static bool mono_screen = FALSE;

/* MONO_ON when monochrome printing enabled. */
static uchar mono_mode = MONO_OFF;

/* cursor position on the mono screen. - abs cursor pos */
static uchar mono_x = 0;
static uchar mono_y = 0;

typedef struct {
   uchar axis;
   char  coord, win;
   uchar w, h;
   uchar x, y;
   uchar *base;
   uchar lcoor[MONO_WIN_MAX][2];
   char  lfocus;
   uchar *addr;
   uchar flags;
   char  *msg;
} page_parms;

static page_parms _mono_screens[MONO_MAX_PAGES];
/* last is for flip/unflip  - init to 1 so setpage 0 at start does stuff */
static char       _cur_mono_page=1, _last_mono_page=-1, _cur_mono_focus=0;
static uint       _inited_mpages=0;    /* bitfielded list of which pages are inited */
static uchar      _cur_card=0;         /* MDA */

static char *def_wrap_msg=MONO_WRAP_MESSAGE;

static int srx_adr[2]={M_SRX_ADR,C_SRX_ADR};
static int srx_data[2]={M_SRX_DATA,C_SRX_DATA};

#define cur_stru     (_mono_screens[_cur_mono_page])
#define split_axis   ((cur_stru).axis)
#define split_coord  ((cur_stru).coord)
#define split_win    ((cur_stru).win)
#define split_w      ((cur_stru).w)
#define split_h      ((cur_stru).h)
#define split_x      ((cur_stru).x)
#define split_y      ((cur_stru).y)
#define split_base   ((cur_stru).base)
#define page_addr    ((cur_stru).addr)
#define split_lcoor  ((cur_stru).lcoor)
#define split_lfocus ((cur_stru).lfocus)
#define split_flags  ((cur_stru).flags)
#define split_msg    ((cur_stru).msg)

#ifdef OLD_WAY
/* static split screen stuff */
static uchar split_axis  = MONO_AXIS_X;
static char  split_coord  = -1;
static char  split_win    = -1;	/* which window is active, -1 for no windows, else 1 or 2 */
static uchar split_w=MONO_WID, split_h=MONO_HGT;
static uchar split_x=0, split_y=0;
static uchar *split_base=MONO_BASE;
static uchar split_lcoor[MONO_WIN_MAX][2]={{0xff,0xff},{0xff,0xff}};
static char  split_lfocus=-1;
#endif

/* current drawing attribute */
static uchar mono_attr = 7; /* MA_NORMAL */

/* handle of the log file or -1 if no log file. */
static int mono_file = -1;
static uchar mlog=0;                   /* last/cur log file parms, mostly for which windows to log */
/* name of last log file */
static char last_lf[_MAX_PATH]=MONO_FILE; /* pretend default was last */

int _mprint(const char *s, int n);

// just extern this and set it if you want to see things go...
void (*mono_spc_func)(char *,int)=NULL;

#define _mono_spc_check(s,n) \
   if (mono_spc_func!=NULL)  \
      (*mono_spc_func)(s,n)

/* memset that sets n shorts starting a s to c. */
#ifdef __WATCOMC__
void *smemset (void *s, short c, int n);
#pragma aux smemset=       \
   "push edi"              \
   "rep  stosw"            \
   "pop  eax"              \
   parm [edi] [eax] [ecx]  \
   modify [edi eax ecx];
#else
__inline void *smemset(void *s, short c, int n)
{
	#pragma warning(disable : 4035)	// disables no return value warning
	__asm
	{
		mov		edi, s
		mov		ax, c
		mov		ecx, n
		push	edi
		rep		stosw
		pop		eax
	}
}
#endif

#define out_mda(reg,val) outp(M_SRX_ADR,reg); outp(M_SRX_DATA,val)

#ifdef __WATCOMC__
#define out_set(card,reg,val)\
   outp(srx_adr[card],reg);\
   outp(srx_data[card],val)
#else
#define out_set(card,reg,val)\
   outp((ushort) srx_adr[card],reg);\
   outp((ushort) srx_data[card],val)
#endif

#ifdef USE_OLD_MONO_DETECT
bool mono_detect(void)
{
   union REGS r;

   r.w.ax = VB_GET_COMBO;
   int386 (VBIOS_INT, &r, &r);
   return ((r.h.bh==COMBO_MONO)||(r.h.bl==COMBO_MONO));
}
#endif /* USE_OLD_MONO_DETECT */

#ifdef USE_VBIO_CALL
/* video bios call */
int vbio_call (int parm_ax)
{
   union REGS r;

   r.w.ax = parm_ax;
   int386 (VBIOS_INT, &r, &r);
   return r.w.bx;
}
#endif /* USE_VBIO_CALL */

#ifdef __WATCOMC__
int mono_get_combo(void);
#pragma aux mono_get_combo=\
   "mov eax,0x1a00" \
   "int 0x10" \
   "mov eax,ebx" \
   "and eax,0xffff" \
   modify exact [ebx];
#else
__inline int mono_get_combo(void)
{
	#pragma warning(disable : 4035)	// disables no return value warning
	__asm
	{
		mov		eax, 0x1a00
		int		0x10
		mov		eax, ebx
		and		eax, 0xffff
	}
}
#endif
/* returns TRUE if the secondary display is monochrome. */
bool mono_detect(void)
{
#ifdef WIN32
   char *pStr;

   // Fake this out under Windows 95, for now assume there is a monochrome
   // board at 0xB000 and monitor present and hope for the best.  The more
   // this crashes on other setups, the sooner we'll have to come up with
   // something else.  Under Windows NT, monochrome output is not supported.

   if (pStr = getenv ("OS"))
      if (! stricmp (pStr, "Windows_NT"))
         return FALSE;

   // Must be Windows 95
   return TRUE;
#else
   int combo;

   combo=mono_get_combo();
   // note the 5 is for Jaemz's machine which bites.
   return (combo&0xff)==COMBO_MONO || (combo>>8)==COMBO_MONO || (combo>>8)==5;
#endif
}

/* sets the monochrome printing mode to the mode passed in.  it can be one of
   MONO_ON, MONO_OFF, or MONO_TOG.  returns new mono mode. */
int mono_setmode (int mode)
{
   if (mono_screen)
   {  /* lets do this only if we have a mono screen */
	   if (mode == MONO_TOG)
	   {
	      if (mono_mode == MONO_ON)
	         mono_mode = MONO_OFF;
	      else
	         mono_mode = MONO_ON;
	   }
	   else
	      mono_mode = mode;
   }
// it really already should be this
// else mono_mode=MONO_OFF;

   return mono_mode;
}

/* setup for wrapping on mono as opposed to scrolling */
void mono_set_flags(int flags, char *msg)
{
   split_flags=flags;
   if ((flags&MONO_FLG_MSG)&&(msg!=NULL))
	   split_msg=msg;
}

/* fire up the monochrome screen printer.  returns TRUE if there is a
   monochrome screen present. */
bool mono_init(void)
{
   bool r;

   mono_done_init=TRUE;
   mono_x=mono_y=0;
   mono_screen=mono_detect();
   r=mono_setmode(mono_screen);
   if (r) mono_setpage(0,TRUE); /* put cursor at upper left, get to page 0 */
   return r;
}

/* fire up the monochrome screen printer.  returns TRUE if there is a
   monochrome screen present. */
bool mono_win_init(bool have_screen)
{
   bool r;

   mono_done_init=TRUE;
   mono_x=mono_y=0;
   mono_screen=have_screen?MONO_ON:MONO_OFF;
   r=mono_setmode(mono_screen);
   if (r) mono_setpage(0,TRUE); /* put cursor at upper left, get to page 0 */
   return r;
}


/* Mono Logging functions, logon, logoff, logdel */

/* turn on logging of monochrome activities.
 *  if how&MONO_LOG_CON use the last file logged to
 *  if how&MONO_LOG_DEF always use default log
 *  if how&MONO_LOG_NEW erase existing log file
 *  if CON and DEF, CON overrides, so there
 * which uses the MONO_LOG_ALLWIN, or FULLSC defines, or | together MONO_WIN_ONE,TWO,etc
 */
int mono_logon (char *fn, int how, int which)
{
	int open_flags=O_WRONLY|O_CREAT;
   if (mono_file != -1)
      return -1;
   if	(how&MONO_LOG_CON)
		fn = last_lf;							/* continue with last */
	else if ((fn == NULL)||(how&MONO_LOG_DEF))
      fn = MONO_FILE;
	if (how&MONO_LOG_NEW)
		open_flags|=O_TRUNC;
	else
		open_flags|=O_APPEND;
	if (fn!=last_lf)
		strcpy(last_lf,fn);
   mono_file = open (fn, open_flags, S_IWRITE);
   mlog=which;
   return mono_file;
}

/* turn off logging of monochrome screen. */
void mono_logoff (void)
{
	if (mono_file!=-1)
	{
	   close (mono_file);
   	mono_file = -1;
	}
}

/* delete log
 *  kill_last is last log/default log
 *  always is even if open delete
 *
 * this is bizarrely broken, what to return re:kill_last
 */
bool mono_logdel (bool kill_last, bool always)
{
	if (mono_file!=-1)
	{
		if (!always)
			return FALSE;
		else
		{
			close(mono_file);
			mono_file=-1;
		}
	}
	if (kill_last)
		return remove(last_lf);
	else
		return remove(MONO_FILE);
}

/* internal routines for memory access, screen parameter setting */

/* this is called for a page we have never visited before */
void _mono_init_page(int pageid)
{
   split_axis=MONO_AXIS_X; split_coord=split_win=split_lfocus-1;
   split_x=split_y=0; split_w=MONO_WID; split_h=MONO_HGT;
   memset(split_lcoor,0xff,4);
   split_base=page_addr=(uchar *)(MONO_BASE+(pageid*MONO_PAGE_SZ));
   split_flags=0; split_msg=def_wrap_msg;
   _inited_mpages|=(1<<pageid);
}

/* get memory address of x,y in current window */
uchar *_maddr(int x, int y)
{
   if ((x<split_w)&&(y<split_h))
		return (uchar *)split_base+MONO_ROW*y+x*2;
   else
      return NULL;
}

/* returns whether the gotten coordinates are or are not on the screen */
static bool _mget(int *x, int *y, bool always)
{
 	if (always||(*x==-1)) *x=mono_x-split_x;
 	if (always||(*y==-1)) *y=mono_y-split_y;
   return (!(((*x)<0)||((*x)>=MONO_WID)||((*y)<0)||((*y)>=MONO_HGT)));
}

static bool _mset(int x, int y)
{
   if ((x<0)||(x>=split_w)||(y<0)||(y>split_h)) return FALSE;
 	mono_x=split_x+x; mono_y=split_y+y; return TRUE;
}

static bool _out_of_win(int x, int y)
{
   return (((x!=-1)&&((x<0)||(x>=split_w)))||((y!=-1)&&((y<0)||(y>=split_h))));
}

/* the beginning of all mono_ external calls - returns true to really do it */
static bool _mono_top(void)
{
   if (!mono_done_init) mono_init ();
   return (mono_mode != MONO_OFF);
}

#define _mono_focused() (_cur_mono_focus==_cur_mono_page)

/* routines to get and put single characters */
/* returns -1 if out of range or no mono screen */
int  mget(uchar *s, int x, int y)   /* get current character (at x,y specified w/o moving cursor ) */
{
   if ((_mono_top())&&(!_out_of_win(x,y))&&(_mget(&x,&y,FALSE)))
      return (int)(*s=*_maddr(x,y));
   else
    { *s=0; return -1; }
}

int  mput(uchar s, int x, int y)    /* put single character  ( or a -1 for x means at current loc) */
{
   if ((_mono_top())&&(!_out_of_win(x,y))&&(_mget(&x,&y,FALSE)))
      return (int)(*_maddr(x,y)=s);
   else
      return -1;
}

/* split screen stuff */
/* internal get/set correct window parameters */
static bool _mwin_set(int pick,int axe,int loc)
{
   int save_focus=split_win;

   split_win=pick; split_base=(uchar *)page_addr;
	split_w=MONO_WID; split_h=MONO_HGT; split_x=split_y=0;
	if (pick==-1) goto no_split;
	if (axe==MONO_AXIS_X)
	{
#ifdef FULL_CHECKING
      if (loc>=MONO_WID) goto no_split;
#endif
		if (pick==1)			split_w=loc;
		else        		 { split_w=MONO_WID-loc; split_base+=2*loc; split_x=loc; }
	}
	else
	{
#ifdef FULL_CHECKING
      if (loc>=MONO_HGT) goto no_split;
#endif
		if (pick==1)			split_h=loc;
		else        		 { split_h=MONO_HGT-loc; split_base+=MONO_ROW*loc; split_y=loc; }
	}
   split_axis=axe; split_coord=loc;
   if (split_lfocus==-1)          /* brand new split */
      memset(split_lcoor,0,(MONO_WIN_MAX*2));
   mono_setxy(split_lcoor[split_win-1][0],split_lcoor[split_win-1][1]); /* go to correct position */
   split_lfocus=split_win;
   return TRUE;
no_split:
   split_lfocus=save_focus;
   return (pick==-1);
}

/* externals */
/* split returns number of windows, or -1 if there are already windows */
int  mono_split (int axe, int loc)
{
   int save_focus=split_lfocus, focus=1;
	if (split_win!=-1) return -1;			/* already an open window */
   if (axe==MONO_AXIS_R)   { axe=split_axis; loc=split_coord; focus=split_lfocus; } else split_lfocus=-1;
   if (_mwin_set(focus,axe,loc)) return MONO_WIN_MAX;						/* there are 2 windows */
   else                    { split_lfocus=save_focus; return -1; }
}

/* returns false if it cannot unsplit the window */
bool mono_unsplit (void)
{
	return (split_win!=-1)&&(_mwin_set(-1,-1,-1));
}

/* returns whether the pickwin was succesful, ie. whether you passed legal in */
bool mono_setwin (int which)
{
#ifdef FULL_CHECKING
	if ((split_win==-1)||(which<0)||(which>MONO_WIN_NXT)) return FALSE;
#endif
	if (which==split_win) return TRUE;	/* already focused on this window */
	if (which==MONO_WIN_NXT)
		if ((which=split_win+1)==MONO_WIN_MAX+1) which=1;
	_mwin_set(which,split_axis,split_coord);
	return TRUE;
}

int mono_getwin(void)
{
// printf("Axe %d, loc %d, cur %d, last %d scr %d %d %d %d\n",split_axis,split_coord,split_win,split_lfocus,split_x,split_y,split_w,split_h);
   return split_win;
}

/* direct page set/get */
bool mono_setpage(int pageid, bool focus)
{
   if (!_mono_top()) return FALSE;
   if (_cur_mono_page==pageid) return FALSE;
   if (pageid>=MONO_MAX_PAGES) return FALSE;
   _cur_mono_page=pageid;
   if ((_inited_mpages&(1<<pageid))==0)
   {
      _mono_init_page(pageid);
      mono_setxy(0,0);
   }
   if (focus)
      mono_setfocus(pageid);
   return TRUE;
}

int mono_getpage(void)
{
   return _cur_mono_page;
}

int mono_setfocus(int pageid)
{
   int page_offs;
   if (!_mono_top()) return FALSE;
   if (_cur_mono_focus==pageid) return FALSE;
   if (pageid<MONO_NUM_PAGES)
    { _cur_card=MONO_CARD; page_offs=(int)(page_addr-MONO_BASE); }
   else
    { _cur_card=CGA_CARD; page_offs=(int)(page_addr-CGA_BASE)>>1; }
// printf("Trying to set to %x on card %d\n",page_offs,_cur_card);
   out_set(_cur_card,SR_AHI,(page_offs)>>8);
   out_set(_cur_card,SR_ALO,(page_offs)&0xff);
// vbio_call(0x0500+pageid-(_cur_card==CGA_CARD?8:0));
   return TRUE;
}

int mono_getfocus(void)
{
   return _cur_mono_focus;
}

/* goofy turn off screen thing */
void mono_scr_disable(void)
{
   int ostate=inp (M_CNTRL);
   ostate&=(~(1<<3));
   outp(M_CNTRL,ostate);
}

void mono_scr_enable(void)
{
   int ostate=inp (M_CNTRL);
   ostate|=(1<<3);
   outp(M_CNTRL,ostate);
}

/* warning, bizarrly broken on some mono cards */
void mono_cursor(bool cur_mon, int start, int stop)
{
   int card=MONO_CARD;
   if (start>stop) {int tmp=start; start=stop; stop=tmp; }
   if (cur_mon) card=_cur_card;
   out_set(card,SR_CTP,start);
   out_set(card,SR_CBT,stop);
}

/* screen flip stuff */
bool mono_flip(int pageid)
{
   int old_page=_cur_mono_page;
   bool retv=mono_setpage(pageid,TRUE);
   if (retv) _last_mono_page=old_page;
   return retv;
}

bool mono_unflip(void)
{
   bool retv;
   if (_last_mono_page==-1) return FALSE;
   if ((retv=mono_setpage(_last_mono_page,TRUE)) != 0)
	   _last_mono_page=-1;              /* so it wont be able to double unflip */
   return retv;
}

/* set the current text attributes. */
void mono_setattr (uchar attrib)
{
   mono_attr = attrib;
}

/* set the (x,y) location of the cursor. - window relative */
bool mono_setxy (int x, int y)
{
	if (!_mset(x,y)) return FALSE;				/* set even if screen disabled */
   if (!_mono_top()) return FALSE;
   if (_mono_focused())
   {
	   int c_offset = MONO_WID*mono_y + mono_x;
	   out_set(_cur_card,SR_CLH,(c_offset)>>8);
	   out_set(_cur_card,SR_CLL,(c_offset)&0xff);
   }
   return TRUE;
}

/* return the current x,y coordinates */
void mono_getxy(int *x, int *y)
{
	_mget(x,y,TRUE);
}

/* clear the monochrome screen and reset cursor. */
void mono_clear (void)
{
	int i;
   if (!_mono_top()) return;
	if ((split_win!=-1)&&(split_axis==MONO_AXIS_X))
		for (i=0; i<MONO_HGT; i++)
			smemset(split_base+i*MONO_ROW,mono_attr<<8,split_w);
	else
		smemset(split_base,mono_attr<<8,MONO_WID*split_h);
   mono_setxy (0, 0);
}

/* scroll the monochrome screen by n lines, blanking bottom n lines. */
void mono_scroll (int n)
{
	int i;
   uchar *src, *dst, *bot;

   if (!_mono_top()) return;
   if (n >= split_h)
      if ((split_flags==0)||((split_flags&MONO_FLG_WRAPCLEAR)==MONO_FLG_WRAPCLEAR))
         { mono_clear (); return; }

   if (split_flags&MONO_FLG_WRAP)
   {
      int x, y;
      _mget(&x,&y,TRUE);
      if (y<split_h-1)
      {
         if (split_flags&MONO_FLG_CLEAR)
	         smemset(split_base+y*MONO_ROW,mono_attr<<8,(split_h-y-1)*MONO_WID);
         n-=(split_h-y-1);

      }  /* now at top of window */
      if (n>0)
      {
         if (split_flags&MONO_FLG_CLEAR)
	         smemset(split_base,mono_attr<<8,n*MONO_WID);
         y=n-1;
      }
      else y=split_h+n-1;
      _mset(0,y);
   }
	else
   {
	   src = split_base+n*MONO_ROW;
	   dst = split_base;
	   bot = split_base+(split_h-n)*MONO_ROW;
	   if ((split_win!=-1)&&(split_axis==MONO_AXIS_X))
		{
			for (i=0; i<(split_h-n); i++)
				memmove(dst+i*MONO_ROW,src+i*MONO_ROW,2*split_w);
			for (i=0; i<n; i++)
				smemset(bot+i*MONO_ROW,mono_attr<<8,split_w);
		}
		else
		{
		   memmove (dst, src, (split_h-n)*MONO_ROW);
		   smemset (bot, mono_attr<<8, n*MONO_WID);
		}
   }
}

bool _mono_log(void)
{
//   return ((mono_spc_func!=NULL)||((mono_file!=-1)&&(!mono_screen||(mlog&split_win))));
   return    (mono_spc_func!=NULL) ||
            ((mono_file!=-1)&&
               (!mono_screen||((mlog==0)||(mlog&split_win))));
}

// doesnt correctly support split width stuff....
int _mscroll_msg(int *x, int *y)
{
   if (split_flags&MONO_FLG_CLEAR)
      smemset(split_base+(*y)*MONO_ROW,mono_attr<<8,split_w);
   if ((split_flags&MONO_FLG_MSG)&&(split_msg!=NULL))
   {
      int old_y=*y, chars_in_msg=strlen(split_msg);
      *y=((*y)+1)%split_h;
      _mset(*x,*y);
      if ((chars_in_msg>0)&&(split_msg[chars_in_msg-1]=='\n'))
       { smemset(split_base+(*y)*MONO_ROW,mono_attr<<8,split_w); chars_in_msg--; }
      _mprint(split_msg,chars_in_msg);
      *y=old_y;
      _mset(*x,*y);
   }
   return *y;
}

int _mscroll_us(int n, int *x, int *y)
{
   if (*y < split_h-n)
	   (*y)+=n;
   else
   {
		mono_scroll (n);
	   _mget(x,y,TRUE);
   }
   *x = 0;
   _mscroll_msg(x,y);
   return *y;
}

/* internal print a buffer a length n to the mono screen */
int _mprint(const char *s, int n)
{
   int i,cur_x,cur_y;
   uchar *p;                           /* pointer to current line. */

   if (_mono_log())
      write (mono_file, s, n);			/* write even if no screen */
#ifdef _WIN32
   if (mono_to_debugger)
      OutputDebugString (s);
#endif
      
   _mono_spc_check((char *)s,n);

	if (mono_mode==MONO_OFF || !mono_screen) return n;/* no screen, so dont print */

	_mget(&cur_x,&cur_y,TRUE);
   for (i=0; i<n; i++)
   {
		p = _maddr(cur_x,cur_y);

      switch (s[i])
      {
      case '\n':
         _mscroll_us(1,&cur_x,&cur_y);
         break;
      case '\r':
         cur_x = 0;
         break;
      case '\b':
         if (cur_x > 0)
            cur_x--;
         else if (cur_y > 0)
            cur_y--;
         break;
      case '\014':
         mono_clear();
         cur_x=cur_y=0;
         break;
      default:
         p[0] = (uchar)s[i];
         p[1] = mono_attr;
         if (cur_x < split_w-1)
            cur_x++;
         else
            _mscroll_us(1,&cur_x,&cur_y);
         break;
      }
   }
   mono_setxy (cur_x, cur_y);
   return n;
}

/* printf to the monochrome screen.  has the same arguments and return value
   as printf (returns number of items printed).  uses vsprintf to print into
   a 1k buffer which is then transferred to the mono screen char by char. */
int mprintf(const char *fmt, ...)
{
   char buf[1024];
   int n;
   va_list ap;

   CoreThreadLock();

   if (!_mono_top())
      if (!_mono_log())
      {
         CoreThreadUnlock();
	      return -1;
	   }

   va_start(ap, fmt);
   n=lg_vsprintf(buf, fmt, ap);
   va_end (ap);

   /* punt if there was an error. */
   if (n < 0)
   {
      CoreThreadUnlock();
      return n;
   }

   /* write this string if we're logging. */
   _mprint(buf,n);

   CoreThreadUnlock();
   return n;
}

/* print s with no formating, it's just text folks, just text, nothing to be seen here */
int mprint(const char *s)
{
   int ret;
   CoreThreadLock();
   if (!_mono_top())
		if (!_mono_log())
		{
         CoreThreadUnlock();
	      return -1;
	   }
	ret =  _mprint(s,strlen(s));
   CoreThreadUnlock();
   return ret;
}

/* dump screen to fn, erase if set */
bool mono_dump(char *fn, bool erase_it, bool readable)
{
	int open_flags=O_WRONLY|O_CREAT|O_BINARY, mono_hnd;
	int x,y,write_wid=MONO_WID;
	uchar *cur_char;
	char on_line[MONO_WID+2];

	if (!_mono_top()) return FALSE;
	if (erase_it) open_flags|=O_TRUNC; else open_flags|=O_APPEND;
	if (fn==NULL) fn=MONO_DUMP;
   if ((mono_hnd = open (fn, open_flags, S_IWRITE))==-1) return FALSE;
   if (readable) { on_line[MONO_WID]=0x0D; on_line[MONO_WID+1]=0x0A; write_wid+=2; }
	for (y=0; y<MONO_HGT; y++)
	{
		cur_char=(uchar *)page_addr+MONO_ROW*y;
		for (x=0; x<MONO_WID; x++)
			on_line[x]=*(cur_char+2*x);
		write(mono_hnd,on_line,write_wid);
	}
	close(mono_hnd);
	return TRUE;
}
