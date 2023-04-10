///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/winsrc/input/RCS/joywin.cpp $
// $Revision: 1.13 $
//

#ifdef _WIN32
#include <windows.h>
#include <string.h>
#include <joywin.h>
#include <stdio.h>
#include <joystick.h>
#include <mprintf.h>
#include <timer.h>
#include <mmsystem.h>

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// The following code was lifted from newer mmsystem.h file than shipped with
// Watcom 10.5.  This should be addressed. (toml 02-07-96)

/****************************************************************************

			    Joystick support

****************************************************************************/

/* joystick error return values */
#define JOYERR_NOERROR        (0)                  /* no error */
#define JOYERR_PARMS          (JOYERR_BASE+5)      /* bad parameters */
#define JOYERR_NOCANDO        (JOYERR_BASE+6)      /* request not completed */
#define JOYERR_UNPLUGGED      (JOYERR_BASE+7)      /* joystick is unplugged */

/* constants used with JOYINFO and JOYINFOEX structures and MM_JOY* messages */
#define JOY_BUTTON1         0x0001
#define JOY_BUTTON2         0x0002
#define JOY_BUTTON3         0x0004
#define JOY_BUTTON4         0x0008
#define JOY_BUTTON1CHG      0x0100
#define JOY_BUTTON2CHG      0x0200
#define JOY_BUTTON3CHG      0x0400
#define JOY_BUTTON4CHG      0x0800

/* constants used with JOYINFOEX */
#define JOY_BUTTON5         0x00000010l
#define JOY_BUTTON6         0x00000020l
#define JOY_BUTTON7         0x00000040l
#define JOY_BUTTON8         0x00000080l
#define JOY_BUTTON9         0x00000100l
#define JOY_BUTTON10        0x00000200l
#define JOY_BUTTON11        0x00000400l
#define JOY_BUTTON12        0x00000800l
#define JOY_BUTTON13        0x00001000l
#define JOY_BUTTON14        0x00002000l
#define JOY_BUTTON15        0x00004000l
#define JOY_BUTTON16        0x00008000l
#define JOY_BUTTON17        0x00010000l
#define JOY_BUTTON18        0x00020000l
#define JOY_BUTTON19        0x00040000l
#define JOY_BUTTON20        0x00080000l
#define JOY_BUTTON21        0x00100000l
#define JOY_BUTTON22        0x00200000l
#define JOY_BUTTON23        0x00400000l
#define JOY_BUTTON24        0x00800000l
#define JOY_BUTTON25        0x01000000l
#define JOY_BUTTON26        0x02000000l
#define JOY_BUTTON27        0x04000000l
#define JOY_BUTTON28        0x08000000l
#define JOY_BUTTON29        0x10000000l
#define JOY_BUTTON30        0x20000000l
#define JOY_BUTTON31        0x40000000l
#define JOY_BUTTON32        0x80000000l

/* constants used with JOYINFOEX structure */
#define JOY_POVCENTERED		(WORD) -1
#define JOY_POVFORWARD		0
#define JOY_POVRIGHT		9000
#define JOY_POVBACKWARD		18000
#define JOY_POVLEFT		27000

#define JOY_RETURNX		0x00000001l
#define JOY_RETURNY		0x00000002l
#define JOY_RETURNZ		0x00000004l
#define JOY_RETURNR		0x00000008l
#define JOY_RETURNU		0x00000010l	/* axis 5 */
#define JOY_RETURNV		0x00000020l	/* axis 6 */
#define JOY_RETURNPOV		0x00000040l
#define JOY_RETURNBUTTONS	0x00000080l
#define JOY_RETURNRAWDATA	0x00000100l
#define JOY_RETURNPOVCTS	0x00000200l
#define JOY_RETURNCENTERED	0x00000400l
#define JOY_USEDEADZONE		0x00000800l
#define JOY_RETURNALL		(JOY_RETURNX | JOY_RETURNY | JOY_RETURNZ | \
				 JOY_RETURNR | JOY_RETURNU | JOY_RETURNV | \
				 JOY_RETURNPOV | JOY_RETURNBUTTONS)
