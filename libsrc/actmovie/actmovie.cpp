///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/actmovie/RCS/actmovie.cpp $
// $Author: DAVET $
// $Date: 1998/01/21 12:28:44 $
// $Revision: 1.23 $
//

#ifdef _WIN32
#include <windows.h>
#endif
#include <lg.h>

#include <comtools.h>
#include <aggmemb.h>
#include <appagg.h>
#include <dispapi.h>
#include <recapi.h>
#include <wappapi.h>
#include <wdispapi.h>

#ifdef _WIN32
#include <strmif.h>

#include <actmovie.h>
#include <getlgvid.h>

#include <ddraw.h>

#include <control.h>
#include <evcode.h>
#include <uuids.h>
#include <2d.h>
#include <imutil.h>
#include <limits.h>
#include <vfwmsgs.h>
#include <util2d.h>

#include <mouse.h>
#include <kb.h>

#include <initguid.h>
#include <lgvdguid.h>

///////////////////////////////////////////////////////////////////////////////

#pragma off(unreferenced)
EXTERN
tResult LGAPI _MoviePlayerCreate(REFIID, void ** ppMoviePlayer, IUnknown * pOuterUnknown, REFCLSID, unsigned flags)
    {
    // Create the application.  Instance adds self to pOuterUnknown
    // @TBD (toml 08-01-96): need to really do this function, i.e. fail if
    // refclsid parameter is not CLSID_ActiveMoviePlayer
    return (new cActiveMoviePlayer1(pOuterUnknown, flags) != 0) ? NOERROR : E_FAIL;
    }
#pragma on(unreferenced)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cActiveMoviePlayer1
//

//
// Pre-fab COM implementations
//
IMPLEMENT_AGGREGATION_SELF_DELETE(cActiveMoviePlayer1);

///////////////////////////////////////

cActiveMoviePlayer1::cActiveMoviePlayer1(IUnknown * pOuterUnknown, unsigned flags)
  : m_state(kMP1NothingOpen),
    m_hNotify(0),
    m_flags(0),
    m_pGraphBuilder(NULL),
    m_pMediaEvent(NULL),
    m_pMediaControl(NULL),
    m_pActiveMovieDraw(NULL),
    m_fCanPlay(FALSE),
    m_fCreateFlags(flags)
{
    // Add internal components to outer aggregate, ensuring IDisplayDevice comes first...
    sRelativeConstraint constraints[] =
    {
        {   kConstrainAfter,    &IID_IDisplayDevice },
        {   kNullConstraint,    NULL }
    };

    INIT_AGGREGATION_1( pOuterUnknown,
                        IID_IMoviePlayer1, this,
                        kPriorityLibrary,
                        constraints );
}


///////////////////////////////////

cActiveMoviePlayer1::~cActiveMoviePlayer1()
{
    AssertMsg(m_state == kMP1NothingOpen, "Movie cannot be open at this point");
}


///////////////////////////////////
//
// Initialize the movie player
//

HRESULT cActiveMoviePlayer1::Init()
{
    if (GetLGVideoRenderer() == S_OK)
        m_fCanPlay = TRUE;
    return CoInitialize(NULL);
}

///////////////////////////////////
//
// Clean up the player
//

HRESULT cActiveMoviePlayer1::End()
{
    m_fCanPlay = FALSE;
    ReleaseLGVideoRenderer();
    Close();
    return NOERROR;
}

#define MAX_PINS 2

// Find a single pin on a filter
// Returns FALSE & sets pin to NULL if zero or multiple pins are found
bool FindSinglePin(IFilter *pFilter, IPin **pPin)
{
   IEnumPins *pEnum;
   ULONG cFetched;
   IPin *pPins[MAX_PINS];
   int i;

   *pPin = NULL;
   if (SUCCEEDED(pFilter->EnumPins(&pEnum)))
   {
      if (SUCCEEDED(pEnum->Next(MAX_PINS, pPins, &cFetched)) && (cFetched == 1))
      {
         *pPin = pPins[0];
      }
      else
         for (i=0; i<cFetched; i++)
         {
            SafeRelease(pPins[i]);
         }
   }
   SafeRelease(pEnum);
   return (*pPin!=NULL);
}

// Set up the interface from the LG Renderer to us
BOOL cActiveMoviePlayer1::SetLGRendererInterface(IFilter *pRendFilter)
{
   IVideoRenderer *pVidRend = NULL;
   IImageExtern *pImageExt = NULL;
   bool success = FALSE;

   if (SUCCEEDED(pRendFilter->QueryInterface(IID_IVideoRenderer, (void**) &pVidRend)))
   {
      if (SUCCEEDED(m_pActiveMovieDraw->QueryInterface(IID_IImageExtern, (void**) &pImageExt)))
      {
         pVidRend->SetImageExtern(pImageExt);
         success = TRUE;
      }
      else
      {
         DebugMsg("Can't get IImageExtern interface");
      }
   }
   else
   {
      DebugMsg("Can't get IVideoRenderer interface");
   }
   SafeRelease(pVidRend);
   SafeRelease(pImageExt);
   return success;
}

IFilter *cActiveMoviePlayer1::FindNamedFilterFromRegistry(LPSTR filterName)
{
    IFilterMapper *pFM=NULL;
    bool found = FALSE;
    ULONG cFetched;
    DWORD err;
    char regFilterName[MAX_FILTER_NAME];
    REGFILTER *pRegFilter = NULL;
    IFilter *pFilter = NULL;
    IEnumRegFilters *pEnum=NULL;
    HRESULT hr;

   // now try to find the sample renderer
   // get the filter mapper interface
   if (SUCCEEDED(m_pGraphBuilder->QueryInterface(IID_IFilterMapper, (void **) &pFM)))
   {
      // now do match for the rendering filters
      if (SUCCEEDED(pFM->EnumMatchingFilters(&pEnum,
                                             MERIT_UNLIKELY, TRUE, GUID_NULL, GUID_NULL,
                                             TRUE, FALSE, GUID_NULL, GUID_NULL)))
      {
         // Look through the rendering filters
         while (!found && SUCCEEDED(pEnum->Next(1, &pRegFilter, &cFetched)) && (cFetched == 1))
         {
            if (!WideCharToMultiByte(CP_ACP, NULL,
                                     pRegFilter->Name, -1, regFilterName,
                                     MAX_FILTER_NAME, NULL, NULL))
            {
               err = GetLastError();
               continue;
            }
            if (strcmp(regFilterName, filterName))
            {
               CoTaskMemFree(pRegFilter);
               pRegFilter = NULL;
            }
            else
               found = TRUE;
         }
         if (pRegFilter)
         {
            if (FAILED(hr = (CoCreateInstance(pRegFilter->Clsid,
                                        NULL, CLSCTX_INPROC_SERVER,
                                        IID_IFilter, (void**) &pFilter))))
               pFilter = NULL;
            CoTaskMemFree(pRegFilter);
         }
      }
      SafeRelease(pFM);
   }
   return pFilter;
}

