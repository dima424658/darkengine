/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/ctimer.c,v 1.2 2000/02/19 12:35:16 toml Exp $

/* --<<= --/-/-/-/-/-/-/ <<< ((( ((( /\ ))) ))) >>> \-\-\-\-\-\-\-- =>>-- *\

   ctimer.c

   This is a simple timing system for C.  Real, real simple.
   We're talking simple here, ok?

\* --<<= --\-\-\-\-\-\-\ <<< ((( ((( \/ ))) ))) >>> /-/-/-/-/-/-/-- =>>-- */

#include <ctimer.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


#ifndef SHIP

////////////////////////////////
// global arrays and such

sCTimer g_aCTimers[kCTimerNumTimers];
int g_iCTimerFrames;


////////////////////////////////
// functions

void CTimerInit()
{
   int i;

   for (i = 0; i < kCTimerNumTimers; ++i) {
      g_aCTimers[i].m_iTotalTime = 0;
   }
   g_iCTimerFrames = 0;
}


void CTimerDump()
{
   int i;
   float fTotalTime = 0.0;
   float fFrames = g_iCTimerFrames;

   mprintf("C Timers for %d frames:\n", g_iCTimerFrames);
   mprintf("   timer  millisec    percent  millisec/frame\n");

   for (i = 0; i < kCTimerNumTimers; ++i)
      fTotalTime += g_aCTimers[i].m_iTotalTime;

   for (i = 0; i < kCTimerNumTimers; ++i) {
      mprintf("%8d  %8d  %8.3g  %8.3g\n", 
              i, 
              g_aCTimers[i].m_iTotalTime, 
              100.0 * ((float) g_aCTimers[i].m_iTotalTime) / fTotalTime,
              ((float) g_aCTimers[i].m_iTotalTime) / fFrames);
   }
}


void CTimerFrame()
{
   ++g_iCTimerFrames;
}

#endif // ~SHIP
