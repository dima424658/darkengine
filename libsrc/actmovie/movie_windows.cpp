#define SDL_ENABLE_OLD_NAMES
#include <SDL3/SDL.h>

#include <windows.h>
#include <mfapi.h>
#include <mfplay.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <string>

#include <aggmemb.h>
#include <movieapi.h>
#include <display_window.h>
#include <platform_services.h>
#include <sdl_events.h>

namespace
{
std::atomic_bool g_moviePlaying{false};

class cMoviePlaybackScope
{
public:
    cMoviePlaybackScope() { g_moviePlaying.store(true); }
    ~cMoviePlaybackScope() { g_moviePlaying.store(false); }
};

template <class T>
void ReleaseInterface(T *&object)
{
    if (object)
    {
        object->Release();
        object = nullptr;
    }
}

bool WidePath(const std::string &path, std::wstring &wide)
{
    UINT codePage = CP_UTF8;
    DWORD flags = MB_ERR_INVALID_CHARS;
    int count = MultiByteToWideChar(codePage, flags, path.c_str(), -1,
        nullptr, 0);
    if (!count)
    {
        codePage = CP_ACP;
        flags = 0;
        count = MultiByteToWideChar(codePage, flags, path.c_str(), -1,
            nullptr, 0);
    }
    if (!count)
        return false;
    wide.resize(static_cast<size_t>(count));
    if (!MultiByteToWideChar(codePage, flags, path.c_str(), -1,
            wide.data(), count))
        return false;
    wide.pop_back();
    return true;
}

bool ResolveMoviePath(const char *legacyPath, std::wstring &path)
{
    if (!legacyPath || !*legacyPath)
        return false;
    char native[4096];
    std::string candidate = legacyPath;
    const size_t dot = candidate.find_last_of('.');
    if (dot != std::string::npos)
    {
        candidate.resize(dot);
        candidate += ".mp4";
    }
    if (!PlatformResolvePath(candidate.c_str(), native, sizeof(native)) &&
        !PlatformResolvePath(legacyPath, native, sizeof(native)))
        return false;
    return WidePath(native, path);
}

class cMFPlayCallback final : public IMFPMediaPlayerCallback
{
public:
    cMFPlayCallback() : m_done(CreateEventW(nullptr, TRUE, FALSE, nullptr)) {}
    ~cMFPlayCallback()
    {
        if (m_done)
            CloseHandle(m_done);
    }

    STDMETHODIMP QueryInterface(REFIID iid, void **object) override
    {
        if (!object)
            return E_POINTER;
        *object = nullptr;
        if (iid != IID_IUnknown && iid != __uuidof(IMFPMediaPlayerCallback))
            return E_NOINTERFACE;
        *object = static_cast<IMFPMediaPlayerCallback *>(this);
        AddRef();
        return S_OK;
    }
    STDMETHODIMP_(ULONG) AddRef() override { return ++m_refs; }
    STDMETHODIMP_(ULONG) Release() override
    {
        const ULONG refs = --m_refs;
        if (!refs)
            delete this;
        return refs;
    }
    void STDMETHODCALLTYPE OnMediaPlayerEvent(MFP_EVENT_HEADER *event) override
    {
        if (!event)
            return;
        if (FAILED(event->hrEvent) || event->eEventType == MFP_EVENT_TYPE_ERROR)
        {
            m_result.store(FAILED(event->hrEvent) ? event->hrEvent : E_FAIL);
            SetEvent(m_done);
        }
        else if (event->eEventType == MFP_EVENT_TYPE_PLAYBACK_ENDED)
            SetEvent(m_done);
    }

