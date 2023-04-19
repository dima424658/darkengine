///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/winsrc/input/RCS/mousesys.cpp $
// $Author: TOML $
// $Date: 1997/01/28 12:11:00 $
// $Revision: 1.24 $
//
// Device independent mouse code
//

#ifdef _WIN32
#include <windows.h>
#endif

#include <lg.h>
#include <comtools.h>
#include <indevapi.h>
#include <appagg.h>
#include <defehand.h>

// @Note (toml 10-15-96): If/when the timer library is reworked, this dependency should also
#include <gshelapi.h>
#include <wappapi.h>
#include <inpcompo.h>

#include <stdlib.h>
#include <thrdtool.h>
#include <fixedque.h>

#include <lg.h>
#include <timer.h>
#include <tminit.h>

#include <recapi.h>

#include <error.h>
#include <mouse.h>
#include <mousevel.h>

#define __MOUSESYS_SRC

#include <mousesys.h>
#include <mousedev.h>
#include <inputimp.h>

//
// The Watcom C++ parser is verbose in warning of possible integral
// truncation, even in cases where for the given native word size
// there is no problem (i.e., assigning longs to  ints).  Here, we
// quiet the warnings, although it wouldn't be bad for someone to
// evaluate them. (toml 09-14-96)
//
#ifdef __WATCOMC__
#pragma warning 389 9
#endif

///////////////////////////////////////////////////////////////////////////////
//
// The mouse event queue
//

#define kMaxMouseEvents 32
typedef cFixedMTQueue<lgMouseEvent, kMaxMouseEvents> tMouseEventQueue;

tMouseEventQueue g_MouseEventQueue;

///////////////////////////////////////

EXTERN void LGAPI mouse_wait_for_queue_mutex(void)
{
    g_MouseEventQueue.Lock();
}

EXTERN void LGAPI mouse_release_queue_mutex(void)
{
    g_MouseEventQueue.Unlock();
}

HANDLE LGAPI mouse_get_queue_available_signal(void)
{
    return g_MouseEventQueue.GetAvailabilitySignalHandle();
}

///////////////////////////////////////////////////////////////////////////////

//  These are global for fast access from interrupt routine & others

short mouseInstantX;                             // instantaneous mouse xpos (int-based)
short mouseInstantY;                             // instantaneous mouse ypos (int-based)
short mouseInstantButts;
short mouseButtMask;                             // amt to mask to get buttons.
ubyte mouseXshift = 0;                           // Extra bits of mouse resolution
ubyte mouseYshift = 1;

ubyte mouseMask = 0xFF;                          // mask of events to put in the queue.
bool mouseLefty = FALSE;                         // is the user left-handed?

#define NUM_MOUSE_CALLBACKS 16
mouse_callfunc mouseCall[NUM_MOUSE_CALLBACKS];
void *mouseCallData[NUM_MOUSE_CALLBACKS];
short mouseCalls = 0;                            // current number of mouse calls.
short mouseCallSize = sizeof(mouse_callfunc);

bool mouse_installed = FALSE;                    // was mouse found?

#ifndef _WIN32
ulong default_mouse_ticks = 0;
ulong volatile *mouse_ticks = &default_mouse_ticks;     // Place to get mouse timestamps.
#define get_mouse_ticks() (*mouse_ticks)
#else
ulong default_mouse_ticks()
{
    if (g_pInputGameShell)
        return g_pInputGameShell->GetTimerTick();
    else
        return 0;
}

fn_mouse_ticks_t pfn_mouse_ticks = default_mouse_ticks; // Place to get mouse timestamps.
#define get_mouse_ticks() ((*pfn_mouse_ticks)())
#endif


// MOUSE VELOCITY STUFF

int mouseVelX = 0,
 mouseVelY = 0;
int mouseVelXmax = 0x7FFFFFFF;
int mouseVelYmax = 0x7FFFFFFF;
int mouseVelXmin = 0x80000000;
int mouseVelYmin = 0x80000000;

