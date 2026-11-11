#include <lgsound.h>

#if defined(_WIN32)
#include <string.h>
#define strcasecmp _stricmp
#else
#include <strings.h>
#endif

extern BOOL SndCreateOpenALMixer(ISndMixer **mixer, IUnknown *outer);

extern "C" BOOL SndCreateMixer(ISndMixer **mixer, IUnknown *outer)
{
    return SndCreateOpenALMixer(mixer, outer);
}

extern "C" BOOL SndCreateMixerNamed(ISndMixer **mixer, IUnknown *outer,
                                      const char *backend)
{
    if (!backend || !*backend || strcasecmp(backend, "auto") == 0 ||
        strcasecmp(backend, "native") == 0 ||
        strcasecmp(backend, "openal") == 0)
        return SndCreateOpenALMixer(mixer, outer);
    *mixer = nullptr;
    return FALSE;
}
