/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <lg.h>
#include <config.h>
#include <cfgdbg.h>

#include <comtools.h>
#include <gshelapi.h>
#include <dispapi.h>
#include <appapi.h>
#include <appagg.h>
#include <loopapi.h>

#include <init.h>
#include <allocovr.h>

// must be last header
#include <dbmem.h>


#ifdef _WIN32
#define MAIN AppMain
#else
#define MAIN main
#endif


int LGAPI MAIN(int argc, const char* argv[])
{
   ILoop* looper = AppGetObj(ILoop);

   // Start the main loop
   ILoop_Go(looper,gPrimordialMode);
   SafeRelease(looper);

   return 0;
}