    HANDLE DoneEvent() const { return m_done; }
    HRESULT Result() const { return m_result.load(); }

private:
    std::atomic_ulong m_refs{1};
    std::atomic<HRESULT> m_result{S_OK};
    HANDLE m_done{};
};

constexpr wchar_t kMovieWindowClass[] = L"DarkEngineMovieWindow";

LRESULT CALLBACK MovieWindowProc(HWND window, UINT message,
    WPARAM wParam, LPARAM lParam)
{
    auto *player = reinterpret_cast<IMFPMediaPlayer *>(
        GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_ERASEBKGND)
        return 1;
    if (message == WM_PAINT)
    {
        PAINTSTRUCT paint{};
        BeginPaint(window, &paint);
        FillRect(paint.hdc, &paint.rcPaint,
            static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
        EndPaint(window, &paint);
        if (player)
            player->UpdateVideo();
        return 0;
    }
    if (message == WM_SIZE && player)
    {
        player->UpdateVideo();
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

HWND CreateMovieWindow()
{
    SDL_Window *sdlWindow = SDLDisplayGetWindow();
    if (!sdlWindow)
        return nullptr;
    HWND parent = static_cast<HWND>(SDL_GetPointerProperty(
        SDL_GetWindowProperties(sdlWindow),
        SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
    if (!parent)
        return nullptr;

    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = MovieWindowProc;
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.hbrBackground =
        static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    windowClass.lpszClassName = kMovieWindowClass;
    if (!RegisterClassW(&windowClass) &&
        GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        return nullptr;

    RECT client{};
    GetClientRect(parent, &client);
    return CreateWindowExW(WS_EX_NOPARENTNOTIFY, kMovieWindowClass, L"",
        WS_CHILD | WS_CLIPSIBLINGS, 0, 0, client.right, client.bottom,
        parent, nullptr, windowClass.hInstance, nullptr);
}
}

extern "C" bool DarkMoviePlaybackActive(void)
{
    return g_moviePlaying.load();
}

class cWindowsMoviePlayer final : public IMoviePlayer1
{
    DECLARE_SIMPLE_AGGREGATION(cWindowsMoviePlayer);

public:
    explicit cWindowsMoviePlayer(IUnknown *outer)
    {
        INIT_AGGREGATION_1(outer, IID_IMoviePlayer1, this,
            kPriorityLibrary, nullptr);
    }
    ~cWindowsMoviePlayer() { Close(); }

    BOOL STDMETHODCALLTYPE Open(const char *name, int, int, int, int) override
    {
        Close();
        if (!ResolveMoviePath(name, m_path))
            return FALSE;
        m_state = kMP1Ready;
        return TRUE;
    }
    void STDMETHODCALLTYPE Close() override
    {
        m_stopRequested.store(true);
        m_path.clear();
        m_state = kMP1NothingOpen;
    }
    BOOL STDMETHODCALLTYPE CanPlay() override { return m_state == kMP1Ready; }
    BOOL STDMETHODCALLTYPE Pause() override { return FALSE; }
    BOOL STDMETHODCALLTYPE Stop() override
    {
        m_stopRequested.store(true);
        return TRUE;
    }
    BOOL STDMETHODCALLTYPE GetVolume(long *volume) override
    {
        if (!volume)
            return FALSE;
        *volume = m_volume;
        return TRUE;
    }
    BOOL STDMETHODCALLTYPE SetVolume(long volume) override
    {
        m_volume = std::clamp(volume, -10000L, 0L);
        return TRUE;
    }
    eMP1State STDMETHODCALLTYPE GetState() override { return m_state; }
    void STDMETHODCALLTYPE SetTermKeys(char *keys) override
    {
        m_termKeys = keys ? keys : "";
        m_useTermKeys = keys != nullptr;
    }
    BOOL STDMETHODCALLTYPE Play(int flags) override;

private:
    std::wstring m_path;
    std::string m_termKeys;
    std::atomic_bool m_stopRequested{false};
    eMP1State m_state{kMP1NothingOpen};
    long m_volume{};
    bool m_useTermKeys{};
};

IMPLEMENT_SIMPLE_AGGREGATION_SELF_DELETE(cWindowsMoviePlayer);

extern "C" tResult LGAPI _MoviePlayerCreate(REFIID, void **player,
    IUnknown *outer, REFCLSID, unsigned)
{
    if (!outer)
        return E_INVALIDARG;
    auto *created = new cWindowsMoviePlayer(outer);
    if (player)
        *player = created;
    return created ? S_OK : E_FAIL;
}

BOOL STDMETHODCALLTYPE cWindowsMoviePlayer::Play(int flags)
{
    if (m_state != kMP1Ready || !(flags & kMoviePlayBlock))
        return FALSE;

    const HRESULT comResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool uninitializeCom = SUCCEEDED(comResult);
    if (FAILED(comResult) && comResult != RPC_E_CHANGED_MODE)
        return FALSE;
    if (FAILED(MFStartup(MF_VERSION)))
    {
        if (uninitializeCom)
            CoUninitialize();
        return FALSE;
    }

    HWND window = CreateMovieWindow();
    auto *callback = new cMFPlayCallback;
    IMFPMediaPlayer *player = nullptr;
    HRESULT result = window && callback->DoneEvent() ?
        MFPCreateMediaPlayer(m_path.c_str(), TRUE, MFP_OPTION_NONE,
            callback, window, &player) : E_FAIL;
    bool played = false;
    if (SUCCEEDED(result))
    {
        const float volume = m_volume <= -10000 ? 0.0f :
            (std::min)(1.0f, static_cast<float>(
                std::pow(10.0, static_cast<double>(m_volume) / 2000.0)));
        player->SetVolume(volume);
        player->SetBorderColor(RGB(0, 0, 0));
        SetWindowLongPtrW(window, GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(player));
        ShowWindow(window, SW_SHOW);
        UpdateWindow(window);

        m_stopRequested.store(false);
        m_state = kMP1Playing;
        cMoviePlaybackScope playbackScope;
        const bool allowSkip = !(flags & kMoviePlayNoStop);
        const char *termKeys = m_useTermKeys ? m_termKeys.c_str() : nullptr;
        for (;;)
        {
            const DWORD wait = WaitForSingleObject(callback->DoneEvent(), 10);
            if (wait == WAIT_OBJECT_0)
            {
                result = callback->Result();
                played = SUCCEEDED(result);
                break;
            }
            if (wait == WAIT_FAILED)
            {
                result = HRESULT_FROM_WIN32(GetLastError());
                break;
            }
            if (m_stopRequested.load() ||
                DarkPumpMovieEvents(allowSkip, termKeys))
            {
                player->Stop();
                played = true;
                break;
            }
        }
        m_state = kMP1Ready;
    }

    if (player)
    {
        SetWindowLongPtrW(window, GWLP_USERDATA, 0);
        player->Shutdown();
        ReleaseInterface(player);
    }
    callback->Release();
    if (window)
    {
        HWND parent = GetParent(window);
        DestroyWindow(window);
        InvalidateRect(parent, nullptr, FALSE);
    }
    MFShutdown();
    if (uninitializeCom)
        CoUninitialize();
    return played && SUCCEEDED(result);
}
