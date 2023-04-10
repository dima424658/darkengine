/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/hybtable.c,v 1.6 2000/02/19 13:16:18 toml Exp $

#include <gametabl.h>

#include <Drkinit.h>
#include <shkinit.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 
////////////////////////////////////////////////////////////
// GAME TABLE FOR HYBRID EXECUTABLE
//


const GameTableElem GameTable[] = 
{
#ifdef AIR
   { "air", CreateAIR }, 
#endif
   {"dark", CreateDark },
   {"shock", CreateShock },
   { NULL,   CreateDark },  // default
};

