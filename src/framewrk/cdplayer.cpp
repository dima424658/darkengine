/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/cdplayer.cpp,v 1.2 2000/02/19 12:29:38 toml Exp $

// @TODO: Add support for save/load of BGM and nexttrack!

#include <lgassert.h>

#include <windows.h>
#include <mmsystem.h>

#include <dbg.h>

#include <appagg.h>
#include <aggmemb.h>

#include <cdplayer.h>
#include <cdconst.h>
#include <command.h>

//#include <linkguid.h>
//#include <resapp.h>
#include <propguid.h>

// MUST BE LAST!
#include <initguid.h>
#include <cdguid.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

/* ------------------------------------------------------------ */
//////////////////////////////
// Our actual COM implementation!

static sRelativeConstraint CDPlayerConstraints[] =
{
//   { kConstrainAfter, &IID_ILinkManager},
//   { kConstrainAfter, &IID_IContainSys},
//   { kConstrainAfter, &IID_Res},
   { kConstrainAfter, &IID_IPropertyManager},
   { kNullConstraint, }
};

class cCDPlayer : public cCTDelegating<ICDPlayer>,
                   public cCTAggregateMemberControl<kCTU_Default>
{

protected:

   int m_bgm, m_nexttrack;
   BOOL m_active, m_fading;

public:

   cCDPlayer(IUnknown* pOuter)
   {
      MI_INIT_AGGREGATION_1(pOuter, ICDPlayer, kPriorityNormal, CDPlayerConstraints);
   }

/* ------------------------------------------------------------ */
   STDMETHOD(Init)(void)
   {
      m_active = FALSE;
      m_fading = FALSE;
      m_bgm = m_nexttrack = TRACK_SILENCE;
      return(S_OK);
   }
/* ------------------------------------------------------------ */
// Basic CD Player interface
/* ------------------------------------------------------------ */
   STDMETHOD(CDOpen)(MCI_OPEN_PARMS *pMCIOpen)
   {
      pMCIOpen->wDeviceID = MCI_DEVTYPE_CD_AUDIO; 
      pMCIOpen->lpstrDeviceType = (LPCTSTR)MCI_DEVTYPE_CD_AUDIO; 
      if (mciSendCommand(NULL, MCI_OPEN, MCI_OPEN_TYPE|MCI_OPEN_TYPE_ID, (DWORD)pMCIOpen))
      {
         Warning(("CDPlayTrack:: Couldn't open CD player"));
         return (E_FAIL);
      } 
      return (S_OK);
   }

/* ------------------------------------------------------------ */
   STDMETHOD(CDPlayTrack)(int track)
   {
      MCI_OPEN_PARMS mciOpen;
      MCI_SET_PARMS mciSet;
      MCI_PLAY_PARMS mciPlay;

      if (CDOpen(&mciOpen) == E_FAIL)
         return(E_FAIL);

      // Set the time format to track/minute/second/frame (TMSF).  
      mciSet.dwTimeFormat = MCI_FORMAT_TMSF; 
      if (mciSendCommand(mciOpen.wDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD)&mciSet))
      {
         mciSendCommand(mciOpen.wDeviceID, MCI_CLOSE, 0, NULL); 
         return (E_FAIL);
      }

      mciPlay.dwCallback = 0; 
      mciPlay.dwFrom = MCI_MAKE_TMSF(track, 0, 0, 0); 
      if (mciSendCommand(mciOpen.wDeviceID, MCI_PLAY, MCI_FROM, (DWORD)&mciPlay))
      {
         Warning(("CDPlayTrack::Error playing track"));
      }
 
      mciSendCommand(mciOpen.wDeviceID, MCI_CLOSE, 0, NULL);

      return(S_OK);
   }

/* ------------------------------------------------------------ */
   STDMETHOD(CDStopPlay)(void)
   {
      MCI_OPEN_PARMS mciOpen;
      MCI_GENERIC_PARMS mciParms;

      if (CDOpen(&mciOpen) == E_FAIL)
         return(E_FAIL);

      mciParms.dwCallback = NULL; 
      mciSendCommand(mciOpen.wDeviceID, MCI_STOP, 0, (DWORD)&mciParms);
      mciSendCommand(mciOpen.wDeviceID, MCI_CLOSE, 0, NULL);

      return(S_OK);
   }

/* ------------------------------------------------------------ */
// Higher level music control system
/* ------------------------------------------------------------ */

   STDMETHOD(CDSetBGMTrack)(int track)
   {
      m_bgm = track;
      if (m_active)
      {
         // begin fade
         m_fading = TRUE;
      }
      else
         CheckBGM();
      return(S_OK);
   }

/* ------------------------------------------------------------ */
   STDMETHOD(CDPlayTransition)(int track, uint flags)
   {
      if (flags & CDFLAG_FADE)
      {
         // begin fade
         m_fading = TRUE;
         m_nexttrack = track;
      }
      else
      {
         // just play it
         CDStopPlay();
         CDPlayTrack(track);
         m_active = TRUE;
      }

      return(S_OK);
   }

/* ------------------------------------------------------------ */
   STDMETHOD(Update)(void)
   {
      BOOL done;
      if (!m_active)
         return(S_OK);

      // are we finishing a fade?
      if (m_fading)
         done = TRUE; // figure out if we are done fading;
      else
         done = FALSE; // figure out if the track has hit the end

      // if we are ready for a new track, ask for it
      if (done)
      {
         CDStopPlay();
         CheckBGM();
      }

      return(S_OK);
   }
/* ------------------------------------------------------------ */
   STDMETHOD(CheckBGM)(void)
   {
      int t;

      if (m_nexttrack == TRACK_SILENCE)
      {
         // someday this will decide randomly among various tracks and whatnot
         t = m_bgm;
      }
      else
      {
         t = m_nexttrack;
         m_nexttrack = TRACK_SILENCE;
      }

      // old thing finished, time to start a new one, whatever that is
      if (t == TRACK_SILENCE)
         return(S_OK);

      // it's a real track, so play it
      CDPlayTrack(t);

      m_active = TRUE;
      return(S_OK);
   }
/* ------------------------------------------------------------ */

};

static void CDPlayTrack(int track)
{
   AutoAppIPtr(CDPlayer);
   pCDPlayer->CDPlayTrack(track);
};

static void CDStopPlay(void)
{
   AutoAppIPtr(CDPlayer);
   pCDPlayer->CDStopPlay();
}

static Command CDPlayerCommands[] =
{
   { "play_cd", FUNC_INT, CDPlayTrack, "play a CD track" },
   { "stop_cd", FUNC_VOID, CDStopPlay, "stop playing a CD track" },
};

// creation func
void CDPlayerCreate()
{
   COMMANDS(CDPlayerCommands,HK_ALL);

   AutoAppIPtr(Unknown); 
   new cCDPlayer(pUnknown); 
}


