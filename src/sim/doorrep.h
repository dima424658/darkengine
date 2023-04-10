/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sim/doorrep.h,v 1.2 2000/01/29 13:41:02 adurant Exp $
//
// Door report generation header
//
#pragma once

#ifndef __DOORREP_H
#define __DOORREP_H

#ifdef REPORT

EXTERN void InitDoorReports();
EXTERN void TermDoorReports();

#else

#define InitDoorReports()
#define TermDoorReports()

#endif

#endif
