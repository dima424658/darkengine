#include <win32_platform.h>

#include <openal_api.h>

#include <string.h>

static HMODULE g_openal;

template <class T>
static bool LoadProc(T &target, const char *name)
{
   target = reinterpret_cast<T>(GetProcAddress(g_openal, name));
   return target != NULL;
}

bool OpenALLoad(sOpenALApi *api)
{
   if (api == NULL)
      return false;
   memset(api, 0, sizeof(*api));

   if (g_openal == NULL)
      g_openal = LoadLibraryA("OpenAL32.dll");
   if (g_openal == NULL)
      return false;

#define LOAD(name) if (!LoadProc(api->name, #name)) { OpenALUnload(); return false; }
   LOAD(alcOpenDevice)
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
   if (g_openal != NULL)
      FreeLibrary(g_openal);
   g_openal = NULL;
}
