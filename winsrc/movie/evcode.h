//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

//
// list of standard Quartz event codes and the expected params
//

// Event codes are broken into two groups
//   -- system event codes
//   -- extension event codes
// All system event codes are below EC_USER

#define EC_SYSTEMBASE                       0x00
#define EC_USER                             0x8000


// system-defined event codes

// Asynchronous Operation complete
// lParam1 is HRESULT, lParam2 is 0
#define EC_COMPLETE                         0x01

// User abort of async operation
// params 0
#define EC_USERABORT                        0x02

// Operation aborted because of error
// lParam1 is HRESULT, lParam2 is 0
#define EC_ERRORABORT                       0x03

// The requested reference time occurred
// lParam1 is low dword of ref time, lParam2 is high dword of reftime
#define EC_TIME                             0x04

// A repaint is required - lParam1 contains the (IPin *) that needs the data
// to be sent again. If the output pin which it is attached to supports the
// IMediaEventSink interface then it will be called with the EC_REPAINT first
// If that fails then normal repaint processing is done by the filtergraph
#define EC_REPAINT                          0x05

// Stream error notifications
// lParam 1 is major code, lParam2 is minor code
// Pity - it would have been nice to have a time or position!
#define EC_STREAM_ERROR_STOPPED             0x06
#define EC_STREAM_ERROR_STILLPLAYING        0x07

// a playback error has occurred, but the graph is still playing
#define EC_ERROR_STILLPLAYING               0x08

// notify application that the video palette has changed
#define EC_PALETTE_CHANGED                  0x09

// notify application that the native video size has changed
// LOWORD of lParam 1 is the width, HIWORD is the new height
#define EC_VIDEO_SIZE_CHANGED               0x0A

// notify application that degradation has ocurred
// lParam1   0
// lParam2   0
#define EC_QUALITY_CHANGE                   0x0B

// notify plug-in distributors that shutdown of filgraph is starting
#define EC_SHUTTING_DOWN                    0x0C

// notify application that the clock has changed
#define EC_CLOCK_CHANGED                    0x0D

// lParam1 == 1   --> starting to open file or buffer data
// lParam1 == 0   --> not opening or buffering any more
#define EC_OPENING_FILE                     0x10
#define EC_BUFFERING_DATA                   0x11

// sent by full renderers when switched away from
#define EC_FULLSCREEN_LOST                  0x12

// sent by video renderers when they lose or gain activation
// lParam is set to 1 if gained or 0 if lost
// lParam2 is the IFilter* for the filter that is sending the message
// Used for sound follows focus and fullscreen switching
#define EC_ACTIVATE                         0x13

// sent by renderers when they regain a resource (eg waveout). Causes
// a restart by Pause/put_Current/Run (if running)
// a seek to current position
#define EC_NEED_RESTART                     0x14

// sent by video renderers when the window has been destroyed. Handled
// by the filtergraph/distributor telling the resource manager.
// lParam1 is the IFilter* of the filter whose window is being destroyed
#define EC_WINDOW_DESTROYED                 0x15

// sent by renderers when they detect a display change. the filtergraph
// will arrange for the graph to be stopped and the pin send in lParam1
// to be reconnected. by being reconnected it allows a renderer to reset
// and connect with a more appropriate format for the new display mode
// lParam1 contains an (IPin *) that should be reconnected by the graph
#define EC_DISPLAY_CHANGED                  0x16

// sent by a filter when it detects starvation. Default handling (only when
// running) is for the graph to be paused until all filters enter the
// paused state and then run. Normally this would be sent by a parser or source
// filter when too little data is arriving.
// parameters unused
#define EC_STARVATION			    0x17

// Sent by a filter to pass a text string to the application. lParam1 is a
// BSTR (lParam2 is unused). You must free the BSTR by calling
// IMediaEvent::FreeEventParams.
#define EC_OLE_EVENT			    0x18