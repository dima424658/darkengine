/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/ctimer.h,v 1.4 2000/01/29 13:38:39 adurant Exp $
#pragma once

/* --<<= --/-/-/-/-/-/-/ <<< ((( ((( /\ ))) ))) >>> \-\-\-\-\-\-\-- =>>-- *\

   ctimer.h

   This is a simple timing system for C.

\* --<<= --\-\-\-\-\-\-\ <<< ((( ((( \/ ))) ))) >>> /-/-/-/-/-/-/-- =>>-- */

#include <mprintf.h>
#include <timer.h>

extern void CTimerInit();
extern void CTimerDump();
extern void CTimerFrame();

enum eCTimer
{
   // rendering
   kCTimerRendTotal,
   kCTimerRendObjTotal,
   kCTimerRendMesh,
   kCTimerRendMD,

   kCTimerNumTimers,
   kCTimerFiller = 0xffffffff
};

typedef struct sCTimer
{
   ulong m_iStartTime;
   ulong m_iTotalTime;
} sCTimer;

extern sCTimer g_aCTimers[];
extern int g_iCTimerFrames;

// This is for activating this style of timers globally.  Each file using
// them could also have a line like this for more selective profiling.
//#define CTIMER_ON

////////////////////////////////////////////////////////////
// when CTIMER_ON is defined
//
#ifdef CTIMER_ON

#define CTIMER_START(dCTimerIndex) \
{ g_aCTimers[dCTimerIndex].m_iStartTime = tm_get_millisec_unrecorded(); }

#define CTIMER_END(dCTimerIndex) \
{ g_aCTimers[dCTimerIndex].m_iTotalTime \
     += (tm_get_millisec_unrecorded() \
         - g_aCTimers[dCTimerIndex].m_iStartTime); }

#define CTIMER_RETURN(dCTimerIndex) \
{ CTIMER_END(dCTimerIndex); return; }

#define CTIMER_RETVAL(dCTimerIndex, dCTimerRetVal) \
{ CTIMER_END(dCTimerIndex); return (dCTimerRetVal); }

////////////////////////////////////////////////////////////
// when CTIMER_ON is not defined
//
#else // CTIMER_ON

#define CTIMER_START(dCTimerIndex)
#define CTIMER_END(dCTimerIndex)
#define CTIMER_RETURN(dCTimerIndex) \
{ return; }
#define CTIMER_RETVAL(dCTimerIndex, dCTimerRetVal) \
{ return (dCTimerRetVal); }

#endif // CTIMER_ON