// Try to resize the media type agreed on by our connection
BOOL cActiveMoviePlayer1::ResizeConnection(IGraphBuilder *pGraphBuilder, IPin *pOutputPin, IPin *pInputPin, LONG w, LONG h)
{
   AM_MEDIA_TYPE oldType, newType;
   VIDEOINFO *pNewVideoInfo, *pOldVideoInfo;

   if (SUCCEEDED(pOutputPin->ConnectionMediaType(&oldType)) &&
       (oldType.majortype == MEDIATYPE_Video))
   {
      // copy the media type
      newType = oldType;
      pOldVideoInfo = (VIDEOINFO*)oldType.pbFormat;
      Assert_(pOldVideoInfo != NULL);
      // construct a new video info block - copy from old
      pNewVideoInfo = new VIDEOINFO;
      *pNewVideoInfo = *pOldVideoInfo;
      newType.pbFormat = (BYTE*)pNewVideoInfo;
      LPBITMAPINFOHEADER lpbi = HEADER(pNewVideoInfo);
      // fill in the resized video info
      lpbi->biWidth = w;
      lpbi->biHeight = h;
      lpbi->biSizeImage = (lpbi->biBitCount/8)*lpbi->biWidth*lpbi->biHeight;
      newType.lSampleSize = (LONG)lpbi->biSizeImage;
      LARGE_INTEGER li;
      li.QuadPart = pOldVideoInfo->AvgTimePerFrame;
      pNewVideoInfo->dwBitRate = MulDiv(lpbi->biSizeImage, 80000000, li.LowPart);
      // disconnect the current connections
      pGraphBuilder->Disconnect(pOutputPin);
      pGraphBuilder->Disconnect(pInputPin);
      // try reconnecting with the resized media type
      if (SUCCEEDED(pOutputPin->Connect(pInputPin, &newType)))
         return TRUE;
      else
      {
         // we failed, so give up an reconnect with the old type
         DebugMsg("ResizeConnection: Can't reconnect pins with new type");
         if (SUCCEEDED(pOutputPin->Connect(pInputPin, &oldType)))
            return TRUE;
         else
            DebugMsg("ResizeConnection: Can't reconnect pins with original type");
      }
   }
   else
      DebugMsg("ResizeConnection: Bad media type for output pin");
   return FALSE;
}

///////////////////////////////////
//
// Try to substitute the a renderer for the current
// video renderer. Assumes that the current renderer is the
// "default" Video Renderer, and is named as such.  If we don't
// find it, then we do a registry lookup using CLSID\guid. 
//
BOOL cActiveMoviePlayer1::SubstituteLGRenderer()
{
    BEGIN_DEBUG_MSG("cActiveMoviePlayer1::SubstituteLGRenderer()");

    IFilter *pRendFilter=NULL, *pLGFilter=NULL;
    char defaultFilterName[MAX_FILTER_NAME] = "Video Renderer";
    char LGFilterName[MAX_FILTER_NAME] = "LG Video Renderer";
    WCHAR wDefaultFilterName[MAX_FILTER_NAME];
    WCHAR wLGFilterName[MAX_FILTER_NAME];
    IPin *pRendPin=NULL, *pInPin=NULL, *pOutPin=NULL;
    BOOL substituted = FALSE;

    // See if the LG filter is already in the graph
    MultiByteToWideChar(CP_ACP, 0, LGFilterName, -1, wLGFilterName, MAX_FILTER_NAME);
    if (!SUCCEEDED(m_pGraphBuilder->FindFilterByName(wLGFilterName, &pLGFilter)))
    {
       // OK, the requested filter wasn't in the graph
       // so first find the current rendering filter in the graph
       // If we don't find it as 'Video Renderer', then look up the
       // name via the key.
       MultiByteToWideChar(CP_ACP, 0, defaultFilterName, 
          -1, wDefaultFilterName, MAX_FILTER_NAME);
       m_pGraphBuilder->FindFilterByName(wDefaultFilterName, &pRendFilter);
       if (!pRendFilter)      // did not find it via 'Video Renderer'
       {
          HKEY hKey;
          CHAR RegPath[128];
          unsigned long lDefaultRendererNameSize=128;
          unsigned char szDefaultRendererName[128];
          DWORD  retCode;

          strcpy(RegPath,"CLSID\\{70e102b0-5556-11ce-97c0-00aa0055595a}");
          retCode = RegOpenKeyEx (HKEY_CLASSES_ROOT, 
                          RegPath,     // Path name of child key.
                          0,           // Reserved.
                          KEY_EXECUTE, // Requesting read access.
                          &hKey);      // Address of key to be returned.

          if(retCode==ERROR_SUCCESS) { // oxymoron
             retCode= RegQueryValueEx(hKey, 
                          NULL, 
                          NULL,
                          NULL,
                          szDefaultRendererName,
                          &lDefaultRendererNameSize);
             if(retCode==ERROR_SUCCESS) { 
               MultiByteToWideChar(CP_ACP, 0, (char *) szDefaultRendererName,
                  -1,wDefaultFilterName, MAX_FILTER_NAME);
               m_pGraphBuilder->FindFilterByName
                  (wDefaultFilterName, &pRendFilter);
             }
          }
       }

       if (!pRendFilter)
          DebugMsg("Warning: can't find default renderer in graph");

       pLGFilter = FindNamedFilterFromRegistry(LGFilterName);
       if (!pLGFilter)
       {
          DebugMsg("Warning: can't find LG renderer in registry");
       }
       else
       {
          if (!SetLGRendererInterface(pLGFilter))
          {
             SafeRelease(pLGFilter);
          }
       }
       if (pRendFilter && pLGFilter)
       {
          // OK, found the both renderers, so now find their input pins
          if (!FindSinglePin(pRendFilter, &pRendPin))
          {
             DebugMsg("Warning: can't find input pin on default renderer");
          }
          if (pRendPin)
             // find the pin that is connected to the renderer
             if (FAILED(pRendPin->ConnectedTo(&pOutPin)))
             {
                DebugMsg("Warning: can't find output pin connecting to default renderer");
                pOutPin = NULL;
             }
          if (!FindSinglePin(pLGFilter, &pInPin))
          {
             DebugMsg("Warning: can't find input pin on LG renderer");
          }
          if (pOutPin && pInPin)
          {
             // found the right pins, so do the reconnect
             m_pGraphBuilder->Disconnect(pOutPin);
             m_pGraphBuilder->Disconnect(pInPin);
             m_pGraphBuilder->RemoveFilter(pRendFilter);
             m_pGraphBuilder->AddFilter(pLGFilter, wLGFilterName);
             if (SUCCEEDED(m_pGraphBuilder->Connect(pOutPin, pInPin)))
             {
                substituted = TRUE;
                //              substituted = ResizeConnection(m_pGraphBuilder, pOutPin, pInPin, grd_canvas->bm.w, grd_canvas->bm.h);
             }
             else
                 DebugMsg("Warning: can't connect LG filter");
          }
       }
    }
    else
    {
       DebugMsg("Warning: LG Filter already in graph");
    }
    SafeRelease(pRendFilter);
    SafeRelease(pLGFilter);
    SafeRelease(pInPin);
    SafeRelease(pOutPin);
    SafeRelease(pRendPin);

    return substituted;
    END_DEBUG;
}

