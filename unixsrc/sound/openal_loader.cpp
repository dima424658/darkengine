#include <openal_api.h>

#pragma pack(push, 8)
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#pragma pack(pop)

static void *g_openal;

template <class T>
static bool LoadProc(T &target, const char *name)
{
    target = reinterpret_cast<T>(dlsym(g_openal, name));
    return target != nullptr;
}

bool OpenALLoad(sOpenALApi *api)
{
    if (!api)
        return false;
    memset(api, 0, sizeof(*api));
    // WSLg exports audio through PulseAudio. Its ALSA probe can only emit
    // misleading no-card diagnostics after a Pulse failure.
    if (getenv("WSL2_GUI_APPS_ENABLED") && !getenv("ALSOFT_DRIVERS"))
        setenv("ALSOFT_DRIVERS", "pulse", 0);
    if (!g_openal)
    {
#if defined(__ANDROID__)
        // OpenAL Soft is built and packaged with the APK. Android shared
        // libraries are unversioned, unlike the usual Linux SONAME.
        g_openal = dlopen("libopenal.so", RTLD_NOW | RTLD_LOCAL);
#endif
        if (!g_openal)
            g_openal = dlopen("libopenal.so.1", RTLD_NOW | RTLD_LOCAL);
    }
    if (!g_openal)
        return false;
#define LOAD(name) if (!LoadProc(api->name, #name)) { OpenALUnload(); return false; }
    LOAD(alcOpenDevice) LOAD(alcCloseDevice) LOAD(alcCreateContext)
    LOAD(alcMakeContextCurrent) LOAD(alcProcessContext) LOAD(alcSuspendContext)
    LOAD(alcDestroyContext) LOAD(alcIsExtensionPresent) LOAD(alcGetProcAddress)
    LOAD(alGetError) LOAD(alGetProcAddress) LOAD(alGenSources)
    LOAD(alDeleteSources) LOAD(alSourcePlay) LOAD(alSourcePause)
    LOAD(alSourceStop) LOAD(alSourcei) LOAD(alSourcef) LOAD(alSource3f)
    LOAD(alSource3i) LOAD(alGetSourcei) LOAD(alGetSourcef) LOAD(alGenBuffers)
    LOAD(alDeleteBuffers) LOAD(alBufferData) LOAD(alSourceQueueBuffers)
    LOAD(alSourceUnqueueBuffers) LOAD(alListener3f) LOAD(alListenerfv)
    LOAD(alDopplerFactor) LOAD(alSpeedOfSound) LOAD(alDistanceModel)
#undef LOAD
    return true;
}

void OpenALUnload()
{
    //Causes crash because audio threads still active so we leave open for process lifetime
    // if (g_openal)
    //     dlclose(g_openal);
    // g_openal = nullptr;
}
