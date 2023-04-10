/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/headmove.h,v 1.12 2000/01/29 13:41:15 adurant Exp $
#pragma once

#ifndef __HEADMOVE_H
#define __HEADMOVE_H

#include <objtype.h>

#define RELAX_MODE_INSTANT   1
#define RELAX_MODE_ONMOVE    2
#define RELAX_MODE_NEVER     3

#define RELAX_DEF_TIME  400   // how long to spend returning to rest (in ms)
#define RELAX_DEF_PAUSE 750   // default wait in instant mode

// parses the config file for reset_* stuff
EXTERN void headmoveInit(void);


//sets the relative movement of the mouse for use by the headmove code
EXTERN void headmoveSetRelPosX (float x);
EXTERN void headmoveSetRelPosY (float y);

//sets the current turn state as controlled by the keyboard
EXTERN void headmoveSetKeyboardInput(float turn_rate);
EXTERN void headmoveKeyboardReset(void);

//sets the current lookup/down rate as controlled by the keyboard.
EXTERN void headmoveSetKeyboardTilt(float tilt_rate);
EXTERN void headmoveKeyboardTiltReset(void);

// if you want to adjust mouse control of head movement.
// negative values cause default values to be used
// speed is in fixang/100 ms, and zone is pixel radius in
// which move movement is ignored.
EXTERN void headmoveSetMouseParams(int speed, int zone);

// if you want to change head motion stuff
// currently defaults to mode=ONMOVE, param N/A, rtime 400ms as above
EXTERN void headmoveSetup(int rmode, int param, int rtime);

// To allow external head-moving systems that the head is being used
EXTERN void headmoveTouch(void);

// Actual call to make in from the app
EXTERN void headmoveCheck(struct Camera *cam, int ms);

// if you want the camera resetted now
// instant means just do it, instant=FALSE means use the relaxtime
EXTERN void headmoveReset(struct Camera *cam, bool instant);

// this does frame to frame head tracking and camera/body insta-connection
EXTERN void playerHeadControl(void);

EXTERN int   headmove_ang_disp;   // net angular displacement of the head
EXTERN float headmove_vec2_disp;  // net displacement squared of the head in space
EXTERN uchar headbody_axis;       // which axis are insta-connected
       // axis is a bitfield, (1<<0) is x, (1<<1) is y, (1<<2) is z, default currently (0100)

#endif  // __HEADMOVE_H