///////////////////////////////////
//
// Open a movie file
//

STDMETHODIMP_(BOOL) cActiveMoviePlayer1::Open(const char * pszName, int xOrig, int yOrig, int width, int height)
{
    BEGIN_DEBUG_MSG("cActiveMoviePlayer1::Open()");

    AutoAppIPtr(WinDisplayDevice);

    if (!m_fCanPlay)
        return FALSE;
        
    pWinDisplayDevice->WaitForMutex();
    
    RECT targetRect = {xOrig, yOrig, xOrig+width-1, yOrig+height-1};
    RECT sourceRect;
    WCHAR wszName[MAX_PATH];

    if (m_pActiveMovieDraw == NULL)
       m_pActiveMovieDraw = new cActiveMovieDraw(this);

    Close();

    if (!CreateGraphBuilder())
    {
        DebugMsg("Warning: can't create graph builder instance");
        pWinDisplayDevice->ReleaseMutex();
        return FALSE;
    }

    MultiByteToWideChar(CP_ACP, 0, pszName, -1, wszName, MAX_PATH);

    BEGIN_DEBUG_MSG("RenderFile...");
    if (m_pGraphBuilder->RenderFile(wszName, NULL) != NOERROR)
    {
        DebugMsg1("Warning: can't create filter graph for movie %s", pszName);
        pWinDisplayDevice->ReleaseMutex();
        return FALSE;
    }
    END_DEBUG;

    if (!SubstituteLGRenderer())
    {
       DebugMsg("Warning: can't find LG Video Renderer filter\n");
       pWinDisplayDevice->ReleaseMutex();
       return FALSE;
    }

    m_pActiveMovieDraw->GetSourceRect(&sourceRect);
    if (width == -1)
       targetRect.right = targetRect.left+(sourceRect.right-sourceRect.left)-1;
    if (height == -1)
       targetRect.bottom = targetRect.top+(sourceRect.bottom-sourceRect.top)-1;
    m_pActiveMovieDraw->SetTargetRect(&targetRect);

    m_state = kMP1Ready;

   pWinDisplayDevice->ReleaseMutex();
   return TRUE;

   END_DEBUG;
}


///////////////////////////////////
//
// Close the previously opened movie file
//

STDMETHODIMP_(void) cActiveMoviePlayer1::Close()
{
    if (!m_fCanPlay)
        return;

    if (m_pGraphBuilder)
    {
        SafeRelease(m_pGraphBuilder);
        SafeRelease(m_pMediaEvent);
        SafeRelease(m_pMediaControl);
    }

    m_hNotify = NULL;
    m_state = kMP1NothingOpen;
}


///////////////////////////////////
//
// Query if can currently play
//

STDMETHODIMP_(BOOL) cActiveMoviePlayer1::CanPlay()
{
    return (m_state == kMP1Ready);
}


///////////////////////////////////
//
// Play the currently opened movie
//

