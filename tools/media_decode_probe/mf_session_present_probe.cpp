#include <win32_platform.h>
#include <evr.h>
#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>
#include <propvarutil.h>

#include <stdint.h>
#include <stdio.h>

#include "../../libsrc/actmovie/mfaudio.h"

#pragma comment(lib, "evr.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "ole32.lib")

static const GUID kVideoRenderService = {
    0x1092a86c, 0xab1a, 0x459a,
    {0xa3, 0x36, 0x83, 0x1f, 0xbc, 0x4d, 0x11, 0xff}};

template <class T> static void Release(T *&value) {
  if (value) {
    value->Release();
    value = nullptr;
  }
}

static HRESULT AddBranch(IMFTopology *topology, IMFMediaSource *source,
                         IMFPresentationDescriptor *presentation,
                         IMFStreamDescriptor *stream, HWND window) {
  IMFMediaTypeHandler *handler = nullptr;
  IMFActivate *renderer = nullptr;
  IMFTopologyNode *sourceNode = nullptr;
  IMFTopologyNode *outputNode = nullptr;
  GUID major = GUID_NULL;
  HRESULT hr = stream->GetMediaTypeHandler(&handler);
  if (SUCCEEDED(hr))
    hr = handler->GetMajorType(&major);
  if (SUCCEEDED(hr) && major == MFMediaType_Video)
    hr = MFCreateVideoRendererActivate(window, &renderer);
  else if (SUCCEEDED(hr) && major == MFMediaType_Audio)
    hr = MF_E_INVALIDMEDIATYPE;
  else if (SUCCEEDED(hr))
    hr = MF_E_INVALIDMEDIATYPE;
  if (SUCCEEDED(hr))
    hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &sourceNode);
  if (SUCCEEDED(hr))
    hr = sourceNode->SetUnknown(MF_TOPONODE_SOURCE, source);
  if (SUCCEEDED(hr))
    hr = sourceNode->SetUnknown(MF_TOPONODE_PRESENTATION_DESCRIPTOR,
                                presentation);
  if (SUCCEEDED(hr))
    hr = sourceNode->SetUnknown(MF_TOPONODE_STREAM_DESCRIPTOR, stream);
  if (SUCCEEDED(hr))
    hr = topology->AddNode(sourceNode);
  if (SUCCEEDED(hr))
    hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &outputNode);
  if (SUCCEEDED(hr))
    hr = outputNode->SetObject(renderer);
  if (SUCCEEDED(hr))
    hr = outputNode->SetUINT32(MF_TOPONODE_STREAMID, 0);
  if (SUCCEEDED(hr))
    hr = outputNode->SetUINT32(MF_TOPONODE_NOSHUTDOWN_ON_REMOVE, FALSE);
  if (SUCCEEDED(hr))
    hr = topology->AddNode(outputNode);
  if (SUCCEEDED(hr))
    hr = sourceNode->ConnectOutput(0, outputNode, 0);
  Release(outputNode);
  Release(sourceNode);
  Release(renderer);
  Release(handler);
  return hr;
}

static HRESULT BuildTopology(IMFMediaSource *source, HWND window,
                             IMFTopology **topology) {
  IMFPresentationDescriptor *presentation = nullptr;
  HRESULT hr = MFCreateTopology(topology);
  if (SUCCEEDED(hr))
    hr = source->CreatePresentationDescriptor(&presentation);
  DWORD count = 0;
  if (SUCCEEDED(hr))
    hr = presentation->GetStreamDescriptorCount(&count);
  for (DWORD index = 0; SUCCEEDED(hr) && index < count; ++index) {
    BOOL selected = FALSE;
    IMFStreamDescriptor *stream = nullptr;
    hr = presentation->GetStreamDescriptorByIndex(index, &selected, &stream);
    if (SUCCEEDED(hr) && selected) {
      hr = AddBranch(*topology, source, presentation, stream, window);
      if (hr == MF_E_INVALIDMEDIATYPE) {
        presentation->DeselectStream(index);
        hr = S_OK;
      }
    }
    Release(stream);
  }
  Release(presentation);
  return hr;
}

