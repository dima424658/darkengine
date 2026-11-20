#include <cstdlib>
#include <cstring>

#include <openal_api.h>
#include <platform_library.h>

static void *g_openal;

template <class T>
static bool LoadProc(T &target, const char *name)
{
    target = reinterpret_cast<T>(PlatformFindSymbol(g_openal, name));
    return target != nullptr;
}

bool OpenALLoad(sOpenALApi *api)
{
    if (!api)
        return false;
    std::memset(api, 0, sizeof(*api));

#ifndef _WIN32
    // WSLg exports audio through PulseAudio. Avoid the ALSA fallback probe.
    if (std::getenv("WSL2_GUI_APPS_ENABLED") && !std::getenv("ALSOFT_DRIVERS"))
        setenv("ALSOFT_DRIVERS", "pulse", 0);
#endif
    if (!g_openal)
        g_openal = PlatformLoadLibrary(DARK_OPENAL_LIBRARY);

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
