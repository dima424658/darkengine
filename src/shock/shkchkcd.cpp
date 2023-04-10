/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkchkcd.cpp,v 1.4 2000/02/19 12:36:41 toml Exp $

#include <windows.h>
#include <config.h>
#include <string.h>
#include <shkchkcd.h>
#include <mprintf.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

//
// Shock CD and protection stuff.
//
// 1) We use DiscSafe copy protection, which wraps our executable in
//    encrypted form.  CD authentication occurs before our WinMain()
//    even begins.
//
// 2) So, why this code?  It's mostly here to locate the CD for runtime
//    access.  The cd_path config variable is only a default value to
//    check.  The user might have 2 CD drives, or a removable-medium
//    drive which causes the CD to show up at a different letter.
//

#ifndef SHIP
#define CHKCD_SPEW(a) \
   if (config_is_defined("chkcd_spew")) {mprintf a;}
#define CHKCD_BOOLSPEW(a,b,c)\
   if (config_is_defined("chkcd_spew")) {if((a)){mprintf b;} else {mprintf c;}}
#else
#define CHKCD_SPEW(a)
#define CHKCD_BOOLSPEW(a,b,c)
#endif


static char *path_configs[]=
 { "cd_path", "resname_base", "load_path", "script_module_path", "movie_path" };

// gonna replace " X:\" and "+X:\"
void _FixupCDConfigVars( char *drivePath )
{
   char try_paths[MAX_PATH], old_path[MAX_PATH];
   int i;

   if ( !config_get_raw( path_configs[0], old_path, sizeof( old_path ) ) )
      strcpy( old_path, "D:\\" );
   strlwr( old_path );
   strlwr( drivePath );
   for( i = 0; i < sizeof( path_configs ) / sizeof( path_configs[0] ); i++ )
   {
      if( config_get_raw( path_configs[i], try_paths, sizeof( try_paths ) ) )
      {
         char *p;
         strlwr( try_paths );
         p=try_paths;
         while( ( p = strstr( p, old_path ) ) != NULL)
         {
            CHKCD_SPEW(( "CHKCD : Fixing up config var %s\n", path_configs[i] ));
            CHKCD_SPEW(( "CHKCD :   Was: \"%s\"\n", try_paths ));
            memcpy( p, drivePath, strlen( drivePath ) );
            CHKCD_SPEW(( "CHKCD :   Now: \"%s\"\n", try_paths ));
            p++; // lets not fix ourselves up again
         }
         config_set_string( path_configs[i], try_paths );
         config_set_priority( path_configs[i], PRIORITY_TRANSIENT );
      }
   }
}

static char* _GetNoCDString( char *fill_in, char *type, int fill_in_len )
{
   char cfg_var[128];
   char lang[32];

   // Get language from config.
   if ( !config_get_raw( "language", lang, sizeof( lang ) ) )
      strcpy( lang, "english" );  // yes, gringo bastards, yes

   // Use config variable of form no_cd_XXX_YYY, e.g. no_cd_title_english.
   sprintf( cfg_var, "no_cd_%s_%s", type, lang);
   if ( !config_get_raw( cfg_var, fill_in, fill_in_len) )
      strcpy( fill_in, "NO CD" );

   return fill_in;
}

static BOOL _ShowCDMessage( BOOL copy_prot )
{
   char title[128];
   char message[256];

   _GetNoCDString( title, "title", sizeof( title ) );
   _GetNoCDString( message, "msg", sizeof( message) );
   if ( copy_prot )
      strcat( message," ." );
   if ( MessageBox( NULL, message, title, MB_OKCANCEL | MB_ICONEXCLAMATION ) == IDCANCEL )
      return FALSE;
   return TRUE;
}

static BOOL _CheckIsACD( char* drivePath )
{
   return GetDriveType( drivePath ) == DRIVE_CDROM;
}

static BOOL _CheckFile( char* drivePath )
{
   BOOL wasFound;
   char path[MAX_PATH];
   FILE *fp=NULL;

   // Set up search path.
   path[0] = '\0';
   strcat( path, drivePath );
   strcat( path, "shock\\snd.crf" );

   // Attempt to open known file.
   fp = fopen( path, "rb" );
   if( fp )
   {
      fclose( fp );
      wasFound = TRUE;
   }
   else
   {
      wasFound = FALSE;
   }
   
   return wasFound;
}

static BOOL _CheckDrive( char* drivePath )
{
   return( _CheckIsACD( drivePath ) && _CheckFile( drivePath ) );
}


static BOOL _CheckDrives( BOOL onlyCheckPath )
{
   char path[MAX_PATH];
   BOOL wasFound = FALSE;

   // Check config path setting.
   if( config_get_raw( "cd_path", path, sizeof( path ) ) )
   {
      wasFound = _CheckDrive( path );
      CHKCD_SPEW(("CHKCD : Checking CD at cd_path (%s)... ", path));
      CHKCD_BOOLSPEW( wasFound, ( "Found.\n" ), ( "Not found.\n") );
   }

   // We don't check other drives if onlyCheckPath is flagged or if
   // we already found it.
   if( !onlyCheckPath && !wasFound )
   {
      CHKCD_SPEW(( "CHKCD : Checking other drives... " ));
      strcpy( path, "A:\\" );
      while( !wasFound && path[0] <= 'Z' )
      {
         CHKCD_SPEW(( "%c", path[0] ));
         wasFound = _CheckDrive( path );
         if ( !wasFound )
            path[0]++;
      }

      CHKCD_BOOLSPEW( wasFound, ( "\nCHKCD : Found CD at %s\n", path ), ("\nCHKCD : Not found.\n") );
      if( wasFound )
      {
         _FixupCDConfigVars( path );
      }
   }

   return wasFound;
}

// so defdep sees it
#ifdef THIEF
#endif
#ifdef SHOCK
#endif

// note that we are the default provider of this function
#if !defined(THIEF)
BOOL CheckForCD(void)
{
   BOOL wasFound = FALSE;
   BOOL tryAgain = TRUE;
   
#ifndef COPY_PROTECTION
   // Don't complain about copy protection.
   if( !config_is_defined( "test_copy_protect" ) )
   {
      CHKCD_SPEW(( "CHKCD : Skipping copy protection.\n"));
      // We still do _CheckDrives() so that any FixupConfigVars can happen.
      wasFound = _CheckDrives( config_is_defined( "only_check_path" ) );
      // Force true result.
      wasFound = TRUE;
   }
   else
#endif
   {
      while ( !wasFound && tryAgain )
      {
         // For testing purposes, we can check just one drive.
         wasFound = _CheckDrives( config_is_defined( "only_check_path" ) );

         if ( !wasFound )
         {
            tryAgain = _ShowCDMessage( TRUE );
         }
      }
   }

   return wasFound;
}
#endif
