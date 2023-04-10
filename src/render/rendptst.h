/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/rendptst.h,v 1.2 2000/01/31 09:53:13 adurant Exp $
// rendobj playtest stuff (some ifndef SHIP, some just PLAYTEST)
#pragma once

#ifndef __RENDPTST_H
#define __RENDPTST_H

#ifdef PLAYTEST
EXTERN void RendPlaytestCommandsInit(void);
#else
#define RendPlaytestCommandsInit()
#endif

#endif  // __RENDPTST_H
