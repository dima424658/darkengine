/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkrphlp.h,v 1.4 1998/10/19 01:12:30 dc Exp $

#pragma once

#ifndef __REPHELP_H
#define __REPHELP_H

#ifdef  REPORT

#include <osysbase.h>
#include <matrixs.h>
#include <reprthlp.h>

///////////////    
// various "code aided" report helpers for dark

// call these around using any code helpers
//   so we dont need to get a bunch of properties by name each time
EXTERN void  DarkReportHelpersStartUse(void);
EXTERN void  DarkReportHelpersDoneWith(void);

// fills in at buffer, returns it, if buffer null, puts it in a static buffer
EXTERN char *DarkReportHelperDiffString(ObjID obj, char *buffer);
// short form so your code can be made to fit on a finite number of pages
#define DRHDiffS(obj)        DarkReportHelperDiffString(obj,NULL)

#endif  // REPORT
   
#endif  // __REPHELP_H
