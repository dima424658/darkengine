/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/ui/uiamov.h,v 1.1 1998/10/25 17:57:57 jaemz Exp $

#ifndef _UIAMOV_H
#define _UIAMOV_H
#pragma once

// Asynchronous movie player for ui screens, you can multiply inherit this
// for his and her pleasure

#include <looptype.h>

// The data is derived from the panel_name.
// the movie is called <panel_name>.avi
// and the x,y coords are in a string file <panel_name>a.str 
// under the index "movie"
class cAsynchMovie {
public:
   cAsynchMovie(const char *panel_name,const char *res_path);
protected:
   void InitUI();
   void TermUI();
   void OnLoopMsg(eLoopMessage msg, tLoopMessageData data);
private:
   // x origin and y origin for the movie
   int mXor,mYor;    
   // Full path of the movie (basename+avi)
   char mMoviePath[256];
   // actually play the darn thing
   void mPlayMovie();
};

#endif
