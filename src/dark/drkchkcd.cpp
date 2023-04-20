/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkchkcd.cpp,v 1.7 2000/03/06 14:17:30 adurant Exp $
#include <windows.h>
#include <config.h>
#include <str.h>
#include <string.h>
#include <drkuires.h>
#include <drkchkcd.h>
#include <ctype.h>
#include <drkafp.h>
#include <mprintf.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

// check to see that the CD is in the drive from which the game was 
// installed.  If not, put up a message box asking for such.  If user
// cancels out, return FALSE.

char *get_no_cd_message(char *fill_in, const char *msg)
{
   char cfg_var[128];
   char lang[32];
   if (!config_get_raw("language",lang,sizeof(lang)))
      strcpy(lang,"english");  // yes, gringo bastards, yes
   sprintf(cfg_var,"no_cd_%s_%s",msg,lang);
   if (!config_get_raw(cfg_var,fill_in,128))
      strcpy(fill_in,"NO CD");
   return fill_in;
}

BOOL ShowCDMessage(BOOL copy_prot)
{
   char title[128];
   char message[256];

   get_no_cd_message(title,"title");
   get_no_cd_message(message,"msg");
   if (copy_prot)
      strcat(message," .");
   if (MessageBox(NULL, message, title, MB_OKCANCEL | MB_ICONEXCLAMATION) == IDCANCEL)
      return FALSE;
   return TRUE;
}

// old way w/o copy protection...
BOOL CheckForBigFile (void)
{
   char path[MAX_PATH];

   if (config_get_raw("cd_path",path,sizeof(path))) 
   {
      FILE *tfp=NULL;
      strcat(path,"thief2\\miss8.mis");
      tfp = fopen(path,"rb");
      while (!(tfp))
      {
         if (!ShowCDMessage(FALSE))
            return FALSE;
         tfp = fopen(path,"rb");
      }
      if (tfp)
         fclose(tfp);
   }
   else
   {
#ifdef PLAYTEST
      mprintf("Not looking for big file\n");
#endif      
      return TRUE; // probably a programmer, no cd_path
   }
   return TRUE;
}

// first, we are going to have to go find the CD
//   this takes a "where it might be", and returns a char * pointing at an X:\
//   or NULL to mean "i could not find a CD"
// continue means dont restart scanning from A:\ if im wrong, continue from guess

BOOL IsACD(char *path)
{
   return GetDriveType(path)==DRIVE_CDROM;
}

static char basePath[]="D:\\";

char *FindACD(char *guess, BOOL keep_going)
{
   char drive='A';

   if (guess==NULL)
      return NULL;
   if (keep_going)
      drive=toupper(guess[0])+1; // neat, eh?
   for ( ; drive <= 'Z'; drive++)
   {
      basePath[0] = drive;
      if (IsACD(basePath))
         return basePath;
   }
   return NULL;
}

static char *realPath=NULL;

BOOL CheckCopyProtection(void)
{
   char try_path[MAX_PATH];
   BOOL first=FALSE;

   if (!config_get_raw("cd_path",try_path,sizeof(try_path)))
      strcpy(try_path,"");  // we are just gonna scan through them all
   else
      first=TRUE;

   realPath=FindACD(try_path,FALSE);
   while (realPath!=NULL)
   {
#ifndef SHIP      
      mprintf("NOTE: Found and checking a CD at [%s]\n",realPath);
#endif      
      if (DarkEngine_AFP_Check(realPath[0]))
         break;
      if (first)
      {
         strcpy(realPath,"");
         first=FALSE;
      }
      realPath=FindACD(realPath,TRUE);
   }
   if (realPath==NULL)
      return FALSE;
   return TRUE;
}

//////////////////
// fist one here is the "old path" from which we fix
static const char *path_configs[]=
 { "cd_path", "resname_base", "load_path", "script_module_path", "movie_path" };

// gonna replace " X:\" and "+X:\"
void FixupCDConfigVars(char *path)
{
   char try_paths[MAX_PATH], old_path[MAX_PATH];
   int i;

   if (!config_get_raw(path_configs[0],old_path,sizeof(old_path)))
      strcpy(old_path,"D:\\");
   strlwr(old_path);
   strlwr(path);
   for (i=0; i<sizeof(path_configs)/sizeof(path_configs[0]); i++)
      if (config_get_raw(path_configs[i],try_paths,sizeof(try_paths)))
      {
         char *p;
         strlwr(try_paths);
         p=try_paths;
         while ((p=strstr(p,old_path))!=NULL)
         {
            memcpy(p,path,strlen(path));
            p++; // lets not fix ourselves up again
         }
         config_set_string(path_configs[i],try_paths);
         config_set_priority(path_configs[i],PRIORITY_TRANSIENT);
      }
}

BOOL RealCDCheck(void)
{
   realPath=NULL;
   do {
      if (CheckCopyProtection())
         break;            // we succeeded
      else if (!ShowCDMessage(TRUE))
         return FALSE;     // we failed, and the user said lets quit..
   } while (1);
   if (realPath)
      FixupCDConfigVars(realPath);
   return realPath!=NULL;
}

BOOL RemedialCDCheck(void)
{
   char try_path[MAX_PATH];
   if (!config_get_raw("cd_path",try_path,sizeof(try_path)))
      strcpy(try_path,"D:\\");
   if (IsACD(try_path))
      do {
         if (DarkEngine_AFP_Check(try_path[0]))
            return TRUE;
         if (!ShowCDMessage(TRUE))
            return FALSE;  // user gave up
      } while (1);
   return FALSE;
}

// so defdep sees it
#ifdef THIEF
#endif
#ifdef SHOCK
#endif

#if defined(THIEF) && !defined(SHOCK)
BOOL CheckForCD(void)
{
   BOOL found=FALSE;
#ifndef COPY_PROTECTION
   if (!config_is_defined("test_copy_protect"))
      found=TRUE;
   else
#endif
      if (!config_is_defined("only_check_path"))
         found=RealCDCheck();
      else
         found=RemedialCDCheck();
   if (found)
      return CheckForBigFile();
   return FALSE;
}
#endif 
