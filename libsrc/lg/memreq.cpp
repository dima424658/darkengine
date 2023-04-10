///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lg/RCS/memreq.cpp $
// $Author: TOML $
// $Date: 1997/01/20 15:24:06 $
// $Revision: 1.1 $
//

#ifdef _WIN32
#include <windows.h>
#endif
#include <lg.h>
#include <memreq.h>

///////////////////////////////////////////////////////////////////////////////

EXTERN BOOL LGAPI
VerifyRequirements(const sMemoryRequirements * pRequirements, BOOL fQuiet)
{
#ifdef _WIN32
   MEMORYSTATUS memoryStatus;
   BOOL fEnoughMemory = TRUE;

   memoryStatus.dwLength = sizeof(memoryStatus);
   GlobalMemoryStatus(&memoryStatus);

   if (memoryStatus.dwTotalPhys < pRequirements->physicalMemory)
      fEnoughMemory = FALSE;

   if (memoryStatus.dwAvailPageFile < pRequirements->swapSpace)
      fEnoughMemory = FALSE;

   if (pRequirements->freeMemory)
   {
      void * p = malloc(pRequirements->freeMemory);
      if (!p)
        fEnoughMemory = FALSE;
      free(p);
   }

   if (!fQuiet && !fEnoughMemory)
      MessageBox(NULL, "Sorry, there is not enough free memory to run.\n"
                       "Please close any other applications and check "
                       "your virtual memory settings.", NULL, MB_OK | MB_ICONHAND);
   return fEnoughMemory;
#else
   return TRUE;
#endif
}

///////////////////////////////////////////////////////////////////////////////
