//
// sample C code using sound source & LG sound library
//  Pat McElhatton  June 5 '97
//

#include <windows.h>
#include <lg.h>
#include <res.h>
#include <mprintf.h>
#include <stdio.h>
#include <stdlib.h>
#include <lgsound.h>
#include <wappapi.h>
#include <appapi.h>
#include <appagg.h>
#include <utilmain.h>
#include <timelog.h>

#include <sndutil.h>
#include <utilmain.h>

#include <sndsrc.h>
#include <playlist.h>

static BOOL donePlaying = FALSE;

static void
smpEndCB( ISndSample    *pSample,
          void          *pCBData )
{
   donePlaying = TRUE;
}


static void
srcEndCB( ISndSource    *pSource,
          void          *pCBData )
{
   donePlaying = TRUE;
}


LRESULT FAR PASCAL WndProc (HWND hWnd, UINT iMessage, WPARAM wParam, 
    LPARAM lParam)
{
    switch (iMessage)       
    {
        case WM_COMMAND:    // process menu items 
           break ;
        case WM_DESTROY:
            PostQuitMessage (0) ;
            break ;
        default:            // default windows message processing 
            return DefWindowProc (hWnd, iMessage, wParam, lParam) ;
    }
    return (0L) ;
}


#define APPNAME "playsrc"

// user offsets of 0...9 are mapped to these millisecond values
static uint32 offsetTab[] = {
   0,    20,   40,   60,   80,
   100,  150,  200,  250,  300
};

