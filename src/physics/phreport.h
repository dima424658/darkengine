/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phreport.h,v 1.2 2000/01/31 09:52:07 adurant Exp $
//
// Physics report generation header
//
#pragma once

#ifndef __PHREPORT_H
#define __PHREPORT_H

#ifdef REPORT

EXTERN void InitPhysReports();
EXTERN void TermPhysReports();

#else

#define InitPhysReports()
#define TermPhysReports()

#endif

#endif