static HRESULT WaitForTopology(IMFMediaSession *session) {
  HRESULT hr = S_OK;
  while (SUCCEEDED(hr)) {
    IMFMediaEvent *event = nullptr;
    MediaEventType type = MEUnknown;
    hr = session->GetEvent(0, &event);
    if (SUCCEEDED(hr))
      hr = event->GetStatus(&hr);
    if (SUCCEEDED(hr))
      hr = event->GetType(&type);
    if (SUCCEEDED(hr) && type == MESessionTopologyStatus) {
      UINT32 status = 0;
      if (SUCCEEDED(event->GetUINT32(MF_EVENT_TOPOLOGY_STATUS, &status)) &&
          status == MF_TOPOSTATUS_READY) {
        Release(event);
        return S_OK;
      }
    }
    Release(event);
  }
  return hr;
}

static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam,
                                   LPARAM lparam) {
  if (message == WM_ERASEBKGND)
    return 1;
  return DefWindowProc(window, message, wparam, lparam);
}

static uint64_t PresentedFrameHash(IMFVideoDisplayControl *video,
                                   LONGLONG *timestamp) {
  BITMAPINFOHEADER header = {};
  header.biSize = sizeof(header);
  BYTE *pixels = nullptr;
  DWORD length = 0;
  LONGLONG sampleTime = 0;
  HRESULT hr = video->GetCurrentImage(&header, &pixels, &length, &sampleTime);
  uint64_t hash = 0;
  if (SUCCEEDED(hr) && pixels && length) {
    hash = 1469598103934665603ULL;
    const DWORD stride = length > 8192 ? length / 8192 : 1;
    for (DWORD offset = 0; offset < length; offset += stride) {
      hash ^= pixels[offset];
      hash *= 1099511628211ULL;
    }
    *timestamp = sampleTime;
  }
  CoTaskMemFree(pixels);
  return hash;
}

static void PumpMessages() {
  MSG message = {};
  while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE)) {
    TranslateMessage(&message);
    DispatchMessage(&message);
  }
}

