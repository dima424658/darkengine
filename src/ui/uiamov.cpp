/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/ui/uiamov.cpp,v 1.4 2000/02/19 13:28:22 toml Exp $

// Asynchronous movie player for ui screens, you can multiply inherit this

#include <comtools.h>
#include <appagg.h>

#include <resapi.h>
#include <resistr.h>
#include <strrstyp.h>

#include <movie.h>
#include <cfgtool.h>
#include <uiamov.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

// Finds the movie and the offset
cAsynchMovie::cAsynchMovie(const char *panel_name,const char *res_path)
{
   char name[256];

   // Avi needs to be in movie path cause it's not a resource
   strcpy(name,panel_name);
   strcat(name,".avi");
   find_file_in_config_path(mMoviePath,name,"movie_path");

   // get xy from namea.str
   strcpy(name,panel_name);
   strcat(name,"a.str");
   IResMan *pResMan = AppGetObj(IResMan);
   cAutoIPtr<IRes> res = pResMan->Bind(name,RESTYPE_STRING,NULL,res_path); 

   AssertMsg1(res!=NULL,"Couldn't find %s xy for asynch movie",name);

   cAutoIPtr<IStringRes> strres (IID_IStringRes,res); 
   const char* s = strres->StringLock("movie"); 

   sscanf(s,"%d,%d",&mXor,&mYor);
   strres->StringUnlock(name); 

   pResMan->Release();
}

// private, actually plays the movie
void cAsynchMovie::mPlayMovie()
{
   MoviePlayAsynchronous(mMoviePath,mXor,mYor,-1,-1, 0);
}
   
// Start the movie
void cAsynchMovie::InitUI()
{
   mPlayMovie();
}

// End the movie
void cAsynchMovie::TermUI()
{
   MovieOnExit();
}

// Keep looping and testing to see if the movie is done.
// If so, start it up again.  Ideally, you wouldn't have to 
// close the movie and start it again, but the actmovie stuff
// is broken somewhere in the allocator, and this is the hack
// to fix it.
// @HACK the actmovie allocator is broken, you can't call ->play after ->stop
void cAsynchMovie::OnLoopMsg(eLoopMessage msg, tLoopMessageData data) 
{
   // Keep the movie looping
   if (msg==kMsgNormalFrame) {
      if (MovieStatus()==kMP1Ready) {
         MovieOnExit();
         mPlayMovie();
      }
   }
}

