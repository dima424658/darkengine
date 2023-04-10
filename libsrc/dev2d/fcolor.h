// $Header: x:/prj/tech/libsrc/dev2d/RCS/fcolor.h 1.2 1998/04/01 11:17:56 KEVIN Exp $

#ifndef __FCOLOR_H
#define __FCOLOR_H

// get an fcolor appropriate for the current screen mode given an 888rgb (red low)
// needs an ipal to work in 8 bit modes.

EXTERN int gr_make_screen_fcolor(int lrgb);

#endif
