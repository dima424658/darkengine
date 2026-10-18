///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lg/RCS/memreq.cpp $
// $Author: TOML $
// $Date: 1997/01/20 15:24:06 $
// $Revision: 1.1 $
//

#include <lg.h>
#include <memreq.h>
#include <platform_services.h>

///////////////////////////////////////////////////////////////////////////////

EXTERN BOOL LGAPI
VerifyRequirements(const sMemoryRequirements * pRequirements, BOOL fQuiet)
{
   sPlatformMemoryStatus memoryStatus;
   BOOL fEnoughMemory = TRUE;

   PlatformGetMemoryStatus(&memoryStatus);

   if (memoryStatus.total_physical_bytes < pRequirements->physicalMemory)
      fEnoughMemory = FALSE;

   if (memoryStatus.available_page_file_bytes < pRequirements->swapSpace)
      fEnoughMemory = FALSE;

   if (pRequirements->freeMemory)
   {
      void * p = malloc(pRequirements->freeMemory);
      if (!p)
        fEnoughMemory = FALSE;
      free(p);
   }

   if (!fQuiet && !fEnoughMemory)
      PlatformShowError("Dark Engine",
                        "Sorry, there is not enough free memory to run.\n"
                        "Please close any other applications and check "
                        "your virtual memory settings.");
   return fEnoughMemory;
}

///////////////////////////////////////////////////////////////////////////////