int LGAPI
AppMain( int argc,
         const char **argv )
{
   ISndMixer      *pMixer;
   int            nTimer, i;
   HWND           hWnd ;     // a handle to a message 
   MSG            msg ;      // a message 
   WNDCLASS       wndclass ; // the window class 
   IWinApp        *pWinApp;
   HINSTANCE      hInstance;
   ISndSample     *pSample;
   int32          startOff;
   uint32         playlist[100];
   uint32         *pOp;
   ISndSource     *pSndSrc;
   SSPLFileSingle *pSingleOp;
   SSPLFileDual   *pDualOp;
   SSPLSilence    *pSilenceOp;
   FILE           *inFile;
   char           hdrBuffer[16384];
   BOOL           result, doDual;
   void           *pRawData;
   uint32         rawDataLen;
   uint32         nSamples;
   sSndAttribs    attribs;
   int            offset;

   mono_clear();
   if ( argc < 4 ) {
      mprintf("%s\n%s\n%s\n%s\n%s\n%s\n%s\n",
              "USAGE: playsrc <startoff> <fname1> <offset1> ... <fnameN> <offsetN>",
              " if fname does not have an extension, .wav extension is added.",
              " offsets are in milliseconds, with negative offsets indicating",
              " overlap, positive offsets indicate silence gaps.",
              " offsets in the range 1...9 are remapped to:",
              "   1   2   3   4   5   6   7   8   9",
              "  20  40  60  80 100 150 200 250 300" );
      exit( 1 );
   }

   TIMELOG_INIT( 8192 );
   hInstance = GetModuleHandle( NULL );

   // load data into window class struct. 
   wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
   wndclass.lpfnWndProc   = WndProc ;
   wndclass.cbClsExtra    = 0 ;
   wndclass.cbWndExtra    = 0 ;
   wndclass.hInstance     = hInstance ;
   wndclass.hIcon         = LoadIcon (hInstance, APPNAME) ;
   wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
   wndclass.hbrBackground = GetStockObject (WHITE_BRUSH) ;
   wndclass.lpszMenuName  = APPNAME;
   wndclass.lpszClassName = APPNAME;
          
   // register the window class 
   if (!RegisterClass (&wndclass)) {
      return FALSE ;
   }
    
   hWnd = CreateWindow (       // create the program's window here 
                        APPNAME,                // class name 
                        APPNAME,                // window name 
                        WS_OVERLAPPEDWINDOW,    // window style 
                        CW_USEDEFAULT,          // x position on screen 
                        CW_USEDEFAULT,          // y position on screen 
                        50,                     // width of window 
                        50,                     // height of window 
                        NULL,                   // parent window handle (null = none) 
                        NULL,                   // menu handle (null = class menu) 
                        hInstance,              // instance handle 
                        NULL) ;                 // lpstr (null = not used) 
   
   pWinApp = AppGetObj( IWinApp );
   IWinApp_SetMainWnd( pWinApp, hWnd );
   SafeRelease( pWinApp );

   ShowWindow (hWnd, SW_SHOW) ;   
   UpdateWindow (hWnd) ;       // send first WM_PAINT message 
    
   pMixer = SoundInit();

   ///////////////////////////////////////////////////////////////
   //
   // set the playback attribs to be those of the first file
   //
   inFile = fopen( argv[2], "rb" );
   if ( fread( hdrBuffer, 1, sizeof(hdrBuffer), inFile ) != sizeof(hdrBuffer) ) {
      fprintf( stderr, "error reading header\n" );
   }
   fclose( inFile );

   if ( SndCrackRezHeader( hdrBuffer, sizeof(hdrBuffer), &pRawData, &rawDataLen,
                           &nSamples, &attribs )  ) {
      fprintf( stderr, "error cracking header\n" );
   } else {
      if ( attribs.dataType == kSndDataIMAADPCM ) {
         attribs.dataType = kSndDataPCM;
         attribs.bitsPerSample = 16;
      }
   }
   attribs.numSamples = ~0;

   ///////////////////////////////////////////////////////////////
   //
   // create the ISndSample which will play these files
   //
   pSample = ISndMixer_CreateRawSample( pMixer, kSndSampleStream,
                                        pRawData, sizeof(hdrBuffer),
                                        attribs.numSamples, &attribs );
   if ( pSample == NULL ) {
      fprintf( stderr, "error createing sample\n" );
   }

   ///////////////////////////////////////////////////////////////
   //
   // build the playlist
   //
   pOp = playlist;
   for ( i = 2; i < argc; i += 2 ) {

      // undo anarkey's conversion of '-' to '/'
      if ( *argv[i+1] == '/' ) {
         *argv[i+1] = '-';
      }

      offset = atoi( argv[i+1] );
      if ( offset < 0 ) {
         offset = (-offset);
         doDual = TRUE;
      } else {
         doDual = FALSE;
      }
      if ( offset < 10 ) {
         // remap 0...9 to values from offsetTab
         offset = offsetTab[offset];
      }
      nSamples = ISndSample_TimeToSamples( pSample, offset );

      pSingleOp = (SSPLFileSingle *) pOp;
      pSingleOp->op = plFileSingle;
      strcpy( pSingleOp->name, argv[i] );
      pOp += sizeof( SSPLFileSingle ) >> 2;

      if ( doDual ) {
         pDualOp = (SSPLFileDual *) pOp;
         pDualOp->op = plFileDual;
         pDualOp->nSamples = nSamples;
         pOp += sizeof( SSPLFileDual ) >> 2;
      } else if ( offset != 0 ) {
         pSilenceOp = (SSPLSilence *) pOp;
         pSilenceOp->op = plSilence;
         pSilenceOp->nSamples = nSamples;
         pOp += sizeof( SSPLSilence ) >> 2;
      }

      //mprintf( "%s %ld\n", argv[i], offset );
   }

   // add an end-list op
   *pOp = plEndList;

   ///////////////////////////////////////////////////////////////
   //
   // create the sound source & set the playlist
   //

   pSndSrc = SndCreateSource( NULL );
   if ( pSndSrc == NULL ) {
      fprintf( stderr, "error creating sound source\n" );
   }
   result = ISndSource_ConnectToPlayer( pSndSrc, pSample );
   result = ISndSource_SetPlaylist( pSndSrc, playlist );
   ISndSource_RegisterEndCallback( pSndSrc, srcEndCB, NULL );

   // set the play start offset
   startOff = ISndSample_TimeToSamples( pSample, atoi( argv[1] ) );
   ISndSample_SetPosition( pSample, startOff );

   ///////////////////////////////////////////////////////////////
   //
   // start playback & wait for it to finish
   //
   ISndSample_DumpData( pSample, TRUE );
   ISndSample_RegisterEndCallback( pSample, smpEndCB, NULL );
   ISndSample_Play( pSample );

   nTimer = SetTimer( hWnd, 1, 60, NULL );  // send timer events every 60 milliseconds
   while (GetMessage (&msg, NULL, 0, 0)) {  // the message loop 
      //mprintf( "message loop\n" );
      TranslateMessage( &msg ) ;
      SoundRecur( pMixer );
      DispatchMessage( &msg ) ;
      if ( donePlaying ) {
         KillTimer( hWnd, nTimer );
         break;
      }
   }
   ///////////////////////////////////////////////////////////////
   //
   // cleanup - release the sound source & sample
   //
   ISndSample_DumpData( pSample, FALSE );
   ISndSample_Release( pSample );
   ISndSource_Release( pSndSrc );

   TIMELOG_DUMP( "sndlog.txt" );

   return msg.wParam ;
}

tResult LGAPI AppCreateObjects(int argc, const char * argv[])
{
   GenericApplicationCreate( argc, argv, "streamer", NULL );

   return NOERROR;
}

