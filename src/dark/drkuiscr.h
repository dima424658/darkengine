/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkuiscr.h,v 1.4 1998/11/02 02:59:27 mahk Exp $
#pragma once  
#ifndef __DRKUISCR_H
#define __DRKUISCR_H

#include <drkuiscrapi.h>
#include <uigame.h>

////////////////////////////////////////////////////////////
// DARK UI SCRIPT SERVICE 
//


#ifdef SCRIPT
inline int rgb(int r, int g, int b) { return uiRGB(r,g,b); }; 
#endif // SCRIPT

#endif // __DRKUISCR_H
