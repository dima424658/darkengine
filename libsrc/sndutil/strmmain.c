//
// sample C code using sound utilities & LG sound library
//  Pat McElhatton  August 26 '96
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


#define APPNAME "stream"

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
   uchar          *pBuffer;
   ISndSample     *pSample;
   int32          startOff;

   if ( argc < 2 ) {
      mprintf("%s\n%s\n",
              "USAGE: stream <fname1>   play a file in stream mode",
              " if fname does not have an extension, .wav extension is added.");
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

   //SoundPlayFile( argv[1], pMixer, 127, 64 );
   pBuffer = NULL;
   pSample = CreateSoundFileStreamer( pMixer, argv[1], pBuffer, 65536, endItAll, NULL );
   if ( argc > 2 ) {
      startOff = atoi( argv[2] );
      ISndSample_SetPosition( pSample, startOff );
   }
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
   TIMELOG_DUMP( "sndlog.txt" );
   return msg.wParam ;

}

tResult LGAPI AppCreateObjects(int argc, const char * argv[])
{
   GenericApplicationCreate( argc, argv, "streamer", NULL );

   return NOERROR;
}

