/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aisched.h,v 1.2 2000/01/29 12:45:50 adurant Exp $
//
//
//
#pragma once

#ifndef __AISCHED_H
#define __AISCHED_H

struct sAIScheduleSettings
{
   BOOL fActive;
   long budget;
};

extern sAIScheduleSettings g_AIScheduleSettings;

#endif /* !__AISCHED_H */
