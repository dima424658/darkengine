#include <win32_platform.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>
#include <mfplay.h>
#include <evr.h>
#include <shlobj.h>
#include <math.h>
#include <wctype.h>
#include <wchar.h>

#include <appagg.h>
#include <control.h>
#include <evcode.h>
#include <strmif.h>
#include <uuids.h>
#include <wappapi.h>

#include <movie_backend.h>
#include "mfaudio.h"
#include <render_backend.h>

// Implemented by the application's screen manager.  Keeping the declaration
// here avoids making the reusable movie library depend on src/render headers.
extern "C" void ScrnClear(void);

#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "evr.lib")
#pragma comment(lib, "mfplay.lib")

// Linking strmiids.lib would collide with the engine's legacy DirectShow GUID
// definitions, so keep the one EVR service identifier needed here local.
static const GUID kVideoRenderService = {
    0x1092a86c, 0xab1a, 0x459a,
    {0xa3, 0x36, 0x83, 0x1f, 0xbc, 0x4d, 0x11, 0xff}
};
static const GUID kVideoWindowInterface = {
    0x56a868b4, 0x0ad4, 0x11ce,
    {0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}
};

template <class T>
static void ReleaseInterface(T *&object)
{
    if (object)
    {
        object->Release();
        object = NULL;
    }
}

static BOOL FindModernMovie(const char *legacyPath, WCHAR *modernPath,
                            size_t modernPathCount)
{
    WCHAR *extension;
    if (!legacyPath ||
        !MultiByteToWideChar(CP_ACP, 0, legacyPath, -1, modernPath,
                             (int)modernPathCount))
        return FALSE;

    extension = wcsrchr(modernPath, L'.');
    if (!extension || extension + 5 > modernPath + modernPathCount)
        return FALSE;
    wcscpy(extension, L".mp4");
    return GetFileAttributesW(modernPath) != INVALID_FILE_ATTRIBUTES;
}

static HRESULT AddTopologyBranch(IMFTopology *topology, IMFMediaSource *source,
                                 IMFPresentationDescriptor *presentation,
                                 IMFStreamDescriptor *stream, HWND hwnd)
{
    IMFMediaTypeHandler *handler = NULL;
    IMFActivate *renderer = NULL;
    IMFTopologyNode *sourceNode = NULL;
    IMFTopologyNode *outputNode = NULL;
    GUID majorType = GUID_NULL;
    HRESULT result = stream->GetMediaTypeHandler(&handler);
    if (SUCCEEDED(result))
        result = handler->GetMajorType(&majorType);

    if (SUCCEEDED(result) && majorType == MFMediaType_Video)
        result = MFCreateVideoRendererActivate(hwnd, &renderer);
    else if (SUCCEEDED(result) && majorType == MFMediaType_Audio)
        // Audio is decoded separately to PCM.  Allowing the MF audio renderer
        // to own this session's presentation clock slows the stock 22.05 kHz
        // movie tracks to roughly one fifth speed on some modern devices.
        result = MF_E_INVALIDMEDIATYPE;
    else if (SUCCEEDED(result))
        result = MF_E_INVALIDMEDIATYPE;

    if (SUCCEEDED(result))
        result = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE,
                                      &sourceNode);
    if (SUCCEEDED(result))
        result = sourceNode->SetUnknown(MF_TOPONODE_SOURCE, source);
    if (SUCCEEDED(result))
        result = sourceNode->SetUnknown(MF_TOPONODE_PRESENTATION_DESCRIPTOR,
                                        presentation);
    if (SUCCEEDED(result))
        result = sourceNode->SetUnknown(MF_TOPONODE_STREAM_DESCRIPTOR, stream);
    if (SUCCEEDED(result))
        result = topology->AddNode(sourceNode);

    if (SUCCEEDED(result))
        result = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &outputNode);
    if (SUCCEEDED(result))
        result = outputNode->SetObject(renderer);
    if (SUCCEEDED(result))
        result = outputNode->SetUINT32(MF_TOPONODE_STREAMID, 0);
    if (SUCCEEDED(result))
        result = outputNode->SetUINT32(MF_TOPONODE_NOSHUTDOWN_ON_REMOVE, FALSE);
    if (SUCCEEDED(result))
        result = topology->AddNode(outputNode);
    if (SUCCEEDED(result))
        result = sourceNode->ConnectOutput(0, outputNode, 0);

    ReleaseInterface(outputNode);
    ReleaseInterface(sourceNode);
    ReleaseInterface(renderer);
    ReleaseInterface(handler);
    return result;
}

static HRESULT BuildTopology(IMFMediaSource *source, HWND hwnd,
                             IMFTopology **topology)
{
    IMFPresentationDescriptor *presentation = NULL;
    HRESULT result = MFCreateTopology(topology);
    if (SUCCEEDED(result))
        result = source->CreatePresentationDescriptor(&presentation);

    DWORD streamCount = 0;
    if (SUCCEEDED(result))
        result = presentation->GetStreamDescriptorCount(&streamCount);

    for (DWORD index = 0; SUCCEEDED(result) && index < streamCount; ++index)
    {
        BOOL selected = FALSE;
        IMFStreamDescriptor *stream = NULL;
        result = presentation->GetStreamDescriptorByIndex(index, &selected,
                                                           &stream);
        if (SUCCEEDED(result) && selected)
        {
            result = AddTopologyBranch(*topology, source, presentation,
                                       stream, hwnd);
            if (result == MF_E_INVALIDMEDIATYPE)
            {
                presentation->DeselectStream(index);
                result = S_OK;
            }
        }
        ReleaseInterface(stream);
    }

    ReleaseInterface(presentation);
    return result;
}