STDMETHODIMP_(BOOL) cActiveMoviePlayer1::Play(int flags)
{
    BEGIN_DEBUG_MSG1("cActiveMoviePlayer1::Play(0x%x)", flags);

    AutoAppIPtr(WinApp);
    AutoAppIPtr(Recorder);

    if (!m_fCanPlay || !pWinApp)
        return FALSE;

    if (!!pRecorder)
    {
        if (!(m_fCreateFlags & kMoviesWhenRecorderActive) && 
             pRecorder->GetMode() != kRecInactive)
        return FALSE;
        
        pRecorder->Pause();
    }
    
    BOOL retVal = TRUE;
    MSG     msg;

    m_flags = flags;

    m_ErrorSignal.Reset();
    
    AutoAppIPtr(DisplayDevice);
    int iPreviousLock = pDisplayDevice->BreakLock();
    InvalidateRect(pWinApp->GetMainWnd(), NULL, FALSE);
    UpdateWindow(pWinApp->GetMainWnd());
    
    // Flush all input
    pWinApp->PumpEvents(kPumpAll, kPumpUntilEmpty);

    mouse_flush();
    kb_flush();

    retVal = PlayAsynchronous();

    if (retVal && (flags & kMoviePlayBlock))
    {
        HANDLE  waitObjectHandles[4];
        long    eventCode, pIgnored1, pIgnored2;
        DWORD   result;

        AssertMsg(m_hNotify, "notification handle cannot be NULL if PlayAsynchronous() succeeded");

        BEGIN_DEBUG_MSG("Blocking loop...");

        waitObjectHandles[0] = m_hNotify;
        waitObjectHandles[1] = mouse_get_queue_available_signal();
        waitObjectHandles[2] = kb_get_queue_available_signal();
        waitObjectHandles[3] = m_ErrorSignal;
        
        while (m_state == kMP1Playing)
        {
            // wait for messages or notification of graph changes
            result = MsgWaitForMultipleObjects(3, waitObjectHandles, FALSE, INFINITE, QS_ALLINPUT);
            if (result == WAIT_OBJECT_0)
            {
                // hey, we got a notification about our filter graph
                if (m_pMediaEvent->GetEvent(&eventCode, &pIgnored1, &pIgnored2, 0) == NOERROR)
                {
                    DebugMsg1("Play() got event %d\n", eventCode);
                    if ((eventCode == EC_COMPLETE) ||
                        (eventCode == EC_USERABORT) ||
                        (eventCode == EC_ERRORABORT))
                    {
                        Stop();
                    }
                }
                continue;
            }
            else
            {
                BOOL fQuit = m_ErrorSignal.Check();
                
                if (!fQuit)
                {
                    // We got a Windows message or input...
                    if (pDisplayDevice->Lock() != E_FAIL)
                    {
                        IWinApp_PumpEvents(pWinApp, kPumpAll, kPumpOne);

                        // If client allowed key and mouse stop, then stop
                        lgMouseEvent mouseEvent;
                        kbs_event    keyEvent;

                        if (mouse_look_next(&mouseEvent) == OK)
                        {
                            fQuit = !!(mouseEvent.type & (MOUSE_LUP | MOUSE_RUP | MOUSE_CUP));
                            mouse_flush();
                        }

                        keyEvent = kb_look_next();
                        if (keyEvent.code != KBC_NONE)
                        {
                            fQuit = TRUE;
                        }

                        pDisplayDevice->Unlock();
                    }
                    else 
                        fQuit = TRUE;
                }

                if (fQuit && !(flags & kMoviePlayNoStop))
                {
                    Stop();
                    break;
                }
            }
        }

        END_DEBUG;
    }

    InvalidateRect(pWinApp->GetMainWnd(), NULL, FALSE);
    UpdateWindow(pWinApp->GetMainWnd());
    IWinApp_PumpEvents(pWinApp, kPumpAll, kPumpUntilEmpty);

    // Flush all input
    pWinApp->PumpEvents(kPumpAll, kPumpUntilEmpty);

    mouse_flush();
    kb_flush();

    pDisplayDevice->RestoreLock(iPreviousLock);

    if (!!pRecorder)
        pRecorder->Resume();
    
    return retVal;
    END_DEBUG;
}


///////////////////////////////////
//
// Pause the currently playing movie
//

STDMETHODIMP_(BOOL) cActiveMoviePlayer1::Pause()
{
    CriticalMsg("cActiveMoviePlayer1::Pause() is not implemented");
    return FALSE;
}


///////////////////////////////////
//
// Stop the currently playing movie
//

STDMETHODIMP_(BOOL) cActiveMoviePlayer1::Stop()
{
    DebugMsg("cActiveMoviePlayer1::Stop()");

    if (m_state != kMP1Playing)
    {
        DebugMsg("MovieStop: can't stop a movie that isn't running");
        return FALSE;
    }

    HRESULT hResult = m_pMediaControl->Stop();
    m_state = kMP1Ready;

    AssertMsg1(SUCCEEDED(hResult), "Failed to stop movie! (0x%x)", hResult);
    return FALSE;
}


///////////////////////////////////
//
// Query the state of the player
//

STDMETHODIMP_(eMP1State) cActiveMoviePlayer1::GetState()
{
    return m_state;
}


///////////////////////////////////
//
// Asynchronously play the movie
//

BOOL cActiveMoviePlayer1::PlayAsynchronous()
{
    BEGIN_DEBUG_MSG("cActiveMoviePlayer1::PlayAsynchronous()");

    if (m_pMediaControl && (m_state == kMP1Ready || m_state == kMP1Paused))
    {
        HRESULT hResult = m_pMediaControl->Run();

        if (hResult >= S_OK)
        {
            m_state = kMP1Playing;
            DebugMsg("Playing movie");
            DebugMsgTrue(hResult > S_OK, "At least one filter reported an error");
            return TRUE;
        }

        CriticalMsg1("IMediaControl::Run() failed, (%d)", hResult);
    }

    DebugMsg("Not playing movie");

    return FALSE;
    END_DEBUG;
}


///////////////////////////////////
//
// Create a graph builder
//

BOOL cActiveMoviePlayer1::CreateGraphBuilder()
{
    BEGIN_DEBUG_MSG("CreateGraphBuilder()");

    AssertMsg(!m_pGraphBuilder, "Already have a filer graph");

    HRESULT hResult = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void **)&m_pGraphBuilder);

    if (hResult == NOERROR)
    {
        m_pGraphBuilder->QueryInterface(IID_IMediaEvent, (void **)&m_pMediaEvent);
        m_pGraphBuilder->QueryInterface(IID_IMediaControl, (void **)&m_pMediaControl);

        if (m_pMediaEvent && m_pMediaControl)
        {
            if (m_pMediaEvent->GetEventHandle((OAEVENT*)&m_hNotify) == NOERROR)
                return TRUE;
        }
    }

    Warning(("CreateGraphBuilder() failed!"));
    
    m_fCanPlay = FALSE;

    SafeRelease(m_pGraphBuilder);
    SafeRelease(m_pMediaEvent);
    SafeRelease(m_pMediaControl);

    return FALSE;
    END_DEBUG;
}