static short gXSize = 0,
 gYSize = 0;


//  Macros & defines

// i question heavily the device independence of this sort of thing
#define DEFAULT_XRATE 16  // Default mouse sensitivity parameters
#define DEFAULT_YRATE 8
#define DEFAULT_ACCEL 100
#define HIRES_XRATE 2
#define HIRES_YRATE 2
#define LO_RES_SCREEN_WIDTH 320


#ifdef _WIN32
// This is probably not an ideal place to put this hook, but will suffice.
// Mouse should be considered closely in light of this experience (toml 02-05-96)

#define SyncInstantToInputDevices() \
    if (g_pInputDevices)  \
        {            \
        int x, y, b; \
        if (IInputDevices_GetMouseState(g_pInputDevices, &x, &y, &b)) \
            {                              \
            mouseInstantX     = (short) x; \
            mouseInstantY     = (short) y; \
            mouseInstantButts = (short) b; \
            } \
        }     \
    else

#define SyncInputDevicesToInstant() \
    if (g_pInputDevices) \
        IInputDevices_SetMousePos(g_pInputDevices, mouseInstantX << mouseXshift , mouseInstantY << mouseYshift );

#else
#define SyncThisToInputDevices()
#define SyncInputDevicesToThis()
#endif

// Initialize the mouse, specifying screen size.
errtype mouse_init(short xsize, short ysize)
{
    // Windoze, mouse is always installed or the user is lame.
    mouse_installed = TRUE;

    if (mouse_installed)
    {
        DBG(DSRC_MOUSE_Init,
            {
            if (!mouse_installed)
                Warning(("mouse_init(): Mouse not installed\n"));
        })
        mouseInstantX = xsize / 2;
        mouseInstantY = ysize / 2;
        mouseInstantButts = 0;
        // Do the sensitivity scaling thang.
        mouse_set_screensize(xsize, ysize);
    }
    // /  AtExit(mouse_shutdown);

    // Return whether or not mouse found

    // @TBD (toml 05-16-96): This is a poor way to handle this. Should correct by creating
    // proper COM interface to the input libraries.
    GetInputComponents();
    ConnectInput();

    return (mouse_installed ? OK : ERR_NODEV);
}

// shutdown mouse system
void mouse_shutdown(void)
{
    // @TBD (toml 05-16-96): This is a poor way to handle this. Should correct by creating
    // proper COM interface to the input libraries.
    DisconnectInput();
    ReleaseInputComponents();
}

// ---------------------------------------------------------
// mouse_set_screensize() sets the screen size, scaling mouse sensitivity.
errtype mouse_set_screensize(short x, short y)
{
    short xrate = DEFAULT_XRATE,
     yrate = DEFAULT_YRATE,
     t = DEFAULT_ACCEL;
#ifndef _WIN32
    if (x > LO_RES_SCREEN_WIDTH)
    {
        xrate = HIRES_XRATE;
        yrate = HIRES_YRATE;
        mouseXshift = 3;
        mouseYshift = 3;
    }
    else
    {
        xrate /= 2;
        mouseXshift = 1;
        mouseYshift = 0;
    }
#else
    mouseXshift = 0;
    mouseYshift = 0;
#endif
    gXSize = x;
    gYSize = y;
    mouse_set_rate(xrate, yrate, t);
    mouse_constrain_xy(0, 0, x - 1, y - 1);
    mouse_put_xy(mouseInstantX, mouseInstantY);
    return OK;
}

//---------------------------------------------------------
// _mouse_update_vel() updates coordinates based on mouse
//  velocity.  Generates a motion event if there's any change to position.

#define sign(x) (((x) > 0) ? 1 : ((x) < 0) ? -1 : 0)

