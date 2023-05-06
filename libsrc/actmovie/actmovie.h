///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/actmovie/RCS/actmovie.h $
// $Author: TOML $
// $Date: 1997/06/18 04:42:51 $
// $Revision: 1.8 $
//
// Active movie player
// Note: #ifdef AM_DDRAW stuff is commented out unless we want
// to try and get the DD blitter working with this code

#ifndef __ACTMOVIE_H
#define __ACTMOVIE_H

#include <aggmemb.h>
#include <movieapi.h>
#include <lgvidapi.h>
#include <amalloc.h>
#include <amvideo.h>
#include <dev2d.h>
#include <thrdtool.h>

F_DECLARE_INTERFACE(IGraphBuilder);
F_DECLARE_INTERFACE(IMediaEvent);
F_DECLARE_INTERFACE(IMediaControl);
F_DECLARE_INTERFACE(IVideoWindow);

class cActiveMoviePlayer1;
class cActiveMovieAlloc;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cActiveMovieDraw
//
class cActiveMovieDraw: public IImageExtern
{
public:
   cActiveMovieDraw(cActiveMoviePlayer1 *pActiveMoviePlayer1);

   DECLARE_UNAGGREGATABLE();

public:
   ///////////////////////////////////
   //
   // IImageExtern implementation functions
   //
   STDMETHOD_(BOOL, DrawImage) (THIS_ IMediaSample *pMediaSample);
   STDMETHOD_(void, SetSourceRect) (THIS_ RECT *pSourceRect);
   STDMETHOD_(void, SetTargetRect) (THIS_ RECT *pTargetRect);
   STDMETHOD_(void, GetTargetRect) (THIS_ RECT *pTargetRect);
   STDMETHOD_(void, GetSourceRect) (THIS_ RECT *pSourceRect);

   STDMETHOD(CheckMediaType) (THIS_ const CMediaType *pmtIn);
   STDMETHOD(NotifyMediaType) (THIS_ CMediaType *pmt);
   STDMETHOD(GetMediaType) (THIS_ int iPosition, CMediaType *pMediaType);

   STDMETHOD(GetAllocator) (THIS_ IMemAllocator **ppAllocator);
   STDMETHOD(NotifyAllocator) (THIS_ IMemAllocator *pAllocator, BOOL bReadOnly);

   // internal functions
   HRESULT CheckVideoType(const VIDEOINFO *pInput);

   BOOL GetBitmapFromMedia(uchar *type, uint *flags, LONG *w, LONG *h, int *bitcount);

#ifdef AM_DDRAW
   IDirectDraw *GetDirectDraw();
   void SetPrimarySurface(IDirectDrawSurface *pPrimarySurface);
#endif

   void SetAllocator(cActiveMovieAlloc *pActiveMovieAlloc);

protected:
   RECT m_TargetRect;              // Target destination rectangle
   RECT m_SourceRect;              // Source image rectangle
   BOOL m_bStretch;                // Do we have to stretch the images
   CMediaType *m_pMediaType;       // Pointer to the current format
   cActiveMoviePlayer1 *m_pActiveMoviePlayer1; // Owning active movie player
   cActiveMovieAlloc *  m_pActiveMovieAlloc;  // allocator for our bitmaps
#ifdef AM_DDRAW
   IDirectDraw *m_pDirectDraw;      // NULL if we aren't using direct draw?
   IDirectDrawSurface *m_pPrimarySurface; // if we're in DD mode
#endif

   ///////////////////////////////////
   //
   // Internal functions
   //
   void SetStretchMode();
   void InitDestinationVideoInfo(VIDEOINFO *pVideoInfo, DWORD dwComppression, int nBitCount);
   // setup a bitmap from a media sample
   void SetupBitmapFromMediaSample(grs_bitmap *pBitmap, IMediaSample *pMediaSample);
   const DWORD *GetBitMasks(const VIDEOINFO *pVideoInfo);

#ifdef AM_DDRAW
   // callback for the surface enumerator
   static HRESULT WINAPI SetPrimarySurfaceCallback(LPDIRECTDRAWSURFACE pDDSurface,
                                    LPDDSURFACEDESC pDDSurfaceDesc,
                                    LPVOID pContext);
#endif

