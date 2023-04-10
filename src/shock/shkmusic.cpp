/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkmusic.cpp,v 1.5 2000/02/19 13:25:47 toml Exp $

/*
#include <shkmusic.h>

#include <windows.h>
#include <mmsystem.h>

#include <dbg.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

BOOL CDOpen(MCI_OPEN_PARMS *pMCIOpen)
{
   pMCIOpen->wDeviceID = MCI_DEVTYPE_CD_AUDIO; 
   pMCIOpen->lpstrDeviceType = (LPCTSTR)MCI_DEVTYPE_CD_AUDIO; 
   if (mciSendCommand(NULL, MCI_OPEN, MCI_OPEN_TYPE|MCI_OPEN_TYPE_ID, (DWORD)pMCIOpen))
   {
      Warning(("CDPlayTrack:: Couldn't open CD player"));
      return FALSE;
   } 
   return TRUE;
}

void CDPlayTrack(int track)
{
   MCI_OPEN_PARMS mciOpen;
   MCI_SET_PARMS mciSet;
   MCI_PLAY_PARMS mciPlay;

   if (!CDOpen(&mciOpen))
      return;

   // Set the time format to track/minute/second/frame (TMSF).  
   mciSet.dwTimeFormat = MCI_FORMAT_TMSF; 
   if (mciSendCommand(mciOpen.wDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD)&mciSet))
   {
      mciSendCommand(mciOpen.wDeviceID, MCI_CLOSE, 0, NULL); return;
   }

   mciPlay.dwCallback = 0; 
   mciPlay.dwFrom = MCI_MAKE_TMSF(track, 0, 0, 0); 
   if (mciSendCommand(mciOpen.wDeviceID, MCI_PLAY, MCI_FROM, (DWORD)&mciPlay))
   {
      Warning(("CDPlayTrack::Error playing track"));
   }
 
   mciSendCommand(mciOpen.wDeviceID, MCI_CLOSE, 0, NULL);
}

void CDStopPlay(void)
{
   MCI_OPEN_PARMS mciOpen;
   MCI_GENERIC_PARMS mciParms;

   if (!CDOpen(&mciOpen))
      return;
   mciParms.dwCallback = NULL; 
   mciSendCommand(mciOpen.wDeviceID, MCI_STOP, 0, (DWORD)&mciParms);
   mciSendCommand(mciOpen.wDeviceID, MCI_CLOSE, 0, NULL);
}
*/