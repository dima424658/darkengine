/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/cdscript.cpp,v 1.2 2000/02/19 12:36:14 toml Exp $

#include <appagg.h>

#include <scrptapi.h>
#include <scrptsrv.h>
#include <scrptbas.h>

#include <cdplayer.h>
#include <cdscript.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

DECLARE_SCRIPT_SERVICE_IMPL(cCDSrv, CD)
{
public:

   STDMETHOD(SetBGM)(int track)
   {
      AutoAppIPtr(CDPlayer);
      pCDPlayer->CDSetBGMTrack(track);
      return(S_OK);
   }

   STDMETHOD(SetTrack)(int track, uint flags) 
   {
      AutoAppIPtr(CDPlayer);
      pCDPlayer->CDPlayTransition(track,flags);
      return(S_OK);
   }

};

IMPLEMENT_SCRIPT_SERVICE_IMPL(cCDSrv, CD);

