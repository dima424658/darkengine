/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/netvoice.cpp,v 1.1 1999/04/28 14:36:58 Justin Exp $
//
// Voice chat code
//
// This currently just invokes Roger Wilco if the user has requested it
// and has it installed
//
// The code is essentially boilerplate, which was given to use by RW.
//

#include <windows.h>
#include <netvoice.h>

// This should be last:
#include <dbmem.h>

/* Use this function if you have an IP address in string form for the
   Roger Wilco channel host. For example, "foo.some-domain.com" or
   "10.213.8.42". If you want to create a channel rather than join
   one, call this with a NULL parameter.
 */
void StartVoiceChat( char *address )
{
   HKEY  hKey;
   char  path[MAX_PATH];
   DWORD  len;

   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\roger.exe",
                    0, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS)
      return;

   len = MAX_PATH;
   if  (RegQueryValueEx( hKey, NULL, NULL, NULL, (uchar *) path, &len)
       != ERROR_SUCCESS )
      return;

   // comment out this line if using RW 0.23 -- it had a bug
   // that caused /leave to gum up the works when used in combination
   // with /create or /join -- the upcoming Roger Wilco release
   // fixes this bug
   //strcat( path, " /leave" );

   if (!address)
   {
      strcat( path, " /create" );
   } else {
      strcat( path, " /join " );
      strcat( path, address );
   }

   WinExec( path, SW_SHOW );
}

/* Use this function to cause Roger Wilco to drop off whatever channel
   its currently tuned to. You may or may not want to do this when your
   integrated application terminates.
 */
void StopVoiceChat()
{
   HKEY  hKey;
   char  path[MAX_PATH];
   DWORD  len;

   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\roger.exe",
                    0, KEY_ALL_ACCESS, &hKey)
       != ERROR_SUCCESS)
      return;

   len = MAX_PATH;
   if (RegQueryValueEx(hKey, NULL, NULL, NULL, (uchar *) path, &len)
       != ERROR_SUCCESS)
      return;

   strcat( path, " /leave" );
   WinExec( path, SW_SHOW );
}