static const int g_movieSkipKeys[] = {
    VK_ESCAPE,
    VK_RETURN,
    VK_SPACE,
    VK_BACK,
    VK_LBUTTON,
    VK_RBUTTON,
};
static BOOL g_movieSkipArmed = FALSE;

static BOOL IsMovieSkipKey(WPARAM virtualKey)
{
    size_t index;
    for (index = 0;
         index < sizeof(g_movieSkipKeys) / sizeof(g_movieSkipKeys[0]);
         ++index)
    {
        if (virtualKey == (WPARAM)g_movieSkipKeys[index])
            return TRUE;
    }
    return FALSE;
}

static BOOL IsMovieSkipKeyDown()
{
    size_t index;
    for (index = 0;
         index < sizeof(g_movieSkipKeys) / sizeof(g_movieSkipKeys[0]);
         ++index)
    {
        if (GetAsyncKeyState(g_movieSkipKeys[index]) & 0x8000)
            return TRUE;
    }
    return FALSE;
}

static void ResetMovieSkipInput()
{
    size_t index;

    // The mouse/key event which selected Review Briefing is still observable
    // through GetAsyncKeyState when the synchronous player starts.  Require a
    // complete release before accepting another input as a request to skip.
    g_movieSkipArmed = FALSE;
    for (index = 0;
         index < sizeof(g_movieSkipKeys) / sizeof(g_movieSkipKeys[0]);
         ++index)
    {
        (void)GetAsyncKeyState(g_movieSkipKeys[index]);
    }
}

static BOOL PumpMovieMessages()
{
    MSG message;
    BOOL skipRequested = FALSE;
    while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE))
    {
        if (message.message == WM_QUIT)
        {
            PostQuitMessage((int)message.wParam);
            return FALSE;
        }
        if (g_movieSkipArmed &&
            (((message.message == WM_KEYDOWN ||
               message.message == WM_SYSKEYDOWN) &&
              IsMovieSkipKey(message.wParam)) ||
             message.message == WM_LBUTTONDOWN ||
             message.message == WM_RBUTTONDOWN))
        {
            skipRequested = TRUE;
        }
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    if (!g_movieSkipArmed)
    {
        if (!IsMovieSkipKeyDown())
            g_movieSkipArmed = TRUE;
        return TRUE;
    }

    return !skipRequested && !IsMovieSkipKeyDown();
}

enum ePreparedMovieResult
{
    kPreparedMovieUnavailable,
    kPreparedMovieReady,
    kPreparedMovieCanceled,
};

static ULONGLONG HashMovieIdentity(const WCHAR *path,
                                   const WIN32_FILE_ATTRIBUTE_DATA &attributes)
{
    const ULONGLONG fnvOffset = 14695981039346656037ULL;
    const ULONGLONG fnvPrime = 1099511628211ULL;
    ULONGLONG hash = fnvOffset;
    const WCHAR *cursor;
    ULARGE_INTEGER size;
    ULARGE_INTEGER writeTime;
    int byteIndex;

    for (cursor = path; *cursor; ++cursor)
    {
        WCHAR character = (WCHAR)towlower(*cursor);
        hash = (hash ^ (BYTE)(character & 0xff)) * fnvPrime;
        hash = (hash ^ (BYTE)(character >> 8)) * fnvPrime;
    }

    size.HighPart = attributes.nFileSizeHigh;
    size.LowPart = attributes.nFileSizeLow;
    writeTime.HighPart = attributes.ftLastWriteTime.dwHighDateTime;
    writeTime.LowPart = attributes.ftLastWriteTime.dwLowDateTime;
    for (byteIndex = 0; byteIndex < 8; ++byteIndex)
    {
        hash = (hash ^ (BYTE)(size.QuadPart >> (byteIndex * 8))) * fnvPrime;
        hash = (hash ^ (BYTE)(writeTime.QuadPart >> (byteIndex * 8))) * fnvPrime;
    }
    return hash;
}

