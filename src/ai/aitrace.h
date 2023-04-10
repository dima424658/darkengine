/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aitrace.h,v 1.2 2000/01/31 09:34:12 adurant Exp $
//
//
//
#pragma once

#ifndef __AITRACE_H
#define __AITRACE_H

enum eAITraceCategories
{
   kAIT_M, // Management
   kAIT_D, // Decision
   kAIT_P, // Pathfind
   kAIT_G, // General
};


#define AITraceMsg(level, category, s)
#define AITraceMsg1(level, category, s, a)
#define AITraceMsg2(level, category, s, a, b)
#define AITraceMsg3(level, category, s, a, b, c)
#define AITraceMsg4(level, category, s, a, b, c, d)
#define AITraceMsg5(level, category, s, a, b, c, d, e)
#define AITraceMsg6(level, category, s, a, b, c, d, e, f)
#define AITraceMsg7(level, category, s, a, b, c, d, e, f, g)
#define AITraceMsg8(level, category, s, a, b, c, d, e, f, g, h)
#define AITraceMsg9(level, category, s, a, b, c, d, e, f, g, h, i)

#endif /* !__AITRACE_H */
