/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/ged_line.h,v 1.6 2000/01/29 13:12:11 adurant Exp $
// line overlay system for gedit
#pragma once

// this system is responsible for overlaying lines in ged wireframe views
// it contains methods for adding lines to current lists, activating and deactivating them,
//   writing lists to files, reading from files, and so on

#ifndef __GED_LINE_H
#define __GED_LINE_H

#include <matrixs.h>

// preset channels
#define LINE_CH_USER      1
#define LINE_CH_REND      4
#define LINE_CH_SOUND     6
#define LINE_CH_PHYS      8
#define LINE_CH_LINKS    10
#define LINE_CH_AI       12
#define LINE_CH_ROOM     14

// public flags
#define LINE_FLG_DIR    (1<<0)  // this line is directional (somehow)
#define LINE_FLG_NO3D   (1<<1)  // dont draw in 3d views
#define LINE_FLG_NO2D   (1<<2)  // dont draw in 2d views
#define LINE_FLG_OVER   (1<<3)  // draw even if out of current hotregion
#define LINE_FLG_CURVE  (1<<4)  // not a straight line, but a curved one

// what channel do new lines go to
EXTERN int  ged_line_load_channel;
// what channels are currently being watched
EXTERN int  ged_line_view_channels;
// do we auto add the current channel on loads
EXTERN bool ged_line_autoremote;

// startup/slowdown
EXTERN void gedLineInit(void);
EXTERN void gedLineTerm(void);

// get a color from an RGB?
EXTERN int gedLineConvertRGB(int r, int g, int b);

// add to line setup
EXTERN void gedLineAddPal(const mxs_vector *p1, const mxs_vector *p2, int flags, int color);
EXTERN void gedLineAddRGB(const mxs_vector *p1, const mxs_vector *p2, int flags, int r, int g, int b);

// delete all lines in this channel
EXTERN void gedDeleteChannelLines(int channel);

// render all the lines in the setup
EXTERN void gedLineRenderAll(void);

// load a line data file into channel
EXTERN int gedLineReadFileToChannel(char *fname);

#endif  // __GED_LINE_H
