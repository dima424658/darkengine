/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkcolor.cpp,v 1.3 2000/02/19 12:27:13 toml Exp $

// Set the 16 bit mode for resource reading 
// and the 16 bit chromakey

#include <dev2d.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

void DarkColorInit()
{
   extern ulong gResImageFlat16Format;
   extern ushort gResImageChromaKey;

   // Autodetect at load time
   gResImageFlat16Format = 0;
   // bright purple
   gResImageChromaKey = 0xF81F;
}