BOOL cActiveMoviePlayer1::BlankLines()
{
   return (m_flags&kMovieBlankLines);
}

BOOL cActiveMoviePlayer1::GetVolume(int* pOutVol)
{
    // TODO
    return FALSE;
}

BOOL cActiveMoviePlayer1::SetVolume(int inVol)
{
    // TODO
    return FALSE;
}

void cActiveMoviePlayer1::SetTermKeys(char* keylist)
{
    // TODO
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cActiveMovieDraw
//

//
// Pre-fab COM implementations
//
IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cActiveMovieDraw, IImageExtern);

cActiveMovieDraw::cActiveMovieDraw(cActiveMoviePlayer1 *pActiveMoviePlayer1) :
   m_bStretch(FALSE),
   m_pMediaType(NULL),
   m_pActiveMoviePlayer1(pActiveMoviePlayer1),
#ifdef AM_DDRAW
   m_pDirectDraw(NULL),
   m_pPrimarySurface(NULL),
#endif
   m_pActiveMovieAlloc(NULL)
{
   AutoAppIPtr(WinDisplayDevice);

   SetRectEmpty(&m_TargetRect);
   SetRectEmpty(&m_SourceRect);
#ifdef AM_DDRAW
   // determine if we're in DD mode
   if (pWinDisplayDevice)
   {
      if (SUCCEEDED(pWinDisplayDevice->GetDirectDraw(&m_pDirectDraw)) &&
          (m_pDirectDraw != NULL))
      {
         // get the primary surface
         DDSURFACEDESC ddsd;
         memset(&ddsd, 0, sizeof(ddsd));
         ddsd.dwSize = sizeof(ddsd);
         ddsd.dwFlags = DDSD_CAPS;
         ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
         if (FAILED(m_pDirectDraw->EnumSurfaces(DDENUMSURFACES_MATCH,
                                           &ddsd,
                                           this,
                                           (LPDDENUMSURFACESCALLBACK)&SetPrimarySurfaceCallback)))
            DebugMsg("cActiveMovieDraw::cActiveMovieDraw: can't find primary surface");
      }
   }
#endif
}

#ifdef AM_DDRAW
void cActiveMovieDraw::SetPrimarySurface(IDirectDrawSurface *pPrimarySurface)
{
   m_pPrimarySurface = pPrimarySurface;
}

#pragma off(unreferenced)
static HRESULT WINAPI cActiveMovieDraw::SetPrimarySurfaceCallback(LPDIRECTDRAWSURFACE pDDSurface,
                                         LPDDSURFACEDESC pDDSurfaceDesc,
                                         LPVOID pContext)
{
   cActiveMovieDraw *AMDraw = (cActiveMovieDraw*)pContext;

   if (pDDSurface != NULL)
   {
      IDirectDrawSurface *pIDDSurface;

      if (SUCCEEDED(pDDSurface->QueryInterface(IID_IDirectDrawSurface, (void**) &pIDDSurface)))
      {
         AMDraw->SetPrimarySurface(pIDDSurface);
         return DDENUMRET_OK;
      }
   }
   return DDENUMRET_CANCEL;
}
#pragma on(unreferenced)
#endif

void cActiveMovieDraw::SetupBitmapFromMediaSample(grs_bitmap *pBitmap, IMediaSample *pMediaSample)
{
   VIDEOINFO *pVideoInfo;

   pMediaSample->GetPointer((BYTE**)&pBitmap->bits);
   pVideoInfo = (VIDEOINFO*)m_pMediaType->pbFormat;
   gr_init_bitmap(pBitmap,
                  pBitmap->bits,
                  BitmapTypeFromBMIHeader(&pVideoInfo->bmiHeader),
                  BitmapFlagsFromBMIHeader(&pVideoInfo->bmiHeader),
                  (int)pVideoInfo->bmiHeader.biWidth,
                  (int)pVideoInfo->bmiHeader.biHeight);
}

#pragma off(unreferenced)
// This is called with an IMediaSample interface on the image to be drawn.
STDMETHODIMP_(BOOL) cActiveMovieDraw::DrawImage(IMediaSample *pMediaSample)
{
   BEGIN_DEBUG_MSG("cActiveMovieDraw::DrawImage");

   AutoAppIPtr(DisplayDevice);
   grs_bitmap bm;
   uchar flags;

   if (IDisplayDevice_Lock(pDisplayDevice) == E_FAIL)
   {
      m_pActiveMoviePlayer1->SignalPlaybackError();
      return FALSE;
   }

#ifdef AM_DDRAW
   if (m_pDirectDraw == NULL)
   {
#endif
      if (!m_pActiveMovieAlloc->UsingCanvas())
      {
         // we need to blit (and stretch)
         grs_bitmap *pBitmap;
         void *ptr;

         pBitmap = ((cBitmapSample*)pMediaSample)->GetBitmap();
         Assert_(pBitmap != NULL);
         //         gr_bitmap(pBitmap, m_TargetRect.left, m_TargetRect.top);
         /*
         gr_scale_bitmap(pBitmap,
                         m_TargetRect.left,
                         m_TargetRect.top,
                         m_TargetRect.right-m_TargetRect.left,
                         m_TargetRect.bottom-m_TargetRect.top);
         */
         flags = AM_BLIT_V_FLIP;
         if (m_pActiveMoviePlayer1->BlankLines())
            flags |= AM_BLIT_V_GRATE;
         AMBlit(pBitmap->bits,
                (ULONG)pBitmap->w,
                (ULONG)pBitmap->h,
                grd_canvas->bm.bits,
                (ULONG)grd_canvas->bm.w,
                (ULONG)grd_canvas->bm.h,
                (int)grd_canvas->bm.row,
                &m_TargetRect,
                (uchar)gr_bitmap_type_count(pBitmap),
                flags);
      }
#ifdef AM_DDRAW
   }
   else
   {
      // direct draw mode
      LPDIRECTDRAWSURFACE pDDSurface;
      Assert_(m_pPrimarySurface != NULL);

      pDDSurface = ((cDDSample*)pMediaSample)->GetDDSurface();
      Assert_(pDDSurface != NULL);
      m_pPrimarySurface->Blt(&m_TargetRect, pDDSurface, &m_SourceRect, 0, NULL);
   }
#endif
   pDisplayDevice->FlushRect(m_TargetRect.left, m_TargetRect.top, m_TargetRect.right, m_TargetRect.bottom);
   IDisplayDevice_Unlock(pDisplayDevice);
   SafeRelease(pDisplayDevice);

   return TRUE;
   END_DEBUG;
}
#pragma on(unreferenced)

