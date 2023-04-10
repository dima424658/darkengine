/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/scrndump.cpp,v 1.2 2000/02/19 13:16:34 toml Exp $
#include <comtools.h>
#include <appagg.h>
#include <scrndump.h>
#include <string.h>

// for screen dump 
#include <dispapi.h>
#include <dump.h>
#include <status.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

static BOOL dump_now = FALSE; 
static char dump_fname[16]; 


void SetScreenDump(const char* fname)
{
   if (fname && *fname)
   {
      strncpy(dump_fname,fname,sizeof(dump_fname)); 
      dump_fname[sizeof(dump_fname)-1] = '\0'; 
   }
   else 
      strcpy(dump_fname,"dump"); 
   dump_now = TRUE; 
}


void DumpScreenMaybe(void)
{
   if (!dump_now)
      return; 
   dump_now = FALSE; 

   AutoAppIPtr_(DisplayDevice,pDisp); 

   pDisp->Lock(); 

   char buffer[128];
   int rv=dmp_pcx_file(buffer, dump_fname);

   pDisp->Unlock(); 


   if (rv==-1)
      Status("Couldn't Screen Dump");
   else
   {
      strcat(buffer," has been saved");
      Status(buffer);
   }
}



