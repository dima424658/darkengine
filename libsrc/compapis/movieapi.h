///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/compapis/RCS/movieapi.h $
// $Author: TOML $
// $Date: 1997/06/18 04:42:40 $
// $Revision: 1.3 $
//
// The Movie API is likely to change in the next few months, so the first
// revision has interface version numbers from the start (toml 08-01-96)
//

#ifndef __MOVIEAPI_H
#define __MOVIEAPI_H

///////////////////////////////////////

#include "moviguid.h"
#include "movconst.h"

///////////////////////////////////////////////////////////////////////////////
//
// Forward Declarations
//

///////////////////////////////////////////////////////////////////////////////
//
// Movie API
//

enum eMoviePlayerCreateFlags
{
    kMoviesWhenRecorderActive = 0x01
};

//
// Create a display device and add it to the global app-object
//
#define MoviePlayer1Create() \
    { \
       IUnknown * pAppUnknown = AppGetObj(IUnknown); \
       _MoviePlayerCreate(IID_TO_REFIID(IID_IMoviePlayer1), NULL, pAppUnknown, IID_TO_REFIID(CLSID_ActiveMoviePlayer), 0); \
       COMRelease(pAppUnknown); \
    }

#define MoviePlayer1Create2(flags) \
    { \
       IUnknown * pAppUnknown = AppGetObj(IUnknown); \
       _MoviePlayerCreate(IID_TO_REFIID(IID_IMoviePlayer1), NULL, pAppUnknown, IID_TO_REFIID(CLSID_ActiveMoviePlayer), flags); \
       COMRelease(pAppUnknown); \
    }

//
// Creates a movie player, aggregating it with specfied pOuter,
// use IAggregate protocol if ppMoviePlayer is NULL, else self-init
//
// Last parameter specifies which kind of movie player to implement
//
EXTERN tResult LGAPI _MoviePlayerCreate(REFIID, void ** ppMoviePlayer, IUnknown * pOuter, REFCLSID, unsigned flags);


///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IMoviePlayer1
//
// This is a pretty simple interface that is likely to be replaced with
// something more sophisticated in the future.  Because of its simplicity
// its probably always going to be supported,  so you can feel safe coding
// to it. (toml 08-01-96)
//
// Usage:
//
//    1. Open a movie using IMoviePlayer1::Open(). Coordinates are
//    always relative to the main window.
//
//    2. Play the movie using IMoviePlayer1::Play().  Optional flags
//    specify whether to play asynchronously or not, and whether to
//    stop the movie if the user hits a key or clicks the mouse.
//
//    3. Either re-Play() the movie or Close() it. Note that you can
//    only have one movie open at a time.
//
//    @TBD (toml 08-01-96): We need to work out how the app is to be
//    informed when asynchronous play is complete
//

enum eMP1State
{
    // No movie opemn
    kMP1NothingOpen,

    // Movie is open and stopped, ready to play
    kMP1Ready,

    // Movie is playing
    kMP1Playing,

    // Movie is paused
    kMP1Paused

};

typedef enum eMP1State eMP1State;


#undef INTERFACE
#define INTERFACE IMoviePlayer1

DECLARE_INTERFACE_(IMoviePlayer1, IUnknown)
{
    //
    // IUnknown methods
    //
    DECLARE_UNKNOWN_PURE();

    //
    // Open a movie file
    //
    STDMETHOD_(BOOL, Open)(THIS_ const char * pszName,
                                 int xOrig, int yOrig,
                                 int width, int height ) PURE;

    //
    // Close the previously opened movie file
    //
    STDMETHOD_(void, Close)(THIS) PURE;

    //
    // Query if can currently play
    //
    STDMETHOD_(BOOL, CanPlay)(THIS) PURE;

    //
    // Play the currently opened movie
    //
    STDMETHOD_(BOOL, Play)(THIS_ int flags) PURE;

    //
    // Pause the currently playing movie
    //
    STDMETHOD_(BOOL, Pause)(THIS) PURE;

    //
    // Stop the currently playing movie
    //
    STDMETHOD_(BOOL, Stop)(THIS) PURE;

    //
    // Query the state of the player
    //
    STDMETHOD_(eMP1State, GetState)(THIS) PURE;

};

#define IMoviePlayer1_QueryInterface(p, a, b)   COMQueryInterface(p, a, b)
#define IMoviePlayer1_AddRef(p)                 COMAddRef(p)
#define IMoviePlayer1_Release(p)                COMRelease(p)
#define IMoviePlayer1_Open(p, a, b, c, d, e)    COMCall5(p, Open, a, b, c, d, e)
#define IMoviePlayer1_CanPlay(p)                COMCall0(p, CanPlay)
#define IMoviePlayer1_Close(p)                  COMCall0(p, Close)
#define IMoviePlayer1_Play(p, a)                COMCall1(p, Play, a)
#define IMoviePlayer1_Pause(p)                  COMCall0(p, Pause)
#define IMoviePlayer1_Stop(p)                   COMCall0(p, Stop)
#define IMoviePlayer1_GetState(p)               COMCall0(p, GetState)

#endif /* !__MOVIEAPI_H */