// This is called to set the target rectangle in the video window, it will be
// called whenever a WM_SIZE message is retrieved from the message queue. We
// simply store the rectangle and use it later when we do the drawing calls

STDMETHODIMP_(void) cActiveMovieDraw::SetTargetRect(RECT *pTargetRect)
{
   Assert_(pTargetRect);
   m_TargetRect = *pTargetRect;
   SetStretchMode();
}


// Return the current target rectangle

STDMETHODIMP_(void) cActiveMovieDraw::GetTargetRect(RECT *pTargetRect)
{
   Assert_(pTargetRect);
   *pTargetRect = m_TargetRect;
}


// This is called when we want to change the section of the image to draw. We
// use this information in the drawing operation calls later on. We must also
// see if the source and destination rectangles have the same dimensions. If
// not we must stretch during the drawing rather than a direct pixel copy

STDMETHODIMP_(void) cActiveMovieDraw::SetSourceRect(RECT *pSourceRect)
{
   Assert_(pSourceRect);
   m_SourceRect = *pSourceRect;
   SetStretchMode();
}


// Return the current source rectangle

STDMETHODIMP_(void) cActiveMovieDraw::GetSourceRect(RECT *pSourceRect)
{
   Assert_(pSourceRect);
   *pSourceRect = m_SourceRect;
}


// This is called when either the source or destination rectanges change so we
// can update the stretch flag. If the rectangles don't match we stretch the
// video during the drawing otherwise we call the fast pixel copy functions
// NOTE the source and/or the destination rectangle may be completely empty

void cActiveMovieDraw::SetStretchMode()
{
   // Calculate the overall rectangle dimensions

   LONG SourceWidth = m_SourceRect.right - m_SourceRect.left;
   LONG SinkWidth = m_TargetRect.right - m_TargetRect.left;
   LONG SourceHeight = m_SourceRect.bottom - m_SourceRect.top;
   LONG SinkHeight = m_TargetRect.bottom - m_TargetRect.top;

   m_bStretch = TRUE;
   if (SourceWidth == SinkWidth) {
      if (SourceHeight == SinkHeight) {
         m_bStretch = FALSE;
      }
   }
}

HRESULT cActiveMovieDraw::CheckVideoType(const VIDEOINFO *pInput)
{
    // First of all check the VIDEOINFO looks correct

    if (CheckHeaderValidity(pInput) == FALSE)
    {
        return E_INVALIDARG;
    }

    // we don't support palettised images yet
    if (gr_bitmap_type_count(&grd_canvas->bm) > pInput->bmiHeader.biBitCount)
    {
       return E_FAIL;
    }

    // Do the colour depths match

    if (gr_bitmap_type_count(&grd_canvas->bm) != pInput->bmiHeader.biBitCount)
    {
        return E_INVALIDARG;
    }

    // Check bit fields match

    const DWORD *pInputMask = GetBitMasks(pInput);
    IDisplayDevice *pDisplayDevice = AppGetObj(IDisplayDevice);
    sGrModeInfoEx ModeInfo;

    IDisplayDevice_GetMode(pDisplayDevice,&ModeInfo);
    if (pInputMask[iRED] != ModeInfo.redMask ||
        pInputMask[iGREEN] != ModeInfo.greenMask ||
        pInputMask[iBLUE] != ModeInfo.blueMask)
    {
       return E_INVALIDARG;
    }

    // Both input and display formats are either BI_RGB or BI_BITFIELDS

    Assert_((PALETTISED(pInput) == FALSE));

    return NOERROR;
}

const DWORD *cActiveMovieDraw::GetBitMasks(const VIDEOINFO *pVideoInfo)
{
    static DWORD FailMasks[] = {0,0,0};

    if (pVideoInfo->bmiHeader.biCompression == BI_BITFIELDS)
    {
        return pVideoInfo->dwBitMasks;
    }

    Assert_(pVideoInfo->bmiHeader.biCompression == BI_RGB);

    switch (pVideoInfo->bmiHeader.biBitCount)
    {
        case 16: return bits555;
        case 24: return bits888;
        case 32: return bits888;
        default: return FailMasks;
    }
}

STDMETHODIMP cActiveMovieDraw::CheckMediaType(const CMediaType *pmtIn)
{
   // Does this have a VIDEOINFO format block

   BEGIN_DEBUG_MSG("cActiveMovieDraw::CheckMediaType");

   const GUID *pFormatType = pmtIn->FormatType();
   if (*pFormatType != FORMAT_VideoInfo)
   {
      DebugMsg("Format GUID not a VIDEOINFO");
      return E_INVALIDARG;
   }

   // Check the format looks reasonably ok

   ULONG Length = pmtIn->FormatLength();
   if (Length < SIZE_VIDEOHEADER)
   {
      DebugMsg("Format smaller than a VIDEOHEADER");
      return E_FAIL;
   }

   VIDEOINFO *pInput = (VIDEOINFO *) pmtIn->Format();

    // Check the major type is MEDIATYPE_Video

   const GUID *pMajorType = pmtIn->Type();
   if (*pMajorType != MEDIATYPE_Video)
   {
      DebugMsg("Major type not MEDIATYPE_Video");
      return E_INVALIDARG;
   }

   // Check we can identify the media subtype

   const GUID *pSubType = pmtIn->Subtype();
   if (GetBitCount(pSubType) == USHRT_MAX)
   {
      DebugMsg("Invalid video media subtype");
      return E_INVALIDARG;
   }

   return CheckVideoType(pInput);
   END_DEBUG;
}

