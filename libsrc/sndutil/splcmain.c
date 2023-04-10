//
// sample C code using sound utilities & LG sound library
//  Pat McElhatton  August 18 '96
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

#include <sndutil.h>
#include <utilmain.h>

static BOOL donePlaying = FALSE;

static void
endItAll( ISndSample    *pSample,
          void          *pCBData )
{
   donePlaying = TRUE;

   ISndSample_Release( pSample );
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


#define APPNAME "splice"

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
   int            nTimer;
   HWND           hWnd ;     // a handle to a message 
   MSG            msg ;      // a message 
   WNDCLASS       wndclass ; // the window class 
   IWinApp        *pWinApp;
   HINSTANCE      hInstance;
   SndFileSegment segs[30];
   int            i, nSegs;
   uchar          *pBuffer;
   ISndSample     *pSample;
   int32          offset, startOff;

   if ( argc < 4 ) {
      mprintf("%s\n%s\n%s\n%s\n%s\n%s\n%s\n",
              "USAGE: splice <startoff> <fname1> <offset1> ... <fnameN> <offsetN>",
              " if fname does not have an extension, .wav extension is added.",
              " offsets are in milliseconds, with negative offsets indicating",
              " overlap, positive offsets indicate silence gaps.",
              " offsets in the range 1...9 are remapped to:",
              "   1   2   3   4   5   6   7   8   9",
              "  20  40  60  80 100 150 200 250 300" );
      exit( 1 );
   }

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
    
   mono_clear();

   pMixer = SoundInit();

   startOff = atoi( argv[1] );
   nSegs = 0;
   for ( i = 2; i < argc; i += 2 ) {

      segs[ nSegs ].fileName = argv[i];

      // undo anarkey's conversion of '-' to '/'
      if ( *argv[i+1] == '/' ) {
         offset = -atoi( argv[i+1] + 1 );
      } else {
         if ( (*argv[i+1] == 'l' ) || (*argv[i+1] == 'L') ) {
            // secret loop code
            offset = 0x80000000;
         } else {
            offset = atoi( argv[i+1] );
         }
      }
      if ( (offset != 0x80000000) &&
           (abs(offset) < 10) ) {
         // remap 0...9 to values from offsetTab
         if ( offset < 0 ) {
            offset = -offsetTab[-offset];
         } else {
            offset = offsetTab[offset];
         }
      }

      segs[ nSegs ].offset = offset;

      mprintf( "%s %ld\n", argv[i], offset );
      nSegs++;
   }

   //SoundPlayFile( argv[1], pMixer, 127, 64 );
   pBuffer = malloc( 32768 );
   pSample = CreateSoundFileSplicer( pMixer, segs, nSegs, pBuffer, 32768, endItAll, NULL );
   if ( startOff != 0 ) {
      ISndSample_SetPosition( pSample, startOff );
   }
   //ISndSample_DumpData( pSample, 1 );
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
   return msg.wParam ;

}

tResult LGAPI AppCreateObjects(int argc, const char * argv[])
{
   GenericApplicationCreate( argc, argv, "testsnd1", NULL );

   return NOERROR;
}

