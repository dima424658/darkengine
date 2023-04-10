/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/miscrprt.h,v 1.1 1998/10/21 23:41:32 dc Exp $
// misc report stuff

#pragma once

#ifndef __MISCRPRT_H
#define __MISCRPRT_H

#ifdef REPORT
EXTERN void MiscReportFuncsInit(void);
EXTERN void MiscReportFuncsTerm(void);
#else
#define MiscReportFuncsInit()
#define MiscReportFuncsTerm()
#endif

#endif  // __MISCRPRT_H