STDMETHODIMP cActiveMovieDraw::NotifyMediaType(CMediaType *pmt)
{
   m_pMediaType = pmt;
   return S_OK;
}

// this is now not being used
#pragma off(unreferenced)
STDMETHODIMP cActiveMovieDraw::GetMediaType (int iPosition, CMediaType *pmt)
{
   return VFW_S_NO_MORE_ITEMS;
}
#pragma on(unreferenced)

STDMETHODIMP cActiveMovieDraw::GetAllocator(IMemAllocator **ppAllocator)
{
   BEGIN_DEBUG_MSG("cActiveMovieDraw::GetAllocator");

   HRESULT phr;

   if (m_pActiveMovieAlloc == NULL)
       m_pActiveMovieAlloc = new cActiveMovieAlloc(this, "Active Movie Allocator", NULL, &phr);
   if (FAILED(m_pActiveMovieAlloc->QueryInterface(IID_IMemAllocator,
                                                 (void**)ppAllocator)))
   {
      DebugMsg("Can't get IMemAllocator interface");
      return S_FALSE;
   }

   return S_OK;
   END_DEBUG;
}

// Inform us to which allocator the output pin proposes to use
#pragma off(unreferenced)
STDMETHODIMP cActiveMovieDraw::NotifyAllocator(IMemAllocator *pAllocator, BOOL bReadOnly)
{
   return S_OK;
}
#pragma on(unreferenced)

BOOL cActiveMovieDraw::GetBitmapFromMedia(uchar *type, uint *flags, LONG *w, LONG *h, int *bitcount)
{
   if (m_pMediaType == NULL)
      return FALSE;
   VIDEOINFO *pVideoInfo = (VIDEOINFO *) m_pMediaType->Format();
   if (pVideoInfo == NULL)
      return FALSE;
   LPBITMAPINFOHEADER lpbi = HEADER(pVideoInfo);
   if (lpbi == NULL)
      return FALSE;
   *type = BitmapTypeFromBMIHeader(lpbi);
   *flags = BitmapFlagsFromBMIHeader(lpbi);
   *w = lpbi->biWidth;
   *h = lpbi->biHeight;
   *bitcount = (int)lpbi->biBitCount;
   return TRUE;
}

#ifdef AM_DDRAW
IDirectDraw *cActiveMovieDraw::GetDirectDraw()
{
   return m_pDirectDraw;
}
#endif

