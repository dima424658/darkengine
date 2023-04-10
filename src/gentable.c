/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/gentable.c,v 1.2 2000/02/19 12:14:15 toml Exp $

#include <gametabl.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 
////////////////////////////////////////////////////////////
// GAME TABLE FOR AIR
//


const GameTableElem GameTable[] = 
{
   {"air",   CreateAIR },
   { NULL,   CreateAIR },  // default
};