   enum MEDIA_TYPES {MT_RGB565};
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cActiveMovieAlloc
//
// helper class to handle memory allocation

const int iBuffersMax = 2;

class cActiveMovieAlloc: public cBaseAllocator
{
public:
   cActiveMovieAlloc(cActiveMovieDraw *pActiveMovieDraw,
                     const char *pName, LPUNKNOWN pUnk, HRESULT *phr);
   ~cActiveMovieAlloc();
   STDMETHOD(SetProperties)(THIS_ ALLOCATOR_PROPERTIES *pRequest, ALLOCATOR_PROPERTIES *pActual);
   BOOL UsingCanvas();

protected:
   // called by commit and decommit from base class
   void Free(void);
   HRESULT Alloc(void);
   // utility functions
   DWORD CanvasSize() {return (DWORD)(grd_canvas->bm.h*grd_canvas->bm.row);};
   HRESULT AllocBitmapSample(grs_bitmap *pBitmap, LONG size);
#ifdef AM_DDRAW
   HRESULT AllocDDSample(LPDIRECTDRAWSURFACE pDDSurface, LONG size);
#endif
   // data
   cActiveMovieDraw *m_pActiveMovieDraw; // containing object
   ALLOCATOR_PROPERTIES m_AllocatorProperties; // established alloc properties
   BYTE *bms[iBuffersMax];     // buffers
   BOOL m_bUsingCanvas;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cActiveMoviePlayer1
//

class cActiveMoviePlayer1 : public IMoviePlayer1
{
public:
    // Construct an instance...
    cActiveMoviePlayer1(IUnknown * pOuterUnknown, unsigned flags);

    // Destroy an instance...
    virtual ~cActiveMoviePlayer1();

    // should we insert blank lines in playback?
    BOOL BlankLines();
    
    void SignalPlaybackError();

private:

    ///////////////////////////////////
    //
    // IMoviePlayer1 implementation functions
    //

    // IUnknown methods
    DECLARE_AGGREGATION(cActiveMoviePlayer1);

    // Open a movie file
    STDMETHOD_(BOOL, Open)(const char * pszName,
                          int xOrig, int yOrig,
                          int width, int height);

    // Close the previously opened movie file
    STDMETHOD_(void, Close)();

    // Query if can currently play
    STDMETHOD_(BOOL, CanPlay)();

    // Play the currently opened movie
    STDMETHOD_(BOOL, Play)(int flags);

    // Pause the currently playing movie
    STDMETHOD_(BOOL, Pause)();

    // Stop the currently playing movie
    STDMETHOD_(BOOL, Stop)();

    // Get/Set the sound volume (-10000..0).
    STDMETHOD_(BOOL, GetVolume)(int* pOutVol);
    STDMETHOD_(BOOL, SetVolume)(int inVol);

    // Query the state of the player
    STDMETHOD_(eMP1State, GetState)();

    // Specify the list of keys that terminate movie playing
    STDMETHOD_(void, SetTermKeys)(char* keylist);

    // Asynchronously play the currently opened movie
    BOOL PlayAsynchronous();

    // Create a graph builder
    BOOL CreateGraphBuilder();

    // Set up the interface from the LG Renderer to us
    BOOL SetLGRendererInterface(IFilter *pRendFilter);

    // Find a named filter in the registry
    IFilter *FindNamedFilterFromRegistry(LPSTR filterName);

    // Substitute a rendering filter
    BOOL SubstituteLGRenderer();

    BOOL ResizeConnection(IGraphBuilder *pGraphBuilder, IPin *pOutputPin, IPin *pInputPin, LONG w, LONG h);

    ///////////////////////////////////
    //
    // Aggregate member protocol
    //

    HRESULT Init();
    HRESULT End();

    ///////////////////////////////////
    //
    // Data
    //

    eMP1State  m_state;
    HANDLE              m_hNotify;               // notify events for the currently playing movie
    ulong               m_flags;
    BOOL                m_fCanPlay;
    cThreadEvent        m_ErrorSignal;

    IGraphBuilder *     m_pGraphBuilder;         // the filter graph for the current movie
    IMediaEvent *       m_pMediaEvent;
    IMediaControl *     m_pMediaControl;

    cActiveMovieDraw *  m_pActiveMovieDraw;     // the helper class for drawing
    
    unsigned m_fCreateFlags;

};


inline void cActiveMoviePlayer1::SignalPlaybackError()
{
    m_ErrorSignal.Set();
}


#endif /* !__ACTMOVIE_H */