void cActiveMovieDraw::SetAllocator(cActiveMovieAlloc *pActiveMovieAlloc)
{
   m_pActiveMovieAlloc = pActiveMovieAlloc;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cActiveMovieAlloc
//

cActiveMovieAlloc::cActiveMovieAlloc(cActiveMovieDraw *pActiveMovieDraw,
                                     const char *pName, LPUNKNOWN pUnk, HRESULT *phr):
   cBaseAllocator(pName, pUnk, phr),
   m_bUsingCanvas(FALSE)
{
   Assert_(phr);
   Assert_(pActiveMovieDraw != NULL);
   m_pActiveMovieDraw = pActiveMovieDraw;
}

cActiveMovieAlloc::~cActiveMovieAlloc()
{
    Assert_(m_bCommitted == FALSE);
    if (m_pActiveMovieDraw != NULL)
       m_pActiveMovieDraw->SetAllocator(NULL);
}

STDMETHODIMP cActiveMovieAlloc::SetProperties(ALLOCATOR_PROPERTIES *pRequest, ALLOCATOR_PROPERTIES *pActual)
{
   BEGIN_DEBUG_MSG("cActiveMovieAlloc::SetProperties");

   HRESULT hr;

   if ((DWORD)pRequest->cbBuffer!=CanvasSize())
   {
      // well, the decompressor wouldn't go to the size we wanted
      // so we'll have to provide two buffers of the right size
      // and resize in the blit
      pRequest->cBuffers = min(pRequest->cBuffers, 2);
      m_bUsingCanvas = FALSE;
   }
   else
   {
      // we just have one buffer!
      pRequest->cBuffers = 1;
      m_bUsingCanvas = TRUE;
   }
   // reject buffer prefixes
   if (pRequest->cbPrefix>0)
      return E_INVALIDARG;
   hr = cBaseAllocator::SetProperties(pRequest, pActual);
   m_AllocatorProperties = *pActual;
   return hr;

   END_DEBUG;
}

void cActiveMovieAlloc::Free(void)
{
   Assert_(m_lAllocated == m_lFree.GetCount());
   cMediaSample *pSample;
   cBitmapSample *pBitmapSample;
   grs_bitmap *pBitmap;
#ifdef AM_DDRAW
   cDDSample *pDDSample;
   IDirectDrawSurface *pIDDSurface;
#endif

   while (m_lFree.GetCount() != 0)
   {
      pSample = m_lFree.RemoveHead();
#ifdef AM_DDRAW
      if (m_pActiveMovieDraw->GetDirectDraw() == NULL)
      {
#endif
         pBitmapSample = (cBitmapSample*) pSample;
         if ((pBitmap = pBitmapSample->GetBitmap()) != NULL)
            if (pBitmap != &(grd_canvas->bm))
               gr_free(pBitmap);
#ifdef AM_DDRAW
      }
      else
      {
         pDDSample = (cDDSample*) pSample;
         pIDDSurface = pDDSample->GetIDDSurface();
         pIDDSurface->Unlock((LPVOID)(pDDSample->GetDDSurfaceDesc()));
         SafeRelease(pIDDSurface);
      }
#endif
      delete pSample;
   }
   m_lAllocated = 0;
}

HRESULT cActiveMovieAlloc::AllocBitmapSample(grs_bitmap *pBitmap, LONG size)
{
    HRESULT hr;
    cBitmapSample *pSample;

    pSample = new cBitmapSample("Video sample",      // DEBUG name
                              (cBaseAllocator *) this,   // Base class
                              &hr,           // Return code
                              pBitmap,
                              size);
    if (pSample == NULL)
    {
       return E_OUTOFMEMORY;
    }

    // Add the completed sample to the available list
    // and decrement its reference count to zero
    // need to addref the allocator as releasebuffer decs its ref count
    //    AddRef();
    pSample->Release();
    m_lAllocated++;
    return hr;
}

#ifdef AM_DDRAW
HRESULT cActiveMovieAlloc::AllocDDSample(LPDIRECTDRAWSURFACE pDDSurface, LONG size)
{
    BEGIN_DEBUG_MSG("AllocDDSample");

    HRESULT hr;
    cDDSample *pSample;
    IDirectDrawSurface *pIDDSurface;
    RECT SourceRect;
    DDSURFACEDESC DDSurfaceDesc;

    if (FAILED(hr = pDDSurface->QueryInterface(IID_IDirectDrawSurface, (void**)&pIDDSurface)))
    {
       DebugMsg("cActiveMovieAlloc:AllocDDSample: can't get DDSurface interface");
       return E_UNEXPECTED;
    }
    m_pActiveMovieDraw->GetSourceRect(&SourceRect);
    memset(&DDSurfaceDesc, 0, sizeof(DDSurfaceDesc));
    DDSurfaceDesc.dwSize = sizeof(DDSurfaceDesc);
    if (FAILED(hr = pIDDSurface->Lock(&SourceRect, &DDSurfaceDesc, 0, NULL)))
    {
       DebugMsg("cActiveMovieAlloc:AllocDDSample: can't lock DDSurface");
       return E_UNEXPECTED;
    }

    pSample = new cDDSample("Video sample",      // DEBUG name
                            (cBaseAllocator *) this,   // Base class
                            &hr,           // Return code
                            pDDSurface,
                            pIDDSurface,
                            &DDSurfaceDesc,
                            size);
    if (pSample == NULL)
    {
       return E_OUTOFMEMORY;
    }

    // Add the completed sample to the available list
    // and decrement its reference count to zero
    // need to addref the allocator as releasebuffer decs its ref count
    //    AddRef();
    pSample->Release();
    m_lAllocated++;
    return hr;

    END_DEBUG;
}
#endif

// Are we using the canvas, or do we have seconday buffers?
BOOL cActiveMovieAlloc::UsingCanvas()
{
   return (m_AllocatorProperties.cbBuffer == 1);
}

HRESULT cActiveMovieAlloc::Alloc(void)
{
    BEGIN_DEBUG_MSG("cActiveMovieAlloc::Alloc");

    uchar type;
    uint flags;
    LONG w, h;
    int i;
    grs_bitmap *pBitmap;
    int bitcount;

    // Check the base allocator says it's ok to continue
    HRESULT hr = cBaseAllocator::Alloc();
    if (FAILED(hr))
    {
        return hr;
    }

    Assert_(m_lAllocated == 0);
    // Just one sample - the current canvas
    if (m_bUsingCanvas)
    {
       // Allocate the lone sample
       AllocBitmapSample((grs_bitmap*)&grd_canvas->bm, CanvasSize());
    }
    // allocate multiple samples - we need to blit them
    else if (m_lCount <= iBuffersMax)
    {
       if (!m_pActiveMovieDraw->GetBitmapFromMedia(&type, &flags, &w, &h, &bitcount))
          return E_UNEXPECTED;
       Assert_(w*h*(bitcount/8) == m_AllocatorProperties.cbBuffer);
       for (i=0; i<m_lCount; i++)
       {
#ifdef AM_DDRAW
          // we're not in direct draw mode, so use 2d bitmaps
          if ((pIDirectDraw = m_pActiveMovieDraw->GetDirectDraw()) == NULL)
          {
#endif
             pBitmap = gr_alloc_bitmap(type, flags, w, h);
             if (pBitmap != NULL)
                hr = AllocBitmapSample(pBitmap, m_AllocatorProperties.cbBuffer);
             else
                hr = E_OUTOFMEMORY;
#ifdef AM_DDRAW
          }
          else
          {
             DDSURFACEDESC DDSurfaceDesc;
             LPDIRECTDRAWSURFACE pDDSurface;
             IDirectDraw *pIDirectDraw;

             // we're in direct draw mode, so allocator DD surfaces
             memset(&DDSurfaceDesc, 0, sizeof(DDSurfaceDesc));
             DDSurfaceDesc.dwSize = sizeof(DDSurfaceDesc);
             DDSurfaceDesc.dwFlags = DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT |                      DDSD_CAPS;
             DDSurfaceDesc.dwHeight = h;
             DDSurfaceDesc.dwWidth = w;
             DDSurfaceDesc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
             DDSurfaceDesc.ddpfPixelFormat.dwRGBBitCount = 16;
             DDSurfaceDesc.ddpfPixelFormat.dwFlags = DDPF_RGB;
             DDSurfaceDesc.ddpfPixelFormat.dwRBitMask = RGB_RED_565;
             DDSurfaceDesc.ddpfPixelFormat.dwGBitMask = RGB_GREEN_565;
             DDSurfaceDesc.ddpfPixelFormat.dwBBitMask = RGB_BLUE_565;
             DDSurfaceDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
             if (SUCCEEDED(hr = pIDirectDraw->CreateSurface(&DDSurfaceDesc, &pDDSurface, NULL)))
                AllocDDSample(pDDSurface, m_AllocatorProperties.cbBuffer);
             else
                return E_OUTOFMEMORY;
          }
#endif
          if (FAILED(hr))
          {
             DebugMsg("cActiveMovieAlloc: can't alloc all buffers");
             return hr;
          }
       }
    }
    // hey! how did this happen?
    else
    {
       DebugMsg("cActiveMovieAlloc::Alloc:asked for too many buffers\n");
       return E_UNEXPECTED;
    }
    return NOERROR;

    END_DEBUG;
}

#else

#pragma off(unreferenced)
EXTERN
tResult LGAPI _MoviePlayerCreate(REFIID, void ** ppMoviePlayer, IUnknown * pOuterUnknown, REFCLSID)
    {
    if (ppMoviePlayer)
        ppMoviePlayer = NULL;
    return E_NOTIMPL;
    }
#pragma on(unreferenced)

#endif
