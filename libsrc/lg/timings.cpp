///////////////////////////////////////////////////////////////////////////////
// $Header: x:/prj/tech/libsrc/lg/RCS/timings.cpp 1.4 1998/05/22 14:17:49 kate Exp $
//
//
//

#include <stdio.h>
#include <timings.h>


///////////////////////////////////////////////////////////////////////////////

void SimpleTimerOutputResult(const char * pszName, ulong totalTime)
{
    mprintf("Total time for %s: %lu ms\n", pszName, totalTime);
}

///////////////////////////////////////

void DiffTimerOutputResult(const char * pszName, ulong diffTime)
{
    mprintf("Total time for %s: %lu ms\n", pszName, diffTime);
}

///////////////////////////////////////

void AverageTimerOutputResult(const char * pszName, double avg, ulong max, ulong min, ulong totalTime, ulong iters)
{
   char buf[256];
   
   sprintf(buf, 
          "%7lu ms; %6lu iters; avg %#8.6f ms;%3lu;%3lu: %-22s\n",
          totalTime, iters, avg, max, min, pszName);
   
   mprint(buf);
}

///////////////////////////////////////////////////////////////////////////////
