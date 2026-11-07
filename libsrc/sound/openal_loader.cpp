#ifdef _WIN32
#pragma pack(push, 8)
#include <windows.h>
#include <string.h>
#pragma pack(pop)
#else
#pragma pack(push, 8)
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#pragma pack(pop)
#endif

#include <openal_api.h>

#ifdef _WIN32
static HMODULE g_openal;
#else
static void *g_openal;
#endif

template <class T>
static bool LoadProc(T &target, const char *name)
{
#ifdef _WIN32
    target = reinterpret_cast<T>(GetProcAddress(g_openal, name));
#else
    target = reinterpret_cast<T>(dlsym(g_openal, name));
#endif
    return target != nullptr;
}

bool OpenALLoad(sOpenALApi *api)
{
    if (!api)
        return false;
    memset(api, 0, sizeof(*api));

#ifdef _WIN32
    if (!g_openal)
        g_openal = LoadLibraryA("OpenAL32.dll");
#else
    // WSLg exports audio through PulseAudio. Avoid OpenAL Soft's ALSA probe
    // after a Pulse failure because WSL does not expose an ALSA device.
    if (getenv("WSL2_GUI_APPS_ENABLED") && !getenv("ALSOFT_DRIVERS"))
        setenv("ALSOFT_DRIVERS", "pulse", 0);
    if (!g_openal)
    {
#if defined(__ANDROID__)
        // OpenAL Soft is packaged in the APK and uses an unversioned name.
        g_openal = dlopen("libopenal.so", RTLD_NOW | RTLD_LOCAL);
#elif defined(__APPLE__)
        g_openal = dlopen(
            "/System/Library/Frameworks/OpenAL.framework/OpenAL",
            RTLD_NOW | RTLD_LOCAL);
#endif
        if (!g_openal)
            g_openal = dlopen("libopenal.so.1", RTLD_NOW | RTLD_LOCAL);
    }
#endif
    if (!g_openal)
        return false;

#define LOAD(name) if (!LoadProc(api->name, #name)) { OpenALUnload(); return false; }
    LOAD(alcOpenDevice)
    LOAD(alcGetIntegerv)
    LOAD(alcCloseDevice)
    LOAD(alcCreateContext)
    LOAD(alcMakeContextCurrent)
    LOAD(alcProcessContext)
    LOAD(alcSuspendContext)
    LOAD(alcDestroyContext)
    LOAD(alcIsExtensionPresent)
    LOAD(alcGetProcAddress)
    LOAD(alGetError)
    LOAD(alGetProcAddress)
    LOAD(alGenSources)
    LOAD(alDeleteSources)
    LOAD(alSourcePlay)
    LOAD(alSourcePause)
    LOAD(alSourceStop)
    LOAD(alSourcei)
    LOAD(alSourcef)
    LOAD(alSource3f)
    LOAD(alSource3i)
    LOAD(alGetSourcei)
    LOAD(alGetSourcef)
    LOAD(alGenBuffers)
    LOAD(alDeleteBuffers)
    LOAD(alBufferData)
    LOAD(alSourceQueueBuffers)
    LOAD(alSourceUnqueueBuffers)
    LOAD(alListener3f)
    LOAD(alListenerfv)
    LOAD(alDopplerFactor)
    LOAD(alSpeedOfSound)
    LOAD(alDistanceModel)
#undef LOAD
    return true;
}

void OpenALUnload(void)
{
    // Keep the library loaded until process exit; active OpenAL worker threads
    // may still be returning through its code during mixer teardown.
}
