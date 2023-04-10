/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/propinst.h,v 1.1 1998/10/01 18:40:38 mahk Exp $
#pragma once  
#ifndef __PROPINST_H
#define __PROPINST_H

////////////////////////////////////////////////////////////
// PROPERTY MEMORY INSTRUMENTATION 
// 

#ifdef PROPERTY_BLAME

//
// copy the name into a static buffer, return a pointer to 
// be used for blaming 
//
EXTERN const char* PropBlameName(const char* name); 
#define PROP_BLAME_NAME(x) PropBlameName(x)

//
// Push property blame
// 
EXTERN void PropPushBlame(const char* name);
#define PROP_PUSH_BLAME(x) PropPushBlame(x)

//
// Pop property blame
// 
EXTERN void PropPopBlame(void);
#define PROP_POP_BLAME() PropPopBlame()

class cPropAutoBlamer
{
public:
   cPropAutoBlamer(const char* s)
   {
      PropPushBlame(s); 
   }

   ~cPropAutoBlamer()
   {
      PropPopBlame(); 
   }
}; 

#define PROP_AUTO_BLAME(s) cPropAutoBlamer blamer__(s)

#else // !PROPERTY_BLAME

#define PROP_BLAME_NAME(x) (x)
#define PROP_PUSH_BLAME(x)
#define PROP_POP_BLAME() 
#define PROP_AUTO_BLAME(x)

#endif  // !PROPERTY_BLAME 

EXTERN BOOL gBlameProperties;


////////////////////////////////////////////////////////////
// PROPERTY TIME INSTRUMENTATION
//

#ifdef PROPERTY_TIME

class cPropTimer 
{
protected:
   ulong* mpStat; 
   ulong* mpStatBack; 
   long start; 

   static ulong gDummyStat; 

   static long GetTime(); 

public:

   void Start()
   {
      start = GetTime(); 
      mpStat = mpStatBack; 
   }

   void Stop()
   {
      *mpStat += GetTime() - start; 
      mpStat = &gDummyStat; 
   }

   void Restart(ulong* pstat)
   {
      mpStatBack = pstat; 
      Start(); 
   }

   cPropTimer(ulong* pstat)
      : mpStatBack(pstat)
   {
      Start(); 
   }


   ~cPropTimer()
   {
      Stop();
   }

}; 

//
// MACROS FOR CREATING TIMERS 
//

#define PROP_TIMER_ULONG_PTR(x) cPropTimer prop_timer__(x)
#define PROP_TIMER_STATS_IDX(s,i) PROP_TIMER_ULONG_PTR((ulong*)&(s).time[i])
#define PROP_TIMER_STOP() prop_timer__.Stop()
#define PROP_TIMER_START() prop_timer__.Start()
#define PROP_TIMER_RESTART_STATS_IDX(s,i) prop_timer__.Restart((ulong*)&(s).time[i])


#else // !PROPERTY_TIME

#define PROP_TIMER_ULONG_PTR(x)
#define PROP_TIMER_STATS_IDX(s,i)
#define PROP_TIMER_STOP() 
#define PROP_TIMER_START() 
#define PROP_TIMER_RESTART_STATS_IDX(s,i) 

#endif // !PROPERTY_TIME

#endif // __PROPINST_H