int wmain(int argc, wchar_t **argv) {
  if (argc != 2) {
    fwprintf(stderr, L"usage: mf_session_present_probe <movie>\n");
    return 2;
  }
  HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  const bool uninitializeCom = SUCCEEDED(hr);
  if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
    return 1;
  hr = MFStartup(MF_VERSION);

  WNDCLASS windowClass = {};
  windowClass.lpfnWndProc = WindowProc;
  windowClass.hInstance = GetModuleHandle(nullptr);
  windowClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
  windowClass.lpszClassName = L"DarkEngineMFSessionPresentProbe";
  RegisterClass(&windowClass);
  HWND window = CreateWindowEx(
      WS_EX_TOPMOST, windowClass.lpszClassName, L"MF session presentation probe",
      WS_OVERLAPPEDWINDOW | WS_VISIBLE, 32, 32, 656, 519, nullptr, nullptr,
      windowClass.hInstance, nullptr);

  IMFSourceResolver *resolver = nullptr;
  IUnknown *sourceObject = nullptr;
  IMFMediaSource *source = nullptr;
  IMFTopology *topology = nullptr;
  IMFMediaSession *session = nullptr;
  IMFClock *clock = nullptr;
  IMFPresentationClock *presentationClock = nullptr;
  IMFVideoDisplayControl *video = nullptr;
  cMFMovieAudio movieAudio;
  MF_OBJECT_TYPE objectType = MF_OBJECT_INVALID;
  if (!window && SUCCEEDED(hr))
    hr = E_OUTOFMEMORY;
  if (SUCCEEDED(hr))
    hr = MFCreateSourceResolver(&resolver);
  if (SUCCEEDED(hr))
    hr = resolver->CreateObjectFromURL(argv[1], MF_RESOLUTION_MEDIASOURCE,
                                       nullptr, &objectType, &sourceObject);
  if (SUCCEEDED(hr))
    hr = sourceObject->QueryInterface(IID_PPV_ARGS(&source));
  if (SUCCEEDED(hr))
    hr = BuildTopology(source, window, &topology);
  if (SUCCEEDED(hr))
    hr = MFCreateMediaSession(nullptr, &session);
  if (SUCCEEDED(hr))
    hr = session->SetTopology(0, topology);
  if (SUCCEEDED(hr))
    hr = WaitForTopology(session);
  if (SUCCEEDED(hr))
    hr = MFGetService(session, kVideoRenderService, IID_PPV_ARGS(&video));
  if (SUCCEEDED(hr)) {
    RECT client = {};
    GetClientRect(window, &client);
    hr = video->SetVideoPosition(nullptr, &client);
  }
  if (SUCCEEDED(hr)) {
    movieAudio.Prepare(argv[1], 0);
    PROPVARIANT start;
    PropVariantInit(&start);
    hr = session->Start(&GUID_NULL, &start);
    PropVariantClear(&start);
    if (SUCCEEDED(hr))
      movieAudio.Play();
  }
  if (SUCCEEDED(hr))
    hr = session->GetClock(&clock);
  if (SUCCEEDED(hr))
    hr = clock->QueryInterface(IID_PPV_ARGS(&presentationClock));

  LARGE_INTEGER frequency = {}, started = {}, now = {};
  QueryPerformanceFrequency(&frequency);
  QueryPerformanceCounter(&started);
  MFTIME firstPosition = -1;
  MFTIME lastPosition = -1;
  LONGLONG firstFrameTime = -1;
  LONGLONG lastFrameTime = -1;
  uint64_t previousHash = 0;
  unsigned captures = 0;
  unsigned changedCaptures = 0;
  while (SUCCEEDED(hr)) {
    PumpMessages();
    for (;;) {
      IMFMediaEvent *event = nullptr;
      HRESULT eventResult = session->GetEvent(MF_EVENT_FLAG_NO_WAIT, &event);
      if (eventResult == MF_E_NO_EVENTS_AVAILABLE)
        break;
      if (FAILED(eventResult)) {
        hr = eventResult;
        break;
      }
      MediaEventType type = MEUnknown;
      HRESULT eventStatus = S_OK;
      event->GetType(&type);
      event->GetStatus(&eventStatus);
      if (FAILED(eventStatus))
        hr = eventStatus;
      Release(event);
    }
    if (FAILED(hr))
      break;
    QueryPerformanceCounter(&now);
    const double wallSeconds =
        double(now.QuadPart - started.QuadPart) / frequency.QuadPart;
    if (wallSeconds >= 10.0)
      break;
    MFTIME position = 0;
    if (SUCCEEDED(presentationClock->GetTime(&position))) {
      if (firstPosition < 0)
        firstPosition = position;
      lastPosition = position;
    }
    LONGLONG frameTime = 0;
    const uint64_t hash = PresentedFrameHash(video, &frameTime);
    if (hash) {
      if (firstFrameTime < 0)
        firstFrameTime = frameTime;
      lastFrameTime = frameTime;
    }
    ++captures;
    if (captures == 1 || (hash && hash != previousHash))
      ++changedCaptures;
    previousHash = hash;
    Sleep(50);
  }
  QueryPerformanceCounter(&now);
  const double wallSeconds =
      double(now.QuadPart - started.QuadPart) / frequency.QuadPart;
  const double clockSeconds =
      lastPosition > firstPosition ? (lastPosition - firstPosition) / 1e7 : 0.0;
  const double frameSeconds =
      lastFrameTime > firstFrameTime
          ? (lastFrameTime - firstFrameTime) / 1e7
          : 0.0;
  const double mediaSeconds = frameSeconds > 0.0 ? frameSeconds : clockSeconds;
  const double playbackRate = wallSeconds > 0 ? mediaSeconds / wallSeconds : 0.0;
  wprintf(L"movie: %ls\n", argv[1]);
  wprintf(L"clock: %.3f media seconds in %.3f wall seconds (%.2fx)\n",
          mediaSeconds, wallSeconds, playbackRate);
  wprintf(L"presentation: %u changed frames out of %u samples "
          L"(frame timestamps %.3fs)\n",
          changedCaptures, captures, frameSeconds);
  const bool passed = SUCCEEDED(hr) && playbackRate >= 0.8 &&
                      changedCaptures >= 100 && captures >= 160;
  if (passed)
    wprintf(L"PASS\n");
  else
    fwprintf(stderr,
             L"FAIL: expected >=0.8x media clock and >=100 changed screen "
             L"captures (hr=0x%08lx)\n",
             hr);

  movieAudio.Stop();
  if (session) {
    session->Stop();
    session->Close();
    session->Shutdown();
  }
  if (source)
    source->Shutdown();
  Release(video);
  Release(presentationClock);
  Release(clock);
  Release(session);
  Release(topology);
  Release(source);
  Release(sourceObject);
  Release(resolver);
  if (window)
    DestroyWindow(window);
  MFShutdown();
  if (uninitializeCom)
    CoUninitialize();
  return passed ? 0 : 1;
}
