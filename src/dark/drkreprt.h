/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkreprt.h,v 1.3 1998/09/25 12:55:16 TOML Exp $
// dark specfic report functions

#pragma once

#ifndef __DRKREPRT_H
#define __DRKREPRT_H

/////////////////////////
// hey look

#ifdef REPORT
EXTERN void DarkReportInit(void);
EXTERN void DarkReportTerm(void);
#else
#define DarkReportInit()
#define DarkReportTerm()
#endif

#endif  // __DRKREPRT_H