void _mouse_update_vel(void)
{
    static long x_remain,
     y_remain;
    static ulong last_ticks = 0;

    ulong ticks = get_mouse_ticks();
    if (ticks != last_ticks && (mouseVelX != 0 || mouseVelY != 0))
    {
        short newx = mouseInstantX;
        short newy = mouseInstantY;
        ulong dt = ticks - last_ticks;
        long xprod = mouseVelX * dt + x_remain;
        long yprod = mouseVelY * dt + y_remain;
        // compute sign and magnitude
        long xm = abs(xprod);
        long ym = abs(yprod);
        short xs = sign(xprod);
        short ys = sign(yprod);

        short dx = xs * (xm / MOUSE_VEL_UNIT);
        short dy = ys * (ym / MOUSE_VEL_UNIT);
        x_remain = xs * (xm % MOUSE_VEL_UNIT);
        y_remain = ys * (ym % MOUSE_VEL_UNIT);

        mouse_put_xy(newx + dx, newy + dy);
    }
    last_ticks = ticks;
}

// --------------------------------------------------------
// _mouse_get_xy() gets coords of mouse
errtype _mouse_get_xy(short *x, short *y)
{
    if (!mouse_installed)
    {
        Warning(("mouse_get_xy(): mouse not installed.\n"));
        return ERR_NODEV;
    }
    _mouse_update_vel();

    SyncInstantToInputDevices();

    *x = mouseInstantX;
    *y = mouseInstantY;

    Spew(DSRC_MOUSE_GetXY, ("_mouse_get_xy(): *x = %d, *y = %d\n", *x, *y));
    return OK;
}
// Set the mouse position
errtype mouse_put_xy(short x, short y)
{
    // mouse_state ms;
    Spew(DSRC_MOUSE_PutXY, ("_mouse_put_xy(%d,%d)\n", x, y));
    if (!mouse_installed)
    {
        Warning(("_mouse_put_xy(): mouse not installed.\n"));
        return ERR_NODEV;
    }

    mouseInstantX = x >> mouseXshift;
    mouseInstantY = y >> mouseYshift;

    SyncInputDevicesToInstant();

    if ((mouseMask & MOUSE_MOTION) != 0)
    {
        lgMouseEvent me;
        me.x = mouseInstantX;
        me.y = mouseInstantY;
        me.type = MOUSE_MOTION;
        me.timestamp = get_mouse_ticks();
        me.buttons = mouseInstantButts;
        mouse_generate(me);
    }
    return OK;
}
// Tell the mouse library where to get timestamps from.
#ifndef _WIN32
errtype mouse_set_timestamp_register(ulong * tstamp)
{
    mouse_ticks = tstamp;
    return OK;
}
#else
errtype mouse_set_timestamp_register(fn_mouse_ticks_t tstamp)
{
    pfn_mouse_ticks = tstamp;
    return OK;
}
#endif


// Get the current mouse timestamp
ulong mouse_get_time(void)
{
    return get_mouse_ticks();
}

// Check the state of a mouse button
//   res = ptr to result
//   button = button number 0-2

errtype _mouse_check_btn(int button, bool * res)
{
#ifndef _WIN32
    if (!mouse_installed)
    {
        Warning(("_mouse_check_btn(): mouse not installed.\n"));
        return ERR_NODEV;
    }
    *res = (mouseInstantButts >> button) & 1;
#else
    static const int buttonsToVKCodes[3] = {VK_LBUTTON, VK_RBUTTON, VK_MBUTTON};
    *res = (GetAsyncKeyState(buttonsToVKCodes[button]) < 0);
#endif
    Spew(DSRC_MOUSE_CheckBtn, ("_mouse_check_btn(%d,%x) *res = %d\n", button, res, *res));
    return OK;
}

// look at the next mouse event.
// -------------------------------------------------------
// _mouse_look_next gets the event in front the event queue,
// but does not remove the event from the queue.
// res = ptr to event to be filled.

