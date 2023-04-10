/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/scrnstr.h,v 1.1 1998/01/29 17:28:16 dc Exp $
// dorky put strings on screen hacks

#pragma once
#ifndef __SCRNSTR_H
#define __SCRNSTR_H

#ifdef PLAYTEST
EXTERN void AddScreenString(char *str);
#else
#define AddScreenString(str)
#endif

#endif  // __SCRNSTR_H