static BOOL ExistingNonemptyFile(const WCHAR *path)
{
    WIN32_FILE_ATTRIBUTE_DATA attributes;
    ULARGE_INTEGER size;
    if (!GetFileAttributesExW(path, GetFileExInfoStandard, &attributes) ||
        (attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        return FALSE;
    size.HighPart = attributes.nFileSizeHigh;
    size.LowPart = attributes.nFileSizeLow;
    return size.QuadPart != 0;
}

static BOOL BuildPreparedMoviePath(const WCHAR *legacyPath,
                                   WCHAR *preparedPath,
                                   size_t preparedPathCount)
{
    WCHAR cacheDirectory[MAX_PATH];
    WCHAR *baseName;
    WCHAR safeName[MAX_PATH];
    WCHAR *extension;
    WIN32_FILE_ATTRIBUTE_DATA attributes;
    ULONGLONG identity;
    DWORD cacheLength;
    int createResult;
    size_t index;
    int written;

    if (!GetFileAttributesExW(legacyPath, GetFileExInfoStandard, &attributes))
        return FALSE;

    cacheLength = GetEnvironmentVariableW(L"DARK_MOVIE_CACHE",
                                          cacheDirectory, MAX_PATH);
    if (cacheLength >= MAX_PATH)
        return FALSE;
    if (!cacheLength)
    {
        DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", cacheDirectory,
                                               MAX_PATH);
        if (!length || length >= MAX_PATH ||
            length + wcslen(L"\\OpenDarkEngine\\MovieCache") >= MAX_PATH)
            return FALSE;
        wcscat(cacheDirectory, L"\\OpenDarkEngine\\MovieCache");
    }
    createResult = SHCreateDirectoryExW(NULL, cacheDirectory, NULL);
    if (createResult != ERROR_SUCCESS &&
        createResult != ERROR_ALREADY_EXISTS &&
        createResult != ERROR_FILE_EXISTS)
        return FALSE;

    baseName = wcsrchr((WCHAR *)legacyPath, L'\\');
    if (!baseName)
        baseName = wcsrchr((WCHAR *)legacyPath, L'/');
    baseName = baseName ? baseName + 1 : (WCHAR *)legacyPath;
    wcsncpy(safeName, baseName, MAX_PATH - 1);
    safeName[MAX_PATH - 1] = L'\0';
    extension = wcsrchr(safeName, L'.');
    if (extension)
        *extension = L'\0';
    for (index = 0; safeName[index]; ++index)
    {
        if (!iswalnum(safeName[index]) && safeName[index] != L'-' &&
            safeName[index] != L'_')
            safeName[index] = L'_';
    }

    identity = HashMovieIdentity(legacyPath, attributes);
    written = _snwprintf(preparedPath, preparedPathCount,
                         L"%ls\\%ls-%016I64x.mp4", cacheDirectory,
                         safeName, identity);
    if (written < 0 || (size_t)written >= preparedPathCount)
    {
        preparedPath[preparedPathCount - 1] = L'\0';
        return FALSE;
    }
    return TRUE;
}

static ePreparedMovieResult PrepareModernMovie(const WCHAR *legacyPath,
                                               WCHAR *preparedPath,
                                               size_t preparedPathCount)
{
    WCHAR ffmpegPath[MAX_PATH];
    WCHAR temporaryPath[MAX_PATH];
    WCHAR commandLine[4 * MAX_PATH];
    SECURITY_ATTRIBUTES security = {
        sizeof(SECURITY_ATTRIBUTES), NULL, TRUE
    };
    STARTUPINFOW startup;
    PROCESS_INFORMATION process;
    HANDLE nullInput = INVALID_HANDLE_VALUE;
    HANDLE nullOutput = INVALID_HANDLE_VALUE;
    DWORD exitCode = 1;
    BOOL canceled = FALSE;
    BOOL inheritHandles = FALSE;
    DWORD ffmpegLength;
    int written;

    if (!BuildPreparedMoviePath(legacyPath, preparedPath,
                                preparedPathCount))
        return kPreparedMovieUnavailable;
    if (ExistingNonemptyFile(preparedPath))
        return kPreparedMovieReady;
    ffmpegLength = SearchPathW(NULL, L"ffmpeg.exe", NULL, MAX_PATH,
                              ffmpegPath, NULL);
    if (!ffmpegLength || ffmpegLength >= MAX_PATH)
    {
        RenderBackendTrace("movie prepare unavailable: ffmpeg.exe not found");
        return kPreparedMovieUnavailable;
    }

    written = _snwprintf(temporaryPath, MAX_PATH,
                         L"%ls.%lu.preparing.mp4", preparedPath,
                         GetCurrentProcessId());
    if (written < 0 || written >= MAX_PATH)
        return kPreparedMovieUnavailable;
    DeleteFileW(temporaryPath);

    written = _snwprintf(
        commandLine, sizeof(commandLine) / sizeof(commandLine[0]),
        L"\"%ls\" -nostdin -v error -y -i \"%ls\" "
        L"-map 0:v:0 -map 0:a? -c:v libx264 -preset veryfast -crf 20 "
        L"-pix_fmt yuv420p -c:a aac -b:a 128k -movflags +faststart \"%ls\"",
        ffmpegPath, legacyPath, temporaryPath);
    if (written < 0 ||
        (size_t)written >= sizeof(commandLine) / sizeof(commandLine[0]))
        return kPreparedMovieUnavailable;

    ZeroMemory(&startup, sizeof(startup));
    ZeroMemory(&process, sizeof(process));
    startup.cb = sizeof(startup);
    nullInput = CreateFileW(L"NUL", GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE, &security,
                            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    nullOutput = CreateFileW(L"NUL", GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE, &security,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (nullInput != INVALID_HANDLE_VALUE &&
        nullOutput != INVALID_HANDLE_VALUE)
    {
        inheritHandles = TRUE;
        startup.dwFlags = STARTF_USESTDHANDLES;
        startup.hStdInput = nullInput;
        startup.hStdOutput = nullOutput;
        startup.hStdError = nullOutput;
    }

    RenderBackendTrace("movie prepare source=%ls cache=%ls", legacyPath,
                       preparedPath);
    if (!CreateProcessW(ffmpegPath, commandLine, NULL, NULL, inheritHandles,
                        CREATE_NO_WINDOW, NULL, NULL, &startup, &process))
    {
        RenderBackendTrace("movie prepare failed create-process=%lu",
                           GetLastError());
    }
    else
    {
        for (;;)
        {
            DWORD waitResult = WaitForSingleObject(process.hProcess, 50);
            if (waitResult == WAIT_OBJECT_0)
                break;
            if (waitResult == WAIT_FAILED)
            {
                TerminateProcess(process.hProcess, GetLastError());
                WaitForSingleObject(process.hProcess, INFINITE);
                break;
            }
            if (!PumpMovieMessages())
            {
                TerminateProcess(process.hProcess, ERROR_CANCELLED);
                WaitForSingleObject(process.hProcess, INFINITE);
                canceled = TRUE;
                break;
            }
        }
        GetExitCodeProcess(process.hProcess, &exitCode);
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
    }
    if (nullOutput != INVALID_HANDLE_VALUE)
        CloseHandle(nullOutput);
    if (nullInput != INVALID_HANDLE_VALUE)
        CloseHandle(nullInput);

    if (canceled)
    {
        DeleteFileW(temporaryPath);
        RenderBackendTrace("movie prepare canceled");
        return kPreparedMovieCanceled;
    }
    if (exitCode != 0 || !ExistingNonemptyFile(temporaryPath))
    {
        DeleteFileW(temporaryPath);
        RenderBackendTrace("movie prepare failed exit=%lu", exitCode);
        return kPreparedMovieUnavailable;
    }
    if (!MoveFileExW(temporaryPath, preparedPath,
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        DeleteFileW(temporaryPath);
        RenderBackendTrace("movie prepare failed move=%lu", GetLastError());
        return kPreparedMovieUnavailable;
    }
    RenderBackendTrace("movie prepare complete");
    return kPreparedMovieReady;
}

class cMFPlayCallback : public IMFPMediaPlayerCallback
{
public:
    cMFPlayCallback() : m_refs(1), m_done(CreateEvent(NULL, TRUE, FALSE, NULL)),
                        m_result(S_OK) {}
    ~cMFPlayCallback()
    {
        if (m_done)
            CloseHandle(m_done);
    }

    STDMETHODIMP QueryInterface(REFIID iid, void **object)
    {
        if (!object)
            return E_POINTER;
        *object = NULL;
        if (iid == IID_IUnknown || iid == __uuidof(IMFPMediaPlayerCallback))
        {
            *object = static_cast<IMFPMediaPlayerCallback *>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef()
    {
        return (ULONG)InterlockedIncrement(&m_refs);
    }

    STDMETHODIMP_(ULONG) Release()
    {
        LONG refs = InterlockedDecrement(&m_refs);
        if (!refs)
            delete this;
        return (ULONG)refs;
    }

    void STDMETHODCALLTYPE OnMediaPlayerEvent(MFP_EVENT_HEADER *eventHeader)
    {
        if (!eventHeader)
            return;
        if (FAILED(eventHeader->hrEvent) ||
            eventHeader->eEventType == MFP_EVENT_TYPE_ERROR)
        {
            m_result = FAILED(eventHeader->hrEvent) ? eventHeader->hrEvent :
                                                     E_FAIL;
            SetEvent(m_done);
        }
        else if (eventHeader->eEventType == MFP_EVENT_TYPE_PLAYBACK_ENDED)
        {
            m_result = S_OK;
            SetEvent(m_done);
        }
    }

    HANDLE DoneEvent() const { return m_done; }
    HRESULT Result() const { return m_result; }

private:
    LONG m_refs;
    HANDLE m_done;
    HRESULT m_result;
};

static const char kMovieHostClass[] = "DarkEngineMovieHost";

static LRESULT CALLBACK MovieHostWindowProc(HWND hwnd, UINT message,
                                            WPARAM wParam, LPARAM lParam)
{
    IMFPMediaPlayer *player =
        reinterpret_cast<IMFPMediaPlayer *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    if (message == WM_ERASEBKGND)
        return 1;

    if (message == WM_PAINT)
    {
        PAINTSTRUCT paint;
        BeginPaint(hwnd, &paint);
        FillRect(paint.hdc, &paint.rcPaint,
                 (HBRUSH)GetStockObject(BLACK_BRUSH));
        EndPaint(hwnd, &paint);
        if (player)
            player->UpdateVideo();
        return 0;
    }

    if (message == WM_SIZE && player)
    {
        player->UpdateVideo();
        return 0;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

static BOOL EnsureMovieHostWindowClass()
{
    static BOOL classRegistered = FALSE;

    if (!classRegistered)
    {
        WNDCLASSA windowClass;
        ZeroMemory(&windowClass, sizeof(windowClass));
        windowClass.lpfnWndProc = MovieHostWindowProc;
        windowClass.hInstance = GetModuleHandle(NULL);
        windowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        windowClass.lpszClassName = kMovieHostClass;
        if (!RegisterClassA(&windowClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            return FALSE;
        classRegistered = TRUE;
    }
    return TRUE;
}

static HWND CreateMovieHostWindow(HWND parent)
{
    RECT clientRect;
    POINT upperLeft = { 0, 0 };

    if (!EnsureMovieHostWindowClass())
        return NULL;

    if (!GetClientRect(parent, &clientRect) ||
        !ClientToScreen(parent, &upperLeft))
        return NULL;

    HWND host = CreateWindowExA(
        WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, kMovieHostClass, "",
        WS_POPUP,
        upperLeft.x, upperLeft.y,
        clientRect.right - clientRect.left,
        clientRect.bottom - clientRect.top,
        parent, NULL, GetModuleHandle(NULL), NULL);
    if (host)
    {
        SetWindowPos(host, HWND_TOP, upperLeft.x, upperLeft.y,
                     clientRect.right - clientRect.left,
                     clientRect.bottom - clientRect.top,
                     SWP_NOACTIVATE);
    }
    return host;
}

static HWND CreateMovieBlackCover(HWND parent)
{
    RECT clientRect;
    if (!EnsureMovieHostWindowClass() || !GetClientRect(parent, &clientRect))
        return NULL;

    HWND cover = CreateWindowExA(
        WS_EX_NOPARENTNOTIFY | WS_EX_NOACTIVATE, kMovieHostClass, "",
        WS_CHILD | WS_CLIPSIBLINGS,
        0, 0, clientRect.right - clientRect.left,
        clientRect.bottom - clientRect.top, parent, NULL,
        GetModuleHandle(NULL), NULL);
    if (cover)
    {
        SetWindowPos(cover, HWND_TOP, 0, 0,
                     clientRect.right - clientRect.left,
                     clientRect.bottom - clientRect.top,
                     SWP_NOACTIVATE | SWP_SHOWWINDOW);
        RedrawWindow(cover, NULL, NULL,
                     RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
    }
    return cover;
}

// MFPlay is Windows' high-level, clocked playback API.  It owns demuxing,
// decoding, audio synchronization, and EVR presentation; the game only pumps
// messages and watches for completion or a skip key.
static BOOL PlayMFPlayMovie(const WCHAR *moviePath, HWND hwnd, int volume)
{
    IMFPMediaPlayer *player = NULL;
    cMFPlayCallback *callback = new cMFPlayCallback;
    HRESULT result = MFStartup(MF_VERSION);
    BOOL mediaFoundationStarted = SUCCEEDED(result);
    BOOL played = FALSE;

    if (!mediaFoundationStarted || !callback || !callback->DoneEvent())
    {
        if (callback)
            callback->Release();
        if (mediaFoundationStarted)
            MFShutdown();
        return FALSE;
    }

    result = MFPCreateMediaPlayer(moviePath, TRUE, MFP_OPTION_NONE,
                                  callback, hwnd, &player);
    if (SUCCEEDED(result))
    {
        float scalar = volume <= -10000 ? 0.0f :
                       (float)pow(10.0, (double)volume / 2000.0);
        if (scalar > 1.0f)
            scalar = 1.0f;
        player->SetVolume(scalar);
        player->SetBorderColor(RGB(0, 0, 0));
        SetWindowLongPtr(hwnd, GWLP_USERDATA,
                         reinterpret_cast<LONG_PTR>(player));
        ShowWindow(hwnd, SW_SHOWNA);
        UpdateWindow(hwnd);
        player->UpdateVideo();

        for (;;)
        {
            DWORD waitResult = WaitForSingleObject(callback->DoneEvent(), 10);
            if (waitResult == WAIT_OBJECT_0)
            {
                result = callback->Result();
                played = SUCCEEDED(result);
                break;
            }
            if (waitResult == WAIT_FAILED)
            {
                result = HRESULT_FROM_WIN32(GetLastError());
                break;
            }
            if (!PumpMovieMessages())
            {
                player->Stop();
                result = S_OK;
                played = TRUE;
                break;
            }
        }
    }

    if (player)
    {
        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
        player->Shutdown();
        player->Release();
    }
    callback->Release();
    MFShutdown();
    return played && SUCCEEDED(result);
}

// The retail AVIs already build a valid DirectShow graph on supported Thief 2
// installations.  Let that graph keep its normal video renderer.  The legacy
// player used to remove it and insert the LG renderer, forcing every decoded
// frame back through Dark's 16-bit canvas before the D3D11 presenter uploaded
// it again.  A child video window gives DirectShow ownership of presentation
// and clocking while preserving the game's normal input-to-skip behavior.
static BOOL PlayDirectShowMovie(const WCHAR *moviePath, HWND hwnd, int volume)
{
    IGraphBuilder *graph = NULL;
    IMediaControl *control = NULL;
    IMediaEvent *events = NULL;
    IVideoWindow *video = NULL;
    IBasicAudio *audio = NULL;
    OAEVENT eventHandle = 0;
    RECT clientRect;
    HRESULT result;
    BOOL played = FALSE;
    BOOL running = TRUE;

    result = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
                              IID_IGraphBuilder, (void **)&graph);
    if (SUCCEEDED(result))
        result = graph->RenderFile(moviePath, NULL);
    if (SUCCEEDED(result))
        result = graph->QueryInterface(IID_IMediaControl, (void **)&control);
    if (SUCCEEDED(result))
        result = graph->QueryInterface(IID_IMediaEvent, (void **)&events);
    if (SUCCEEDED(result))
        result = graph->QueryInterface(kVideoWindowInterface,
                                       (void **)&video);
    if (SUCCEEDED(result))
        graph->QueryInterface(IID_IBasicAudio, (void **)&audio);
    if (SUCCEEDED(result))
        result = events->GetEventHandle(&eventHandle);
    if (SUCCEEDED(result))
        result = video->put_Owner((OAHWND)hwnd);
    if (SUCCEEDED(result))
        result = video->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS);
    if (SUCCEEDED(result))
    {
        GetClientRect(hwnd, &clientRect);
        result = video->SetWindowPosition(0, 0, clientRect.right,
                                          clientRect.bottom);
    }
    if (SUCCEEDED(result))
        result = video->put_MessageDrain((OAHWND)hwnd);
    if (SUCCEEDED(result))
        result = video->put_Visible(-1L);
    if (SUCCEEDED(result) && audio)
        audio->put_Volume(volume);
    if (SUCCEEDED(result))
    {
        ShowWindow(hwnd, SW_SHOWNA);
        UpdateWindow(hwnd);
    }
    if (SUCCEEDED(result))
        result = control->Run();

    while (SUCCEEDED(result) && running)
    {
        HANDLE handle = (HANDLE)eventHandle;
        DWORD waitResult = MsgWaitForMultipleObjects(
            1, &handle, FALSE, 50, QS_ALLINPUT);
        if (waitResult == WAIT_OBJECT_0)
        {
            long eventCode, param1, param2;
            while (events->GetEvent(&eventCode, &param1, &param2, 0) == S_OK)
            {
                if (eventCode == EC_COMPLETE)
                {
                    played = TRUE;
                    running = FALSE;
                }
                else if (eventCode == EC_USERABORT ||
                         eventCode == EC_ERRORABORT)
                {
                    if (eventCode == EC_USERABORT)
                        played = TRUE;
                    running = FALSE;
                    if (eventCode == EC_ERRORABORT)
                        result = E_FAIL;
                }
                events->FreeEventParams(eventCode, param1, param2);
            }
        }
        else if (waitResult == WAIT_OBJECT_0 + 1)
        {
            if (!PumpMovieMessages())
            {
                played = TRUE;
                running = FALSE;
            }
        }
        else if (waitResult == WAIT_FAILED)
        {
            result = HRESULT_FROM_WIN32(GetLastError());
            running = FALSE;
        }
    }

    if (control)
        control->Stop();
    if (video)
    {
        video->put_Visible(0L);
        video->put_MessageDrain((OAHWND)NULL);
        video->put_Owner((OAHWND)NULL);
    }
    ReleaseInterface(audio);
    ReleaseInterface(video);
    ReleaseInterface(events);
    ReleaseInterface(control);
    ReleaseInterface(graph);
    return played && SUCCEEDED(result);
}

static HRESULT WaitForTopology(IMFMediaSession *session)
{
    HRESULT result = S_OK;
    while (SUCCEEDED(result))
    {
        IMFMediaEvent *event = NULL;
        MediaEventType type = MEUnknown;
        result = session->GetEvent(0, &event);
        if (SUCCEEDED(result))
        {
            HRESULT eventStatus = S_OK;
            result = event->GetStatus(&eventStatus);
            if (SUCCEEDED(result))
                result = eventStatus;
        }
        if (SUCCEEDED(result))
            result = event->GetType(&type);
        if (SUCCEEDED(result) && type == MESessionTopologyStatus)
        {
            UINT32 status = 0;
            if (SUCCEEDED(event->GetUINT32(MF_EVENT_TOPOLOGY_STATUS, &status)) &&
                status == MF_TOPOSTATUS_READY)
            {
                ReleaseInterface(event);
                return S_OK;
            }
        }
        ReleaseInterface(event);
    }
    return result;
}

static HRESULT RunSession(IMFMediaSession *session, HWND hwnd, HWND blackCover,
                          cMFMovieAudio *movieAudio)
{
    IMFVideoDisplayControl *video = NULL;
    PROPVARIANT startPosition;
    RECT clientRect;
    HRESULT result;

    result = WaitForTopology(session);
    if (FAILED(result))
        return result;

    result = MFGetService(session, kVideoRenderService,
                          IID_PPV_ARGS(&video));
    if (FAILED(result))
        return result;
    result = video->SetBorderColor(RGB(0, 0, 0));
    if (FAILED(result))
    {
        ReleaseInterface(video);
        return result;
    }
    GetClientRect(hwnd, &clientRect);
    result = video->SetVideoPosition(NULL, &clientRect);
    if (FAILED(result))
    {
        ReleaseInterface(video);
        return result;
    }
    PropVariantInit(&startPosition);
    result = session->Start(&GUID_NULL, &startPosition);
    PropVariantClear(&startPosition);
    if (FAILED(result))
    {
        ReleaseInterface(video);
        return result;
    }

    // Keep the host hidden until EVR has decoded its first frame.  Merely
    // waiting for the topology is not enough: the empty renderer window can
    // otherwise be painted with the system background color for one refresh.
    DWORD firstFrameStart = GetTickCount();
    while (GetTickCount() - firstFrameStart < 300)
    {
        BITMAPINFOHEADER header = {};
        BYTE *dib = NULL;
        DWORD dibSize = 0;
        LONGLONG timestamp = 0;
        HRESULT frameResult = video->GetCurrentImage(
            &header, &dib, &dibSize, &timestamp);
        if (dib)
            CoTaskMemFree(dib);
        if (SUCCEEDED(frameResult) && dibSize != 0)
            break;
        if (!PumpMovieMessages())
        {
            session->Stop();
            ReleaseInterface(video);
            return S_OK;
        }
        Sleep(5);
    }

    // EVR can expose its newly visible swap surface before its first decoded
    // frame has reached the compositor. Keep an independent black child above
    // it while revealing the video HWND.  Showing the EVR child first and
    // raising the cover afterward leaves a one-refresh gap containing the
    // renderer's default grey surface.
    if (blackCover)
    {
        RECT coverRect;
        GetClientRect(GetParent(hwnd), &coverRect);
        SetWindowPos(blackCover, HWND_TOP, 0, 0, coverRect.right,
                     coverRect.bottom, SWP_NOACTIVATE | SWP_SHOWWINDOW);
        UpdateWindow(blackCover);
        SetWindowPos(hwnd, blackCover, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE |
                     SWP_SHOWWINDOW);
        UpdateWindow(hwnd);
        DWORD coverStart = GetTickCount();
        while (GetTickCount() - coverStart < 100)
        {
            if (!PumpMovieMessages())
            {
                session->Stop();
                ShowWindow(blackCover, SW_HIDE);
                ReleaseInterface(video);
                return S_OK;
            }
            Sleep(5);
        }
        ShowWindow(blackCover, SW_HIDE);
    }
    else
    {
        ShowWindow(hwnd, SW_SHOWNA);
        UpdateWindow(hwnd);
    }
    if (movieAudio)
        movieAudio->Play();

    BOOL playing = TRUE;
    while (playing)
    {
        IMFMediaEvent *event = NULL;
        MediaEventType type = MEUnknown;
        result = session->GetEvent(MF_EVENT_FLAG_NO_WAIT, &event);
        if (result == MF_E_NO_EVENTS_AVAILABLE)
        {
            if (!PumpMovieMessages())
            {
                session->Stop();
                result = S_OK;
                break;
            }
            Sleep(10);
            continue;
        }
        if (FAILED(result))
            break;

        HRESULT eventStatus = S_OK;
        result = event->GetStatus(&eventStatus);
        if (SUCCEEDED(result))
            result = eventStatus;
        if (SUCCEEDED(result))
            result = event->GetType(&type);
        if (type == MESessionEnded || type == MESessionStopped ||
            type == MEError)
            playing = FALSE;
        ReleaseInterface(event);
    }
    ReleaseInterface(video);
    return result;
}

static BOOL PlayMediaFoundationMovie(const WCHAR *moviePath, int volume)
{
    IMFSourceResolver *resolver = NULL;
    IUnknown *sourceObject = NULL;
    IMFMediaSource *source = NULL;
    IMFTopology *topology = NULL;
    IMFMediaSession *session = NULL;
    MF_OBJECT_TYPE objectType = MF_OBJECT_INVALID;
    HRESULT result;
    BOOL mediaFoundationStarted = FALSE;
    BOOL played = FALSE;
    BOOL audioPrepared = FALSE;
    cMFMovieAudio movieAudio;
    HWND parentWindow;
    HWND videoWindow = NULL;
    HWND blackCover = NULL;
    RECT clientRect;

    AutoAppIPtr(WinApp);
    if (!pWinApp)
        return FALSE;

    parentWindow = pWinApp->GetMainWnd();
    GetClientRect(parentWindow, &clientRect);
    if (!EnsureMovieHostWindowClass())
        return FALSE;
    videoWindow = CreateWindowExA(
        WS_EX_NOPARENTNOTIFY | WS_EX_NOACTIVATE, kMovieHostClass, "",
        WS_CHILD | WS_CLIPSIBLINGS,
        0, 0, clientRect.right - clientRect.left,
        clientRect.bottom - clientRect.top, parentWindow, NULL,
        GetModuleHandle(NULL), NULL);
    if (!videoWindow)
        return FALSE;
    SetWindowPos(videoWindow, HWND_TOP, 0, 0,
                 clientRect.right - clientRect.left,
                 clientRect.bottom - clientRect.top,
                 SWP_NOACTIVATE | SWP_NOREDRAW);
    blackCover = CreateMovieBlackCover(parentWindow);

    result = MFStartup(MF_VERSION);
    if (SUCCEEDED(result))
    {
        mediaFoundationStarted = TRUE;
        result = MFCreateSourceResolver(&resolver);
    }
    if (SUCCEEDED(result))
        result = resolver->CreateObjectFromURL(
            moviePath, MF_RESOLUTION_MEDIASOURCE, NULL, &objectType,
            &sourceObject);
    if (SUCCEEDED(result))
        result = sourceObject->QueryInterface(IID_PPV_ARGS(&source));
    if (SUCCEEDED(result))
        result = BuildTopology(source, videoWindow, &topology);
    if (SUCCEEDED(result))
        result = MFCreateMediaSession(NULL, &session);
    if (SUCCEEDED(result))
        result = session->SetTopology(0, topology);
    if (SUCCEEDED(result))
    {
        audioPrepared = movieAudio.Prepare(moviePath, volume);
        result = RunSession(session, videoWindow, blackCover,
                            audioPrepared ? &movieAudio : NULL);
        played = SUCCEEDED(result);
    }

    // Conceal the EVR child before shutting down its renderer so its final
    // surface cannot disappear visibly during teardown.
    ShowWindow(videoWindow, SW_HIDE);
    movieAudio.Stop();

    if (session)
    {
        session->Close();
        session->Shutdown();
    }
    if (source)
        source->Shutdown();
    ReleaseInterface(session);
    ReleaseInterface(topology);
    ReleaseInterface(source);
    ReleaseInterface(sourceObject);
    ReleaseInterface(resolver);
    if (mediaFoundationStarted)
        MFShutdown();
    DestroyWindow(videoWindow);
    if (blackCover)
        DestroyWindow(blackCover);
    InvalidateRect(parentWindow, NULL, FALSE);
    return played;
}

static BOOL PlayModernMovie(const WCHAR *moviePath, int volume)
{
    BOOL played = PlayMediaFoundationMovie(moviePath, volume);
    HWND movieHost = NULL;
    AutoAppIPtr(WinApp);

    RenderBackendTrace("movie backend=media-session played=%d", played);
    if (!played)
    {
        movieHost = CreateMovieHostWindow(pWinApp->GetMainWnd());
        if (!movieHost)
            return FALSE;
        played = PlayMFPlayMovie(moviePath, movieHost, volume);
        RenderBackendTrace("movie backend=mfplay played=%d", played);
    }
    if (!played)
    {
        played = PlayDirectShowMovie(moviePath, movieHost, volume);
        RenderBackendTrace("movie backend=directshow-mp4 played=%d", played);
    }
    if (movieHost)
        DestroyWindow(movieHost);
    InvalidateRect(pWinApp->GetMainWnd(), NULL, FALSE);
    return played;
}

bool MovieBackendPlaySynchronous(const char *legacyPath, int volume)
{
    WCHAR legacyMoviePath[MAX_PATH];
    WCHAR moviePath[MAX_PATH];
    HWND movieHost = NULL;
    BOOL played;
    ePreparedMovieResult prepared;
    AutoAppIPtr(WinApp);

    if (!pWinApp || !legacyPath)
        return FALSE;

    ResetMovieSkipInput();

    // Commit an explicit black frame before decoder/topology setup without
    // replacing palette slot zero.  The suspended menu must be able to redraw
    // with its existing indexed palette when the movie mode exits.
    ScrnClear();

    RenderBackendTrace("movie requested=%s", legacyPath);

    // Prefer a modern sibling when one is installed.  MFPlay owns the native
    // MP4/H.264 clock, decoder, and EVR presentation without copying frames
    // through Dark's legacy canvas.  Keep the other system renderers as
    // compatibility fallbacks for Windows installations missing MFPlay parts.
    if (FindModernMovie(legacyPath, moviePath,
                        sizeof(moviePath) / sizeof(moviePath[0])))
    {
        // Use a video-only media session plus independently clocked PCM audio.
        // MFPlay and a combined audio/video MF session both inherit the broken
        // audio-renderer clock on affected systems and visibly run at ~1 fps.
        return PlayModernMovie(moviePath, volume);
    }
    if (!MultiByteToWideChar(CP_ACP, 0, legacyPath, -1, legacyMoviePath,
                             sizeof(legacyMoviePath) /
                                 sizeof(legacyMoviePath[0])))
    {
        return FALSE;
    }
    movieHost = CreateMovieHostWindow(pWinApp->GetMainWnd());
    if (!movieHost)
        return FALSE;
    played = PlayDirectShowMovie(legacyMoviePath, movieHost, volume);
    RenderBackendTrace("movie backend=directshow-legacy played=%d", played);
    DestroyWindow(movieHost);
    InvalidateRect(pWinApp->GetMainWnd(), NULL, FALSE);
    if (played)
        return TRUE;

    // Stock Thief movies use Indeo 5, which current 32- and 64-bit Windows
    // installations normally cannot decode.  If ffmpeg is available, prepare
    // an H.264/AAC cache entry outside the retail installation and use the
    // same Media Foundation path as an installed MP4 sibling.  The cache key
    // includes the source path, size, and timestamp, so FM movies with the
    // same basename cannot collide and changed files are rebuilt.
    prepared = PrepareModernMovie(legacyMoviePath, moviePath,
                                  sizeof(moviePath) / sizeof(moviePath[0]));
    if (prepared == kPreparedMovieCanceled)
        return TRUE;
    if (prepared == kPreparedMovieReady)
        return PlayModernMovie(moviePath, volume);
    return FALSE;
}