errtype _mouse_look_next(lgMouseEvent * pMouseEvent)
{
    if (g_MouseEventQueue.IsEmpty())
    {
        if (g_pInputGameShell)
            g_pInputGameShell->PumpEvents(kPumpAll);
        else if (g_pInputWinApp)
            g_pInputWinApp->PumpEvents(kPumpAll, kPumpUntilEmpty);
    }

    if (g_MouseEventQueue.IsEmpty())
        _mouse_update_vel();

    if (!g_MouseEventQueue.PeekNext(pMouseEvent))
        return ERR_DUNDERFLOW;

    return OK;
}


// -------------------------------------------------------
// _mouse_next gets the event in the front event queue,
// and removes the event from the queue.
// res = ptr to event to be filled.
// Hey, keep in mind that mouseQueueOut and In can NEVER be invalid,
// not even for a sec, because  the interrupt handler might
// discover them in that invalid state and this would be BAD(TM).

errtype _mouse_next(lgMouseEvent * pMouseEvent)
{
    if (g_MouseEventQueue.IsEmpty())
    {
        if (g_pInputGameShell)
            g_pInputGameShell->PumpEvents(kPumpAll);
        else if (g_pInputWinApp)
            g_pInputWinApp->PumpEvents(kPumpAll, kPumpUntilEmpty);
    }

    if (g_MouseEventQueue.IsEmpty())
        _mouse_update_vel();

    if (!g_MouseEventQueue.GetNext(pMouseEvent))
        return ERR_DUNDERFLOW;

    return OK;
}

// -------------------------------------------------------
//
// mouse_flush() flushes the mouse event queue.

errtype mouse_flush(void)
{
    if (g_MouseEventQueue.IsEmpty())
    {
        if (g_pInputGameShell)
            g_pInputGameShell->PumpEvents(kPumpAll);
        else if (g_pInputWinApp)
            g_pInputWinApp->PumpEvents(kPumpAll, kPumpUntilEmpty);
    }

    g_MouseEventQueue.Flush();
    return OK;
}

// -------------------------------------------------------
//
// mouse_generate() adds an event to the back of the
//  mouse event queue.  If this overflows the queue,

errtype mouse_generate(lgMouseEvent mouseEvent)
{
    errtype result;

    if (g_MouseEventQueue.Add(&mouseEvent))
        result = OK;
    else
        result = ERR_DOVERFLOW;

    mouseInstantX = mouseEvent.x;
    mouseInstantY = mouseEvent.y;
    mouseInstantButts = mouseEvent.buttons;

    for (int i = 0; i < mouseCalls; i++)
        if (mouseCall[i] != NULL)
            mouseCall[i] (&mouseEvent, mouseCallData[i]); // @TBD (toml 11-06-96): need to trace through this to verify switching to passing clients "&mouseEvent" is cool (copied, not stored)

    return result;
}

// ------------------------------------------------------
//
// mouse_set_callback() registers a callback with the interrupt handler
// f = func to be called back.
// data = data to be given to the func when called
// *id = set to a unique id of the callback.

errtype mouse_set_callback(mouse_callfunc f, void *data, int *id)
{
    Spew(DSRC_MOUSE_SetCallback, ("entering mouse_set_callback(%x,%x,%x)\n", f, data, id));
    for (*id = 0; *id < mouseCalls; ++*id)
        if (mouseCall[*id] == NULL)
            break;
    if (*id == NUM_MOUSE_CALLBACKS)
    {
        Spew(DSRC_MOUSE_SetCallback, ("mouse_set_callback(): Table Overflow.\n"));
        return ERR_DOVERFLOW;
    }
    if (*id == mouseCalls)
        mouseCalls++;
    Spew(DSRC_MOUSE_SetCallback, ("mouse_set_callback(): *id = %d, mouseCalls = %d\n", *id, mouseCalls));
    mouseCall[*id] = f;
    mouseCallData[*id] = data;
    return OK;
}

// -------------------------------------------------------
//
// mouse_unset_callback() un-registers a callback function
// id = unique id of function to unset

