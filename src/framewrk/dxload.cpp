/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/dxload.cpp,v 1.9 2000/02/27 15:31:22 adurant Exp $

extern BOOL LoadDirectSound();
extern BOOL LoadDirectDraw();

#define WIN95_LEAN_AND_MEAN
#include <windows.h>
#include <config.h>
#include <lgd3d.h>

#include <dxload.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

static char *sNoDSound = 
        "DirectSound is not installed.\n\
You will not be able to hear any sound or music while playing Thief 2\n\
and you may experience other difficulties.  Thief 2 was designed and \n\
tested assuming all users have access to sound.\n\
We strongly recommend installing DirectX to enable sound support.\n\
Continue Anyway?";

static char *sNoDDraw = 
        "DirectX version 6 or higher is not installed.\n\
Please install DirectX 6 before playing.  DirectX installer\n\
Can be accessed through the autoplay dialog.\n";

static char *sNoD3DDevice = 
"Your video card is not compatible with Thief 2.\n";
EXTERN BOOL g_no_dx6 = FALSE;

static BOOL GetErrorMessage(char *msgbuffer,const char *basename, int buflen)
{
   char lang[28];
   char shortbase[32];
   char cfgvar[64];
   char configline[80];
   int numstrings = 0;
   BOOL successful = TRUE;

   strncpy(shortbase,basename,32);
   if (!config_get_raw("language",lang,28))
      strcpy(lang,"english");
   sprintf(cfgvar,"%snum_%s",shortbase,lang);
   if (!config_get_int(cfgvar,&numstrings))
      return FALSE;
   strcpy(msgbuffer,"");
   for (int i=1;i<=numstrings;i++)
   {
      sprintf(cfgvar,"%s%d_%s",shortbase,i,lang);
      if (!config_get_raw(cfgvar,configline,80))
      {
         successful = FALSE;
      }
      else
      {
         if ((i>1) && (buflen>0))
         {
            strcat(msgbuffer,"\n");
            buflen--;
         }
         buflen -= strlen(configline);
         //test for fit.
         if (buflen>=0)
            strcat(msgbuffer,configline);
         else         
            successful = FALSE;
      }
   }
   return successful;
}

BOOL LoadDirectX(void)
{
   char msgbuffer[512];
   if (!GetErrorMessage(msgbuffer,"no_directsound_",512))
     strcpy(msgbuffer,sNoDSound); //default message for sound
   if (!config_is_defined("skip_dsound_check"))
      if (config_is_defined("fail_dsound_check") || (!LoadDirectSound()))
      {
         HRESULT retval = MessageBox ((HWND)NULL, msgbuffer, (LPCSTR)NULL, MB_YESNO | MB_ICONWARNING);
         if (retval == IDNO)
            return FALSE;
         config_set_int("no_sound",1);
      }
   if (!GetErrorMessage(msgbuffer,"no_directdraw_",512))
      strcpy(msgbuffer,sNoDDraw); //default message for ddraw
   if (!config_is_defined("skip_ddraw_check"))
      if (config_is_defined("fail_ddraw_check") || (!LoadDirectDraw()))
      {
         HRESULT retval = MessageBox ((HWND)NULL, msgbuffer, (LPCSTR)NULL, MB_ICONWARNING);
         /*
         if (retval == IDNO)
            return FALSE;
         g_no_dx6 = TRUE;
         */
         // this isn't game-configurable, but in Shock and all future Exodus level
         // games we don't want software support, so just bomb out.
         // Xemu 6/3/99
         return FALSE;
      }

   if (!GetErrorMessage(msgbuffer,"no_d3ddevice_",512))
      strcpy(msgbuffer,sNoD3DDevice);
   if( config_is_defined("fail_d3d_enum") || (lgd3d_enumerate_devices() == -1) )
   {
       HRESULT retval = MessageBox ((HWND)NULL, msgbuffer, (LPCSTR)NULL, MB_ICONWARNING);
       return FALSE;
   }

   //zb   lgd3d_enumerate_devices();
   // which I suppose I really shouldn't be, but it's too close to ship to
   // fuck around, and probably this should go somewhere else anyway - Kevin
   //
   //   else
   //      AtExit(lgd3d_unenumerate_devices);
   return TRUE;
}
