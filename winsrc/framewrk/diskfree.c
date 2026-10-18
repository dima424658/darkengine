/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/diskfree.c,v 1.5 2000/02/19 13:16:36 toml Exp $

#include <config.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

#include <diskfree.h>

#define WIN32_LEAN_AND_MEAN
#include <win32_platform.h>
#include <memall.h>
#include <dbmem.h>   // must be last header!

int compute_free_diskspace(char *str)
{
   ULARGE_INTEGER freeBytesAvailable;

   // GetDiskFreeSpace returns three 32-bit factors.  Multiplying them in an
   // unsigned long wraps on modern large volumes before the old INT_MAX cap
   // can be applied (for example, a healthy drive can appear to have 8 MB).
   // Ask Windows for the already-combined 64-bit byte count instead.
   if (GetDiskFreeSpaceExA(str, &freeBytesAvailable, NULL, NULL))
      return freeBytesAvailable.QuadPart > INT_MAX
         ? INT_MAX : (int)freeBytesAvailable.QuadPart;

   return -1;
}

#define MB_SHIFT 20
#define GetMB(x) (x<<MB_SHIFT)

BOOL CheckForDiskspaceAndMessage(char *str, int minMB)
{
   int space = compute_free_diskspace(str); // current drive
   if (space == -1) {
      // the query failed, oops
      // let's assume everything is ok, since it would suck
      // if everything was ok except this call mysteriously failing
      // but if we were smart we'd somehow note this somewhere so
      // if something goes wrong later the user could tell tech
      // support about this failing
      return TRUE;
   }
   else if (space < GetMB(minMB))
   {
      char buffer[512], tmp_buf[512];
      char lang[32], cfgvar[64];

      if (!config_get_raw("language",lang,32))
         strcpy(lang,"english");    // we are such self-centered nationalist twerps
      strcpy(cfgvar,"no_diskspace_");
      strcat(cfgvar,lang);
      if (!config_get_raw(cfgvar,tmp_buf,512))
         strcpy(tmp_buf,"You have %dM of free disk space.  Thief 2 requires %dM.\n");
      sprintf(buffer, tmp_buf, space >> MB_SHIFT, minMB);
      MessageBox(NULL, buffer, "Thief 2", MB_ICONERROR);
      return FALSE;
   }
   return TRUE;
}