errtype mouse_unset_callback(int id)
{
    Spew(DSRC_MOUSE_UnsetCallback, ("entering mouse_unset_callback(%d)\n", id));
    if (id >= mouseCalls || id < 0)
    {
        Spew(DSRC_MOUSE_UnsetCallback, ("mouse_unset_callback(): id out of range \n"));
        return ERR_RANGE;
    }
    mouseCall[id] = NULL;
    while (mouseCalls > 0 && mouseCall[mouseCalls - 1] == NULL)
        mouseCalls--;
    return OK;
}

// Constrain the mouse coordinates
errtype mouse_constrain_xy(short xl, short yl, short xh, short yh)
{
    Spew(DSRC_MOUSE_ConstrainXY, ("mouse_constrain_xy(%d,%d,%d,%d)\n", xl, yl, xh, yh));
    if (!mouse_installed)
    {
        Warning(("mouse_constrain_xy(): mouse not installed.\n"));
        return ERR_NODEV;
    }
    return OK;
}

// Set the mouse rate and accelleration threshhold
errtype mouse_set_rate(short xr, short yr, short thold)
{
    return OK;
}

// Get the mouse rate and accelleration threshhold
errtype _mouse_get_rate(short *xr, short *yr, short *thold)
{
    return OK;
}

// Find the min and max "virtual" coordinates of the mouse position
errtype _mouse_extremes(short *xmin, short *ymin, short *xmax, short *ymax)
{
    *xmin = 0;
    *ymin = 0;
    *xmax = gXSize;
    *ymax = gYSize;
    Spew(DSRC_MOUSE_Extremes, ("_mouse_extremes(): <%d %d> to <%d %d>\n", *xmin, *ymin, *xmax, *ymax));
    return OK;
}

#define SHIFTDIFF 1
// ------------------------------------------------------
static short shifted_button_state(short bstate)
{
    short tmp = (bstate & MOUSE_RBUTTON) >> SHIFTDIFF;
    tmp |= (bstate & MOUSE_LBUTTON) << SHIFTDIFF;
    tmp |= bstate & MOUSE_CBUTTON;
    return tmp;
}

// mouse_set_lefty() sets mouse handedness (true for left-handed)
errtype mouse_set_lefty(bool lefty)
{
    if (lefty == mouseLefty)
        return ERR_NOEFFECT;
    mouseInstantButts = shifted_button_state(mouseInstantButts);
    mouseLefty = lefty;
    return OK;
}

#define MOUSE_VEL_UNIT_SHF 16
#define MOUSE_VEL_UNIT (1 << MOUSE_VEL_UNIT_SHF)

// Specifies the range of the mouse pointer velocity.
// (xl,yl) is the low end of the range, whereas (xh,yh)
// is the high end.  For most applications xl and xh will have the same
// absolute value, as with yl and yh.
errtype mouse_set_velocity_range(int xl, int yl, int xh, int yh)
{
    mouseVelXmin = xl;
    mouseVelYmin = yl;
    mouseVelXmax = xh;
    mouseVelYmax = yh;
    return OK;
}

// Sets the velocity of the mouse pointer,
// in units of pixels per MOUSE_VEL_UNIT ticks.
errtype mouse_set_velocity(int x, int y)
{
    mouseVelX = max(mouseVelXmin, min(x, mouseVelXmax));
    mouseVelY = max(mouseVelYmin, min(y, mouseVelYmax));
    if (mouseVelX != x || mouseVelY != y)
        return ERR_RANGE;
    return OK;
}

// Adds x and y to the mouse pointer velocity, constraining it
// to remain withing the range specified by mouse_set_velocity_range()
errtype mouse_add_velocity(int x, int y)
{
    return mouse_set_velocity(mouseVelX + x, mouseVelY + y);
}

// Gets the current value of the mouse pointer velocity, as
// set by mouse_set_velocity and mouse_add_velocity.
errtype mouse_get_velocity(int *x, int *y)
{
    *x = mouseVelX;
    *y = mouseVelY;
    return OK;
}