#define JOY_CAL_READALWAYS	0x00010000l
#define JOY_CAL_READXYONLY	0x00020000l
#define JOY_CAL_READ3		0x00040000l
#define JOY_CAL_READ4		0x00080000l
#define JOY_CAL_READXONLY	0x00100000l
#define JOY_CAL_READYONLY	0x00200000l
#define JOY_CAL_READ5		0x00400000l
#define JOY_CAL_READ6		0x00800000l
#define JOY_CAL_READZONLY	0x01000000l
#define JOY_CAL_READRONLY	0x02000000l
#define JOY_CAL_READUONLY	0x04000000l
#define JOY_CAL_READVONLY	0x08000000l

/* joystick ID constants */
#define JOYSTICKID1         0
#define JOYSTICKID2         1

/* joystick driver capabilites */
#define JOYCAPS_HASZ		0x0001
#define JOYCAPS_HASR		0x0002
#define JOYCAPS_HASU		0x0004
#define JOYCAPS_HASV		0x0008
#define JOYCAPS_HASPOV		0x0010
#define JOYCAPS_POV4DIR		0x0020
#define JOYCAPS_POVCTS		0x0040

#if (__WATCOMC__ == 1050)
#pragma pack(1)
typedef struct joyinfoex_tag {
    DWORD dwSize;		 /* size of structure */
    DWORD dwFlags;		 /* flags to indicate what to return */
    DWORD dwXpos;                /* x position */
    DWORD dwYpos;                /* y position */
    DWORD dwZpos;                /* z position */
    DWORD dwRpos;		 /* rudder/4th axis position */
    DWORD dwUpos;		 /* 5th axis position */
    DWORD dwVpos;		 /* 6th axis position */
    DWORD dwButtons;             /* button states */
    DWORD dwButtonNumber;        /* current button number pressed */
    DWORD dwPOV;                 /* point of view state */
    DWORD dwReserved1;		 /* reserved for communication between winmm & driver */
    DWORD dwReserved2;		 /* reserved for future expansion */
} JOYINFOEX, *PJOYINFOEX, NEAR *NPJOYINFOEX, FAR *LPJOYINFOEX;
#pragma pack()

MMRESULT APIENTRY joyGetPosEx(UINT uJoyID, LPJOYINFOEX pji);
#endif

MMRESULT (APIENTRY *pfnJoyGetPosEx)(UINT uJoyID, LPJOYINFOEX pji);

typedef MMRESULT (APIENTRY * tJoyGetPosEx)(UINT, LPJOYINFOEX);

#if (__WATCOMC__ == 1050)
///////////////////////////////////////////////////////////////////////////////
//
// This structure represents the Win 4.0 joycaps.  Remove when
// watcom provides up-tp-date headers
//
#define MAX_JOYSTICKOEMVXDNAME 260 /* max oem vxd name length (including NULL) */
#pragma pack(1)
typedef struct tagJOYCAPSA_2 {
    WORD    wMid;                /* manufacturer ID */
    WORD    wPid;                /* product ID */
    CHAR    szPname[MAXPNAMELEN];/* product name (NULL terminated string) */
    UINT    wXmin;               /* minimum x position value */
    UINT    wXmax;               /* maximum x position value */
    UINT    wYmin;               /* minimum y position value */
    UINT    wYmax;               /* maximum y position value */
    UINT    wZmin;               /* minimum z position value */
    UINT    wZmax;               /* maximum z position value */
    UINT    wNumButtons;         /* number of buttons */
    UINT    wPeriodMin;          /* minimum message period when captured */
    UINT    wPeriodMax;          /* maximum message period when captured */
    UINT    wRmin;               /* minimum r position value */
    UINT    wRmax;               /* maximum r position value */
    UINT    wUmin;               /* minimum u (5th axis) position value */
    UINT    wUmax;               /* maximum u (5th axis) position value */
    UINT    wVmin;               /* minimum v (6th axis) position value */
    UINT    wVmax;               /* maximum v (6th axis) position value */
    UINT    wCaps;	 	 /* joystick capabilites */
    UINT    wMaxAxes;	 	 /* maximum number of axes supported */
    UINT    wNumAxes;	 	 /* number of axes in use */
    UINT    wMaxButtons;	 /* maximum number of buttons supported */
    CHAR    szRegKey[MAXPNAMELEN];/* registry key */
    CHAR    szOEMVxD[MAX_JOYSTICKOEMVXDNAME]; /* OEM VxD in use */
} JOYCAPS_2;
#pragma pack()
#else
typedef JOYCAPS JOYCAPS_2;
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#ifdef LDEBUG
#pragma message ("Joystick messages enabled")
static ulong ulJoyTime = tm_get_millisec();
#define JoyMsg(s)  { ulong ulTime = tm_get_millisec(); mprintf("JOY %d: " s "\n", ulTime - ulJoyTime); ulJoyTime = ulTime; }
#define JoyMsg1(s, a)  { ulong ulTime = tm_get_millisec(); mprintf("JOY %d: " s "\n", ulTime - ulJoyTime, a); ulJoyTime = ulTime; }
#define JoyMsg4(s, a, b, c, d)  { ulong ulTime = tm_get_millisec(); mprintf("JOY %d: " s "\n", ulTime - ulJoyTime, a, b, c, d ); ulJoyTime = ulTime; }
#define JoyMsg8(s, a, b, c, d, e, f, g, h)  { ulong ulTime = tm_get_millisec(); mprintf("JOY %d: " s "\n", ulTime - ulJoyTime, a, b, c, d, e, f, g, h); ulJoyTime = ulTime; }
#else
#define JoyMsg(s)
#define JoyMsg1(s, a)
#define JoyMsg4(s, a, b, c, d)
#define JoyMsg8(s, a, b, c, d, e, f, g, h)
#endif

