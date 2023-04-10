/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/ai/aireport.h,v 1.2 2000/01/29 12:45:49 adurant Exp $
// woo woo
#pragma once

#ifndef __AIREPORT_H
#define __AIREPORT_H

#ifdef REPORT
EXTERN void AIReportInit(void);
EXTERN void AIReportTerm(void);
#else
#define AIReportInit()
#define AIReportTerm()
#endif

#endif  // __AIREPORT_H
