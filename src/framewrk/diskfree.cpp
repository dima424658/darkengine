/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <config.h>
#include <diskfree.h>

#include <limits.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <win32_platform.h>
#else
#include <stdint.h>
#include <sys/statvfs.h>
#endif

extern "C" int compute_free_diskspace(char *path)
{
#ifdef _WIN32
   ULARGE_INTEGER freeBytesAvailable;

   if (GetDiskFreeSpaceExA(path, &freeBytesAvailable, NULL, NULL))
      return freeBytesAvailable.QuadPart > INT_MAX
         ? INT_MAX : (int)freeBytesAvailable.QuadPart;
#else
   struct statvfs info;
   unsigned long long bytes;

   if (statvfs(path && *path ? path : ".", &info) == 0)
   {
      bytes = (unsigned long long)info.f_bavail * info.f_frsize;
      return bytes > INT_MAX ? INT_MAX : (int)bytes;
   }
#endif

   return -1;
}

#define MB_SHIFT 20
#define GetMB(x) ((x) << MB_SHIFT)

extern "C" BOOL CheckForDiskspaceAndMessage(char *path, int minMB)
{
   int space = compute_free_diskspace(path);
   if (space < 0)
      return TRUE;

   if (space < GetMB(minMB))
   {
#ifdef _WIN32
      char buffer[512], tmp_buf[512];
      char lang[32], cfgvar[64];

      if (!config_get_raw("language", lang, 32))
         strcpy(lang, "english");
      strcpy(cfgvar, "no_diskspace_");
      strcat(cfgvar, lang);
      if (!config_get_raw(cfgvar, tmp_buf, 512))
         strcpy(tmp_buf, "You have %dM of free disk space.  Thief 2 requires %dM.\n");
      sprintf(buffer, tmp_buf, space >> MB_SHIFT, minMB);
      MessageBox(NULL, buffer, "Thief 2", MB_ICONERROR);
#else
      fprintf(stderr, "Dark Engine needs %d MB free; only %d MB is available.\n",
              minMB, space / (1024 * 1024));
#endif
      return FALSE;
   }

   return TRUE;
}

#ifndef _WIN32
typedef void (*EditorMenuCommandCallback)(unsigned);
static EditorMenuCommandCallback g_editorMenuCommandCallback;

extern "C" void EditorPlatformSetMenuCommandCallback(
   EditorMenuCommandCallback callback)
{
   g_editorMenuCommandCallback = callback;
}

extern "C" void MenuCommand(unsigned command)
{
   if (g_editorMenuCommandCallback)
      g_editorMenuCommandCallback(command);
}
#endif
