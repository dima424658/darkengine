/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ui/movie.cpp,v 1.9 2000/03/16 17:16:11 patmac Exp $
//
// Movie playing APIs
//

#include <lg.h>
#include <comtools.h>
#include <appagg.h>
#include <filespec.h>

#include <movieapi.h>
#include <dispapi.h>
#include <2d.h>
#include <keydefs.h>
#include <gamma.h>

#include <movie.h>
#include <mprintf.h>

// Must be last header 
#include <dbmem.h>


EXTERN BOOL MoviePlaySynchronous(const char * pszFile, int volume)
{
   static char termlist[] = { KEY_ENTER, KEY_ESC, KEY_SPACE, KEY_BS, '\0'};

   AutoAppIPtr(MoviePlayer1);
   AutoAppIPtr(DisplayDevice);
   cFileSpec fileSpec(pszFile);
   float oldGamma;

   if (!pMoviePlayer1)
      return FALSE;
      
   fileSpec.MakeFullPath();
   if (!fileSpec.FileExists())
      return FALSE;
   
   // remember old gamma setting, set gamma to default
   //  this is to keep movies from looking bad if user has cranked up gamma
   oldGamma = g_gamma;
   g_gamma = DEFAULT_GAMMA;
   gamma_update();

   // @TODO (toml 06-16-97): This is hacked e3 logic and should be distrusted as long as this comment is here
   int iDisplayLock = pDisplayDevice->BreakLock();

   gr_push_canvas(grd_visible_canvas);

   pMoviePlayer1->SetTermKeys(termlist);
   pMoviePlayer1->Open(fileSpec.GetName(), 0, 0, 640, 480);
   pMoviePlayer1->SetVolume(volume);
   pMoviePlayer1->Play(kMoviePlayBlock);
   pMoviePlayer1->Close();
   
   gr_pop_canvas();
    
   pDisplayDevice->RestoreLock(iDisplayLock);

   // restore old gamma setting
   g_gamma = oldGamma;
   gamma_update();

   return TRUE;
}


// Assumes canvas you want played to is already the current one
BOOL MoviePlayAsynchronous(const char *pszFile,int x,int y,int w,int h, int volume)
{
   AutoAppIPtr(MoviePlayer1);
   AutoAppIPtr(DisplayDevice);
   cFileSpec fileSpec(pszFile);
   float oldGamma;
   
   if (!pMoviePlayer1)
      return FALSE;
      
   fileSpec.MakeFullPath();
   if (!fileSpec.FileExists())
      return FALSE;
   
   // remember old gamma setting, set gamma to default
   //  this is to keep movies from looking bad if user has cranked up gamma
   oldGamma = g_gamma;
   g_gamma = DEFAULT_GAMMA;
   gamma_update();

   // @TODO (toml 06-16-97): This is hacked e3 logic and should be distrusted as long as this comment is here
   int iDisplayLock = pDisplayDevice->BreakLock();

   pMoviePlayer1->Open(fileSpec.GetName(), x, y, w, h);
   pMoviePlayer1->SetVolume(volume);
   // Play asynchronously.
   pMoviePlayer1->Play(kMoviePlayNoStop);
    
   pDisplayDevice->RestoreLock(iDisplayLock);

   // restore old gamma setting
   g_gamma = oldGamma;
   gamma_update();

   return TRUE;
}



void MovieOnExit()
{
   AutoAppIPtr(MoviePlayer1);
   pMoviePlayer1->Stop();
   pMoviePlayer1->Close();
}

// Get the current status
eMP1State MovieStatus()
{
   AutoAppIPtr(MoviePlayer1);
   return pMoviePlayer1->GetState();
}




