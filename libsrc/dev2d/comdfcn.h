//------------------------------------------------------------------------------
// $Source: x:/prj/tech/libsrc/dev2d/RCS/comdfcn.h $
// $Author: KEVIN $
// $Date: 1998/03/12 10:40:23 $
// $Revision: 1.4 $
//------------------------------------------------------------------------------
// Description :
// This file contains all of the COM-based device functions that can be
// accessed from the device function table.
//------------------------------------------------------------------------------

#ifndef __COM_DEVICE_FUNCTIONS_H
#define __COM_DEVICE_FUNCTIONS_H

#include <grs.h>
#include <comtools.h>

F_DECLARE_INTERFACE(IDisplayDevice);
typedef struct _VideoState VideoState;


//------------------------------------------------------------------------------
// Make the functions look like C functions (but we'll implement them
// however we like)
//------------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------------------------------------
// This stores the interface to the display device that the COM device
// functions use.
//-----------------------------------------------------------------------------

extern IDisplayDevice * g_pDev2dDisplayDevice;

//------------------------------------------------------------------------------
// This function initializes the device and sets the mode list in
// INFO to indicate all of the currently supported modes. It doesn't
// necessarily create a new window.
//------------------------------------------------------------------------------

int   com_init(grs_sys_info *info);

//------------------------------------------------------------------------------
// This function closes the device and destroys any currently active
// windows.
//------------------------------------------------------------------------------

int   com_close(grs_sys_info *info);

//------------------------------------------------------------------------------
// Set the mode, creating a new window (and perhaps clearing the window
// as well.
//------------------------------------------------------------------------------

int   com_set_mode(int mode, int clear);

//------------------------------------------------------------------------------
// Return the mode associated with the current window.
//------------------------------------------------------------------------------

int   com_get_mode(void);

//------------------------------------------------------------------------------
// Set the palette associated with the window.
//------------------------------------------------------------------------------

void  com_set_pal(int start, int n, uint8 *pal);

//------------------------------------------------------------------------------
// Get the palette associated with the window.
//------------------------------------------------------------------------------

void  com_get_pal(int start, int n, uint8 *pal);

//------------------------------------------------------------------------------
// Set screen width
//------------------------------------------------------------------------------

int  com_set_width(int w);

//------------------------------------------------------------------------------
// Set the focus (page flip)
//------------------------------------------------------------------------------

void  com_set_focus(int x, int y);

//------------------------------------------------------------------------------
// Get the focus. We're spoofing this to only allow pageflipping, not
// panning. So there.
//------------------------------------------------------------------------------

void  com_get_focus(int *x, int *y);

//------------------------------------------------------------------------------
// Save the video state before we take over the screen. We're assuming
// that this is only called at startup time, and not during the middle
// of operation, which is a subset of what the 2D actually assumes (it
// assumes you can save and restore any time).
//------------------------------------------------------------------------------

int   com_save_state(VideoState *state);

//------------------------------------------------------------------------------
// Restore a saved video state.
//------------------------------------------------------------------------------

int   com_restore_state(VideoState *state);

//------------------------------------------------------------------------------
// Check the status of the horitzontal retrace (??)
//------------------------------------------------------------------------------

int   com_stat_htrace(void);

//------------------------------------------------------------------------------
// Returns TRUE if we're in the blanking interval.
//------------------------------------------------------------------------------

int   com_stat_vtrace(void);

//------------------------------------------------------------------------------
// Get color bitmasks for the current mode
//------------------------------------------------------------------------------

void com_get_rgb_bitmask(grs_rgb_bitmask *bitmask);

#ifdef __cplusplus
}
#endif

#endif
