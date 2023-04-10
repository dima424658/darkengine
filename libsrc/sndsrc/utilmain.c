////////////////////////////////////////////////////////////////////////
//
// (c) 1996 Looking Glass Technologies Inc.
// Pat McElhatton
//
// Module name: Sound utilities standalone main routines
// File name: utilmain.c
//
// Description: Set of functions for setting up sound library which
//    are used by several standalone programs
//
////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <lg.h>
#include <mprintf.h>
#include <stdio.h>
#include <stdlib.h>
#include <lgsound.h>
#include <utilmain.h>


////////////////////////////////////////////////////////////////////////
// 
// Function name: SoundInit()
//
// Description: Initialize the sound module.
// 
////////////////////////////////////////////////////////////////////////
ISndMixer *
SoundInit ( void )
{
   ISndMixer *pMixer = NULL;

   if (SndCreateMixer (&pMixer, NULL) == FALSE) {
      printf ("Sound system not started\n");
   } else {
      sSndSetup setup;
      setup.hwnd = NULL;
        
      if (ISndMixer_Init (pMixer, &setup, MAX_SOUNDS, NULL) != kSndOk) {
         printf ("Sound system init failed\n");
      }
   }            

   return pMixer;
}


////////////////////////////////////////////////////////////////////////
// 
// Function name: SoundTerm()
//
// Description: Terminate the sound module.
// 
////////////////////////////////////////////////////////////////////////
void
SoundTerm ( ISndMixer *pMixer )
{                              
   if (pMixer) {
      ISndMixer_StopAllSamples (pMixer);
      ISndMixer_Release (pMixer);
      pMixer = NULL;
   }    
}


////////////////////////////////////////////////////////////////////////
// 
// Function name: SoundRecur()
//
// Description: Recur into the sound module for management purposes.
// 
////////////////////////////////////////////////////////////////////////
void
SoundRecur( ISndMixer *pMixer )
{
   if ( pMixer ) {
      // allow any samples which have finished playing to do
      //   end-of-sample cleanup
      ISndMixer_Update( pMixer );
   }
}

        
