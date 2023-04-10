/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/ui/joypoll.h,v 1.2 1999/01/08 02:13:02 DC Exp $


#ifndef __JOYPOLL_H
#define __JOYPOLL_H
#pragma once

#include <inpbnd_i.h>

#define UI_EVENT_FRAME (0x40000000)

EXTERN void UiJoyInit(void);
EXTERN void uiJoystickPoller(void);
// set midpoint of x, y and rudders to current joystick position
EXTERN void uiSetControlCenter( void );

#ifdef __cplusplus
class cDarkJoyProcControl : public cIBJoyAxisProcess
{
public:
   cDarkJoyProcControl();
};

extern cDarkJoyProcControl g_dark_joy_control;
#endif

#endif