///////////////////////////////////////////////////////////////////////////////

static BOOL gbJoyAvailable;
static JOYCAPS_2 gJoyCaps;

#undef abs
#define abs(i) ((i < 0) ? -i : i)

// Note that the unnormalize is 0 - 65535, so this
// centers at 128, but has a range from -128 to 256+128
#define normalize_pot(val) (short)((val/128) - 128)

static void normalize_pots(int x, int y, int r, int t, short *raw)
{
   if (!raw)
      return;
   raw[0] = normalize_pot(x);
   raw[1] = normalize_pot(y);
   if (gJoyCaps.wCaps & JOYCAPS_HASR)
      raw[2] = normalize_pot(r);
   else
      raw[2] = 0;

   if (gJoyCaps.wCaps & JOYCAPS_HASZ)
      raw[3] = normalize_pot(t);
   else
      raw[3] = 0;
}

void win32joy_read_pots_raw(short *raw)
{
   JOYINFOEX info;

   info.dwSize = sizeof(JOYINFOEX);
   info.dwFlags = (JOY_RETURNX | JOY_RETURNY);

   if (gJoyCaps.wCaps & JOYCAPS_HASR)
      info.dwFlags |= JOY_RETURNR;

   if (gJoyCaps.wCaps & JOYCAPS_HASZ)
      info.dwFlags |= JOY_RETURNZ;

   if (pfnJoyGetPosEx && pfnJoyGetPosEx(JOYSTICKID1, &info) != JOYERR_UNPLUGGED)
   {
      normalize_pots(info.dwXpos, info.dwYpos, info.dwRpos, info.dwZpos, raw);
   }
   JoyMsg8("win32joy_read_pots_raw() returns (%d, %d, %d, %d) ==> (%d, %d, %d, %d)", info.dwXpos, info.dwYpos, info.dwRpos, info.dwZpos, raw[0], raw[1], raw[2], raw[3]);
}

short win32joy_init(void)
{
   JoyMsg("win32joy_init()");

   HINSTANCE hInstWinMMLib;
   JOYINFOEX info;

   info.dwSize = sizeof(JOYINFOEX);
   info.dwFlags = JOY_RETURNALL;

   if (!pfnJoyGetPosEx)
   {
      hInstWinMMLib = LoadLibrary("winmm.dll");
      if (!hInstWinMMLib)
      {
         JoyMsg("Failed to load winmm.dll");
         return 0;
      }

      pfnJoyGetPosEx = (tJoyGetPosEx) GetProcAddress(hInstWinMMLib, "joyGetPosEx");
   }

   gbJoyAvailable = (joyGetDevCaps(JOYSTICKID1, (JOYCAPS *)&gJoyCaps, sizeof(gJoyCaps)) == JOYERR_NOERROR &&
                     pfnJoyGetPosEx != NULL &&
                     joyGetNumDevs() &&
                     pfnJoyGetPosEx(JOYSTICKID1, &info) != JOYERR_UNPLUGGED);

   if (gbJoyAvailable)
   {
      JoyMsg1("Success! returning %d", (gJoyCaps.wCaps & JOYCAPS_HASZ) ? 4 : (gJoyCaps.wCaps & JOYCAPS_HASR)? 3 : 2);
      return short((gJoyCaps.wCaps & JOYCAPS_HASZ) ? 4 : (gJoyCaps.wCaps & JOYCAPS_HASR)? 3 : 2);
   }

   JoyMsg4("Failed! (%d, %d, %d, %d)",
            (joyGetDevCaps(JOYSTICKID1, (JOYCAPS *)&gJoyCaps, sizeof(gJoyCaps)) == JOYERR_NOERROR),
            (pfnJoyGetPosEx != NULL),
            (joyGetNumDevs()),
            (pfnJoyGetPosEx(JOYSTICKID1, &info) != JOYERR_UNPLUGGED));

   return 0;
}

uchar win32joy_read_buttons(void)
{
   JOYINFOEX info;

   info.dwSize = sizeof(JOYINFOEX);
   info.dwFlags = (JOY_RETURNBUTTONS);

   if (gbJoyAvailable && pfnJoyGetPosEx(JOYSTICKID1, &info) != JOYERR_UNPLUGGED)
   {
      JoyMsg1("win32joy_read_buttons() returns %d", uchar(info.dwButtons));
      return uchar(info.dwButtons);
   }

   JoyMsg("win32joy_read_buttons() returns 0");
   return 0;
}

// Say whether each pot is dead or not
void win32joy_set_dead(bool dead[4])
{
   dead[0] = FALSE;
   dead[1] = FALSE;
   dead[2] = (gJoyCaps.wCaps & JOYCAPS_HASR)==0;
   dead[3] = (gJoyCaps.wCaps & JOYCAPS_HASZ)==0;
}

uchar fancy_win32( short *raw )
{
   JOYINFOEX info;

   if (raw!=NULL)
   {
      if (!(gJoyCaps.wCaps & JOYCAPS_HASZ))
          raw[3] = 0;
      JoyMsg("fancy_win32() returns 0");
      return 0;
   }

   if (!(gJoyCaps.wCaps & (JOYCAPS_HASPOV | JOYCAPS_POV4DIR | JOYCAPS_POVCTS)))
   {
      JoyMsg("fancy_win32() returns 0");
      return 0;
   }

   info.dwSize = sizeof(JOYINFOEX);
   info.dwFlags = JOY_RETURNBUTTONS;

   if (gJoyCaps.wCaps & JOYCAPS_POV4DIR)
      info.dwFlags |= JOY_RETURNPOV;

   else if (gJoyCaps.wCaps & JOYCAPS_POVCTS)
      info.dwFlags |= JOY_RETURNPOVCTS;

   if (gbJoyAvailable && pfnJoyGetPosEx(JOYSTICKID1, &info) != JOYERR_UNPLUGGED)
   {
      uchar uButtons = uchar(((uchar) info.dwButtons) & 0x0f);
      uchar uHatModifier = 0;
      if (info.dwFlags & JOY_RETURNPOV)
      {
         switch (info.dwPOV)
         {
            case JOY_POVBACKWARD:   uHatModifier = JOY_HAT_S; break;
            case JOY_POVFORWARD:    uHatModifier = JOY_HAT_N; break;
            case JOY_POVLEFT:	    uHatModifier = JOY_HAT_W; break;
            case JOY_POVRIGHT:      uHatModifier = JOY_HAT_E; break;
         }
      }
      else if (info.dwFlags & JOY_RETURNPOVCTS && info.dwFlags != (unsigned long) -1)
      {
         if (info.dwFlags < 4500)
            uHatModifier = JOY_HAT_N;

         else if (info.dwFlags < 13500)
            uHatModifier = JOY_HAT_E;

         else if (info.dwFlags < 22500)
            uHatModifier = JOY_HAT_S;

         else if (info.dwFlags < 31500)
            uHatModifier = JOY_HAT_W;

         else
            uHatModifier = JOY_HAT_N;
      }
      JoyMsg1("fancy_win32() returns 0x%x", (uButtons | uHatModifier));
      return (uButtons | uHatModifier);
   }
   JoyMsg("fancy_win32() returns 0");
   return 0;
}

#endif
